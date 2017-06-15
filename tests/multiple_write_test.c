#define _BSD_SOURCE

#include "log.h"
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define NUM 500
#define LENGTH 100

int main(void) {
  int fd = open("/home/xiaxun/worksp/neval/test/output", O_CREAT | O_WRONLY);
  if (fd == -1)
    log_err("open file failure");

  char *str1 = malloc(sizeof(char) * LENGTH),
       *str2 = malloc(sizeof(char) * LENGTH);
  for (int i = 0; i < LENGTH - 1; i++) {
    str1[i] = '1';
    str2[i] = '2';
  }

  str1[LENGTH - 1] = '\n';
  // str1[LENGTH - 1] = '\0';
  str2[LENGTH - 1] = '\n';
  // str2[LENGTH - 2] = '\0';

  switch (fork()) {
  case -1:
    log_err("fork");

  case 0: /* Child */

    for (int i = 0; i < NUM; i++) {
      usleep(1000 * 20);
      write(fd, str1, strlen(str1));
    }

    _exit(EXIT_SUCCESS);

  default:
    break;
  }

  for (int j = 0; j < NUM; j++) {
    usleep(1000 * 20);
    write(fd, str2, strlen(str2));
  }

  wait(NULL);
  close(fd);

  exit(EXIT_SUCCESS);
}