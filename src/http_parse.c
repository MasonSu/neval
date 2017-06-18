#include "http_parse.h"
#include "log.h"
#include <stdio.h>
#include <stdlib.h>

//#define ROOT "/home/xiaxun/worksp/neval/data/www"
#define ROOT "/home/mason/http/data/www"

int ne_http_parse_request_line(ne_http_request *request) {
  enum {
    sw_start = 0,
    sw_method,
    sw_spaces_before_uri,
    sw_after_slash_in_uri,
    sw_http,
    sw_http_H,
    sw_http_HT,
    sw_http_HTT,
    sw_http_HTTP,
    sw_first_major_digit,
    sw_major_digit,
    sw_first_minor_digit,
    sw_minor_digit,
    sw_spaces_after_digit,
    sw_almost_down
  } state;

  state = request->state;

  u_char *p;
  for (p = request->pos; p < request->last; p++) {
    u_char ch = *p;

    switch (state) {
    case sw_start:
      request->request_start = p;

      if (ch == CR || ch == LF)
        break;

      if (ch < 'A' || ch > 'Z')
        return NE_HTTP_PARSE_INVALID_METHOD;

      state = sw_method;
      break;

    case sw_method:
      if (ch == ' ') {
        u_char *m = request->request_start;
        request->method_end = p;

        switch (p - m) {
        case 3:
          if (ne_str3_cmp(m, 'G', 'E', 'T', ' '))
            request->method = NE_HTTP_GET;

          break;

        case 4:
          if (ne_str4_cmp(m, 'P', 'O', 'S', 'T'))
            request->method = NE_HTTP_POST;
          else if (ne_str4_cmp(m, 'H', 'E', 'A', 'D'))
            request->method = NE_HTTP_HEAD;

          break;

        default:
          request->method = NE_HTTP_UNKNOWN;
          break;
        }

        state = sw_spaces_before_uri;
        break;
      }

      if (ch < 'A' || ch > 'Z')
        return NE_HTTP_PARSE_INVALID_METHOD;
      break;

    case sw_spaces_before_uri:
      switch (ch) {
      case '/':
        request->uri_start = p;
        state = sw_after_slash_in_uri;
        break;
      case ' ':
        break;
      default:
        return NE_HTTP_PARSE_INVALID_REQUEST;
      }

      break;

    case sw_after_slash_in_uri:
      if (ch == ' ') {
        request->uri_end = p;
        state = sw_http;
      }
      break;

    case sw_http:
      switch (ch) {
      case 'H':
        state = sw_http_H;
        break;
      case ' ':
        break;
      default:
        return NE_HTTP_PARSE_INVALID_REQUEST;
      }
      break;

    case sw_http_H:
      if (ch == 'T')
        state = sw_http_HT;
      else
        return NE_HTTP_PARSE_INVALID_REQUEST;
      break;

    case sw_http_HT:
      if (ch == 'T')
        state = sw_http_HTT;
      else
        return NE_HTTP_PARSE_INVALID_REQUEST;
      break;

    case sw_http_HTT:
      if (ch == 'P')
        state = sw_http_HTTP;
      else
        return NE_HTTP_PARSE_INVALID_REQUEST;
      break;

    case sw_http_HTTP:
      if (ch == '/')
        state = sw_first_major_digit;
      else
        return NE_HTTP_PARSE_INVALID_REQUEST;
      break;

    case sw_first_major_digit:
      if (ch < '1' || ch > '9')
        return NE_HTTP_PARSE_INVALID_REQUEST;

      request->http_major = ch - '0';
      state = sw_major_digit;
      break;

    case sw_major_digit:
      if (ch == '.') {
        state = sw_first_minor_digit;
        break;
      }

      if (ch < '0' || ch > '9')
        return NE_HTTP_PARSE_INVALID_REQUEST;

      request->http_major = request->http_major * 10 + ch - '0';
      break;

    case sw_first_minor_digit:
      if (ch < '0' || ch > '9')
        return NE_HTTP_PARSE_INVALID_REQUEST;

      request->http_minor = ch - '0';
      state = sw_minor_digit;
      break;

    case sw_minor_digit:
      if (ch == CR) {
        state = sw_almost_down;
        break;
      }

      if (ch == LF)
        goto done;

      if (ch == ' ') {
        state = sw_spaces_after_digit;
        break;
      }

      if (ch < '0' || ch > '9')
        return NE_HTTP_PARSE_INVALID_REQUEST;

      request->http_minor = request->http_minor * 10 + ch - '0';
      break;

    case sw_spaces_after_digit:
      switch (ch) {
      case ' ':
        break;
      case CR:
        state = sw_almost_down;
        break;
      case LF:
        goto done;
      default:
        return NE_HTTP_PARSE_INVALID_REQUEST;
      }
      break;

    case sw_almost_down:
      request->request_end = p - 1;
      if (ch == LF)
        goto done;
      else
        return NE_HTTP_PARSE_INVALID_REQUEST;
    } // end of switch
  }   // end of for

  request->pos = p;
  request->state = state;
  return NE_AGAIN;

done:
  request->pos = p + 1;
  if (request->request_end == NULL)
    request->request_end = p;

  request->state = sw_start;
  return NE_OK;
}

