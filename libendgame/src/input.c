#include "input.h"
#include <assert.h>
#include <endgame/event.h>
#include <endgame/input.h>
#include <errno.h>
#include <poll.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int eg_input_new(eg_input_t **me, FILE *in) {

  if (me == NULL)
    return EINVAL;

  if (in == NULL)
    return EINVAL;

  *me = NULL;
  eg_input_t *i = NULL;
  int rc = 0;

  i = calloc(1, sizeof(*i));
  if (i == NULL) {
    rc = ENOMEM;
    goto done;
  }

  i->in = in;

  // check we can `fileno` the stream
  if (fileno(in) < 0) {
    rc = errno;
    goto done;
  }

  *me = i;
  i = NULL;

done:
  eg_input_free(&i);

  return rc;
}

eg_event_t eg_input_read(eg_input_t *me, int tick) {

  if (me == NULL)
    return (eg_event_t){EG_EVENT_ERROR, EINVAL};

  // wait until we have some data on stdin or from the signal bouncer
  struct pollfd in[] = {{.fd = fileno(me->in), .events = POLLIN},
                        /* {.fd = signal_pipe[0], .events = POLLIN}*/};
  {
    nfds_t nfds = sizeof(in) / sizeof(in[0]);
    while (true) {
      const int r = poll(in, nfds, tick);
      if (r > 0)
        break;
      if (r == 0)
        return (eg_event_t){.type = EG_EVENT_TICK};
      if (errno == EINTR)
        continue;
      return (eg_event_t){EG_EVENT_ERROR, (uint32_t)errno};
    }
  }

#if 0
  // priority 2: did we get a signal?
  if (in[1].revents & POLLIN) {
    int signum = 0;
    do {
      ssize_t len = read(signal_pipe[0], &signum, sizeof(signum));
      if (len >= 0) {
        assert((size_t)len == sizeof(signum) && "incomplete read");

        // if this was the signal indicating a window resize, update our
        // knowledge of the dimensions
        if (signum == SIGWINCH) {
          int rc = set_window_size();
          if (rc != 0)
            return (eg_event_t){EG_EVENT_ERROR, (uint32_t)rc};
        }

        return (eg_event_t){EG_EVENT_SIGNAL, (uint32_t)signum};
      }
    } while (errno == EINTR);
    return (eg_event_t){EG_EVENT_ERROR, (uint32_t)errno};
  }
#endif

  assert(in[0].revents & POLLIN);

  // priority 3: read a character from stdin
  unsigned char buffer[4] = {
      0}; // enough for a UTF-8 character or escape sequence
  if (read(fileno(me->in), &buffer, 1) < 0)
    return (eg_event_t){EG_EVENT_ERROR, errno};

  // is this a multi-byte sequence?
  size_t more = 0;
  if (buffer[0] == 0x1b) {             // ESC
    more = 3;                          // up to 3 extra bytes
  } else if ((buffer[0] >> 3) == 30) { // 4-byte UTF-8
    more = 3;
  } else if ((buffer[0] >> 4) == 14) { // 3-byte UTF-8
    more = 2;
  } else if ((buffer[0] >> 5) == 6) { // 2-byte UTF-8
    more = 1;
  }

  if (more > 0) {
    // does stdin have remaining ready data?
    struct pollfd input[] = {{.fd = fileno(me->in), .events = POLLIN}};
    nfds_t nfds = sizeof(input) / sizeof(input[0]);
    if (poll(input, nfds, 0) > 0) {
      assert(more <= sizeof(buffer) / sizeof(buffer[0]) - 1);
      ssize_t ignored = read(fileno(me->in), &buffer[1], more);
      (void)ignored;
    }
  }

  // construct a key press event
  eg_event_t key = {.type = EG_EVENT_KEYPRESS};
  for (size_t i = 0; i < sizeof(buffer) / sizeof(buffer[0]); ++i)
    key.value |= (uint32_t)buffer[i] << (i * 8);
  return key;
}

void eg_input_free(eg_input_t **me) {

  if (me == NULL)
    return;

  if (*me == NULL)
    return;

  free(*me);
  *me = NULL;
}
