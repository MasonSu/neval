#include "ne.h"
#include <stdlib.h>
#include <sys/epoll.h>
#include <unistd.h>

/* turn off warning: defined but not used [-Wunused-function] */
#ifdef __GNUC__
#define FUNCTION_IS_NOT_USED __attribute__((unused))
#else
#define FUNCTION_IS_NOT_USED
#endif

static int neApiCreate(neEventLoop *eventLoop) FUNCTION_IS_NOT_USED;
static int neApiResize(neEventLoop *eventLoop,
                       int setsize) FUNCTION_IS_NOT_USED;
static void neApiFree(neEventLoop *eventLoop) FUNCTION_IS_NOT_USED;
static int neApiAddEvent(neEventLoop *eventLoop, int fd,
                         int mask) FUNCTION_IS_NOT_USED;
static void neApiDelEvent(neEventLoop *eventLoop, int fd,
                          int delmask) FUNCTION_IS_NOT_USED;
static int neApiPoll(neEventLoop *eventLoop, long long ms) FUNCTION_IS_NOT_USED;

typedef struct neApiState {
  int epfd;
  struct epoll_event *events;
} neApiState;

int neApiCreate(neEventLoop *eventLoop) {
  neApiState *state = malloc(sizeof(neApiState));
  if (!state)
    return -1;

  state->events = malloc(sizeof(struct epoll_event) * eventLoop->setsize);
  if (!state->events) {
    free(state);
    return -1;
  }

  state->epfd = epoll_create(1024); // 1024 is just a hint for the kernel
  if (state->epfd == -1) {
    free(state->events);
    free(state);
    return -1;
  }

  eventLoop->apidata = state;
  return 0;
}

int neApiResize(neEventLoop *eventLoop, int setsize) {
  neApiState *state = eventLoop->apidata;

  state->events = realloc(state->events, sizeof(struct epoll_event) * setsize);
  if (!state->events)
    return -1;
  return 0;
}

void neApiFree(neEventLoop *eventLoop) {
  neApiState *state = eventLoop->apidata;

  close(state->epfd);
  free(state->events);
  free(state);
}

int neApiAddEvent(neEventLoop *eventLoop, int fd, int mask) {
  neApiState *state = eventLoop->apidata;
  struct epoll_event ee = {0};

  /* If the fd was already monitored for some event, we need
   * a MOD opreation. Otherwise we need an ADD operation.
   */
  int op =
      eventLoop->events[fd].mask == NE_NONE ? EPOLL_CTL_ADD : EPOLL_CTL_MOD;
  ee.events = 0;
  mask |= eventLoop->events[fd].mask; // Merge old events
  if (mask & NE_READABLE)
    ee.events |= EPOLLIN;
  if (mask & NE_WRITABLE)
    ee.events |= EPOLLOUT;
  if (mask & NE_ET)
    ee.events |= EPOLLET;
  ee.data.fd = fd;

  return epoll_ctl(state->epfd, op, fd, &ee);
}

void neApiDelEvent(neEventLoop *eventLoop, int fd, int delmask) {
  neApiState *state = eventLoop->apidata;
  struct epoll_event ee = {0};

  int mask = eventLoop->events[fd].mask & (~delmask);
  ee.events = 0;
  if (mask & NE_READABLE)
    ee.events |= EPOLLIN;
  if (mask & NE_WRITABLE)
    ee.events |= EPOLLOUT;
  if (mask & NE_ET)
    ee.events |= EPOLLET;
  ee.data.fd = fd;
  if (mask == NE_NONE)
    epoll_ctl(state->epfd, EPOLL_CTL_DEL, fd, &ee);
  else
    epoll_ctl(state->epfd, EPOLL_CTL_MOD, fd, &ee);
}

int neApiPoll(neEventLoop *eventLoop, long long ms) {
  neApiState *state = eventLoop->apidata;
  int retval, numevents = 0;

  retval = epoll_wait(state->epfd, state->events, eventLoop->setsize, ms);
  if (retval > 0) {
    numevents = retval;
    for (int i = 0; i < numevents; i++) {
      int mask = 0;
      struct epoll_event *e = state->events + i;

      if (e->events & EPOLLIN)
        mask |= NE_READABLE;
      if (e->events & EPOLLOUT || e->events & EPOLLERR || e->events & EPOLLHUP)
        mask |= NE_WRITABLE;

      eventLoop->fired[i].fd = e->data.fd;
      eventLoop->fired[i].mask = mask;
    }
  }
  return numevents;
}