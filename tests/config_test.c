#include "config.h"
#include <stdio.h>

int main() {
  loadServerConfig("/home/xiaxun/worksp/neval/neval.conf", NULL);
  printf("root is %s\n", server.root);
  printf("port is %d\n", server.port);
  printf("timeout is %d\n", server.timeout);
  return (0);
}