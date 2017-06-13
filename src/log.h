#ifndef LOG_H
#define LOG_H

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NE_OK 0
#define NE_ERR -1
#define NE_AGAIN -2

#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_YELLOW "\x1b[33m"
#define ANSI_COLOR_BLUE "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN "\x1b[36m"
#define ANSI_COLOR_RESET "\x1b[0m"

#ifdef NDEBUG
#define debug(M, ...)
#else
#define debug(M, ...)                                                          \
  fprintf(stderr,                                                              \
          ANSI_COLOR_BLUE "[DEBUG]" ANSI_COLOR_MAGENTA                         \
                          "%s:%d: " ANSI_COLOR_CYAN M ANSI_COLOR_RESET "\n",   \
          __FILE__, __LINE__, ##__VA_ARGS__);
#endif

#define clean_errno() (errno == 0 ? "None" : strerror(errno))

#ifndef NCOLOR_OUTPUT

#define log_err(M, ...)                                                        \
  fprintf(stderr, ANSI_COLOR_RED                                               \
          "[ERROR]" ANSI_COLOR_MAGENTA                                         \
          "%s:%d: errno: %s " ANSI_COLOR_CYAN M ANSI_COLOR_RESET "\n",         \
          __FILE__, __LINE__, clean_errno(), ##__VA_ARGS__)

#define log_warn(M, ...)                                                       \
  fprintf(stderr, ANSI_COLOR_YELLOW                                            \
          "[WARN]" ANSI_COLOR_MAGENTA                                          \
          "%s:%d: errno: %s " ANSI_COLOR_CYAN M ANSI_COLOR_RESET "\n",         \
          __FILE__, __LINE__, clean_errno(), ##__VA_ARGS__)

#define log_info(M, ...)                                                       \
  fprintf(stderr,                                                              \
          ANSI_COLOR_GREEN "[INFO]" ANSI_COLOR_MAGENTA                         \
                           "%s:%d: " ANSI_COLOR_CYAN M ANSI_COLOR_RESET "\n",  \
          __FILE__, __LINE__, ##__VA_ARGS__)

#else

#define log_err(M, ...)                                                        \
  fprintf(stderr, "[ERROR] %s:%d: errno: %s " M "\n", __FILE__, __LINE__,      \
          clean_errno(), ##__VA_ARGS__)

#define log_warn(M, ...)                                                       \
  fprintf(stderr, "[WARN] %s:%d: errno: %s " M "\n", __FILE__, __LINE__,       \
          clean_errno(), ##__VA_ARGS__)

#define log_info(M, ...)                                                       \
  fprintf(stderr, "[INFO] %s:%d " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)

#endif

#define check(A, M, ...)                                                       \
  if (!(A)) {                                                                  \
    log_err(M "\n", ##__VA_ARGS__);                                            \
  }

#define check_exit(A, M, ...)                                                  \
  if (!(A)) {                                                                  \
    log_err(M "\n", ##__VA_ARGS__);                                            \
    exit(1);                                                                   \
  }

#define check_debug(A, M, ...)                                                 \
  if (!(A)) {                                                                  \
    debug(M "\n", ##__VA_ARGS__);                                              \
  }

#endif