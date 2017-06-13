#ifndef LIST_H
#define LIST_H

typedef struct listNode {
  struct listNode *prev;
  struct listNode *next;
  void *value;
} listNode;

typedef struct list {
  listNode *head;
  listNode *tail;
  void (*free)(void *ptr);
  unsigned long len;
} list;

/* Functions implemented as macros */
#define listLength(l) ((l)->len)
#define listFirst(l) ((l)->head)
#define listLast(l) ((l)->tail)
#define listPrevNode(n) ((n)->prev)
#define listNextNode(n) ((n)->next)
#define listNodeValue(n) ((n)->value)
#define listSetFreeMethod(l, m) ((l)->free = (m))
#define listGetFree(l) ((l)->free)

list *listCreate(void);
void listRelease(list *list);
void listEmpty(list *list);
list *listAddNodeHead(list *list, void *value);
list *listAddNodeTail(list *list, void *value);
list *listInsertNode(list *list, listNode *old_node, void *value, int after);
void listDelNode(list *list, listNode *node);

#endif