#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "http_request.h"
#include "log.h"
#include "ne.h"

#include "ne_epoll.c"

#define NE_TIMER_INFINITE -1

static int timer_comp(void *ti, void *tj) {
  neTimeEvent *timeri = (neTimeEvent *)ti, *timerj = (neTimeEvent *)tj;

  return (timeri->when_sec < timerj->when_sec ||
          (timeri->when_sec == timerj->when_sec &&
           timeri->when_ms < timerj->when_ms));
}

neEventLoop *neCreateEventLoop(int setsize) {
  neEventLoop *eventLoop;

  if ((eventLoop = malloc(sizeof(neEventLoop))) == NULL)
    goto err;
  eventLoop->events = malloc(sizeof(neFileEvent) * setsize);
  eventLoop->fired = malloc(sizeof(neFiredEvent) * setsize);
  if (eventLoop->events == NULL || eventLoop->fired == NULL)
    goto err;

  if (ne_hp_init(&eventLoop->timer, timer_comp, NE_HEAP_DEFAULT_SIZE) == -1)
    goto err;

  eventLoop->setsize = setsize;
  eventLoop->lastTime = time(NULL);
  eventLoop->stop = 0;
  eventLoop->maxfd = -1;

  if (neApiCreate(eventLoop) == -1)
    goto err;

  for (int i = 0; i < setsize; i++)
    eventLoop->events[i].mask = NE_NONE;
  return eventLoop;

err:
  if (eventLoop) {
    free(eventLoop->events);
    free(eventLoop->fired);
    free(eventLoop);
  }
  return NULL;
}

int neResizeSetSize(neEventLoop *eventLoop, int setsize) {
  if (setsize == eventLoop->setsize)
    return NE_OK;
  if (eventLoop->maxfd >= setsize)
    return NE_ERR;
  if (neApiResize(eventLoop, setsize) == -1) {
    log_err("neResizeSetSize");
    return NE_ERR;
  }

  eventLoop->events = realloc(eventLoop->events, sizeof(neFileEvent) * setsize);
  eventLoop->fired = realloc(eventLoop->fired, sizeof(neFiredEvent) * setsize);
  eventLoop->setsize = setsize;

  /* Make sure that if we created new slots, they are an NE_NONE mask */
  for (int i = eventLoop->maxfd + 1; i < setsize; i++)
    eventLoop->events[i].mask = NE_NONE;

  return NE_OK;
}

void neDeleteEventLoop(neEventLoop *eventLoop) {
  neApiFree(eventLoop);
  free(eventLoop->events);
  free(eventLoop->fired);
  free(eventLoop);
}

void neStop(neEventLoop *eventLoop) { eventLoop->stop = 1; }

int neCreateFileEvent(neEventLoop *eventLoop, int fd, int mask,
                      neFileProc *proc, void *clientData) {
  if (fd >= eventLoop->setsize) {
    log_err("fd >= eventLoop->setsize");
    errno = ERANGE;
    return NE_ERR;
  }

  neFileEvent *fe = &eventLoop->events[fd];
  if (neApiAddEvent(eventLoop, fd, mask) == -1)
    return NE_ERR;

  fe->mask |= mask;
  if (mask & NE_READABLE)
    fe->rfileProc = proc;
  if (mask & NE_WRITABLE)
    fe->wfileProc = proc;
  fe->clientData = clientData;

  if (fd > eventLoop->maxfd)
    eventLoop->maxfd = fd;
  return NE_OK;
}

void neDeleteFileEvent(neEventLoop *eventLoop, int fd, int mask) {
  if (fd >= eventLoop->setsize)
    return;

  neFileEvent *fe = &eventLoop->events[fd];
  if (fe->mask == NE_NONE)
    return;

  neApiDelEvent(eventLoop, fd, mask);

  fe->mask = fe->mask & (~mask);
  if (fd == eventLoop->maxfd && fe->mask == NE_NONE) {
    int i;
    for (i = eventLoop->maxfd - 1; i >= 0; i--)
      if (eventLoop->events[i].mask != NE_NONE)
        break;

    eventLoop->maxfd = i;
  }
}

static void neGetTime(long *seconds, long *milliseconds) {
  struct timeval tv;

  gettimeofday(&tv, NULL);
  *seconds = tv.tv_sec;
  *milliseconds = tv.tv_usec / 1000;
}

