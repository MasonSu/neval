#ifndef HTTP_PARSE_H
#define HTTP_PARSE_H

#include "http.h"
#include <stddef.h>
#include <stdint.h>

#define CR '\r'
#define LF '\n'

#define NE_HTTP_PARSE_INVALID_METHOD 10
#define NE_HTTP_PARSE_INVALID_REQUEST 11
#define NE_HTTP_PARSE_INVALID_HEADER 12

#define NE_HTTP_UNKNOWN 0x0001
#define NE_HTTP_GET 0x0002
#define NE_HTTP_HEAD 0x0004
#define NE_HTTP_POST 0x0008

#if NE_HAVE_LITTLE_ENDIAN

#define ne_str3_cmp(m, c0, c1, c2, c3)                                         \
  *(uint32_t *)m == ((c3 << 24) | (c2 << 16) | (c1 << 8) | c0)

#define ne_str4_cmp(m, c0, c1, c2, c3)                                         \
  *(uint32_t *)m == ((c3 << 24) | (c2 << 16) | (c1 << 8) | c0)

#else

#define ne_str3_cmp(m, c0, c1, c2, c3) m[0] == c0 &&m[1] == c1 &&m[2] == c2

#define ne_str4_cmp(m, c0, c1, c2, c3)                                         \
  m[0] == c0 &&m[1] == c1 &&m[2] == c2 &&m[3] == c3

#endif

int ne_http_parse_request_line(ne_http_request *request);
int ne_http_parse_header_line(ne_http_request *request);
void ne_http_parse_uri(ne_http_request *request);

#endif
