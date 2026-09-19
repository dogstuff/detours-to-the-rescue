#include "frame_pacing_private.h"
#include "sidecar_private.h"

#ifdef DTTR_MODS_ENABLED
#include "timing_private.h"
#endif

#include <dttr_pcdogs.h>

#define DTTR_GAME_FLAG_SCENE_ACTIVE 0x2u
#define DTTR_GAME_FLAG_UNPACED 0x2000u
#define DTTR_GAME_FLAGS_SCENE_REPLAY_BLOCKED (0x4u | 0x8u | 0x1000u | 0x2000u | 0x4000u)
#define DTTR_GAME_FRAME_LIMITER_WINDOW_MS 0x21u

bool dttr_game_scene_is_steady(void) {
	if (!DTTR_PCDOGS_D_Game_FrameTransitionFlags->IsResolved()
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

bool dttr_game_frame_progress_read(dttr_game_frame_progress *out) {
	if (!out || !DTTR_PCDOGS_D_Game_FrameTransitionFlags->IsResolved()
		|| !DTTR_PCDOGS_D_Graphics_RenderFrame_LastFrameTick->IsResolved()) {
		return false;
	}

	dttr_game_frame_progress progress;
	if (!REQUIRE_PCDOGS_CALL(DTTR_PCDOGS_D_Game_FrameTransitionFlags->Read(&progress.flags))
		|| !REQUIRE_PCDOGS_CALL(
			DTTR_PCDOGS_D_Graphics_RenderFrame_LastFrameTick->Read(&progress.last_tick)
		)) {
		return false;
	}

	*out = progress;
	return true;
}

bool dttr_game_frame_advanced(
	const dttr_game_frame_progress *before,
	const dttr_game_frame_progress *after
) {
	// The unpaced path enters the update pipeline without writing the limiter tick.
	return (before->flags & DTTR_GAME_FLAG_UNPACED) != 0
		   || before->last_tick != after->last_tick;
}

#ifdef DTTR_MODS_ENABLED
void dttr_game_neutralize_frame_limiter(const DTTR_Core_Context *ctx) {
	if (!dttr_timing_fixed_policy_active() || !dttr_game_scene_is_steady()
		|| !DTTR_PCDOGS_D_Graphics_RenderFrame_LastFrameTick->IsResolved()) {
		return;
	}

	int32_t now = 0;
	if (!REQUIRE_PCDOGS_CALL(DTTR_PCDOGS_F_Timer_GetElapsedTickCount->Call(ctx, &now))) {
		return;
	}

	// The unsigned gate accepts equality; this also works when the timer is zero.
	REQUIRE_PCDOGS_CALL(DTTR_PCDOGS_D_Graphics_RenderFrame_LastFrameTick->Write(
		(uint32_t)now - DTTR_GAME_FRAME_LIMITER_WINDOW_MS
	));
}

bool dttr_game_render_only_scene_replay(void) {
	return dttr_timing_fixed_policy_active() && dttr_game_scene_is_steady();
}
#endif
