#pragma once

#include <stddef.h>
#include <stdint.h>

/// a sprite, as it exists within a scene
///
/// This essentially captures the sprite’s definition (`eg_sprite_t`), along
/// with its current state.
typedef struct {
  char **forms;   ///< visual forms this sprite can be in
  size_t n_forms; ///< count of `forms`
  size_t form;    ///< which `forms` entry is currently active?

  int64_t x;
  int64_t y;
  int64_t z;
} sprite_t;
