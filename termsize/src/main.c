#include <endgame/endgame.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {

  eg_io_t *io = NULL;
  int rc = 0;

#define DIE(msg)                                                               \
  do {                                                                         \
    eg_io_free(&io);                                                           \
    fprintf(stderr, msg ": %s\n", strerror(rc));                               \
    goto done;                                                                 \
  } while (0)

  if ((rc = eg_io_new(&io, stdin, stdout)))
    DIE("eg_io_new failed");

  const size_t rows = eg_io_get_rows(io);
  const size_t columns = eg_io_get_columns(io);

  // draw a horizontal spanning line
  for (size_t i = 0; i < columns; ++i) {
    const char *const c = i == 0 ? "◄" : i + 1 == columns ? "►" : "─";
    if ((rc = eg_io_puts(io, i + 1, rows / 2, c)))
      DIE("eg_io_put failed");
  }

  // draw a vertical spanning line
  for (size_t i = 0; i < rows; ++i) {
    const char *const c = i == 0              ? "▲"
                          : i + 1 == rows     ? "▼"
                          : i + 1 == rows / 2 ? "┼"
                                              : "│";
    if ((rc = eg_io_puts(io, columns / 2, i + 1, c)))
      DIE("eg_io_put failed");
  }

  // print the width
  {
    char buffer[128];
    snprintf(buffer, sizeof(buffer), "%zu columns", columns);
    if ((rc = eg_io_puts(io, columns / 4 * 3 - 5, rows / 2 - 1, buffer)))
      DIE("eg_io_put failed");
  }

  // print the height
  {
    char buffer[128];
    snprintf(buffer, sizeof(buffer), "%zu rows", rows);
    if ((rc = eg_io_puts(io, columns / 2 + 1, rows / 4, buffer)))
      DIE("eg_io_put failed");
  }

  if ((rc = eg_io_sync(io)))
    DIE("eg_io_sync failed");

  // block until the user presses a key
  (void)eg_io_read(io);

done:
  eg_io_free(&io);

  return rc == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
