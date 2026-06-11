#include <SDL3/SDL.h>
#include <dttr_pcdogs.h>

#include <dttr_config.h>
#include <dttr_log.h>

#include "hooks_private.h"
#include "inputs_private.h"
#include "rumble_mapping.h"
#include "sidecar_private.h"

DTTR_PCDOGS_F_Input_TriggerRumbleIfAllowed_proto dttr_inputs_hook_rumble_original;

static const DTTR_Mods_Context *rumble_ctx;
static bool sdl_rumble_warning_logged;

static int32_t run_original_rumble(
	int32_t effect_source_id,
	int32_t strong_feedback,
	int32_t force_magnitude_fixed,
	int32_t duration_units
) {
	if (!dttr_inputs_hook_rumble_original) {
		return 0;
	}

	return dttr_inputs_hook_rumble_original(
		effect_source_id,
		strong_feedback,
		force_magnitude_fixed,
		duration_units
	);
}

bool dttr_inputs_hook_rumble_prepare(const DTTR_Mods_Context *ctx) {
	if (!ctx) {
		return false;
	}

	if (!DTTR_PCDOGS_F_Input_TriggerRumbleIfAllowed->IsCallable(&ctx->runtime)
		|| !DTTR_PCDOGS_F_Settings_GetRumbleSuppressFlag->IsCallable(&ctx->runtime)) {
		return false;
	}

	rumble_ctx = ctx;
	return true;
}

void dttr_inputs_hook_rumble_reset() {
	rumble_ctx = NULL;
	dttr_inputs_hook_rumble_original = NULL;
	sdl_rumble_warning_logged = false;
}

int32_t __cdecl dttr_inputs_hook_rumble_callback(
	int32_t effect_source_id,
	int32_t strong_feedback,
	int32_t force_magnitude_fixed,
	int32_t duration_units
) {
	if (!dttr_config.gamepad_enabled) {
		return 0;
	}

	int32_t suppress_rumble = 0;
	if (!rumble_ctx
		|| !DTTR_ResultOK(DTTR_PCDOGS_F_Settings_GetRumbleSuppressFlag
							  ->Call(&rumble_ctx->runtime, &suppress_rumble))) {
		return run_original_rumble(
			effect_source_id,
			strong_feedback,
			force_magnitude_fixed,
			duration_units
		);
	}

	if (suppress_rumble != 0) {
		return 0;
	}

	const dttr_inputs_rumble_request request = dttr_inputs_make_rumble_request(
		strong_feedback,
		force_magnitude_fixed,
		duration_units
	);
	if (!request.active) {
		return 0;
	}

	if (dttr_inputs_gamepad) {
		if (SDL_RumbleGamepad(
				dttr_inputs_gamepad,
				request.low_frequency,
				request.high_frequency,
				request.duration_ms
			)) {
			return 0;
		}

		if (!sdl_rumble_warning_logged) {
			sdl_rumble_warning_logged = true;
			DTTR_LOG_WARN("SDL gamepad rumble failed: %s", SDL_GetError());
		}
	}

	return run_original_rumble(
		effect_source_id,
		strong_feedback,
		force_magnitude_fixed,
		duration_units
	);
}
