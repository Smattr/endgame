#include "screen.h"
#include <assert.h>
#include <endgame/screen.h>
#include <errno.h>
#include <limits.h>
#include <poll.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

static int set_window_size(eg_screen_t *scr) {
  assert(scr != NULL);

  struct winsize ws = {0};
  if (ioctl(fileno(scr->out), TIOCGWINSZ, &ws) < 0)
    return errno;

  scr->rows = ws.ws_row;
  scr->columns = ws.ws_col;

  return 0;
}

int eg_screen_new(eg_screen_t **me, FILE *out, FILE *in) {

  if (me == NULL)
    return EINVAL;

  if (out == NULL)
    return EINVAL;

  if (in == NULL)
    return EINVAL;

  *me = NULL;
  eg_screen_t *s = NULL;
  int rc = 0;

  s = calloc(1, sizeof(*s));
  if (s == NULL) {
    rc = ENOMEM;
    goto done;
  }

  s->out = out;
  s->in = in;

  // check we can `fileno` both streams
  const int out_fd = fileno(out);
  if (out_fd < 0) {
    rc = errno;
    goto done;
  }
  const int in_fd = fileno(in);
  if (in_fd < 0) {
    rc = errno;
    goto done;
  }

  // drain any pending output so it will not be subject to our termios changes
  if (fflush(out) < 0) {
    rc = errno;
    goto done;
  }

  // determine terminal dimensions
  if ((rc = set_window_size(s)))
    goto done;

  // read terminal characteristics
  if (tcgetattr(out_fd, &s->original_termios) < 0) {
    rc = errno;
    goto done;
  }

  // End of read-only actions. Anything after this point should attempt to be
  // undone on failure.

  // set terminal characteristics
  {
    struct termios new = s->original_termios;
    new.c_lflag &= ~ECHO;   // turn off echo
    new.c_lflag &= ~ICANON; // turn off canonical mode
    if (tcsetattr(out_fd, TCSANOW, &new) < 0) {
      rc = errno;
      goto done;
    }
  }

  s->active = true;

  // switch to the alternate screen
  if (fprintf(out, "\033[?1049h") < 0) {
    rc = EIO;
    goto done;
  }

  // hide the cursor
  if (fprintf(out, "\033[?25l") < 0) {
    rc = EIO;
    goto done;
  }

  if ((rc = eg_screen_clear(s)))
    goto done;

  // ensure our changes take effect
  if (fflush(out) < 0) {
    rc = errno;
    goto done;
  }

  *me = s;
  s = NULL;

done:
  eg_screen_free(&s);

  return rc;
}

size_t eg_screen_get_columns(eg_screen_t *me) {
  assert(me != NULL);
  return me->columns;
}

size_t eg_screen_get_rows(eg_screen_t *me) {
  assert(me != NULL);
  return me->rows;
}

int eg_screen_put(eg_screen_t *me, size_t x, size_t y, const char *text,
                  size_t len) {
  if (me == NULL)
    return EINVAL;
  if (len > INT_MAX)
    return ERANGE;
  if (x > me->columns)
    return ERANGE;
  if (y > me->rows)
    return ERANGE;
  if (fprintf(me->out, "\033[%zu;%zuH%.*s", y, x, (int)len, text) < 0)
    return EIO;
  return 0;
}

int eg_screen_puts(eg_screen_t *me, size_t x, size_t y, const char *text) {
  return eg_screen_put(me, x, y, text, strlen(text));
}

int eg_screen_sync(eg_screen_t *me) {

  if (me == NULL)
    return EINVAL;

  if (fflush(me->out) < 0)
    return errno;

  return 0;
}

eg_event_t eg_screen_read(eg_screen_t *me, int tick) {

  if (me == NULL)
    return (eg_event_t){EG_EVENT_ERROR, EINVAL};

  if (!me->active)
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

int eg_screen_clear(eg_screen_t *me) {

  if (me == NULL)
    return EINVAL;

  // clear screen and move to upper left
  if (fprintf(me->out, "\033[2J") < 0)
    return EIO;

  return 0;
}

void eg_screen_free(eg_screen_t **me) {

  if (me == NULL)
    return;

  if (*me == NULL)
    return;

  if ((*me)->active) {

    // drain anything pending to avoid it coming out once we switch away from
    // the alternate screen
    fflush((*me)->out);

    (void)eg_screen_clear(*me);

    // show the cursor
    fprintf((*me)->out, "\033[?25h");

    // switch out of the alternate screen
    fprintf((*me)->out, "\033[?1049l");

    // restore the original terminal characteristics
    (void)tcsetattr(fileno((*me)->out), TCSANOW, &(*me)->original_termios);

    fflush((*me)->out);
  }

  free(*me);
  *me = NULL;
}