int ne_http_parse_header_line(ne_http_request *request) {
  enum {
    sw_start = 0,
    sw_key,
    sw_space_before_value,
    sw_value,
    sw_cr,
    sw_crlf,
    sw_crlfcr
  } state;

  state = request->state;

  u_char *p;
  ne_http_header *header;
  // debug("request is %s", request->pos);
  for (p = request->pos; p < request->last; p++) {
    u_char ch = *p;

    switch (state) {
    case sw_start:
      if (ch == CR || ch == LF)
        break;
      request->cur_header_key_start = p;
      state = sw_key;
      break;

    case sw_key:
      if (ch == ':') {
        request->cur_header_key_end = p;
        state = sw_space_before_value;
      }

      break;

    case sw_space_before_value:
      if (ch == ' ')
        break;
      state = sw_value;
      request->cur_header_value_start = p;
      break;

    case sw_value:
      if (ch == CR) {
        request->cur_header_value_end = p;
        state = sw_cr;
        break;
      }

      if (ch == LF) {
        request->cur_header_value_end = p;
        state = sw_crlf;
        break;
      }

      break;

    case sw_cr:
      if (ch == LF)
        state = sw_crlf;
      else
        return NE_HTTP_PARSE_INVALID_HEADER;

      break;

    case sw_crlf:
      // save the current http header
      header = (ne_http_header *)malloc(sizeof(ne_http_header));
      if (header == NULL) {
        log_err("malloc failure");
        exit(1);
      }
      header->key_start = request->cur_header_key_start;
      header->key_end = request->cur_header_key_end;
      header->value_start = request->cur_header_value_start;
      header->value_end = request->cur_header_value_end;
      // debug("header= %.*s, value= %.*s",
      //       (int)(header->key_end - header->key_start), header->key_start,
      //       (int)(header->value_end - header->value_start),
      //       header->value_start);
      request->list = listAddNodeTail(request->list, header);

      if (ch == CR)
        state = sw_crlfcr;
      else {
        request->cur_header_key_start = p;
        state = sw_key;
      }

      break;

    case sw_crlfcr:
      if (ch == LF)
        goto done;
      else
        return NE_HTTP_PARSE_INVALID_HEADER;

      break;
    }
  }

  request->pos = p;
  request->state = state;

  return NE_AGAIN;

done:
  request->pos = p + 1;
  request->state = sw_start;

  return NE_OK;
}

void ne_http_parse_uri(ne_http_request *request) {
  u_char *uri = request->uri_start;
  int uri_length = request->uri_end - request->uri_start;
  uri[uri_length] = '\0';
  /* free doesn't clear the buffer */
  memset(request->filename, 0, strlen(request->filename));

  char *fileName = request->filename;
  strncpy(fileName, ROOT, strlen(ROOT));

  if (uri_length > MAX_LENGTH) {
    request->status_code = 414;
    return;
  }

  strncat(fileName, (char *)uri, uri_length);

  char *last_slash = strrchr(fileName, '/');
  char *last_dot = strrchr(last_slash, '.');
  if (last_dot == NULL && fileName[strlen(fileName) - 1] != '/')
    strcat(fileName, "/");

  if (fileName[strlen(fileName) - 1] == '/')
    strcat(fileName, "index.html");

  return;
}