static void neAddMillisecondsToNow(long long milliseconds, long *sec,
                                   long *ms) {
  long cur_sec, cur_ms, when_sec, when_ms;

  neGetTime(&cur_sec, &cur_ms);
  when_sec = cur_sec + milliseconds / 1000;
  when_ms = cur_ms + milliseconds % 1000;
  if (when_ms >= 1000) {
    when_sec++;
    when_ms -= 1000;
  }

  *sec = when_sec;
  *ms = when_ms;
}

int neCreateTimeEvent(neEventLoop *eventLoop, long long milliseconds,
                      neTimeProc *proc, void *clientData) {
  neTimeEvent *te = (neTimeEvent *)malloc(sizeof(neTimeEvent));
  if (te == NULL)
    return NE_ERR;
  debug("neCreateTimeEvent");
  neAddMillisecondsToNow(milliseconds, &te->when_sec, &te->when_ms);
  te->timeProc = proc;
  te->clientData = clientData;
  te->deleted = 0;

  int rc = ne_hp_insert(&eventLoop->timer, te);
  check_exit(rc == 0, "neCreateTimeEvent failed");

  ne_http_request *request = (ne_http_request *)clientData;
  request->timer = te;

  return NE_OK;
}

void neDeleteTimeEvent(void *clientData) {
  ne_http_request *request = (ne_http_request *)clientData;

  request->timer->deleted = 1;
}

long long neSearchNearestTimer(neEventLoop *eventLoop) {
  minHeap *ne_hp = &eventLoop->timer;
  long long time = NE_TIMER_INFINITE;

  while (!ne_hp_is_empty(ne_hp)) {
    neTimeEvent *timeNode = (neTimeEvent *)ne_hp_min(ne_hp);
    check(timeNode != NULL, "ne_hp_min error");

    if (timeNode->deleted) {
      ne_hp_delmin(ne_hp);
      free(timeNode);
      continue;
    }

    long now_sec, now_ms;
    neGetTime(&now_sec, &now_ms);
    /* How many milliseconds we need to wait for the next
     * time event to fire
     */
    time = (timeNode->when_sec - now_sec) * 1000 + timeNode->when_ms - now_ms;
    time = time > 0 ? time : 0;
    break;
  }

  return time;
}

int neProcessTimeEvents(neEventLoop *eventLoop) {
  debug("neProcessTimeEvents");
  int processed = 0;
  minHeap *ne_hp = &eventLoop->timer;

  while (!ne_hp_is_empty(ne_hp)) {
    debug("handle timer");
    neTimeEvent *timeNode = (neTimeEvent *)ne_hp_min(ne_hp);
    check(timeNode != NULL, "ne_hp_min error");

    if (timeNode->deleted) {
      ne_hp_delmin(ne_hp);
      free(timeNode);
      continue;
    }

    long now_sec, now_ms;
    neGetTime(&now_sec, &now_ms);

    if (timeNode->when_sec > now_sec ||
        (timeNode->when_sec == now_sec && timeNode->when_ms > now_ms))
      return processed;

    if (timeNode->timeProc)
      timeNode->timeProc(eventLoop, timeNode->clientData);

    /* Delete timeNode directly */
    ne_hp_delmin(ne_hp);
    free(timeNode);

    processed++;
  }

  return processed;
}

int neProcessEvents(neEventLoop *eventLoop, int flags) {
  int processed = 0;

  if (!(flags & NE_TIME_EVENTS) && !(flags & NE_FILE_EVENTS))
    return 0;

  long long time = neSearchNearestTimer(eventLoop);
  int numevents = neApiPoll(eventLoop, time);
  debug("numevents = %d", numevents);
  for (int i = 0; i < numevents; i++) {
    int fd = eventLoop->fired[i].fd;
    int mask = eventLoop->fired[i].mask;
    neFileEvent *fe = &eventLoop->events[fd];

    if (fe->mask & mask & NE_READABLE) {
      debug("read event");
      fe->rfileProc(eventLoop, fd, fe->clientData);
    }

    if (fe->mask & mask & NE_WRITABLE) {
      debug("write event");
      fe->wfileProc(eventLoop, fd, fe->clientData);
    }

    processed++;
  }

  if (flags & NE_TIME_EVENTS)
    processed += neProcessTimeEvents(eventLoop);

  return processed;
}

void neMain(neEventLoop *eventLoop) {
  eventLoop->stop = 0;
  while (!eventLoop->stop)
    neProcessEvents(eventLoop, NE_ALL_EVENTS);
}