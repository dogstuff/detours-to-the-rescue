#ifndef DTTR_GAME_FRAME_PACING_PRIVATE_H
#define DTTR_GAME_FRAME_PACING_PRIVATE_H

#include <stdbool.h>
#include <stdint.h>

#include <dttr_core.h>

typedef struct {
	uint32_t flags;
	uint32_t last_tick;
} dttr_game_frame_progress;

bool dttr_game_scene_is_steady(void);
bool dttr_game_frame_progress_read(dttr_game_frame_progress *out);
// Compare snapshots taken immediately around a successful original frame call.
// Take the first snapshot after any limiter neutralization.
bool dttr_game_frame_advanced(
	const dttr_game_frame_progress *before,
	const dttr_game_frame_progress *after
);

#ifdef DTTR_MODS_ENABLED
// Opens the native limiter for a fixed simulation step in the steady scene.
void dttr_game_neutralize_frame_limiter(const DTTR_Core_Context *ctx);
bool dttr_game_render_only_scene_replay(void);
#endif

#endif // DTTR_GAME_FRAME_PACING_PRIVATE_H
