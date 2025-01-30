#pragma once

#include "sprite.h"
#include <endgame/scene.h>
#include <stdbool.h>
#include <stddef.h>

struct eg_scene {
  /// sprites in this scene, ordered by {y,x,z}
  ///
  /// We store an array of sprites rather than a grid of cells under the
  /// assumption that scenes are sparse. That is, most cells will be empty.
  sprite_t **sprites;
  size_t n_sprites;
  size_t c_sprites;

  bool needs_sync; ///< are the sprites potentially unsorted?
};
