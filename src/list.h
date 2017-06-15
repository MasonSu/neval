#ifndef LIST_H
#define LIST_H

typedef struct listNode {
  struct listNode *prev;
  struct listNode *next;
  void *value;
} listNode;

typedef struct neList {
  listNode *head;
  listNode *tail;
  void (*free)(void *ptr);
  unsigned long len;
} neList;

/* Functions implemented as macros */
#define listLength(l) ((l)->len)
#define listFirst(l) ((l)->head)
#define listLast(l) ((l)->tail)
#define listPrevNode(n) ((n)->prev)
#define listNextNode(n) ((n)->next)
#define listNodeValue(n) ((n)->value)
#define listSetFreeMethod(l, m) ((l)->free = (m))
#define listGetFree(l) ((l)->free)

neList *listCreate(void);
void listRelease(neList *list);
void listEmpty(neList *list);
neList *listAddNodeHead(neList *list, void *value);
neList *listAddNodeTail(neList *list, void *value);
neList *listInsertNode(neList *list, listNode *old_node, void *value,
                       int after);
void listDelNode(neList *list, listNode *node);

#endif