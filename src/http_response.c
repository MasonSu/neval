#include "http_response.h"
#include "http.h"
#include "http_request.h"
#include "log.h"
#include "string.h"
#include <stdio.h>
#include <sys/sendfile.h>
#include <time.h>
#include <unistd.h>

mime_type neval_mime[] = {{".html", "text/html"},
                          {".xml", "text/xml"},
                          {".xhtml", "application/xhtml+xml"},
                          {".txt", "text/plain"},
                          {".rtf", "application/rtf"},
                          {".pdf", "application/pdf"},
                          {".word", "application/msword"},
                          {".png", "image/png"},
                          {".gif", "image/gif"},
                          {".jpg", "image/jpeg"},
                          {".jpeg", "image/jpeg"},
                          {".au", "audio/basic"},
                          {".mpeg", "video/mpeg"},
                          {".mpg", "video/mpeg"},
                          {".avi", "video/x-msvideo"},
                          {".gz", "application/x-gzip"},
                          {".tar", "application/x-tar"},
                          {".css", "text/css"},
                          {NULL, "text/plain"}};

static void apend_string(ne_http_request *request, const char *str);
static const char *get_file_type(const char *type);
static const char *status_repr(int status);
static void ne_http_response_put_status_line(ne_http_request *request);
static void ne_http_response_put_date(ne_http_request *request);
static void ne_http_response_put_server(ne_http_request *request);
static void ne_http_response_put_connection(ne_http_request *request);
static void ne_http_response_put_content_type(ne_http_request *request);
static void ne_http_response_put_content_length(ne_http_request *request);

void apend_string(ne_http_request *request, const char *str) {
  char *header = request->outbuf;
  size_t length = strlen(str);

  if (MAX_HEADER_BUF - request->buf_end < length)
    log_err("outbuf overflow");

  strncpy(header + request->buf_end, str, length);
  request->buf_end += length;
}

const char *status_repr(int status) {
  switch (status) {
  case 100:
    return "100 Continue";
  case 101:
    return "101 Switching Protocols";
  case 200:
    return "200 OK";
  case 201:
    return "201 Created";
  case 202:
    return "202 Accepted";
  case 203:
    return "203 Non-Authoritative Information";
  case 204:
    return "204 No Content";
  case 205:
    return "205 Reset Content";
  case 206:
    return "206 Partial Content";
  case 300:
    return "300 Multiple Choices";
  case 301:
    return "301 Moved Permanently";
  case 302:
    return "302 Found";
  case 303:
    return "303 See Other";
  case 304:
    return "304 Not Modified";
  case 305:
    return "305 Use Proxy";
  case 307:
    return "307 Temporary Redirect";
  case 400:
    return "400 Bad Request";
  case 401:
    return "401 Unauthorized";
  case 402:
    return "402 Payment Required";
  case 403:
    return "403 Forbidden";
  case 404:
    return "404 Not Found";
  case 405:
    return "405 Method Not Allowed";
  case 406:
    return "406 Not Acceptable";
  case 407:
    return "407 Proxy Authentication Required";
  case 408:
    return "408 Request Time-out";
  case 409:
    return "409 Conflict";
  case 410:
    return "410 Gone";
  case 411:
    return "411 Length Required";
  case 412:
    return "412 Precondition Failed";
  case 413:
    return "413 Request Entity Too Large";
  case 414:
    return "414 Request-URI Too Long";
  case 415:
    return "415 Unsupported Media Type";
  case 416:
    return "416 Requested range not satisfiable";
  case 417:
    return "417 Expectation Failed";
  case 500:
    return "500 Internal Server Error";
  case 501:
    return "501 Not Implemented";
  case 502:
    return "502 Bad Gateway";
  case 503:
    return "503 Service Unavailable";
  case 504:
    return "504 Gateway Time-out";
  case 505:
    return "505 HTTP Version not supported";
  default:
    log_err("Invalid status code");
    return NULL;
  }
}

const char *get_file_type(const char *type) {
  for (int i = 0; neval_mime[i].type != NULL; i++) {
    if (strcmp(neval_mime[i].type, type) == 0)
      return neval_mime[i].value;
  }

  return "text/plain";
}

void ne_http_response_put_status_line(ne_http_request *request) {
  char buf[MAX_HEADER_LINE];

  if (request->http_minor == 1)
    snprintf(buf, MAX_HEADER_LINE, "%s: %s\r\n", "HTTP/1.1",
             status_repr(request->status_code));
  else
    snprintf(buf, MAX_HEADER_LINE, "%s: %s\r\n", "HTTP/1.0",
             status_repr(request->status_code));

  apend_string(request, buf);
}

void ne_http_response_put_date(ne_http_request *request) {
  char buf[MAX_HEADER_LINE];

  time_t t = time(NULL);
  struct tm *date = localtime(&t);
  check(date != NULL, "localtime");

  if (strftime(buf, MAX_HEADER_LINE, "Date: %a, %d %b %Y %H:%M:%S GMT\r\n",
               date) == 0)
    log_err("strftime");

  apend_string(request, buf);
}

void ne_http_response_put_server(ne_http_request *request) {
  char buf[MAX_HEADER_LINE];

  snprintf(buf, MAX_HEADER_LINE, "%s: %s\r\n", "Server: ", SERVER_NAME);
  apend_string(request, buf);
}

