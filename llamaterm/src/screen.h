#pragma once

#include <stddef.h>
#include <stdint.h>

/// type of an event returned from `screen_read`
typedef enum {
  EVENT_ERROR,
  EVENT_KEYPRESS,
  EVENT_SIGNAL,
} event_type_t;

/// return type of `screen_read`
typedef struct {
  event_type_t type; ///< was this event a key, signal, or error?
  uint32_t value;    ///< payload (key, signal number, or errno)
} event_t;

/** setup the terminal for Curses-style output
 *
 * This function must be called before using any of the other functions in this
 * header.
 *
 * \return 0 on success or an errno on failure.
 */
int screen_init(void);

/// get the number of columns in the terminal
size_t screen_get_columns(void);

/// get the number of rows in the terminal
size_t screen_get_rows(void);

int screen_put(size_t x, size_t y, const char *text, size_t len);

void screen_sync(void);

/** get a new event
 *
 * This function blocks until there is a key press or a signal is received, or
 * an error occurs. Control characters and chords are returned as they are seen
 * by reading stdin. This means e.g. Ctrl-D comes out as 0x4. Non-ASCII UTF-8
 * characters are also readable naturally this way.
 *
 * \return Event seen
 */
event_t screen_read(void);

/// blank the screen, clearing all text
void screen_clear(void);

/** reverse the setup steps from `screen_init`
 *
 * After calling this function, `screen_init` must be called again before using
 * any of the other functions in this header.
 */
void screen_free(void);
