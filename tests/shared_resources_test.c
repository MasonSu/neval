#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct test {
  char *str;
};

int main(void) {
  struct test *st = malloc(sizeof(struct test));

  char *s1 = "hello";
  st->str = s1;
  printf("%s\n", st->str);

  char t[10] = "world";
  s1 = t;
  printf("%s\n", st->str);

  char s2[10] = "good";
  st->str = s2;
  printf("%s\n", st->str);

  strncpy(s2, "job", strlen(s2));
  printf("%s\n", st->str);

  return 0;
}