void ne_http_response_put_connection(ne_http_request *request) {
  char buf[MAX_HEADER_LINE];

  if (request->keep_alive)
    snprintf(buf, MAX_HEADER_LINE, "%s: %s\r\n", "Connection", "Keep-Alive");
  else
    snprintf(buf, MAX_HEADER_LINE, "%s: %s\r\n", "Connection", "Close");

  apend_string(request, buf);
}

void ne_http_response_put_content_type(ne_http_request *request) {
  char buf[MAX_HEADER_LINE];
  const char *dot = strrchr(request->filename, '.');
  const char *file_type = get_file_type(dot);

  snprintf(buf, MAX_HEADER_LINE, "%s: %s\r\n", "Content-Type", file_type);
  apend_string(request, buf);
}

void ne_http_response_put_content_length(ne_http_request *request) {
  char buf[MAX_HEADER_LINE];

  snprintf(buf, MAX_HEADER_LINE, "%s: %ld\r\n", "Content-Length",
           request->resource_len);
  apend_string(request, buf);
}

void ne_http_build_error_page(ne_http_request *request) {
  char body[MAX_BODY_BUF], line[MAX_HEADER_LINE];
  strcpy(body, "<html><title>Neval Error</title>");
  strcat(body, "<body bgcolor="
               "ffffff"
               ">\n");
  snprintf(line, MAX_HEADER_LINE, "%d: %s\n", request->status_code,
           status_repr(request->status_code));
  strncat(body, line, strlen(line));
  strcat(body, "<hr><em>Neval web server</em>\n</body></html>");

  ne_http_response_put_status_line(request);
  ne_http_response_put_date(request);
  ne_http_response_put_server(request);

  request->keep_alive = 0;
  ne_http_response_put_connection(request);

  apend_string(request, "Content-Type: text/html\r\n");

  snprintf(line, MAX_HEADER_LINE, "%s: %d\r\n\r\n", "Content-Length",
           (int)strlen(body));
  apend_string(request, line);
  apend_string(request, body);

  return;
}

void ne_http_send_error_page(ne_http_request *request) {
  debug("ne_http_send_error_page");
  ne_http_send_response_buffer(request);

  return;
}

void ne_http_send_response(ne_http_request *request) {
  debug("ne_http_send_response");
  ne_http_response_put_status_line(request);
  ne_http_response_put_date(request);
  ne_http_response_put_server(request);
  ne_http_response_put_connection(request);
  ne_http_response_put_content_type(request);
  ne_http_response_put_content_length(request);
  apend_string(request, CRLF);

  int rc;
  rc = ne_http_send_response_buffer(request);
  if (rc == NE_AGAIN)
    goto again;
  else if (rc == NE_ERR)
    goto err;

  rc = ne_http_send_response_file(request);
  if (rc == NE_AGAIN)
    goto again;
  else if (rc == NE_ERR)
    goto err;

  request_reuse(request);
  return;

again:
  /* Do not support pipelining */
  request->write_event = 1;
  neDeleteFileEvent(request->loop, request->socket, NE_READABLE);
  rc = neCreateFileEvent(request->loop, request->socket, NE_WRITABLE,
                         ne_http_response_handle, request);
  check(rc == NE_OK, "neCreateFileEvent");
  return;

err:
  rc = ne_http_close_conn(request->loop, request);
  check(rc == NE_OK, "ne_http_close_conn");
  return;
}

int ne_http_send_response_buffer(ne_http_request *request) {
  size_t count = request->buf_end - request->buf_begin;
  ssize_t nwritten;
  char *buf = request->outbuf;
  debug("ne_http_send_response_buffer");
  while (count > 0) {
    nwritten = write(request->socket, buf + request->buf_begin, count);
    if (nwritten == -1) {
      if (errno == EAGAIN) {
        log_warn("send_response_buffer return EAGAIN");
        return NE_AGAIN;
      }

      log_err("ne_http_send_response_buffer");
      return NE_ERR;
    }
    request->buf_begin += nwritten;
    count -= nwritten;
  }

  request->out_handler = ne_http_send_response_file;

  return NE_OK;
}

int ne_http_send_response_file(ne_http_request *request) {
  int count = request->resource_len - request->offset;
  int file = request->resource_fd, n;
  debug("ne_http_send_response_file");
  off_t offset;
  while (count > 0) {
    offset = request->offset;
    n = sendfile(request->socket, file, &offset, count);
    debug("send %d bytes", n);
    if (n == -1) {
      if (errno == EAGAIN) {
        log_warn("ne_http_send_response_file return EAGAIN");
        return NE_AGAIN;
      }

      log_err("ne_http_send_response_file");
      return NE_ERR;
    }

    request->offset += n;
    count -= n;
  }
  debug("send file finish");
  /* Remember to close file */
  close(file);
  request->response_done = 1;

  ne_http_resquest_done(request);

  return NE_OK;
}

void ne_http_response_handle(struct neEventLoop *eventLoop, int fd,
                             void *clientData) {
  UNUSED(fd);

  ne_http_request *request = (ne_http_request *)clientData;
  int rc;

  while (!request->response_done) {
    rc = request->out_handler(request);
    if (rc == NE_AGAIN)
      return;
    else if (rc == NE_ERR)
      goto err;
  }

  request_reuse(request);
  return;

err:
  rc = ne_http_close_conn(eventLoop, request);
  check(rc == NE_OK, "ne_http_close_conn");
  return;
}