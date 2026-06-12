#include "context_private.h"
#include "frame_pacing_private.h"
#include "sidecar_private.h"
#include "timing_private.h"

#include <dttr_log.h>
#include <dttr_pcdogs.h>

// Game_FrameTransitionFlags bit marking the active scene update/render state.
#define DTTR_GAME_FLAG_SCENE_ACTIVE 0x2u

// Game_FrameTransitionFlags load/unload/transition bits under which the scene
// render path must not be re-entered.
#define DTTR_GAME_FLAGS_SCENE_REPLAY_BLOCKED (0x4u | 0x8u | 0x1000u | 0x2000u | 0x4000u)

// The game's internal frame limiter skips Graphics_RenderFrame while
// last_frame_tick + 0x21 ms lies in the future.
#define DTTR_GAME_FRAME_LIMITER_WINDOW_MS 0x21u

void dttr_game_neutralize_frame_limiter(const DTTR_Core_Context *ctx) {
	if (!dttr_timing_fixed_policy_active()
		|| !DTTR_PCDOGS_D_Graphics_RenderFrame_LastFrameTick->IsResolved()) {
		return;
	}

	int32_t now = 0;
	if (!REQUIRE_PCDOGS_CALL(DTTR_PCDOGS_F_Timer_GetElapsedTickCount->Call(ctx, &now))) {
		return;
	}

	REQUIRE_PCDOGS_CALL(DTTR_PCDOGS_D_Graphics_RenderFrame_LastFrameTick->Write(
		(uint32_t)now - DTTR_GAME_FRAME_LIMITER_WINDOW_MS - 1u
	));
}

// True when the game is in its steady scene-rendering state and a render-only
// host frame may re-present the last completed scene without re-entering
// scene traversal.
static bool game_scene_represent_allowed(const DTTR_Core_Context *ctx) {
	if (!dttr_timing_fixed_policy_active()
		|| !DTTR_PCDOGS_D_Game_FrameTransitionFlags->IsResolved()
		|| !DTTR_PCDOGS_D_Game_PauseStateCounter->IsResolved()) {
		return false;
	}

	uint32_t flags = 0;
	uint8_t pause_counter = 0;
	if (!REQUIRE_PCDOGS_CALL(DTTR_PCDOGS_D_Game_FrameTransitionFlags->Read(&flags))
		|| !REQUIRE_PCDOGS_CALL(
			DTTR_PCDOGS_D_Game_PauseStateCounter->Read(&pause_counter)
		)) {
		return false;
	}

	return (flags & DTTR_GAME_FLAG_SCENE_ACTIVE) != 0
		   && (flags & DTTR_GAME_FLAGS_SCENE_REPLAY_BLOCKED) == 0 && pause_counter == 0;
}

bool dttr_game_render_only_scene_replay() {
	const DTTR_Core_Context *ctx = dttr_sidecar_runtime_context();
	return game_scene_represent_allowed(ctx);
}
