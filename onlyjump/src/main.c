#include <assert.h>
#include <endgame/endgame.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
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

int main(void) {

  eg_screen_t *screen = NULL;
  const char **scene = NULL;
  int rc = 0;

  srand((unsigned)time(NULL));

#define DIE(msg)                                                               \
  do {                                                                         \
    eg_screen_free(&screen);                                                   \
    fprintf(stderr, msg ": %s\n", strerror(rc));                               \
    goto done;                                                                 \
  } while (0)

  if ((rc = eg_screen_new(&screen, stdout, stdin)))
    DIE("failed to setup screen");

  const size_t rows = eg_screen_get_rows(screen);
  const size_t columns = eg_screen_get_columns(screen);

  // allocate space to store the scenery
  scene = calloc(columns / 2, sizeof(scene[0]));
  if (columns > 0 && scene == NULL) {
    rc = ENOMEM;
    DIE("calloc failed");
  }

  // draw the “floor”
  for (size_t i = 0; i < columns; ++i) {
    if ((rc = eg_screen_puts(screen, i + 1, rows - 10, "═")))
      DIE("eg_screen_puts failed");
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

  uint64_t last_tick;
  if ((rc = get_time(&last_tick)))
    DIE("clock_gettime failed");

  for (size_t me_column = MY_START, me_row = rows - 12;;) {

    // draw the scenery and us
    for (size_t i = 0; i < columns / 2; ++i) {
      const char *const tile = (i == me_column && me_row == rows - 12) ? ME
                               : scene[i] == NULL                      ? "  "
                                                  : scene[i];
      if ((rc = eg_screen_puts(screen, (i + 1) * 2, rows - 11, tile)))
        DIE("eg_screen_puts failed");
    }

    // if we are airborne, draw us
    if (me_row != rows - 12) {
      if ((rc = eg_screen_puts(screen, (me_column + 1) * 2, me_row + 1, ME)))
        DIE("eg_screen_puts failed");
    }

    if ((rc = eg_screen_sync(screen)))
      DIE("eg_screen_sync failed");

    uint64_t now;
    if ((rc = get_time(&now)))
      DIE("clock_gettime failed");
    const int tick = TICK - (int)(now - last_tick);

    const eg_event_t event = tick <= 0 ? (eg_event_t){.type = EG_EVENT_TICK}
                                       : eg_screen_read(screen, tick);

    if (event.type == EG_EVENT_KEYPRESS && event.value == 0x4) // Ctrl-D
      break;
    if (event.type == EG_EVENT_KEYPRESS && event.value == 'q')
      break;

    if (event.type == EG_EVENT_TICK) {
      if ((rc = get_time(&last_tick)))
        DIE("clock_gettime failed");
      if (me_row != rows - 12) {
        // clear us
        if ((rc =
                 eg_screen_puts(screen, (me_column + 1) * 2, me_row + 1, "  ")))
          DIE("eg_screen_puts failed");
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
                if ((rc = eg_screen_puts(screen, c, me_row - i / 2 + k + 1,
                                         "💥")))
                  DIE("eg_screen_puts failed");
              }
            }
            if ((rc = eg_screen_sync(screen)))
              DIE("eg_screen_sync failed");
            (void)sleep(1);
          }
          break;
        }
        // redraw us
        if ((rc = eg_screen_puts(screen, (me_column + 1) * 2, me_row + 1, ME)))
          DIE("eg_screen_puts failed");
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
          if ((rc = eg_screen_puts(screen, (me_column + 1) * 2, me_row + 1,
                                   "  ")))
            DIE("eg_screen_puts failed");
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
          if ((rc = eg_screen_puts(screen, (me_column + 1) * 2, me_row + 1,
                                   "  ")))
            DIE("eg_screen_puts failed");
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
  eg_screen_free(&screen);

  return rc == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
