#include "hash.h"
#include "log.h"

typedef void (*func)(int, int);

void add(int a, int b) { printf("a + b = %d\n", a + b); }

void sub(int a, int b) { printf("a - b = %d\n", a - b); }

int main(void) {
  neDict *dict = dictCreate();

  dictInsert(dict, "aaa", "bbb");
  log_info("the value of aaa is: %s", (char *)dictSearch(dict, "aaa"));

  dictInsert(dict, "aaa", "ccc");
  log_info("the value of aaa is: %s", (char *)dictSearch(dict, "aaa"));

  dictInsert(dict, "eee", "fff");
  log_info("the value of eee is: %s", (char *)dictSearch(dict, "eee"));

  log_info("the size of dict is %d", dict->size);

  dictDelete(dict, "aaa");
  log_info("the size of dict is %d", dict->size);

  dictInsert(dict, "add", add);
  func testfunc = dictSearch(dict, "add");
  testfunc(2, 3);

  dictInsert(dict, "sub", sub);
  testfunc = dictSearch(dict, "sub");
  testfunc(2, 3);
  /* If you want to do this test, you must
   * change insert function to
   * e->key = strdup(key);
   * e->value = strdup(value);
   */
  // char buf[512];
  // for (int i = 0; i < 10000; i++) {
  //   sprintf(buf, "%d", i);
  //   dictInsert(dict, buf, buf);
  // }

  // log_info("the size of dict is %d", dict->size);
  // log_info("the capacity of dict is %d", dict->capacity);
  // log_info("the value of eee is: %s", (char *)dictSearch(dict, "eee"));

  // log_info("the value of 555 is: %s", (char *)dictSearch(dict, "555"));
  // log_info("the value of 423 is: %s", (char *)dictSearch(dict, "423"));

  dictDestory(dict);

  return 0;
}