#include "bootstrap_private.h"

#include <SDL3/SDL.h>

#include <dttr_config.h>
#include <dttr_core.h>
#include <dttr_log.h>
#include <dttr_path.h>
#include <dttr_pcdogs.h>
#include <sds.h>

#include "audio/hooks_private.h"
#include "context_private.h"
#include "crash_private.h"
#include "events_private.h"
#include "game/hooks_private.h"
#include "game_data_private.h"
#include "graphics/graphics_private.h"
#include "graphics/hooks_private.h"
#include "inputs/hooks_private.h"
#include "inputs/inputs_private.h"
#include "movies/hooks_private.h"
#include "movies/movies_private.h"
#include "sidecar_private.h"
#include "timing_private.h"

#ifdef DTTR_MODS_ENABLED
#include "game/frame_pacing_private.h"
#include "graphics/imgui_overlay_private.h"
#include "mods/mods_private.h"
#endif

// Initializes subsystems that own required hooks. The order mirrors
// dttr_bootstrap_cleanup_runtime().
bool dttr_bootstrap_install_required_hooks(const DTTR_Mods_Context *ctx) {
	bool ok = true;
	ok = dttr_game_hooks_init(ctx) && ok;

	dttr_inputs_init();
	ok = dttr_inputs_hooks_init(ctx) && ok;
	ok = dttr_graphics_hooks_init(ctx) && ok;
	ok = dttr_audio_init(ctx) && ok;

	dttr_movies_init();
	ok = dttr_movies_hooks_init(ctx) && ok;

	return ok;
}

// Releases modding runtime hooks and mod state before graphics and audio shutdown.
void dttr_bootstrap_cleanup_runtime(const DTTR_Mods_Context *ctx) {
	dttr_pcdogs_crash_symbols_clear();
	dttr_game_data_cleanup();

#ifdef DTTR_MODS_ENABLED
	dttr_imgui_cleanup();
#endif

	dttr_movies_hooks_cleanup(ctx);
	dttr_movies_cleanup();
	dttr_audio_cleanup(ctx);
	dttr_game_hooks_cleanup(ctx);
	dttr_graphics_hooks_cleanup(ctx);
	dttr_inputs_hooks_cleanup(ctx);
	dttr_inputs_cleanup();
	dttr_graphics_cleanup();
#ifdef DTTR_MODS_ENABLED
	dttr_mods_cleanup();
#endif
	DTTR_Core_HookCleanupAll();
}

// Enters PKG_InitializeSystem through its true per-build entry. EU/SC builds prefix
// the shared body with the instruction that enables the multi-language boot flow
// (the pre-title region/language select); entering past it suppresses that screen.
static bool call_pkg_initialize_system(const DTTR_Core_Context *ctx, int32_t *ret) {
	if (DTTR_PCDOGS_F_PKG_InitializeSystemMultiLanguage->IsCallable(ctx)) {
		return REQUIRE_PCDOGS_CALL(
			DTTR_PCDOGS_F_PKG_InitializeSystemMultiLanguage->Call(ctx, ret)
		);
	}

	return REQUIRE_PCDOGS_CALL(DTTR_PCDOGS_F_PKG_InitializeSystem->Call(ctx, ret));
}

// Runs required PCDOGS startup calls after the game window exists.
bool dttr_bootstrap_initialize_pcdogs_runtime(const DTTR_Core_Context *ctx, HWND hwnd) {
	int32_t ret = 0;

	return REQUIRE_PCDOGS_CALL(DTTR_PCDOGS_F_PKG_FindAndOpenFile->Call(ctx, &ret))
		   && REQUIRE_PCDOGS_CALL(
			   DTTR_PCDOGS_F_PKG_InitializeResourceGameEngine->Call(ctx, &ret)
		   )
		   && REQUIRE_PCDOGS_CALL(
			   DTTR_PCDOGS_F_D3D_InitializeGraphicsSubsystem->Call(ctx, hwnd, NULL, &ret)
		   )
		   && call_pkg_initialize_system(ctx, &ret);
}

// Moves the modding runtime into its started state after initialization succeeds.
bool dttr_bootstrap_start_pcdogs_runtime(const DTTR_Core_Context *ctx, HWND hwnd) {
	int32_t ret = 0;
	int32_t config_ret = 0;
	return REQUIRE_PCDOGS_CALL(DTTR_PCDOGS_F_Display_SetMode->Call(ctx, hwnd, &ret))
		   && REQUIRE_PCDOGS_CALL(DTTR_PCDOGS_F_Input_ResetState->Call(ctx, &ret))
		   && REQUIRE_PCDOGS_CALL(
			   DTTR_PCDOGS_F_Config_LoadAlternateFromINI->Call(ctx, &config_ret)
		   );
}

