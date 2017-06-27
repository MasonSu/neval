#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include "http.h"

#define NE_HTTP_OK 200
#define NE_HTTP_NOT_MODIFIED 304
#define NE_HTTP_BAD_REQUEST 400
#define NE_HTTP_FORBIDDEN 403
#define NE_HTTP_NOT_FOUND 404
#define NE_HTTP_VERSION_NOT_SUPPORTED 505

typedef int (*ne_http_header_handle_pt)(ne_http_request *, char *, int);

typedef struct ne_http_header_handle {
  char *name;
  ne_http_header_handle_pt handler;
} ne_http_header_handle;

ne_http_request *request_init(neEventLoop *loop, int fd);
void ne_http_request_handle(struct neEventLoop *eventLoop, int fd,
                            void *clientData);
void ne_http_resquest_done(struct ne_http_request *request);
int ne_http_close_conn(struct neEventLoop *eventLoop, void *clientData);
void accept_handle(struct neEventLoop *eventLoop, int fd, void *clientData);

void server_init(void);

#endif