#include "io.h"
#include <assert.h>
#include <endgame/event.h>
#include <endgame/input.h>
#include <endgame/io.h>
#include <endgame/output.h>
#include <errno.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

int eg_io_new(eg_io_t **me, FILE *in, FILE *out) {

  if (me == NULL)
    return EINVAL;

  if (in == NULL)
    return EINVAL;

  if (out == NULL)
    return EINVAL;

  *me = NULL;
  eg_io_t *i = NULL;
  int rc = 0;

  i = calloc(1, sizeof(*i));
  if (i == NULL) {
    rc = ENOMEM;
    goto done;
  }

  if ((rc = eg_input_new(&i->in, in)))
    goto done;

  if ((rc = eg_output_new(&i->out, out)))
    goto done;

  *me = i;
  i = NULL;

done:
  eg_io_free(&i);

  return rc;
}

int eg_io_attach(eg_io_t **me, eg_input_t *in, eg_output_t *out) {

  if (me == NULL)
    return EINVAL;

  if (in == NULL)
    return EINVAL;

  if (out == NULL)
    return EINVAL;

  *me = NULL;
  eg_io_t *i = NULL;
  int rc = 0;

  i = calloc(1, sizeof(*i));
  if (i == NULL) {
    rc = ENOMEM;
    goto done;
  }

  i->in = in;
  i->out = out;

  *me = i;
  i = NULL;

done:
  eg_io_free(&i);

  return rc;
}

int eg_io_set_tick(eg_io_t *me, int tick) {

  if (me == NULL)
    return EINVAL;

  me->tick = tick;

  return 0;
}

/// convert a timespec to milliseconds
static uint64_t ts2ms(struct timespec ts) {
  return (uint64_t)ts.tv_sec * 1000 + (uint64_t)ts.tv_nsec / 1000000;
}

/// get the current time in milliseconds
static int get_time(uint64_t *ms) {
  assert(ms != NULL);
  struct timespec ts = {0};
  if (clock_gettime(CLOCK_MONOTONIC, &ts) < 0)
    return errno;
  *ms = ts2ms(ts);
  return 0;
}

eg_event_t eg_io_read(eg_io_t *me) {

  if (me == NULL)
    return (eg_event_t){EG_EVENT_ERROR, EINVAL};

  // default to tickless
  int tick = -1;

  // if this device is tickfull and ≥ a tick has passed, yield that
  if (me->tick > 0) {
    uint64_t now;
    const int err = get_time(&now);
    if (err != 0)
      return (eg_event_t){EG_EVENT_ERROR, err};
    if (now - me->last_tick >= (uint64_t)me->tick) {
      me->last_tick = now;
      return (eg_event_t){.type = EG_EVENT_TICK};
    }

    // if < a tick has passed, we want to account for this slice
    tick = me->tick - (int)(now - me->last_tick);
  }

  const eg_event_t event = eg_input_read(me->in, tick);

  // if we ticked, note this for next time
  if (event.type == EG_EVENT_TICK) {
    const int err = get_time(&me->last_tick);
    if (err != 0)
      return (eg_event_t){EG_EVENT_ERROR, err};
  }

  return event;
}

size_t eg_io_get_columns(const eg_io_t *me) {
  assert(me != NULL);
  return eg_output_get_columns(me->out);
}

size_t eg_io_get_rows(const eg_io_t *me) {
  assert(me != NULL);
  return eg_output_get_rows(me->out);
}

int eg_io_put(eg_io_t *me, size_t x, size_t y, const char *text, size_t len) {

  if (me == NULL)
    return EINVAL;

  return eg_output_put(me->out, x, y, text, len);
}

int eg_io_puts(eg_io_t *me, size_t x, size_t y, const char *text) {

  if (me == NULL)
    return EINVAL;

  return eg_output_puts(me->out, x, y, text);
}

int eg_io_sync(eg_io_t *me) {

  if (me == NULL)
    return EINVAL;

  return eg_output_sync(me->out);
}

int eg_io_clear(eg_io_t *me) {

  if (me == NULL)
    return EINVAL;

  return eg_output_clear(me->out);
}

int eg_io_debug(eg_io_t *me, unsigned pause, const char *format, ...) {

  if (me == NULL)
    return EINVAL;

  if (format == NULL)
    return EINVAL;

  int rc = 0;
  va_list ap;

  va_start(ap, format);

  if ((rc = eg_output_vdebug(me->out, format, ap)))
    goto done;

  // were we asked to resume after a pause?
  if (pause > 0) {

    (void)sleep(pause);

    if ((rc = eg_io_continue(me)))
      goto done;
  }

done:
  va_end(ap);

  return rc;
}

int eg_io_continue(eg_io_t *me) {

  if (me == NULL)
    return EINVAL;

  return eg_output_continue(me->out);
}

void eg_io_free(eg_io_t **me) {

  if (me == NULL)
    return;

  if (*me == NULL)
    return;

  eg_output_free(&(*me)->out);
  eg_input_free(&(*me)->in);

  free(*me);
  *me = NULL;
}
