#include "http_parse.h"
#include "http_request.h"
#include "list.h"
#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char const *argv[]) {
  ne_http_request *request = request_init(NULL, 0);
  char str[] = "GET   /path/resource   H";
  strncpy(request->inbuf, str, sizeof(str));
  request->last = (u_char *)request->inbuf + strlen(str);

  if (ne_http_parse_request_line(request) == NE_AGAIN)
    log_info("%s", "prase request again");

  char str1[] = "TTP/1.1 \r\n";
  strncat(request->inbuf, str1, sizeof(str1));
  request->last += strlen(str1);

  if (ne_http_parse_request_line(request) != NE_OK)
    log_err("%s", "prase request failure");
  log_info("method: %.*s", (int)(request->method_end - request->request_start),
           request->request_start);
  log_info("uri: %.*s", (int)(request->uri_end - request->uri_start),
           request->uri_start);
  log_info("http version: %d.%d", request->http_major, request->http_minor);

  char str2[] = "Host: www.github.com\r\n";
  strncat(request->inbuf, str2, sizeof(str2));
  request->last += strlen(str2);
  if (ne_http_parse_header_line(request) == NE_OK)
    goto done;

  char str3[] = "Connection: ";
  strncat(request->inbuf, str3, sizeof(str3));
  request->last += strlen(str3);
  if (ne_http_parse_header_line(request) == NE_OK)
    goto done;

  char str4[] = " keep-alive\r\n\r\n";
  strncat(request->inbuf, str4, sizeof(str4));
  request->last += strlen(str4);
  if (ne_http_parse_header_line(request) == NE_OK)
    goto done;

  return 0;

done:

  for (listNode *node = request->list->head; node != NULL; node = node->next) {
    ne_http_header *value = (ne_http_header *)(node->value);
    log_info("header= %.*s", (int)(value->key_end - value->key_start),
             value->key_start);
    log_info("value= %.*s", (int)(value->value_end - value->value_start),
             value->value_start);
  }

  return 0;
}