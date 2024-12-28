#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef ENDGAME_API
#ifdef __GNUC__
#define ENDGAME_API __attribute__((visibility("default")))
#elif defined(_MSC_VER)
#define ENDGAME_API __declspec(dllexport)
#else
#define ENDGAME_API /* nothing */
#endif
#endif

/// type of an event returned from `eg_screen_read`
typedef enum {
  EG_EVENT_ERROR,
  EG_EVENT_KEYPRESS,
  EG_EVENT_TICK, ///< a game tick
  EG_EVENT_SIGNAL,
} eg_event_type_t;

/// return type of `eg_screen_read`
typedef struct {
  eg_event_type_t type; ///< was this event a key, signal, or error?
  uint32_t value;       ///< payload (key, signal number, or errno)
} eg_event_t;

/// handle to a TTY-backed terminal for display
typedef struct eg_screen eg_screen_t;

/** setup the terminal for Curses-style output
 *
 * This function must be called before using any of the other functions in this
 * header.
 *
 * \param me [out] Created screen on success
 * \param out Output stream for this screen
 * \param in Input stream for this screen
 * \return 0 on success or an errno on failure.
 */
ENDGAME_API int eg_screen_new(eg_screen_t **me, FILE *out, FILE *in);

/// get the number of columns in the terminal
ENDGAME_API size_t eg_screen_get_columns(eg_screen_t *me);

/// get the number of rows in the terminal
ENDGAME_API size_t eg_screen_get_rows(eg_screen_t *me);

/// write some text to the screen
///
/// \param me Screen to write to
/// \param x Column at which to begin the write
/// \param y Row at which to begin the write
/// \param text Text to write
/// \param len Number of bytes in `text`
/// \return 0 on success or an errno on failure
ENDGAME_API int eg_screen_put(eg_screen_t *me, size_t x, size_t y,
                              const char *text, size_t len);

/// write a null terminated string to the screen
///
/// \param me Screen to write to
/// \param x Column at which to begin the write
/// \param y Row at which to begin the write
/// \param text String to write
/// \return 0 on success or an errno on failure
ENDGAME_API int eg_screen_puts(eg_screen_t *me, size_t x, size_t y,
                               const char *text);

/// flush pending writes to the screen
///
/// \param me Screen to synchronise
/// \return 0 on success or an errno on failure
ENDGAME_API int eg_screen_sync(eg_screen_t *me);

/** get a new event
 *
 * This function blocks until there is a key press or a signal is received, or
 * an error occurs. Control characters and chords are returned as they are seen
 * by reading stdin. This means e.g. Ctrl-D comes out as 0x4. Non-ASCII UTF-8
 * characters are also readable naturally this way.
 *
 * The game “tick” specified by the `tick` parameter can be either:
 *   1. A number of milliseconds. This indirectly determines the framerate of
 *      the game. That is, `fps = 1000 / tick`.
 *   2. -1, indicating there is no tick. This means game actions will only
 *      happen when the user presses keys. That is, `eg_screen_read` blocks
 *      indefinitely until a key is pressed.
 *
 * \param me Screen to read from
 * \param tick Timeout (ms) after which a tick is considered to have happened
 * \return Event seen
 */
ENDGAME_API eg_event_t eg_screen_read(eg_screen_t *me, int tick);

/// blank the screen, clearing all text
///
/// \param me Screen to clear
/// \return 0 on success or an errno on failure
ENDGAME_API int eg_screen_clear(eg_screen_t *me);

/** reverse the setup steps from `eg_screen_new`
 *
 * After calling this function, `eg_screen_new` must be called again before
 * using any of the other functions in this header.
 */
ENDGAME_API void eg_screen_free(eg_screen_t **me);

#ifdef __cplusplus
}
#endif
