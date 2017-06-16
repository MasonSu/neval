#include "http_request.h"
#include "log.h"
#include "ne.h"
#include "networking.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char const *argv[]) {
  int sockfd;
  sockfd = socket_init(3000);
  check_exit(sockfd != -1, "socket_init error");

  int rc = make_socket_non_blocking(sockfd);
  check_exit(rc == 0, "make_socket_non_blocking");

  struct neEventLoop *loop = neCreateEventLoop(NE_MAX_CLIENT);
  check_exit(loop != NULL, "neCreateEventLoop");

  rc =
      neCreateFileEvent(loop, sockfd, NE_READABLE | NE_ET, accept_handle, NULL);
  check_exit(rc == 0, "neCreateFileEvent");

  server_init();

  log_info("Neval start...");
  neMain(loop);

  return 0;
}