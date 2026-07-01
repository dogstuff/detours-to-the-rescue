#include <dttr_gamepad_mapping.h>

#include <dttr_config.h>

static int32_t clamp_symmetric(int32_t value, int32_t limit) {
	if (value > limit) {
		return limit;
	}

	if (value < -limit) {
		return -limit;
	}

	return value;
}

static int32_t clamp_sensitivity(int32_t sensitivity) {
	return sensitivity < 0 ? 0 : sensitivity;
}

int32_t dttr_inputs_scale_dinput_axis(int32_t axis, int32_t sensitivity) {
	const int64_t scaled = (int64_t)axis * clamp_sensitivity(sensitivity) / 100;
	return clamp_symmetric((int32_t)scaled, DTTR_INPUTS_DINPUT_RANGE);
}

int32_t dttr_inputs_ps1_deadzone_from_dinput(int32_t deadzone) {
	if (deadzone < 0) {
		deadzone = 0;
	} else if (deadzone > DTTR_INPUTS_DINPUT_RANGE) {
		deadzone = DTTR_INPUTS_DINPUT_RANGE;
	}

	return deadzone * DTTR_INPUTS_PS1_FULL_SCALE / DTTR_INPUTS_DINPUT_RANGE;
}

static int32_t clamp_ps1_deadzone(int32_t deadzone) {
	if (deadzone < 0) {
		return 0;
	}

	if (deadzone > DTTR_INPUTS_PS1_FULL_SCALE) {
		return DTTR_INPUTS_PS1_FULL_SCALE;
	}

	return deadzone;
}

static int32_t ps1_deadzone(int32_t pre, int32_t deadzone) {
	deadzone = clamp_ps1_deadzone(deadzone);
	if (pre >= -deadzone && pre <= deadzone) {
		return 0;
	}

	int32_t t = pre > 0 ? 3 * (pre - deadzone) : 3 * (pre + deadzone);
	return (t + (int32_t)((uint32_t)t >> 31)) >> 1; // t/2 rounded toward zero
}

static int32_t ps1_axis_from_dinput(int32_t axis, int32_t deadzone, int32_t sensitivity) {
	axis = dttr_inputs_scale_dinput_axis(axis, sensitivity);
	const int32_t pre = clamp_symmetric(
		-axis * DTTR_INPUTS_PS1_FULL_SCALE / DTTR_INPUTS_DINPUT_RANGE,
		DTTR_INPUTS_PS1_FULL_SCALE
	);

	return ps1_deadzone(pre, deadzone);
}

void dttr_inputs_apply_ps1_stick_axes(
	DTTR_StickAxes *out_axes,
	int32_t dinput_x,
	int32_t dinput_y,
	int32_t deadzone_x,
	int32_t deadzone_y,
	int32_t sensitivity_x,
	int32_t sensitivity_y
) {
	if (!out_axes) {
		return;
	}

	out_axes->axis_x = (int16_t)ps1_axis_from_dinput(dinput_x, deadzone_x, sensitivity_x);
	out_axes->axis_y = (int16_t)ps1_axis_from_dinput(dinput_y, deadzone_y, sensitivity_y);
}
