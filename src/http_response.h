#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include "http.h"

#define MAX_HEADER_BUF 8192
#define MAX_HEADER_LINE 128
#define MAX_BODY_BUF 8192

#define CRLF "\r\n"

typedef struct mime_type {
  const char *type;
  const char *value;
} mime_type;

void ne_http_build_error_page(ne_http_request *request);
void ne_http_send_error_page(ne_http_request *request);
int ne_http_send_response_buffer(ne_http_request *request);
int ne_http_send_response_file(ne_http_request *request);
void ne_http_send_response(ne_http_request *request);
void ne_http_response_handle(struct neEventLoop *eventLoop, int fd,
                             void *clientData);

#endif