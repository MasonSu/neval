#ifndef CONFIG_H
#define CONFIG_H

#define MAX_BUFLEN 8192
#define MAX_LINE 128

extern struct nevalServer server;

struct nevalServer {
  char root[MAX_LINE];
  int port;
  int timeout;
};

void loadServerConfig(char *filename, char *options);

#endif