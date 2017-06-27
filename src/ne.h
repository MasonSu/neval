#ifndef NE_H
#define NE_H

#include "heap.h"
#include <time.h>

#define NE_NONE 0
#define NE_READABLE 1
#define NE_WRITABLE 2
#define NE_ET 4

#define NE_FILE_EVENTS 1
#define NE_TIME_EVENTS 2
#define NE_ALL_EVENTS (NE_FILE_EVENTS | NE_TIME_EVENTS)
#define NE_DONT_WAIT 4

#define NE_MAX_CLIENT 1024

struct neEventLoop;

typedef void neFileProc(struct neEventLoop *eventLoop, int fd,
                        void *clientData);
typedef void neEventFinalizerProc(struct neEventLoop *eventLoop,
                                  void *clientData);
typedef int neTimeProc(struct neEventLoop *eventLoop, void *clientData);

typedef struct neFileEvent {
  int mask; // one of NE_(READABLE | WRITABLE)
  neFileProc *rfileProc;
  neFileProc *wfileProc;
  void *clientData;
} neFileEvent;

typedef struct neFiredEvent {
  int fd;
  int mask;
} neFiredEvent;

typedef struct neTimeEvent {
  long when_sec; // seconds
  long when_ms;  // milliseconds
  int deleted;
  neTimeProc *timeProc;
  void *clientData;
} neTimeEvent;

typedef struct neEventLoop {
  int maxfd;           // highest file descriptor currently registered
  int setsize;         // max number of file descriptors tracked
  time_t lastTime;     // Used to detect system clock skew
  neFileEvent *events; // Registered events
  neFiredEvent *fired; // Fired events
  minHeap timer;
  int stop;
  void *apidata; // This is used for polling API specific data
} neEventLoop;

neEventLoop *neCreateEventLoop(int setsize);
int neResizeSetSize(neEventLoop *eventLoop, int setsize);
void neDeleteEventLoop(neEventLoop *eventLoop);
void neStop(neEventLoop *eventLoop);
int neCreateFileEvent(neEventLoop *eventLoop, int fd, int mask,
                      neFileProc *proc, void *clientData);
void neDeleteFileEvent(neEventLoop *eventLoop, int fd, int mask);
int neProcessEvents(neEventLoop *eventLoop, int flags);
void neMain(neEventLoop *eventLoop);

int neCreateTimeEvent(neEventLoop *eventLoop, long long milliseconds,
                      neTimeProc *proc, void *clentData);
void neDeleteTimeEvent(void *clentData);
long long neSearchNearestTimer(neEventLoop *eventLoop);
int neProcessTimeEvents(neEventLoop *eventLoop);

#endif