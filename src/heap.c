#include "heap.h"
#include "log.h"
#include <stdlib.h>

extern inline int ne_hp_is_empty(minHeap *ne_hp);
extern inline size_t ne_hp_size(minHeap *ne_hp);

int ne_hp_init(minHeap *ne_hp, ne_hp_comparator_pt comp, size_t size) {
  ne_hp->data = (void **)malloc(sizeof(void *) * (size + 1));
  if (!ne_hp->data) {
    log_err("ne_hp_init: malloc failed");
    return -1;
  }

  ne_hp->size = 0;
  ne_hp->capacity = size + 1;
  ne_hp->comp = comp;

  return NE_OK;
}

void *ne_hp_min(minHeap *ne_hp) {
  if (ne_hp_is_empty(ne_hp))
    return NULL;
  return ne_hp->data[1];
}

static int ne_hp_resize(minHeap *ne_hp, size_t new_size) {
  if (new_size <= ne_hp->size) {
    log_err("resise: new_size too small");
    return -1;
  }
  /* Maybe it can be done by realloc */
  void **new_ptr = (void **)malloc(sizeof(void *) * new_size);
  if (!new_ptr) {
    log_err("resize: malloc failed");
    return -1;
  }

  memcpy(new_ptr, ne_hp->data, sizeof(void *) * (ne_hp->size + 1));
  free(ne_hp->data);
  ne_hp->data = new_ptr;
  ne_hp->capacity = new_size;

  debug("the capacity of heap is %ld and the size of heap is %ld",
        ne_hp->capacity, ne_hp->size);

  return NE_OK;
}

static void swap(minHeap *ne_hp, size_t i, size_t j) {
  void *tmp = ne_hp->data[i];
  ne_hp->data[i] = ne_hp->data[j];
  ne_hp->data[j] = tmp;
}

static void bubbleUp(minHeap *ne_hp, size_t k) {
  while (k > 1 && ne_hp->comp(ne_hp->data[k], ne_hp->data[k / 2])) {
    swap(ne_hp, k, k / 2);
    k /= 2;
  }
}

static void bubbleDown(minHeap *ne_hp, size_t k) {
  size_t size = ne_hp->size;

  while (2 * k <= size) {
    size_t j = 2 * k;
    if (j < size && ne_hp->comp(ne_hp->data[j + 1], ne_hp->data[j]))
      j++;
    if (ne_hp->comp(ne_hp->data[k], ne_hp->data[j]))
      break;
    swap(ne_hp, k, j);
    k = j;
  }
}

int ne_hp_delmin(minHeap *ne_hp) {
  debug("ne_hp_delmin, the size of minHeap is %d", ne_hp->size);
  swap(ne_hp, 1, ne_hp->size);
  ne_hp->size--;
  bubbleDown(ne_hp, 1);

  if (ne_hp->size > 0 && ne_hp->size <= (ne_hp->capacity - 1) / 4) {
    if (ne_hp_resize(ne_hp, ne_hp->capacity / 2) == -1)
      return -1;
  }

  return NE_OK;
}

int ne_hp_insert(minHeap *ne_hp, void *item) {
  if (ne_hp->size + 1 == ne_hp->capacity) {
    if (ne_hp_resize(ne_hp, ne_hp->size * 2) == -1)
      return -1;
  }

  ne_hp->data[++ne_hp->size] = item;
  bubbleUp(ne_hp, ne_hp->size);

  return NE_OK;
}