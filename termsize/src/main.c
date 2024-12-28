#include <endgame/endgame.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {

  eg_screen_t *screen = NULL;
  int rc = 0;

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

  // draw a horizontal spanning line
  for (size_t i = 0; i < columns; ++i) {
    const char *const c = i == 0 ? "◄" : i + 1 == columns ? "►" : "─";
    if ((rc = eg_screen_puts(screen, i + 1, rows / 2, c)))
      DIE("eg_screen_put failed");
  }

  // draw a vertical spanning line
  for (size_t i = 0; i < rows; ++i) {
    const char *const c = i == 0              ? "▲"
                          : i + 1 == rows     ? "▼"
                          : i + 1 == rows / 2 ? "┼"
                                              : "│";
    if ((rc = eg_screen_puts(screen, columns / 2, i + 1, c)))
      DIE("eg_screen_put failed");
  }

  // print the width
  {
    char buffer[128];
    snprintf(buffer, sizeof(buffer), "%zu columns", columns);
    if ((rc =
             eg_screen_puts(screen, columns / 4 * 3 - 5, rows / 2 - 1, buffer)))
      DIE("eg_screen_put failed");
  }

  // print the height
  {
    char buffer[128];
    snprintf(buffer, sizeof(buffer), "%zu rows", rows);
    if ((rc = eg_screen_puts(screen, columns / 2 + 1, rows / 4, buffer)))
      DIE("eg_screen_put failed");
  }

  if ((rc = eg_screen_sync(screen)))
    DIE("failed to sync screen");

  // block until the user presses a key
  (void)eg_screen_read(screen, -1);

done:
  eg_screen_free(&screen);

  return rc == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
