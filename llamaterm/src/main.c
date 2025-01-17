#include <assert.h>
#include <endgame/endgame.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char LLAMA[] = "🦙";

/// how often each game step happens
static const int TICK = 1000; // milliseconds

int main(void) {

  eg_io_t *io = NULL;
  int rc = 0;

  if ((rc = eg_io_new(&io, stdin, stdout))) {
    fprintf(stderr, "failed to setup I/O: %s\n", strerror(rc));
    goto done;
  }
  if ((rc = eg_io_set_tick(io, TICK))) {
    fprintf(stderr, "eg_io_set_tick failed: %s\n", strerror(rc));
    goto done;
  }

  size_t row = 1;
  size_t column = 1;

  while (true) {
    if ((rc = eg_io_puts(io, column, row, LLAMA))) {
      eg_io_free(&io);
      fprintf(stderr, "failed to write llama: %s\n", strerror(rc));
      goto done;
    }

    if ((rc = eg_io_sync(io))) {
      eg_io_free(&io);
      fprintf(stderr, "failed to sync I/O: %s\n", strerror(rc));
      goto done;
    }

    const eg_event_t event = eg_io_read(io);

    if (event.type == EG_EVENT_KEYPRESS && event.value == 0x4) // Ctrl-D
      break;

    if ((rc = eg_io_puts(io, column, row, " "))) {
      eg_io_free(&io);
      fprintf(stderr, "failed to overwrite llama: %s\n", strerror(rc));
      goto done;
    }

    if (event.type == EG_EVENT_TICK) {
    }

    if (event.type == EG_EVENT_KEYPRESS && event.value == 0x445b1b) { // ←
      if (column > 1)
        --column;
    }
    if (event.type == EG_EVENT_KEYPRESS && event.value == 0x435b1b) { // →
      ++column;
    }
    if (event.type == EG_EVENT_KEYPRESS && event.value == 0x415b1b) { // ↑
      if (row > 1)
        --row;
    }
    if (event.type == EG_EVENT_KEYPRESS && event.value == 0x425b1b) { // ↓
      ++row;
    }
  }

done:
  eg_io_free(&io);

  return rc == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
