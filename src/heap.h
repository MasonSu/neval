#ifndef HEAP_H
#define HEAP_H

#include <stddef.h>

#define NE_HEAP_DEFAULT_SIZE 64

typedef int (*ne_hp_comparator_pt)(void *pi, void *pj);

typedef struct minHeap {
  void **data;
  size_t size;
  size_t capacity;
  ne_hp_comparator_pt comp;
} minHeap;

int ne_hp_init(minHeap *ne_hp, ne_hp_comparator_pt comp, size_t size);
void *ne_hp_min(minHeap *ne_hp);
int ne_hp_delmin(minHeap *ne_hp);
int ne_hp_insert(minHeap *ne_hp, void *item);

inline int ne_hp_is_empty(minHeap *ne_hp) { return ne_hp->size == 0; }

inline size_t ne_hp_size(minHeap *ne_hp) { return ne_hp->size; }

#endif