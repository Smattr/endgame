#include "scene.h"
#include "sprite.h"
#include <assert.h>
#include <endgame/io.h>
#include <endgame/scene.h>
#include <endgame/sprite.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

int eg_scene_new(eg_scene_t **me) {

  if (me == NULL)
    return EINVAL;

  *me = NULL;
  eg_scene_t *s = NULL;
  int rc = 0;

  s = calloc(1, sizeof(*s));
  if (s == NULL) {
    rc = ENOMEM;
    goto done;
  }

  *me = s;
  s = NULL;

done:
  eg_scene_free(&s);

  return rc;
}

static int cmp(const void *a, const void *b) {
  const sprite_t *const *const xp = a;
  const sprite_t *const *const yp = b;
  const sprite_t *const x = *xp;
  const sprite_t *const y = *yp;
  if (x->y < y->y)
    return -1;
  if (x->y > y->y)
    return 1;
  if (x->x < y->x)
    return -1;
  if (x->x > y->x)
    return 1;
  if (x->z < y->z)
    return -1;
  if (x->z > y->z)
    return 1;
  return 0;
}

static void sort_sprites(eg_scene_t *me) {
  assert(me != NULL);
  qsort(me->sprites, me->n_sprites, sizeof(me->sprites[0]), cmp);
}

static void sprite_free(sprite_t *s) {

  if (s == NULL)
    return;

  for (size_t i = 0; i < s->n_forms; ++i)
    free(s->forms[i]);
  free(s->forms);

  free(s);
}

static int sprite_new(sprite_t **s, const eg_sprite_t defn) {
  assert(s != NULL);

  *s = NULL;
  sprite_t *sp = NULL;
  int rc = 0;

  sp = calloc(1, sizeof(*sp));
  if (sp == NULL) {
    rc = ENOMEM;
    goto done;
  }

  size_t n_forms = 0;
  for (size_t i = 0; defn.forms[i] != NULL; ++i)
    ++n_forms;
  sp->forms = calloc(n_forms, sizeof(sp->forms[0]));
  if (n_forms > 0 && sp->forms == NULL) {
    rc = ENOMEM;
    goto done;
  }
  sp->n_forms = n_forms;

  for (size_t i = 0; i < n_forms; ++i) {
    sp->forms[i] = strdup(defn.forms[i]);
    if (sp->forms[i] == NULL) {
      rc = ENOMEM;
      goto done;
    }
  }

  *s = sp;
  sp = NULL;

done:
  sprite_free(sp);

  return rc;
}

int eg_scene_add(eg_scene_t *me, int64_t x, int64_t y, int64_t z,
                 const eg_sprite_t *sprite, eg_sprite_handle_p *handle) {

  if (me == NULL)
    return EINVAL;

  if (sprite == NULL)
    return EINVAL;

  if (sprite->forms == NULL)
    return EINVAL;

  if (handle == NULL)
    return EINVAL;

  *handle = NULL;
  int rc = 0;

  // do we need to expand the sprite array?
  if (me->n_sprites == me->c_sprites) {
    const size_t c = me->c_sprites == 0 ? 1024 : me->c_sprites * 2;
    sprite_t **const ss = realloc(me->sprites, c * sizeof(me->sprites[0]));
    if (ss == NULL) {
      rc = ENOMEM;
      goto done;
    }
    me->sprites = ss;
    me->c_sprites = c;
  }

  // add the new sprite
  if ((rc = sprite_new(&me->sprites[me->n_sprites], *sprite)))
    goto done;
  me->sprites[me->n_sprites]->x = x;
  me->sprites[me->n_sprites]->y = y;
  me->sprites[me->n_sprites]->z = z;
  ++me->n_sprites;

  // rearrange it into position
  sort_sprites(me);

done:
  return rc;
}

int eg_scene_paint(const eg_scene_t *me, eg_io_t *io, eg_2D_t origin) {

  if (me == NULL)
    return EINVAL;

  if (io == NULL)
    return EINVAL;

  const size_t rows = eg_io_get_rows(io);
  const size_t columns = eg_io_get_columns(io);

  size_t i = 0;

  for (size_t row = 0; row < rows; ++row) {
    for (size_t col = 0; col < columns; ++col) {
      const int64_t rel_x = origin.x + (int64_t)col;
      const int64_t rel_y = origin.y + (int64_t)row;

      // find the sprite at this position
      while (i < me->n_sprites && me->sprites[i]->y < rel_y)
        ++i;
      while (i < me->n_sprites && me->sprites[i]->y == rel_y &&
             me->sprites[i]->x < rel_x)
        ++i;
      while (i + 1 < me->n_sprites && me->sprites[i + 1]->y == rel_y &&
             me->sprites[i + 1]->x == rel_x)
        ++i;

      const char *s = (i < me->n_sprites && me->sprites[i]->y == rel_y &&
                       me->sprites[i]->x == rel_x)
                          ? me->sprites[i]->forms[me->sprites[i]->form]
                          : NULL;

      const int rc = eg_io_puts(io, col + 1, row + 1, s == NULL ? " " : s);
      if (rc != 0)
        return rc;
    }
  }

  return 0;
}

int eg_scene_remove(eg_scene_t *me, eg_sprite_handle_p handle) {

  if (me == NULL)
    return EINVAL;

  if (handle == NULL)
    return EINVAL;

  // FIXME: this scan will be expensive in large scenes
  for (size_t i = 0; i < me->n_sprites; ++i) {
    if (me->sprites[i] == handle) {
      sprite_free(handle);
      for (size_t j = i; j + 1 < me->n_sprites; ++j)
        me->sprites[j] = me->sprites[j + 1];
      --me->n_sprites;
      return 0;
    }
  }

  // if we reached here, the given sprite did not belong to this scene
  return ENOENT;
}

void eg_scene_free(eg_scene_t **me) {

  if (me == NULL)
    return;

  if (*me == NULL)
    return;

  for (size_t i = 0; i < (*me)->n_sprites; ++i)
    sprite_free((*me)->sprites[i]);
  free((*me)->sprites);

  free(*me);
  *me = NULL;
}
