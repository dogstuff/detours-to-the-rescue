#include <SDL3/SDL.h>
#include <dttr_pcdogs.h>

#include <dttr_config.h>
#include <dttr_log.h>

#include "hooks_private.h"
#include "inputs_private.h"
#include "sidecar_private.h"

DTTR_PCDOGS_F_Input_TriggerRumbleIfAllowed_proto dttr_inputs_hook_rumble_original;

static const DTTR_Mods_Context *rumble_ctx;
static bool sdl_rumble_warning_logged;

enum {
	DTTR_INPUTS_RUMBLE_FIXED_ONE = 0x1000,
	DTTR_INPUTS_RUMBLE_WEAK_SCALE = 5000,
	DTTR_INPUTS_RUMBLE_STRONG_SCALE = 10000,
	DTTR_INPUTS_RUMBLE_DURATION_UNIT_MS = 100,
};

typedef struct {
	uint16_t low_frequency;
	uint16_t high_frequency;
	uint32_t duration_ms;
	bool active;
} dttr_inputs_rumble_request;

static int32_t clamp_i32(int32_t value, int32_t min, int32_t max) {
	if (value < min) {
		return min;
	}

	if (value > max) {
		return max;
	}

	return value;
}

static uint32_t clamp_duration_units(int32_t duration_units) {
	if (duration_units <= 0) {
		return 0;
	}

	const uint32_t max_units = UINT32_MAX / DTTR_INPUTS_RUMBLE_DURATION_UNIT_MS;
	if ((uint32_t)duration_units > max_units) {
		return max_units;
	}

	return (uint32_t)duration_units;
}

static uint16_t game_force_to_sdl_intensity(
	int32_t strong_feedback,
	int32_t force_magnitude_fixed
) {
	const int32_t fixed_magnitude = clamp_i32(
		force_magnitude_fixed,
		0,
		DTTR_INPUTS_RUMBLE_FIXED_ONE
	);
	const int32_t force_scale = strong_feedback ? DTTR_INPUTS_RUMBLE_STRONG_SCALE
												: DTTR_INPUTS_RUMBLE_WEAK_SCALE;

	const uint32_t game_force = ((uint32_t)fixed_magnitude * (uint32_t)force_scale)
								/ DTTR_INPUTS_RUMBLE_FIXED_ONE;

	return (uint16_t)((game_force * UINT16_MAX + (DTTR_INPUTS_RUMBLE_STRONG_SCALE / 2))
					  / DTTR_INPUTS_RUMBLE_STRONG_SCALE);
}

static dttr_inputs_rumble_request make_rumble_request(
	int32_t strong_feedback,
	int32_t force_magnitude_fixed,
	int32_t duration_units
) {
	const uint32_t duration_ms = clamp_duration_units(duration_units)
								 * DTTR_INPUTS_RUMBLE_DURATION_UNIT_MS;
	const uint16_t intensity = game_force_to_sdl_intensity(
		strong_feedback,
		force_magnitude_fixed
	);

	if (duration_ms == 0 || intensity == 0) {
		return (dttr_inputs_rumble_request){0};
	}

	return (dttr_inputs_rumble_request){
		.low_frequency = intensity,
		.high_frequency = intensity,
		.duration_ms = duration_ms,
		.active = true,
	};
}

static int32_t safe_call_original_rumble(
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
		return safe_call_original_rumble(
			effect_source_id,
			strong_feedback,
			force_magnitude_fixed,
			duration_units
		);
	}

	if (suppress_rumble != 0) {
		return 0;
	}

	const dttr_inputs_rumble_request request = make_rumble_request(
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

	return safe_call_original_rumble(
		effect_source_id,
		strong_feedback,
		force_magnitude_fixed,
		duration_units
	);
}
