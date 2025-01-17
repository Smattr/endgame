#include "io.h"
#include <assert.h>
#include <endgame/event.h>
#include <endgame/input.h>
#include <endgame/io.h>
#include <endgame/output.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

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

eg_event_t eg_io_read(eg_io_t *me, int tick) {

  if (me == NULL)
    return (eg_event_t){EG_EVENT_ERROR, EINVAL};

  return eg_input_read(me->in, tick);
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
