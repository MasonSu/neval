#ifndef NETWORKING_H
#define NETWORKING_H

#define LISTENQ 1024

int make_socket_non_blocking(int fd);
int socket_init(int port);

#endif