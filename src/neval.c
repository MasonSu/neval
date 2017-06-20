#include "config.h"
#include "http_request.h"
#include "log.h"
#include "ne.h"
#include "networking.h"
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const struct option long_options[] = {
    {"help", no_argument, 0, 'h'},
    {"version", no_argument, 0, 'v'},
    {"conf", required_argument, 0, 'c'},
    {0, 0, 0, 0}};

static void usage(void) {
  fprintf(
      stderr,
      "neval [option]...\n"
      "-c|--config <config file> Specify config file. Default ./neval.conf\n"
      "-h|--help                 This information\n"
      "-v|--version              Display program version\n");
}

int main(int argc, char **argv) {
  char *conf = CONF_FILE;
  int opt, options_index = 0;

  while ((opt = getopt_long(argc, argv, "vc:h", long_options,
                            &options_index)) != -1) {
    switch (opt) {
    case 0:
      break;
    case 'c':
      conf = optarg;
      break;
    case 'v':
      printf("%s\n", PROGRAM_VERSION);
      return 0;
    case 'h':
    case '?':
    case ':':
      usage();
      return 0;
    }
  }

  if (optind < argc) {
    printf("non-option ARGV-elements: ");
    while (optind < argc)
      printf("%s ", argv[optind++]);
    printf("\n");

    return 0;
  }

  loadServerConfig(conf, NULL);

  int sockfd;
  sockfd = socket_init(server.port);
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