#pragma once

#include <endgame/event.h>
#include <endgame/input.h>
#include <endgame/output.h>
#include <stddef.h>
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
#define ENDGAME_API // nothing
#endif
#endif

/// handle to a TTY-backed terminal input+output
typedef struct eg_io eg_io_t;

/// setup the terminal for Curses-style output
///
/// This function or `eg_io_attach` must be called before using any of the other
/// functions in this header.
///
/// \param me [out] Created I/O device on success
/// \param in Input stream for this device
/// \param out Output stream for this device
/// \return 0 on success or an errno on failure.
ENDGAME_API int eg_io_new(eg_io_t **me, FILE *in, FILE *out);

/// setup the terminal for Curses-style output
///
/// This function or `eg_io_new` must be called before using any of the other
/// functions in this header.
///
/// This function is provided as an alternative to `eg_io_new` for callers who
/// already have an input and output setup. Note that the callee takes ownership
/// of `in` and `out`. The caller should not free them later.
///
/// \param me [out] Created I/O device on success
/// \param in Input device to attach
/// \param out Output device to attach
/// \return 0 on success or an errno on failure
ENDGAME_API int eg_io_attach(eg_io_t **me, eg_input_t *in, eg_output_t *out);

/// get the number of columns in the terminal
ENDGAME_API size_t eg_io_get_columns(const eg_io_t *me);

/// get the number of rows in the terminal
ENDGAME_API size_t eg_io_get_rows(const eg_io_t *me);

/// write some text to the I/O device
///
/// \param me I/O device to write to
/// \param x Column at which to begin the write
/// \param y Row at which to begin the write
/// \param text Text to write
/// \param len Number of bytes in `text`
/// \return 0 on success or an errno on failure
ENDGAME_API int eg_io_put(eg_io_t *me, size_t x, size_t y, const char *text,
                          size_t len);

/// write a null terminated string to the I/O device
///
/// \param me Screen to write to
/// \param x Column at which to begin the write
/// \param y Row at which to begin the write
/// \param text String to write
/// \return 0 on success or an errno on failure
ENDGAME_API int eg_io_puts(eg_io_t *me, size_t x, size_t y, const char *text);

/// write `printf`-style arguments to the I/O device
///
/// \param me Screen to write to
/// \param x Column at which to begin the write
/// \param y Row at which to begin the write
/// \param format A `printf`-style format string
/// \param ... `printf`-style format arguments
/// \return 0 on success or an errno on failure
ENDGAME_API int eg_io_print(eg_io_t *me, size_t x, size_t y, const char *format,
                            ...);

/// flush pending writes to the I/O device
///
/// \param me I/O device to synchronise
/// \return 0 on success or an errno on failure
ENDGAME_API int eg_io_sync(eg_io_t *me);

/// set the game “tick” for this device
///
/// The game “tick” is a timeout in ms after which a tick is considered to have
/// happened, interrupting `eg_io_read` and returning to your control loop. The
/// point of this is to give you an iteration in which to update the display,
/// move sprites, adjust countdowns, etc. Tick can be specified as:
///   1. A number of milliseconds. This indirectly determines the framerate of
///      your game. That is, `fps = 1000 / tick`.
///   2. ≤0, indicating there is no tick. This means game actions will only
///      happen when the user presses keys. That is, `eg_io_read` blocks
///      indefinitely until a key is pressed.
/// `io_t` objects are initially created with a 0 tick.
///
/// \param me I/O device to update
/// \param tick Tick value to set
/// \return 0 on success or an errno on failure
ENDGAME_API int eg_io_set_tick(eg_io_t *me, int tick);

/// get a new event
///
/// This function blocks until there is a key press or a signal is received, or
/// an error occurs. Control characters and chords are returned as they are seen
/// by reading stdin. This means e.g. Ctrl-D comes out as 0x4. Non-ASCII UTF-8
/// characters are also readable naturally this way.
///
/// \param me I/O device to read from
/// \return Event seen
ENDGAME_API eg_event_t eg_io_read(eg_io_t *me);

/// blank the screen, clearing all text
///
/// \param me I/O device to clear
/// \return 0 on success or an errno on failure
ENDGAME_API int eg_io_clear(eg_io_t *me);

/// print a debugging message
///
/// This temporarily switches away from the alternate screen and prints a debug
/// message. You can have gameplay resume after a fixed number of seconds, by
/// passing `pause > 0`. Alternatively you can have gameplay still paused when
/// this function returns, by passing `pause == 0`. In the latter case, you need
/// to eventually call `eg_io_continue` to resume gameplay. While gameplay is
/// paused, other functions in this header may be unusable.
///
/// \param me Output to write to
/// \param pause An optional number of seconds to wait before resuming
/// \param format A `printf`-style format string
/// \param ... `printf`-style format arguments
/// \return 0 on success or an errno on failure
ENDGAME_API int eg_io_debug(eg_io_t *me, unsigned pause, const char *format,
                            ...);

/// resume operation after a debug message
///
/// This switches back to the alternate screen, allowing you to resume gameplay.
/// It is an error to call this if you have not previously called
/// `eg_io_debug`.
///
/// \param me Output to resume
/// \return 0 on success or an errno on failure
ENDGAME_API int eg_io_continue(eg_io_t *me);

/// reverse the setup steps from `eg_io_new`
///
/// After calling this function, `eg_io_new` must be called again before
/// using any of the other functions in this header.
ENDGAME_API void eg_io_free(eg_io_t **me);

#ifdef __cplusplus
}
#endif
