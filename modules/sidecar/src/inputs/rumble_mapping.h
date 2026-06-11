#ifndef DTTR_INPUTS_RUMBLE_MAPPING_H
#define DTTR_INPUTS_RUMBLE_MAPPING_H

#include <stdbool.h>
#include <stdint.h>

typedef struct dttr_inputs_rumble_request {
	uint16_t low_frequency;
	uint16_t high_frequency;
	uint32_t duration_ms;
	bool active;
} dttr_inputs_rumble_request;

dttr_inputs_rumble_request dttr_inputs_make_rumble_request(
	int32_t strong_feedback,
	int32_t force_magnitude_fixed,
	int32_t duration_units
);

#endif // DTTR_INPUTS_RUMBLE_MAPPING_H
