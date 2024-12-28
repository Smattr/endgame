#include <assert.h>
#include <endgame/screen.h>
#include <errno.h>
#include <limits.h>
#include <poll.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

/// has `screen_init()` been called?
static bool active;

/// terminal width
static size_t columns;

/// terminal height
static size_t rows;

/// state of the terminal prior to init
static struct termios original_termios;

static int set_window_size(void) {

  struct winsize ws = {0};
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) < 0)
    return errno;

  rows = ws.ws_row;
  columns = ws.ws_col;

  return 0;
}

int eg_screen_init(void) {
  assert(!active && "double screen_init() calls");

  int rc = 0;

  // drain any pending output so it will not be subject to our termios changes
  (void)fflush(stdout);

  // determine terminal dimensions
  if ((rc = set_window_size()))
    return rc;

  // read terminal characteristics
  if (tcgetattr(STDOUT_FILENO, &original_termios) < 0)
    return errno;

  // End of read-only actions. Anything after this point should attempt to be
  // undone on failure.

  // set terminal characteristics
  {
    struct termios new = original_termios;
    new.c_lflag &= ~ECHO;   // turn off echo
    new.c_lflag &= ~ICANON; // turn off canonical mode
    if (tcsetattr(STDOUT_FILENO, TCSANOW, &new) < 0) {
      rc = errno;
      goto done;
    }
  }

  // switch to the alternate screen
  printf("\033[?1049h");

  // hide the cursor
  printf("\033[?25l");

  eg_screen_clear();

  // ensure our changes take effect
  fflush(stdout);

  active = true;

done:
  if (rc != 0)
    eg_screen_free();

  return rc;
}

size_t eg_screen_get_columns(void) { return columns; }

size_t eg_screen_get_rows(void) { return rows; }

int eg_screen_put(size_t x, size_t y, const char *text, size_t len) {
  if (len > INT_MAX)
    return ERANGE;
  if (x > columns)
    return ERANGE;
  if (y > rows)
    return ERANGE;
  if (printf("\033[%zu;%zuH%.*s", y, x, (int)len, text) < 0)
    return EIO;
  return 0;
}

void eg_screen_sync(void) { fflush(stdout); }

eg_event_t eg_screen_read(void) {
  assert(active && "read from screen prior to screen_init()");

  // wait until we have some data on stdin or from the signal bouncer
  struct pollfd in[] = {{.fd = STDIN_FILENO, .events = POLLIN},
                        /* {.fd = signal_pipe[0], .events = POLLIN}*/};
  {
    nfds_t nfds = sizeof(in) / sizeof(in[0]);
    while (true) {
      if (poll(in, nfds, -1) >= 0)
        break;
      if (errno == EINTR)
        continue;
      return (eg_event_t){EG_EVENT_ERROR, (uint32_t)errno};
    }
  }

#if 0
  // priority 2: did we get a signal?
  if (in[1].revents & POLLIN) {
    int signum = 0;
    do {
      ssize_t len = read(signal_pipe[0], &signum, sizeof(signum));
      if (len >= 0) {
        assert((size_t)len == sizeof(signum) && "incomplete read");

        // if this was the signal indicating a window resize, update our
        // knowledge of the dimensions
        if (signum == SIGWINCH) {
          int rc = set_window_size();
          if (rc != 0)
            return (eg_event_t){EG_EVENT_ERROR, (uint32_t)rc};
        }

        return (eg_event_t){EG_EVENT_SIGNAL, (uint32_t)signum};
      }
    } while (errno == EINTR);
    return (eg_event_t){EG_EVENT_ERROR, (uint32_t)errno};
  }
#endif

  assert(in[0].revents & POLLIN);

  // priority 3: read a character from stdin
  unsigned char buffer[4] = {
      0}; // enough for a UTF-8 character or escape sequence
  if (read(STDIN_FILENO, &buffer, 1) < 0)
    return (eg_event_t){EG_EVENT_ERROR, errno};

  // is this a multi-byte sequence?
  size_t more = 0;
  if (buffer[0] == 0x1b) {             // ESC
    more = 3;                          // up to 3 extra bytes
  } else if ((buffer[0] >> 3) == 30) { // 4-byte UTF-8
    more = 3;
  } else if ((buffer[0] >> 4) == 14) { // 3-byte UTF-8
    more = 2;
  } else if ((buffer[0] >> 5) == 6) { // 2-byte UTF-8
    more = 1;
  }

  if (more > 0) {
    // does stdin have remaining ready data?
    struct pollfd input[] = {{.fd = STDIN_FILENO, .events = POLLIN}};
    nfds_t nfds = sizeof(input) / sizeof(input[0]);
    if (poll(input, nfds, 0) > 0) {
      assert(more <= sizeof(buffer) / sizeof(buffer[0]) - 1);
      ssize_t ignored = read(STDIN_FILENO, &buffer[1], more);
      (void)ignored;
    }
  }

  // construct a key press event
  eg_event_t key = {.type = EG_EVENT_KEYPRESS};
  for (size_t i = 0; i < sizeof(buffer) / sizeof(buffer[0]); ++i)
    key.value |= (uint32_t)buffer[i] << (i * 8);
  return key;
}

void eg_screen_clear(void) {
  // clear screen and move to upper left
  printf("\033[2J");
}

void eg_screen_free(void) {

  if (active) {

    // drain anything pending to avoid it coming out once we switch away from
    // the alternate screen
    fflush(stdout);

    eg_screen_clear();

    // show the cursor
    printf("\033[?25h");

    // switch out of the alternate screen
    printf("\033[?1049l");

    active = false;
  }

  // restore the original terminal characteristics
  (void)tcsetattr(STDOUT_FILENO, TCSANOW, &original_termios);

  fflush(stdout);
}
