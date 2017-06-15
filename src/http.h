#ifndef HTTP_H
#define HTTP_H

#include "list.h"
#include "ne.h"
#include <sys/types.h>

#define UNUSED(x) ((void)(x))
#define SERVER_NAME "Neval"
#define MAX_BUF 8124
#define MAX_LENGTH 512
typedef unsigned char u_char;

typedef struct ne_http_header {
  u_char *key_start;
  u_char *key_end;
  u_char *value_start;
  u_char *value_end;
} ne_http_header;

typedef struct ne_http_request {
  neEventLoop *loop;
  int socket;

  int state;
  char inbuf[MAX_BUF];
  u_char *pos;
  u_char *last;
  u_char *request_start;
  u_char *method_end;
  int method;
  u_char *uri_start;
  u_char *uri_end;
  int http_major;
  int http_minor;
  u_char *request_end;

  int keep_alive;

  char filename[MAX_LENGTH];
  int resource_fd;
  size_t resource_len;
  off_t offset; // file offset

  int request_done; // whether request has been done

  neList *list;
  u_char *cur_header_key_start;
  u_char *cur_header_key_end;
  u_char *cur_header_value_start;
  u_char *cur_header_value_end;

  neTimeEvent *timer;

  int status_code;
  char outbuf[MAX_BUF];
  size_t buf_begin;
  size_t buf_end; // current outbuf end

  int write_event;   // whether add write event
  int response_done; // whether response has been done

  int (*in_handler)(struct ne_http_request *request);
  int (*out_handler)(struct ne_http_request *request);
} ne_http_request;

#endif