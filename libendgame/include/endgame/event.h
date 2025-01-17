#pragma once

#include <stdint.h>

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

/// type of an event returned from `eg_input_read`
typedef enum {
  EG_EVENT_ERROR,
  EG_EVENT_KEYPRESS,
  EG_EVENT_TICK, ///< a game tick
  EG_EVENT_SIGNAL,
} eg_event_type_t;

/// return type of `eg_input_read`
typedef struct {
  eg_event_type_t type; ///< was this event a key, signal, or error?
  uint32_t value;       ///< payload (key, signal number, or errno)
} eg_event_t;

#ifdef __cplusplus
}
#endif
