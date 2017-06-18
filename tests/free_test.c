#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct str_a {
  int a;
  long b;
  char buf[10];
};

int main() {
  struct str_a *str1 = malloc(sizeof(struct str_a));
  printf("%p\n", str1);
  strncpy(str1->buf, "hello", sizeof(str1->buf));

  free(str1);

  struct str_a *str3 = malloc(sizeof(struct str_a));
  printf("%p, %s\n", str3, str3->buf);

  struct str_a *str5 = malloc(sizeof(struct str_a));
  printf("%p, %s\n", str5, str5->buf);

  free(str3);
  free(str5);

  return 0;
}