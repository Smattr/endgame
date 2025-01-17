#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/// definition of a sprite, an entity to be displayed
typedef struct {
  /// all visual forms this sprite can take
  ///
  /// Many sprites will have only a single form, e.g. `{"a", NULL}` for a sprite
  /// that always appears as the character 'a'. Others may have multiple forms,
  /// e.g. `{"<", ">", NULL}` for an arrow that can be pointing either left or
  /// right. The list of forms is expected to be terminated with a null entry,
  /// as in the preceding examples.
  const char **forms;
} eg_sprite_t;

#ifdef __cplusplus
}
#endif
