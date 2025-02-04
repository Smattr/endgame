#pragma once

#include <stdarg.h>
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

/// handle to a TTY-backed terminal output stream for display
typedef struct eg_output eg_output_t;

/// setup the terminal for Curses-style output
///
/// This function must be called before using any of the other functions in this
/// header.
///
/// \param me [out] Created output on success
/// \param out Stream for this output
/// \return 0 on success or an errno on failure.
ENDGAME_API int eg_output_new(eg_output_t **me, FILE *out);

/// get the number of columns in the terminal
ENDGAME_API size_t eg_output_get_columns(const eg_output_t *me);

/// get the number of rows in the terminal
ENDGAME_API size_t eg_output_get_rows(const eg_output_t *me);

/// write some text to the output
///
/// \param me Output to write to
/// \param x Column at which to begin the write
/// \param y Row at which to begin the write
/// \param text Text to write
/// \param len Number of bytes in `text`
/// \return 0 on success or an errno on failure
ENDGAME_API int eg_output_put(eg_output_t *me, size_t x, size_t y,
                              const char *text, size_t len);

/// write a null terminated string to the output
///
/// \param me Output to write to
/// \param x Column at which to begin the write
/// \param y Row at which to begin the write
/// \param text String to write
/// \return 0 on success or an errno on failure
ENDGAME_API int eg_output_puts(eg_output_t *me, size_t x, size_t y,
                               const char *text);

/// flush pending writes to the output
///
/// \param me Output to synchronise
/// \return 0 on success or an errno on failure
ENDGAME_API int eg_output_sync(eg_output_t *me);

/// blank the output, clearing all text
///
/// \param me Output to clear
/// \return 0 on success or an errno on failure
ENDGAME_API int eg_output_clear(eg_output_t *me);

/// print a debugging message
///
/// This temporarily switches away from the alternate screen and prints your
/// debug message. The alternate screen will remain inactive until you call
/// `eg_output_continue`. None of the other functions in this header (except
/// `eg_output_debug` itself) will be usable until you do this. Debugging does
/// not nest; a single `eg_output_continue` call is enough to resume after
/// multiple `eg_output_debug` calls.
///
/// \param me Output to write to
/// \param format A `printf`-style format string
/// \param ... `printf`-style format arguments
/// \return 0 on success or an errno on failure
ENDGAME_API int eg_output_debug(eg_output_t *me, const char *format, ...);

/// print a debugging message
///
/// This is the same as `eg_output_debug`, but takes a `va_list`.
///
/// \param me Output to write to
/// \param format A `printf`-style format string
/// \param ap `printf`-style format arguments
/// \return 0 on success or an errno on failure
ENDGAME_API int eg_output_vdebug(eg_output_t *me, const char *format,
                                 va_list ap);

/// resume operation after a debug message
///
/// This switches back to the alternate screen, allowing you to resume gameplay.
/// It is an error to call this if you have not previously called
/// `eg_output_debug`.
///
/// \param me Output to resume
/// \return 0 on success or an errno on failure
ENDGAME_API int eg_output_continue(eg_output_t *me);

/// reverse the setup steps from `eg_output_new`
///
/// After calling this function, `eg_output_new` must be called again before
/// using any of the other functions in this header.
ENDGAME_API void eg_output_free(eg_output_t **me);

#ifdef __cplusplus
}
#endif
