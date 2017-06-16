#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include "hash.h"
#include "http.h"

typedef int (*ne_http_header_handle_pt)(ne_http_request *, char *, int);

typedef struct ne_http_header_handle {
  char *name;
  ne_http_header_handle_pt handler;
} ne_http_header_handle;

void request_reuse(ne_http_request *request);
ne_http_request *request_init(neEventLoop *loop, int fd);
void ne_http_request_handle(struct neEventLoop *eventLoop, int fd,
                            void *clientData);
void ne_http_resquest_done(struct ne_http_request *request);
int ne_http_close_conn(struct neEventLoop *eventLoop, void *clientData);
void accept_handle(struct neEventLoop *eventLoop, int fd, void *clientData);

void server_init(void);

#endif