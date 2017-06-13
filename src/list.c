#include "list.h"
#include <stdlib.h>

list *listCreate(void) {
  struct list *list;
  if ((list = malloc(sizeof(struct list))) == NULL)
    return NULL;
  list->head = list->tail = NULL;
  list->free = NULL;
  list->len = 0;
  return list;
}

/**
 * Remove all the elements from the list without destroying
 * the list itself;
 */
void listEmpty(list *list) {
  listNode *current = list->head, *next;
  unsigned long len = list->len;
  while (len--) {
    next = current->next;
    if (list->free)
      list->free(current->value);
    free(current);
    current = next;
  }
  list->head = list->tail = NULL;
  list->len = 0;
}

/**
 * Free the whole list
 */
void listRelease(list *list) {
  listEmpty(list);
  free(list);
}

/**
 * Add a new node to the list, to head
 * On error, NULL is returned and no operation is performed
 * On success the 'list' pointer you pass to the function is returned
 */
list *listAddNodeHead(list *list, void *value) {
  listNode *node;
  if ((node = malloc(sizeof(listNode))) == NULL)
    return NULL;
  node->value = value;
  if (list->len == 0) {
    list->head = list->tail = node;
    node->prev = node->next = NULL;
  } else {
    node->prev = NULL;
    node->next = list->head;
    list->head->prev = node;
    list->head = node;
  }
  list->len++;
  return list;
}

/**
 * Add a new node to the list, to tail
 * On error, NULL is returned and no operation is performed
 * On success the 'list' pointer you pass to the function is returned
 */
list *listAddNodeTail(list *list, void *value) {
  listNode *node;
  if ((node = malloc(sizeof(listNode))) == NULL)
    return NULL;
  node->value = value;
  if (list->len == 0) {
    list->head = list->tail = node;
    node->prev = node->next = NULL;
  } else {
    node->prev = list->tail;
    node->next = NULL;
    list->tail->next = node;
    list->tail = node;
  }
  list->len++;
  return list;
}

list *listInsertNode(list *list, listNode *old_node, void *value, int after) {
  listNode *node;
  if ((node = malloc(sizeof(listNode))) == NULL)
    return NULL;
  node->value = value;
  if (after) {
    node->prev = old_node;
    node->next = old_node->next;
    if (list->tail == old_node)
      list->tail = node;
  } else {
    node->next = old_node;
    node->prev = old_node->prev;
    if (list->head == old_node)
      list->head = node;
  }
  if (node->prev != NULL)
    node->prev->next = node;
  if (node->next != NULL)
    node->next->prev = node;
  list->len++;
  return list;
}

void listDelNode(list *list, listNode *node) {
  if (node->prev)
    node->prev->next = node->next;
  else
    list->head = node->next;
  if (node->next)
    node->next->prev = node->prev;
  else
    list->tail = node->prev;
  if (list->free)
    list->free(node->value);
  free(node);
  list->len--;
}