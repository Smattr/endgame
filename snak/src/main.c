#include <endgame/endgame.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/// how often each game step happens
static const int TICK = 5; // milliseconds

typedef enum { NORTH = 0, EAST = 1, SOUTH = 2, WEST = 3 } direction_t;

static const char **BLOCKS[] = {
    [NORTH] = (const char *[]){"▁", "▂", "▃", "▄", "▅", "▆", "▇", "█"},
    [EAST] = (const char *[]){"▏", "▎", "▍", "▌", "▋", "▊", "▉", "█"},
    [SOUTH] = (const char *[]){"\033[7m▇\033[0m", "\033[7m▆\033[0m",
                               "\033[7m▅\033[0m", "\033[7m▄\033[0m",
                               "\033[7m▃\033[0m", "\033[7m▂\033[0m",
                               "\033[7m▁\033[0m", "\033[7m \033[0m"},
    [WEST] = (const char *[]){"\033[7m▇\033[0m", "\033[7m▆\033[0m",
                              "\033[7m▅\033[0m", "\033[7m▄\033[0m",
                              "\033[7m▃\033[0m", "\033[7m▂\033[0m",
                              "\033[7m▁\033[0m", "\033[7m \033[0m"},
};

static const size_t BLOCKS_LEN = 8;

int main(void) {

  eg_io_t *io = NULL;
  const char **scene = NULL;
  int rc = 0;

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

  // allocate space to store the board
  scene = calloc(rows * columns, sizeof(scene[0]));
  if (rows > 0 && columns > 0 && scene == NULL) {
    rc = ENOMEM;
    DIE("calloc failed");
  }

  // where do we start?
  size_t x = 1;
  size_t y = rows / 2;
  size_t next_head = 0;
  direction_t dir = EAST;
  direction_t next_dir = EAST;

  while (true) {

    // draw our head
    scene[(y - 1) * columns + (x - 1)] = BLOCKS[dir][next_head];
    if ((rc = eg_io_puts(io, x, y, scene[(y - 1) * columns + (x - 1)])))
      DIE("eg_io_puts failed");

    if ((rc = eg_io_sync(io)))
      DIE("eg_io_sync failed");

    const eg_event_t event = eg_io_read(io);

    if (event.type == EG_EVENT_KEYPRESS && event.value == 0x4) // Ctrl-D
      goto done;
    if (event.type == EG_EVENT_KEYPRESS && event.value == 'q')
      goto done;

    if (event.type == EG_EVENT_TICK) {
      bool moved = false;
      ++next_head;
      if (next_head >= BLOCKS_LEN) {
        next_head = 0;
        dir = next_dir;
        switch (dir) {
        case NORTH:
          --y;
          if ((rc = eg_io_set_tick(io, TICK * 3)))
            DIE("eg_io_set_tick failed");
          break;
        case EAST:
          ++x;
          if ((rc = eg_io_set_tick(io, TICK)))
            DIE("eg_io_set_tick failed");
          break;
        case SOUTH:
          ++y;
          if ((rc = eg_io_set_tick(io, TICK * 3)))
            DIE("eg_io_set_tick failed");
          break;
        case WEST:
          --x;
          if ((rc = eg_io_set_tick(io, TICK)))
            DIE("eg_io_set_tick failed");
          break;
        }
        moved = true;
      }
      if (moved) {
        // did we run off the end of the board?
        if (x > columns || y > rows || x < 1 || y < 1)
          break;
        // did we run into ourselves?
        if (scene[(y - 1) * columns + (x - 1)] != NULL)
          break;
      }
    }

    if (event.type == EG_EVENT_KEYPRESS && event.value == 0x415b1b) { // ↑
      if (dir == EAST || dir == WEST)
        next_dir = NORTH;
    }
    if (event.type == EG_EVENT_KEYPRESS && event.value == 0x435b1b) { // →
      if (dir == NORTH || dir == SOUTH)
        next_dir = EAST;
    }
    if (event.type == EG_EVENT_KEYPRESS && event.value == 0x425b1b) { // ↓
      if (dir == EAST || dir == WEST)
        next_dir = SOUTH;
    }
    if (event.type == EG_EVENT_KEYPRESS && event.value == 0x445b1b) { // ←
      if (dir == NORTH || dir == SOUTH)
        next_dir = WEST;
    }
  }

  (void)sleep(5);

done:
  free(scene);
  eg_io_free(&io);

  return rc == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
