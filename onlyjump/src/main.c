#include <assert.h>
#include <endgame/endgame.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static const char ME[] = "🐑";

static const char *TREES[] = {"🌲", "🌳", "🌷", "🌴", "🌵", "🌱", "🌻",
                              "🌼", "🌺", "🌿", "🌾", "🍀", "🌽", "🍄"};

/// generate a new tile for a square just scrolled into view
static const char *horizon(void) {

  // should this tile be a tree or empty?
  const bool show_tree = rand() % 30 == 0;
  if (!show_tree)
    return NULL;

  // which tree?
  const size_t tree = rand() % (sizeof(TREES) / sizeof(TREES[0]));
  return TREES[tree];
}

/// how often each game step happens
static const int TICK = 200; // milliseconds

int main(void) {

  eg_io_t *io = NULL;
  const char **scene = NULL;
  int rc = 0;

  srand((unsigned)time(NULL));

#define DIE(msg)                                                               \
  do {                                                                         \
    eg_io_free(&io);                                                           \
    fprintf(stderr, msg ": %s\n", strerror(rc));                               \
    goto done;                                                                 \
  } while (0)

  if ((rc = eg_io_new(&io, stdin, stdout)))
    DIE("eg_io_new failed");
  if ((rc = eg_io_set_tick(io, TICK)))
    DIE("eg_io_set_tick failed");

  const size_t rows = eg_io_get_rows(io);
  const size_t columns = eg_io_get_columns(io);

  // allocate space to store the scenery
  scene = calloc(columns / 2, sizeof(scene[0]));
  if (columns > 0 && scene == NULL) {
    rc = ENOMEM;
    DIE("calloc failed");
  }

  // draw the “floor”
  for (size_t i = 0; i < columns; ++i) {
    if ((rc = eg_io_puts(io, i + 1, rows - 10, "═")))
      DIE("eg_io_puts failed");
  }

  // where should our character start?
  static const size_t MY_START = 4;

  // draw an initial scene
  for (size_t i = 0; i < columns / 2; ++i) {
    if (i == MY_START)
      continue;
    scene[i] = horizon();
  }

  int y_velocity = 0;

  for (size_t me_column = MY_START, me_row = rows - 12;;) {

    // draw the scenery and us
    for (size_t i = 0; i < columns / 2; ++i) {
      const char *const tile = (i == me_column && me_row == rows - 12) ? ME
                               : scene[i] == NULL                      ? "  "
                                                  : scene[i];
      if ((rc = eg_io_puts(io, (i + 1) * 2, rows - 11, tile)))
        DIE("eg_io_puts failed");
    }

    // if we are airborne, draw us
    if (me_row != rows - 12) {
      if ((rc = eg_io_puts(io, (me_column + 1) * 2, me_row + 1, ME)))
        DIE("eg_io_puts failed");
    }

    if ((rc = eg_io_sync(io)))
      DIE("eg_io_sync failed");

    const eg_event_t event = eg_io_read(io);

    if (event.type == EG_EVENT_KEYPRESS && event.value == 0x4) // Ctrl-D
      break;
    if (event.type == EG_EVENT_KEYPRESS && event.value == 'q')
      break;

    if (event.type == EG_EVENT_TICK) {
      if (me_row != rows - 12) {
        // clear us
        if ((rc = eg_io_puts(io, (me_column + 1) * 2, me_row + 1, "  ")))
          DIE("eg_io_puts failed");
        // move us
        me_row += y_velocity;
        if (me_row > rows - 12)
          me_row = rows - 12;
        // did we collide with the scenery?
        if (me_row == rows - 12 && scene[me_column] != NULL) {
          for (size_t i = 1; i < 12; i += 2) {
            for (size_t j = 0; j < i; ++j) {
              for (size_t k = 0; k < i; ++k) {
                const size_t c = (me_column - i / 2 + 1 + j) * 2;
                if (c >= columns)
                  continue;
                if ((rc = eg_io_puts(io, c, me_row - i / 2 + k + 1, "💥")))
                  DIE("eg_io_puts failed");
              }
            }
            if ((rc = eg_io_sync(io)))
              DIE("eg_io_sync failed");
            (void)sleep(1);
          }
          break;
        }
        // redraw us
        if ((rc = eg_io_puts(io, (me_column + 1) * 2, me_row + 1, ME)))
          DIE("eg_io_puts failed");
        // apply gravity
        y_velocity += 1;
      }
    }

    if (event.type == EG_EVENT_KEYPRESS && event.value == 0x445b1b) { // ←
      if (scene[me_column - 1] == NULL || me_row != rows - 12) {
        if (me_column - 1 < MY_START) {
          // scroll left
          for (size_t i = columns / 2 - 1; i > 0; --i)
            scene[i] = scene[i - 1];
          scene[0] = horizon();
        } else {
          // clear us
          if ((rc = eg_io_puts(io, (me_column + 1) * 2, me_row + 1, "  ")))
            DIE("eg_io_puts failed");
          // move us
          --me_column;
        }
      }
    }
    if (event.type == EG_EVENT_KEYPRESS && event.value == 0x435b1b) { // →
      if (scene[me_column + 1] == NULL || me_row != rows - 12) {
        if (columns / 2 - me_column < MY_START) {
          // scroll right
          for (size_t i = 0; i + 1 < columns / 2; ++i)
            scene[i] = scene[i + 1];
          scene[columns / 2 - 1] = horizon();
        } else {
          // clear us
          if ((rc = eg_io_puts(io, (me_column + 1) * 2, me_row + 1, "  ")))
            DIE("eg_io_puts failed");
          // move us
          ++me_column;
        }
      }
    }
    if (event.type == EG_EVENT_KEYPRESS && event.value == 0x415b1b) { // ↑
      if (me_row == rows - 12) {
        me_row -= 3;
        y_velocity = -3;
      }
    }
  }

done:
  free(scene);
  eg_io_free(&io);

  return rc == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