// Runs per-frame sidecar systems before yielding back to the original game loop.
bool dttr_bootstrap_tick_main_loop() {
	if (dttr_movies_is_playing()) {
		dttr_movies_tick();
		return true;
	}

	SDL_DelayNS(1);

	int32_t rendering_enabled = 0;
	if (!REQUIRE_PCDOGS_CALL(
			DTTR_PCDOGS_D_Window_RunWinMain_RenderingEnabled->Read(&rendering_enabled)
		)) {
		return false;
	}

	if (rendering_enabled) {
#ifdef DTTR_MODS_ENABLED
		dttr_timing_host_frame_begin();

		bool ran_simulation_step = false;
		while (dttr_timing_should_run_simulation_step()) {
			dttr_timing_before_simulation_step();
			dttr_game_neutralize_frame_limiter(dttr_sidecar_runtime_context());

			uint8_t frame_status = 0;
			const bool rendered = REQUIRE_PCDOGS_CALL(
				DTTR_PCDOGS_F_Graphics_RenderFrame
					->Call(dttr_sidecar_runtime_context(), &frame_status)
			);
			if (!rendered) {
				dttr_timing_after_simulation_step();
				dttr_timing_host_frame_end();
				return false;
			}

			dttr_timing_after_simulation_step();
			dttr_mods_game_frame_advanced();
			ran_simulation_step = true;
		}

		if (dttr_timing_has_deferred_simulation_step()) {
			dttr_timing_simulation_step_deferred();
		}

		if (!ran_simulation_step) {
			dttr_graphics_begin_frame();
			dttr_graphics_set_render_frame_brackets_suppressed(
				dttr_game_render_only_scene_replay()
			);
			dttr_graphics_end_frame();
			dttr_graphics_set_render_frame_brackets_suppressed(false);
		}

		dttr_timing_host_frame_end();

#else
		uint8_t frame_status = 0;
		if (!REQUIRE_PCDOGS_CALL(DTTR_PCDOGS_F_Graphics_RenderFrame->Call(
				dttr_sidecar_runtime_context(),
				&frame_status
			))) {
			return false;
		}
#endif
	}

#ifdef DTTR_MODS_ENABLED
	dttr_mods_tick();
#endif
	return true;
}

// Plays startup movies through the normal sidecar tick loop.
dttr_startup_movies_result dttr_bootstrap_attempt_play_startup_movies() {
	if (dttr_config.skip_intro_movies) {
		return DTTR_STARTUP_MOVIES_CONTINUE;
	}

	const char *prefix = DTTR_PCDOGS_D_Video_PlayMovieIntro_PathPrefix->Ptr();
	char **names = (char **)DTTR_PCDOGS_D_Video_PlayMovieIntro_FileNames->Ptr();
	if (!prefix || !names) {
		DTTR_LOG_WARN("Startup movie metadata unavailable; skipping intro movies");
		return DTTR_STARTUP_MOVIES_CONTINUE;
	}

	for (int i = 0; i < 4; i++) {
		if (!names[i]) {
			break;
		}

		sds path = sdsnew(prefix);
		if (!path || !DTTR_Path_AppendSegment(&path, names[i], '\\')) {
			sdsfree(path);
			break;
		}

		dttr_movies_start(path);
		sdsfree(path);

		while (dttr_movies_is_playing()) {
			dttr_sidecar_poll_sdl_events();
			if (!dttr_bootstrap_tick_main_loop()) {
				dttr_movies_stop();
				return DTTR_STARTUP_MOVIES_FAILED;
			}
		}

		const dttr_movie_result ret = dttr_movies_stop();

		if (ret == DTTR_MOVIE_QUIT) {
			if (!REQUIRE_PCDOGS_CALL(
					DTTR_PCDOGS_D_Input_ProcessWindowMessages_ShouldQuit->Write(1)
				)) {
				return DTTR_STARTUP_MOVIES_FAILED;
			}

			return DTTR_STARTUP_MOVIES_QUIT;
		}

		if (ret != DTTR_MOVIE_ENDED) {
			break;
		}
	}

	return DTTR_STARTUP_MOVIES_CONTINUE;
}
