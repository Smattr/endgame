#include "output.h"
#include <assert.h>
#include <endgame/output.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

static int set_window_size(eg_output_t *out) {
  assert(out != NULL);

  struct winsize ws = {0};
  if (ioctl(fileno(out->out), TIOCGWINSZ, &ws) < 0)
    return errno;

  out->rows = ws.ws_row;
  out->columns = ws.ws_col;

  return 0;
}

int eg_output_new(eg_output_t **me, FILE *out) {

  if (me == NULL)
    return EINVAL;

  if (out == NULL)
    return EINVAL;

  *me = NULL;
  eg_output_t *o = NULL;
  int rc = 0;

  o = calloc(1, sizeof(*o));
  if (o == NULL) {
    rc = ENOMEM;
    goto done;
  }

  o->out = out;

  // check we can `fileno` the stream
  const int fd = fileno(out);
  if (fd < 0) {
    rc = errno;
    goto done;
  }

  // drain any pending output so it will not be subject to our termios changes
  if (fflush(out) < 0) {
    rc = errno;
    goto done;
  }

  // determine terminal dimensions
  if ((rc = set_window_size(o)))
    goto done;

  // read terminal characteristics
  if (tcgetattr(fd, &o->original_termios) < 0) {
    rc = errno;
    goto done;
  }

  // End of read-only actions. Anything after this point should attempt to be
  // undone on failure.

  // set terminal characteristics
  {
    struct termios new = o->original_termios;
    new.c_lflag &= ~ECHO;   // turn off echo
    new.c_lflag &= ~ICANON; // turn off canonical mode
    if (tcsetattr(fd, TCSANOW, &new) < 0) {
      rc = errno;
      goto done;
    }
  }

  o->active = true;

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

  if ((rc = eg_output_clear(o)))
    goto done;

  // ensure our changes take effect
  if (fflush(out) < 0) {
    rc = errno;
    goto done;
  }

  *me = o;
  o = NULL;

done:
  eg_output_free(&o);

  return rc;
}

size_t eg_output_get_columns(const eg_output_t *me) {
  assert(me != NULL);
  return me->columns;
}

size_t eg_output_get_rows(const eg_output_t *me) {
  assert(me != NULL);
  return me->rows;
}

int eg_output_put(eg_output_t *me, size_t x, size_t y, const char *text,
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

int eg_output_puts(eg_output_t *me, size_t x, size_t y, const char *text) {
  return eg_output_put(me, x, y, text, strlen(text));
}

int eg_output_sync(eg_output_t *me) {

  if (me == NULL)
    return EINVAL;

  if (fflush(me->out) < 0)
    return errno;

  return 0;
}

int eg_output_clear(eg_output_t *me) {

  if (me == NULL)
    return EINVAL;

  // clear screen and move to upper left
  if (fprintf(me->out, "\033[2J") < 0)
    return EIO;

  return 0;
}

void eg_output_free(eg_output_t **me) {

  if (me == NULL)
    return;

  if (*me == NULL)
    return;

  if ((*me)->active) {

    // drain anything pending to avoid it coming out once we switch away from
    // the alternate screen
    fflush((*me)->out);

    (void)eg_output_clear(*me);

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
