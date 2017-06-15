#include "log.h"
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

//#define OPENBEFOREFORK
//#define OPENAFTERFORK

int main(void) {
  setbuf(stdout, NULL);

#ifdef OPENBEFOREFORK

  int fd = open("/home/xiaxun/worksp/neval/test/makefile", O_RDONLY);
  if (fd == -1)
    log_err("open file failure");

  switch (fork()) {
  case -1:
    log_err("fork");

  case 0: /* Child */
    log_info("Child do something");
    int offset = lseek(fd, 6, SEEK_SET);
    log_info("Child set offset to %d", offset);

    _exit(EXIT_SUCCESS);

  default:
    break;
  }
  wait(NULL);

  int offset = lseek(fd, 0, SEEK_CUR);
  log_info("The offset in parent process is %d", offset);

  exit(EXIT_SUCCESS);

#endif

#ifdef OPENAFTERFORK

  switch (fork()) {
  case -1:
    log_err("fork");

  case 0: /* Child */
    log_info("Child do something");
    int fd = open("/home/xiaxun/worksp/neval/test/makefile", O_RDONLY);
    if (fd == -1)
      log_err("open file failure");
    int offset = lseek(fd, 6, SEEK_SET);
    log_info("Child set offset to %d", offset);
    sleep(10);
    log_info("Child exit");
    _exit(EXIT_SUCCESS);

  default:
    break;
  }
  sleep(5);
  log_info("Parent do something");
  int fd1 = open("/home/xiaxun/worksp/neval/test/makefile", O_RDONLY);
  if (fd1 == -1)
    log_err("open file failure");
  int offset = lseek(fd1, 0, SEEK_CUR);
  log_info("The offset in parent process is %d", offset);
  wait(NULL);
  exit(EXIT_SUCCESS);

#endif

  return 0;
}
