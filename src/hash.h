#ifndef HASH_H
#define HASH_H

#define INITIAL_SIZE 128
#define GROWTH_FACTOR 2
#define MAX_LOAD_FACTOR 0.7

typedef struct dictEle {
  const char *key;
  void *value;
  struct dictEle *next;
} dictEle;

typedef struct neDict {
  int capacity;
  int size; // number of elements stored
  struct dictEle **table;
} neDict;

neDict *dictCreate(void);
void dictDestory(neDict *dict);
void dictInsert(neDict *dict, const char *key, void *value);
void *dictSearch(neDict *dict, const char *key);
void dictDelete(neDict *dict, const char *key);

#endif