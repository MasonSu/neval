#include "networking.h"
#include "http_request.h"
#include "log.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int socket_init(int port) {
  if (port <= 0)
    port = 3000;

  int listenfd;
  if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    return -1;

  int val = 1;
  if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&val,
                 sizeof(int)) < 0)
    return -1;

  struct sockaddr_in serveraddr;
  memset(&serveraddr, 0, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serveraddr.sin_port = htons((unsigned short)port);

  if (bind(listenfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0)
    return -1;

  if (listen(listenfd, LISTENQ) < 0)
    return -1;

  return listenfd;
}

int make_socket_non_blocking(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags == -1) {
    log_err("fcntl");
    return -1;
  }

  flags |= O_NONBLOCK;
  if (fcntl(fd, F_SETFL, flags) == -1) {
    log_err("fcntl");
    return -1;
  }

  return 0;
}