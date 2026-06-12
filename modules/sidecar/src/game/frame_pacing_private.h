#ifndef DTTR_GAME_FRAME_PACING_PRIVATE_H
#define DTTR_GAME_FRAME_PACING_PRIVATE_H

#include <stdbool.h>

#include <dttr_mods.h>

// Rewinds the game's frame-limiter timestamp so its 30 FPS deadline never skips
// a host-paced simulation step.
void dttr_game_neutralize_frame_limiter(const DTTR_Core_Context *ctx);
// Allows a render-only host frame to re-present the last completed native scene
// frame.
bool dttr_game_render_only_scene_replay();

#endif // DTTR_GAME_FRAME_PACING_PRIVATE_H
