#ifndef DTTR_GAMEPAD_MAPPING_H
#define DTTR_GAMEPAD_MAPPING_H

#include <stdint.h>

enum {
	DTTR_INPUTS_PS1_FULL_SCALE = 0x1000,
	DTTR_INPUTS_DINPUT_RANGE = 1000,
};

typedef struct {
	int16_t axis_x;
	int16_t axis_y;
} DTTR_StickAxes;

int32_t dttr_inputs_scale_dinput_axis(int32_t axis, int32_t sensitivity);
int32_t dttr_inputs_ps1_deadzone_from_dinput(int32_t deadzone);
void dttr_inputs_apply_ps1_stick_axes(
	DTTR_StickAxes *out_axes,
	int32_t dinput_x,
	int32_t dinput_y,
	int32_t deadzone_x,
	int32_t deadzone_y,
	int32_t sensitivity_x,
	int32_t sensitivity_y
);

#endif // DTTR_GAMEPAD_MAPPING_H
