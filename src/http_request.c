#include "http_request.h"
#include "http.h"
#include "http_parse.h"
#include "http_response.h"
#include "log.h"
#include "networking.h"
#include <fcntl.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static void request_reuse(ne_http_request *request);
static int ne_http_handle_request_line(ne_http_request *request);
static int ne_http_handle_uri(ne_http_request *request);
static int ne_http_handle_header_line(ne_http_request *request);

static int ne_http_process_ignore(ne_http_request *request, char *data,
                                  int length);
static int ne_http_process_connection(ne_http_request *request, char *data,
                                      int length);
static int ne_http_process_if_modified_since(ne_http_request *request,
                                             char *data, int length);

ne_http_header_handle ne_http_headers_in[] = {
    {"Host", ne_http_process_ignore},
    {"Connection", ne_http_process_connection},
    {"If-Modified-Since", ne_http_process_if_modified_since},
    {"", ne_http_process_ignore}};

ne_http_request *request_init(neEventLoop *loop, int fd) {
  ne_http_request *request = (ne_http_request *)malloc(sizeof(ne_http_request));
  if (request == NULL)
    return NULL;

  request->loop = loop;
  request->socket = fd;

  request->pos = (u_char *)request->inbuf;
  request->last = (u_char *)request->inbuf;
  request->state = 0;

  request->list = listCreate();
  request->in_handler = ne_http_handle_request_line;
  request->out_handler = ne_http_send_response_buffer;
  request->keep_alive = 1;
  request->status_code = 200;
  request->buf_begin = 0;
  request->buf_end = 0;

  request->write_event = 0;

  request->request_done = 0;
  request->response_done = 0;

  return request;
}

void request_reuse(ne_http_request *request) {
  request->pos = (u_char *)request->inbuf;
  request->last = (u_char *)request->inbuf;
  request->state = 0;

  request->in_handler = ne_http_handle_request_line;
  request->out_handler = ne_http_send_response_buffer;
  request->keep_alive = 1;
  request->status_code = 200;
  request->buf_begin = 0;
  request->buf_end = 0;

  request->write_event = 0;

  request->request_done = 0;
  request->response_done = 0;
}

void accept_handle(struct neEventLoop *eventLoop, int fd, void *clientData) {
  UNUSED(clientData);

  int infd;
  struct sockaddr_in clientaddr;
  socklen_t inlen = 1;
  /* we have one or more incoming connections */
  while (1) {
    infd = accept(fd, (struct sockaddr *)&clientaddr, &inlen);
    if (infd < 0) {
      if (errno != EAGAIN)
        log_err("accept");
      break;
    }

    int rc = make_socket_non_blocking(infd);
    check(rc == NE_OK, "make_socket_non_blocking");
    log_info("new connection fd %d", infd);

    ne_http_request *request = request_init(eventLoop, infd);
    check(request != NULL, "request_init");
    // Add EPOLLET
    rc = neCreateFileEvent(eventLoop, infd, NE_READABLE, ne_http_request_handle,
                           request);
    check(rc == NE_OK, "neCreateFileEvent");
    rc = neCreateTimeEvent(eventLoop, TIMEOUT_DEFAULT, ne_http_close_conn,
                           request);
    check(rc == NE_OK, "neCreateTimeEvent");
  }

  return;
}

void ne_http_request_handle(struct neEventLoop *eventLoop, int fd,
                            void *clientData) {
  UNUSED(fd);

  ne_http_request *request = (ne_http_request *)clientData;
  int rc;
  /* Delete old timer */
  neDeleteTimeEvent(request);
  while (!request->request_done) {
    int remainSize = MAX_BUF - (request->last - request->pos);
    int n = read(request->socket, request->last, remainSize);

    if (n == 0) {
      log_err("read return 0, ready to close fd %d", request->socket);
      goto err;
    }

    if (n < 0) {
      if (errno != EAGAIN) {
        log_err("read err");
        goto err;
      }
      break;
    }

    request->last += n;
    check(request->last - request->pos < MAX_BUF, "request buffer overflow");

    rc = request->in_handler(request);
    if (rc == NE_AGAIN)
      continue;
    else if (rc == NE_ERR)
      goto err;
  }

  /* Send response ASAP, if it can be done,
   * then there is no need to add write event
   */
  if (request->request_done)
    ne_http_send_response(request);

  return;

err:
  rc = ne_http_close_conn(eventLoop, request);
  check(rc == NE_OK, "ne_http_close_conn");
}

