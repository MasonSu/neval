#include "ne.h"
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>

typedef struct neApiState {
  fd_set rfds, wfds;
  /* We need to have a copy of the fd sets as it's not safe
   * to reuse FD sets after select()
   */
  fd_set _rfds, _wfds;
} neApiState;

static int neApiCreate(neEventLoop *eventLoop) {
  neApiState *state = malloc(sizeof(neApiState));

  if (!state)
    return -1;
  FD_ZERO(&state->rfds);
  FD_ZERO(&state->wfds);
  eventLoop->apidata = state;

  return 0;
}

static int neApiResize(neEventLoop *eventLoop, int setsize) {
  (void)eventLoop;
  /* Just ensure we have enough room in the fd_set type */
  if (setsize >= FD_SETSIZE)
    return -1;

  return 0;
}

static void neApiFree(neEventLoop *eventLoop) { free(eventLoop->apidata); }

static int neApiAddEvent(neEventLoop *eventLoop, int fd, int mask) {
  neApiState *state = eventLoop->apidata;

  if (mask & NE_READABLE)
    FD_SET(fd, &state->rfds);
  if (mask & NE_WRITABLE)
    FD_SET(fd, &state->wfds);

  return 0;
}

static void neApiDelEvent(neEventLoop *eventLoop, int fd, int mask) {
  neApiState *state = eventLoop->apidata;

  if (mask & NE_READABLE)
    FD_CLR(fd, &state->rfds);
  if (mask & NE_WRITABLE)
    FD_CLR(fd, &state->wfds);
}

static int neApiPoll(neEventLoop *eventLoop, long long ms) {
  neApiState *state = eventLoop->apidata;
  int retval, numevents = 0;
/* https://stackoverflow.com/questions/5080848 */
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
  struct timeval *tvp;

  if (ms >= 0) {
    tvp->tv_sec = ms / 1000;
    tvp->tv_usec = (ms % 1000) * 1000;
  } else {
    tvp = NULL;
  }

  memcpy(&state->_rfds, &state->rfds, sizeof(fd_set));
  memcpy(&state->_wfds, &state->wfds, sizeof(fd_set));

  retval =
      select(eventLoop->maxfd + 1, &state->_rfds, &state->_wfds, NULL, tvp);
  if (retval > 0) {
    for (int i = 0; i <= eventLoop->maxfd; i++) {
      int mask = 0;
      neFileEvent *fe = &eventLoop->events[i];

      if (fe->mask == NE_NONE)
        continue;
      if (fe->mask & NE_READABLE && FD_ISSET(i, &state->_rfds))
        mask |= NE_READABLE;
      if (fe->mask & NE_WRITABLE && FD_ISSET(i, &state->_wfds))
        mask |= NE_WRITABLE;
      eventLoop->fired[numevents].fd = i;
      eventLoop->fired[numevents].mask = mask;
      numevents++;
    }
  }
  return numevents;
}

static char *neApiName(void) { return "select"; }