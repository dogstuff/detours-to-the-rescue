#ifndef DTTR_GAME_FRAME_PACING_PRIVATE_H
#define DTTR_GAME_FRAME_PACING_PRIVATE_H

#include <stdbool.h>

#include <dttr_mods.h>

// Rewinds the game's frame-limiter timestamp so its 30 FPS deadline never skips
// a host-paced simulation step.
void dttr_game_neutralize_frame_limiter(const DTTR_Core_Context *ctx);
// Re-rasterizes the scene on a render-only host frame inside the render timing
// bracket, so transforms mods blend at interpolation_alpha reach the frame instead
// of re-presenting the previous image. Returns true when the bracket fired and the
// backend end-frame bracket must stay suppressed.
bool dttr_game_render_only_scene_replay();

#endif // DTTR_GAME_FRAME_PACING_PRIVATE_H