int ne_http_handle_request_line(ne_http_request *request) {
  int rc = ne_http_parse_request_line(request);
  if (rc == NE_AGAIN)
    return rc;
  else if (rc == NE_HTTP_PARSE_INVALID_REQUEST) {
    request->status_code = 400;
    goto err;
  }

  /* Supports only HTTP/1.1 */
  if (request->http_major != 1 || request->http_minor > 1) {
    request->status_code = 505;
    goto err;
  }

  /* HTTP/1.1: persistent connection default */
  request->keep_alive = (request->http_minor == 1);

  request->in_handler = ne_http_handle_header_line;
  return ne_http_handle_uri(request);

err:
  ne_http_build_error_page(request);
  ne_http_send_error_page(request);
  return NE_ERR;
}

int ne_http_handle_uri(ne_http_request *request) {
  ne_http_parse_uri(request);

  struct stat sbuf;
  if (stat(request->filename, &sbuf) == -1) {
    request->status_code = 404;
    goto err;
  }

  if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {
    request->status_code = 403;
    goto err;
  }

  request->resource_fd = open(request->filename, O_RDONLY);
  check(request->resource_fd != -1, "open error");
  request->resource_len = sbuf.st_size;

  return NE_OK;

err:
  ne_http_build_error_page(request);
  ne_http_send_error_page(request);
  return NE_ERR;
}

int ne_http_handle_header_line(ne_http_request *request) {
  int rc = ne_http_parse_header_line(request);
  if (rc == NE_AGAIN)
    return rc;
  else if (rc == NE_HTTP_PARSE_INVALID_HEADER) {
    request->status_code = 400;
    goto err;
  }

  for (listNode *node = request->list->head; node != NULL; node = node->next) {
    ne_http_header *value = (ne_http_header *)node->value;
    for (ne_http_header_handle *header_in = ne_http_headers_in;
         strlen(header_in->name); header_in++) {
      debug("header= %.*s", (int)(value->key_end - value->key_start),
            value->key_start);
      if (strncmp(header_in->name, (char *)value->key_start,
                  value->key_end - value->key_start) == 0) {
        header_in->handler(request, (char *)value->value_start,
                           value->value_end - value->value_end);
        break;
      }
    }
    listDelNode(request->list, node);
  }

  check_debug(request->list->len == 0, "the list length error");
  request->request_done = 1;

  return NE_OK;

err:
  ne_http_build_error_page(request);
  ne_http_send_error_page(request);
  return NE_ERR;
}

int ne_http_close_conn(neEventLoop *eventLoop, void *clientData) {
  debug("close connection");
  ne_http_request *request = (ne_http_request *)clientData;
  /* timeEvent has been deleted before */
  neDeleteFileEvent(eventLoop, request->socket, NE_READABLE | NE_WRITABLE);
  close(request->socket);

  free(request);

  return NE_OK;
}

int ne_http_process_ignore(ne_http_request *request, char *data, int length) {
  (void)request;
  (void)data;
  (void)length;

  return NE_OK;
}

int ne_http_process_connection(ne_http_request *request, char *data,
                               int length) {
  if (strncasecmp("Keep-Alive", data, length) == 0)
    request->keep_alive = 1;

  return NE_OK;
}

int ne_http_process_if_modified_since(ne_http_request *request, char *data,
                                      int length) {
  return ne_http_process_ignore(request, data, length);
}

void ne_http_resquest_done(ne_http_request *request) {
  if (request->keep_alive) {
    int rc;
    if (request->write_event) {
      neDeleteFileEvent(request->loop, request->socket, NE_WRITABLE);
      rc = neCreateFileEvent(request->loop, request->socket, NE_READABLE,
                             ne_http_request_handle, request);
      check(rc == NE_OK, "neCreateFileEvent");
    }

    rc = neCreateTimeEvent(request->loop, TIMEOUT_DEFAULT, ne_http_close_conn,
                           request);
    check(rc == NE_OK, "neCreateTimeEvent");

    request_reuse(request);
  } else {
    ne_http_close_conn(request->loop, request);
  }

  return;
}