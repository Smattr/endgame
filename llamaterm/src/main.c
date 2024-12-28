#include <assert.h>
#include <endgame/endgame.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static const char LLAMA[] = "🦙";

/// how often each game step happens
static const int TICK = 1000; // milliseconds

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
  int rc = 0;

  if ((rc = eg_screen_new(&screen, stdout, stdin))) {
    fprintf(stderr, "failed to setup screen: %s\n", strerror(rc));
    goto done;
  }

  size_t row = 1;
  size_t column = 1;

  uint64_t last_tick;
  if ((rc = get_time(&last_tick))) {
    eg_screen_free(&screen);
    fprintf(stderr, "clock_gettime failed: %s\n", strerror(rc));
    goto done;
  }

  while (true) {
    if ((rc = eg_screen_puts(screen, column, row, LLAMA))) {
      eg_screen_free(&screen);
      fprintf(stderr, "failed to write llama: %s\n", strerror(rc));
      goto done;
    }

    if ((rc = eg_screen_sync(screen))) {
      eg_screen_free(&screen);
      fprintf(stderr, "failed to sync screen: %s\n", strerror(rc));
      goto done;
    }

    uint64_t now;
    if ((rc = get_time(&now))) {
      eg_screen_free(&screen);
      fprintf(stderr, "clock_gettime failed: %s\n", strerror(rc));
      goto done;
    }
    const int tick = TICK - (int)(now - last_tick);

    const eg_event_t event = tick <= 0 ? (eg_event_t){.type = EG_EVENT_TICK}
                                       : eg_screen_read(screen, tick);

    if (event.type == EG_EVENT_KEYPRESS && event.value == 0x4) // Ctrl-D
      break;

    if ((rc = eg_screen_puts(screen, column, row, " "))) {
      eg_screen_free(&screen);
      fprintf(stderr, "failed to overwrite llama: %s\n", strerror(rc));
      goto done;
    }

    if (event.type == EG_EVENT_TICK) {
      if ((rc = get_time(&last_tick))) {
        eg_screen_free(&screen);
        fprintf(stderr, "clock_gettime failed: %s\n", strerror(rc));
        goto done;
      }
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
  eg_screen_free(&screen);

  return rc == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
