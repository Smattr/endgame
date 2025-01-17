#pragma once

#include <endgame/event.h>
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

/// handle to a TTY-backed terminal input for reading key presses
typedef struct eg_input eg_input_t;

/// setup an input device for reading
///
/// This function must be called before using any of the other functions in this
/// header.
///
/// \param me [out] Created Input device on success
/// \param in Input stream for this device
/// \return 0 on success or an errno on failure.
ENDGAME_API int eg_input_new(eg_input_t **me, FILE *in);

/// get a new event
///
/// This function blocks until there is a key press or a signal is received, or
/// an error occurs. Control characters and chords are returned as they are seen
/// by reading stdin. This means e.g. Ctrl-D comes out as 0x4. Non-ASCII UTF-8
/// characters are also readable naturally this way.
///
/// The game “tick” specified by the `tick` parameter can be either:
///   1. A number of milliseconds. This indirectly determines the framerate of
///      the game. That is, `fps = 1000 / tick`.
///   2. -1, indicating there is no tick. This means game actions will only
///      happen when the user presses keys. That is, `eg_screen_read` blocks
///      indefinitely until a key is pressed.
///
/// \param me Input device to read from
/// \param tick Timeout (ms) after which a tick is considered to have happened
/// \return Event seen
ENDGAME_API eg_event_t eg_input_read(eg_input_t *me, int tick);

/// reverse the setup steps from `eg_input_new`
///
/// After calling this function, `eg_input_new` must be called again before
/// using any of the other functions in this header.
ENDGAME_API void eg_input_free(eg_input_t **me);

#ifdef __cplusplus
}
#endif
