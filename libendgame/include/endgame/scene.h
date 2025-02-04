#pragma once

#include <endgame/io.h>
#include <endgame/sprite.h>
#include <stddef.h>
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

/// a 2-D space populated with sprites
///
/// Sprites themselves are positioned with 3-D coordinates. But the Z axis is
/// used only to prioritise one sprite to be positioned “on top” of another when
/// displayed in 2-D.
typedef struct eg_scene eg_scene_t;

/// an opaque handle for a sprite installed in a scene
typedef void *eg_sprite_handle_p;

/// create a new scene
///
/// This function must be called before using any of the other functions in this
/// header.
///
/// \param me [out] Created scene on success
/// \return 0 on success or an errno on failure
ENDGAME_API int eg_scene_new(eg_scene_t **me);

/// add a sprite to a scene
///
/// \param me Scene to operate on
/// \param x Starting X position of the sprite
/// \param y Starting Y position of the sprite
/// \param z Starting Z position of the sprite
/// \param sprite Definition of sprite to place
/// \param handle [out] Handle to the added sprite on success
/// \return 0 on success or an errno on failure
ENDGAME_API int eg_scene_add(eg_scene_t *me, int64_t x, int64_t y, int64_t z,
                             const eg_sprite_t *sprite,
                             eg_sprite_handle_p *handle);

/// synchronise internal bookkeeping
///
/// After modifying the sprites in a scene, internal data structures must be
/// updated before rendering to an output device. This function does these
/// updates and should be called any time you are modifying sprites.
///
/// The reason this exists separately rather than being done automatically any
/// time sprite modification happens is to allow you to amortise the cost of
/// this operation. E.g. you can add multiple sprites and then only call this
/// function once just before output.
///
/// What counts as modification:
///   • `eg_scene_add`
///   • `eg_scene_move`
///   • `eg_scene_remove`
///
/// If you do not call this before calling `eg_scene_paint`, it will be done for
/// you.
///
/// \param me Scene to synchronise
ENDGAME_API void eg_scene_sync(eg_scene_t *me);

/// a point within 2-D space
typedef struct {
  int64_t x;
  int64_t y;
} eg_2D_t;

/// draw a partial view of a scene
///
/// The idea behind this function is to draw a finite “view box” within a
/// (potentially infinite-ish height/width) scene onto a display device. E.g.:
///
///                   scene Y
///                      ▲
///                      │
///                      │ ┌──────────┐
///                      │ │ view box │
///   ◄──────────────────┼─┼──────────┼─────► scene X
///                   0,0│ └──────────┘
///                      │
///                      │
///                      ▼
///
/// \param me Scene to display
/// \param io Device to draw onto
/// \param origin Coordinates within `scene` to consider the top left limit of
///   the I/O’s view box
/// \return 0 on success or an errno on failure
ENDGAME_API int eg_scene_paint(eg_scene_t *me, eg_io_t *io, eg_2D_t origin);

/// move an existing sprite within a scene
///
/// If you pass a sprite handle derived from a different scene than the one
/// passed, then behaviour is undefined.
///
/// \param me Scene to operate on
/// \param subject Handle to sprite to move
/// \param x New X position of the sprite
/// \param y New Y position of the sprite
/// \param z New Z position of the sprite
/// \return 0 on success or an errno on failure
ENDGAME_API int eg_scene_move(eg_scene_t *me, eg_sprite_handle_p subject,
                              int64_t x, int64_t y, int64_t z);

/// change the current form of a sprite
///
/// If you pass a sprite handle derived from a different scene than the one
/// passed, then behaviour is undefined.
///
/// \param me Scene to operate on
/// \param subject Handle to sprite to alter
/// \param form Index of form to switch to
/// \return 0 on success or an errno on failure
ENDGAME_API int eg_scene_morph(eg_scene_t *me, eg_sprite_handle_p subject,
                               size_t form);

/// remove a sprite from a scene
///
/// This function assumes the sprite being removed was previously added.
///
/// \param me Scene to operate on
/// \param handle Handle to sprite to remove
/// \return 0 on success or an errno on failure
ENDGAME_API int eg_scene_remove(eg_scene_t *me, eg_sprite_handle_p handle);

/// reverse the setup steps from `eg_scene_new`
///
/// After calling this function, `eg_scene_new` must be called again before
/// using any of the other functions in this header.
ENDGAME_API void eg_scene_free(eg_scene_t **me);

#ifdef __cplusplus
}
#endif
