#include "hash.h"
#include "log.h"
#include <stdlib.h>
#include <string.h>

/* http://www.cse.yorku.ca/~oz/hash.html */
static unsigned long hash_function(const char *s) {
  unsigned long hash = 5381;
  unsigned char *str = (unsigned char *)s;

  while (*str) {
    hash = ((hash << 5) + hash) + *str;
    str++;
  }

  return hash;
}

/* dictionary initialization code used in both dictCreate and grow */
neDict *internalDictCreate(int size) {
  neDict *dict = (neDict *)malloc(sizeof(neDict));
  if (dict == NULL)
    goto err;

  dict->table = (dictEle **)malloc(sizeof(dictEle *) * size);
  if (dict->table == NULL)
    goto err;

  dict->capacity = size;
  dict->size = 0;

  for (int i = 0; i < size; i++)
    dict->table[i] = NULL;

  return dict;

err:
  if (dict) {
    free(dict->table);
    free(dict);
  }
  return NULL;
}

neDict *dictCreate(void) { return internalDictCreate(INITIAL_SIZE); }

void dictDestory(neDict *dict) {
  for (int i = 0; i < dict->capacity; i++) {
    dictEle *next;
    for (dictEle *e = dict->table[i]; e != NULL; e = next) {
      next = e->next;

      free(e);
    }
  }

  free(dict->table);
  free(dict);

  return;
}

/* It can not be done by simple realloc!!!
 * the capacity has been changed, so the index
 * has been changed(the value doesn't sit on
 * the old position)
 */
static void grow(neDict *dict) {
  neDict *newDict = internalDictCreate(dict->capacity * GROWTH_FACTOR);

  if (newDict == NULL) {
    log_err("grow failure");
    return;
  }

  for (int i = 0; i < dict->capacity; i++) {
    for (dictEle *e = dict->table[i]; e != NULL; e = e->next) {
      dictInsert(newDict, e->key, e->value);
    }
  }

  neDict tmp = *dict;
  *dict = *newDict;
  *newDict = tmp;

  dictDestory(newDict);
}

void dictInsert(neDict *dict, const char *key, void *value) {
  dictEle *e = (dictEle *)malloc(sizeof(dictEle));
  if (e == NULL) {
    log_err("dictInsert error");
    return;
  }

  /* BE CAREFUL, if key or value changed, the
   * element in the hash table will be changed,
   * see shared_resources_test.c
   */
  e->key = key;
  e->value = value;

  int pos = hash_function(key) % dict->capacity;
  e->next = dict->table[pos];
  dict->table[pos] = e;

  dict->size++;

  if (dict->size >= dict->capacity * MAX_LOAD_FACTOR)
    grow(dict);

  return;
}

/* return the most recently inserted value associated with a key*/
void *dictSearch(neDict *dict, const char *key) {
  int pos = hash_function(key) % dict->capacity;

  for (dictEle *e = dict->table[pos]; e != NULL; e = e->next) {
    if (strcmp(e->key, key) == 0)
      return e->value;
  }

  return NULL;
}

/* delete all record with the given key.
 * if there is no such record, has no effect
 */
void dictDelete(neDict *dict, const char *key) {
  struct dictEle **curr, *prev;
  int pos = hash_function(key) % dict->capacity;

  for (curr = &(dict->table[pos]); *curr != NULL;) {
    if (strcmp((*curr)->key, key) == 0) {
      prev = *curr;
      *curr = prev->next;
      free(prev);

      dict->size--;
      continue;
    }
    curr = &((*curr)->next);
  }

  return;
}