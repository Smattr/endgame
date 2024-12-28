#include "screen.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char LLAMA[] = "🦙";

int main(void) {

  int rc = 0;

  if ((rc = screen_init())) {
    fprintf(stderr, "failed to setup screen: %s\n", strerror(rc));
    goto done;
  }

  size_t row = 1;
  size_t column = 1;

  while (true) {
    if ((rc = screen_put(column, row, LLAMA, strlen(LLAMA)))) {
      screen_free();
      fprintf(stderr, "failed to write llama: %s\n", strerror(rc));
      goto done;
    }

    screen_sync();

    const event_t event = screen_read();

    if (event.type == EVENT_KEYPRESS && event.value == 0x4) // Ctrl-D
      break;

    if ((rc = screen_put(column, row, " ", 1))) {
      screen_free();
      fprintf(stderr, "failed to overwrite llama: %s\n", strerror(rc));
      goto done;
    }

    if (event.type == EVENT_KEYPRESS && event.value == 0x445b1b) { // ←
      if (column > 1)
        --column;
    }
    if (event.type == EVENT_KEYPRESS && event.value == 0x435b1b) { // →
      ++column;
    }
    if (event.type == EVENT_KEYPRESS && event.value == 0x415b1b) { // ↑
      if (row > 1)
        --row;
    }
    if (event.type == EVENT_KEYPRESS && event.value == 0x425b1b) { // ↓
      ++row;
    }
  }

done:
  screen_free();

  return rc == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
