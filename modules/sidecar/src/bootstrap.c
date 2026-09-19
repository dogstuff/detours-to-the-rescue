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
#include "game/frame_pacing_private.h"
#include "game/hooks_private.h"
#include "game/host_pacing.h"
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
#include "graphics/imgui_overlay_private.h"
#include "mods/mods_private.h"
#endif

static dttr_host_pacing host_pacing;
static bool update_rate_limiter_cap_warned;

static int effective_update_rate_limiter_cap() {
	const int cap = DTTR_Config_EffectiveUpdateRateLimiterCap(&dttr_config);

	if (dttr_config.update_rate_limiter && cap != dttr_config.update_rate_limiter_cap
		&& !update_rate_limiter_cap_warned) {
		DTTR_LOG_WARN(
			"Invalid update_rate_limiter_cap=%d; using %d",
			dttr_config.update_rate_limiter_cap,
			cap
		);
		update_rate_limiter_cap_warned = true;
	}

	return cap;
}

static uint64_t pace_host_tick(int cap, bool fixed_policy, bool steady_scene) {
	uint64_t now = SDL_GetTicksNS();
	const uint64_t wait = dttr_host_pacing_wait_ns(
		&host_pacing,
		now,
		cap,
		fixed_policy,
		steady_scene
	);
	if (wait > 0) {
		SDL_DelayPrecise(wait);
		now = SDL_GetTicksNS();
	}
	return now;
}

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
	host_pacing = (dttr_host_pacing){0};
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
static bool call_pkg_initialize_system(const DTTR_Core_Context *ctx, HWND hwnd) {
	HINSTANCE h_instance = GetModuleHandleA(NULL);

	if (DTTR_PCDOGS_F_PKG_InitializeSystemMultiLanguage->IsCallable(ctx)) {
		return REQUIRE_PCDOGS_CALL(
			DTTR_PCDOGS_F_PKG_InitializeSystemMultiLanguage->Call(ctx, hwnd, h_instance)
		);
	}

	return REQUIRE_PCDOGS_CALL(
		DTTR_PCDOGS_F_PKG_InitializeSystem->Call(ctx, hwnd, h_instance)
	);
}

// Runs required PCDOGS startup calls after the game window exists.
bool dttr_bootstrap_initialize_pcdogs_runtime(const DTTR_Core_Context *ctx, HWND hwnd) {
	bool resource_engine_initialized = false;

	return REQUIRE_PCDOGS_CALL(DTTR_PCDOGS_F_PKG_LocatePackagePath->Call(ctx))
		   && REQUIRE_PCDOGS_CALL(DTTR_PCDOGS_F_PKG_InitializeResourceGameEngine
									  ->Call(ctx, &resource_engine_initialized))
		   && REQUIRE_PCDOGS_CALL(
			   DTTR_PCDOGS_F_Input_InitializeInputSubsystem->Call(ctx, hwnd, NULL)
		   )
		   && call_pkg_initialize_system(ctx, hwnd);
}

// Moves the modding runtime into its started state after initialization succeeds.
bool dttr_bootstrap_start_pcdogs_runtime(const DTTR_Core_Context *ctx, HWND hwnd) {
	int32_t ret = 0;
	return REQUIRE_PCDOGS_CALL(DTTR_PCDOGS_F_Display_SetMode->Call(ctx, hwnd, &ret))
		   && REQUIRE_PCDOGS_CALL(DTTR_PCDOGS_F_Input_ResetState->Call(ctx))
		   && REQUIRE_PCDOGS_CALL(DTTR_PCDOGS_F_Config_LoadAlternateFromINI->Call(ctx));
}

// Runs per-frame sidecar systems before yielding back to the original game loop.
bool dttr_bootstrap_tick_main_loop() {
	if (dttr_movies_is_playing()) {
		host_pacing = (dttr_host_pacing){0};
		dttr_sidecar_poll_sdl_events();
		dttr_movies_tick();
		return true;
	}

	const int cap = effective_update_rate_limiter_cap();
	bool fixed_policy = false;
#ifdef DTTR_MODS_ENABLED
	fixed_policy = dttr_timing_fixed_policy_active();
#endif
	const uint64_t tick_start = pace_host_tick(
		cap,
		fixed_policy,
		dttr_game_scene_is_steady()
	);
	// Sample input after waiting so simulation sees the latest device state.
	dttr_sidecar_poll_sdl_events();
	const bool steady_scene = dttr_game_scene_is_steady();

	int32_t rendering_enabled = 0;
	if (!REQUIRE_PCDOGS_CALL(
			DTTR_PCDOGS_D_Window_RunWinMain_RenderingEnabled->Read(&rendering_enabled)
		)) {
		return false;
	}

	bool advanced = false;
	bool progress_available = true;
	if (rendering_enabled) {
#ifdef DTTR_MODS_ENABLED
		dttr_timing_host_frame_begin(!steady_scene);
		fixed_policy = dttr_timing_fixed_policy_active();

		while (dttr_timing_should_run_simulation_step()) {
			dttr_timing_before_simulation_step();
			dttr_game_neutralize_frame_limiter(dttr_sidecar_runtime_context());

			dttr_game_frame_progress before, after;
			progress_available = dttr_game_frame_progress_read(&before);
			uint8_t frame_status = 0;
			if (!REQUIRE_PCDOGS_CALL(DTTR_PCDOGS_F_Graphics_RenderFrame->Call(
					dttr_sidecar_runtime_context(),
					&frame_status
				))) {
				dttr_timing_cancel_simulation_step();
				dttr_timing_host_frame_end();
				return false;
			}
			progress_available = progress_available
								 && dttr_game_frame_progress_read(&after);
			if (!progress_available) {
				// Optional timing symbols must never prevent the original game loop
				// from progressing, including loading and unsupported game builds.
				dttr_timing_cancel_simulation_step();
				dttr_timing_use_native_policy();
				fixed_policy = false;
				break;
			}

			if (!dttr_game_frame_advanced(&before, &after)) {
				dttr_timing_cancel_simulation_step();
				break;
			}
			dttr_timing_after_simulation_step();
			dttr_mods_game_frame_advanced();
			advanced = true;
			// A step may initiate loading. Resume unrestricted host ticks instead
			// of consuming the remaining gameplay catch-up budget.
			if (!dttr_game_scene_is_steady()) {
				break;
			}
		}

		if (dttr_timing_has_deferred_simulation_step()) {
			dttr_timing_simulation_step_deferred();
		}

		if (!advanced && dttr_game_render_only_scene_replay()) {
			dttr_graphics_begin_frame();
			dttr_graphics_end_frame();
		}
		dttr_timing_host_frame_end();
#else
		const bool track_progress = cap > 0 && cap <= 30 && steady_scene;
		dttr_game_frame_progress before, after;
		if (track_progress) {
			progress_available = dttr_game_frame_progress_read(&before);
		}
		uint8_t frame_status = 0;
		if (!REQUIRE_PCDOGS_CALL(DTTR_PCDOGS_F_Graphics_RenderFrame->Call(
				dttr_sidecar_runtime_context(),
				&frame_status
			))) {
			return false;
		}
		if (track_progress && progress_available) {
			progress_available = dttr_game_frame_progress_read(&after);
			advanced = progress_available && dttr_game_frame_advanced(&before, &after);
		}
#endif
	}

	dttr_host_pacing_complete(
		&host_pacing,
		tick_start,
		cap,
		fixed_policy,
		rendering_enabled && progress_available && dttr_game_scene_is_steady(),
		advanced
	);
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
