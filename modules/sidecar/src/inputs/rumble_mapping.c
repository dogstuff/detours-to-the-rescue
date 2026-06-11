#include "inputs/rumble_mapping.h"

#include <stdint.h>

enum {
	DTTR_INPUTS_RUMBLE_FIXED_ONE = 0x1000,
	DTTR_INPUTS_RUMBLE_WEAK_SCALE = 5000,
	DTTR_INPUTS_RUMBLE_STRONG_SCALE = 10000,
	DTTR_INPUTS_RUMBLE_DURATION_UNIT_MS = 100,
};

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

dttr_inputs_rumble_request dttr_inputs_make_rumble_request(
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
