#include <SDL3/SDL.h>
#include <dttr_pcdogs.h>
#include <stdint.h>
#include <string.h>
#include <windows.h>

#include <dttr_config.h>
#include <dttr_gamepad_mapping.h>
#include <dttr_log.h>

#include "hooks_private.h"
#include "inputs_private.h"
#include "sidecar_private.h"

typedef struct {
	LONG x;
	LONG y;
	LONG z;
	LONG rx;
	LONG ry;
	LONG rz;
	LONG sliders[2];
	DWORD pov[4];
	BYTE buttons[32];
} di_joy_state;

enum {
	DTTR_DINPUT_AXIS_FULL_DEFLECTION = 1000,
};

#define DINPUT_POV_CENTERED 0xFFFFFFFF

static void init_poll_state(di_joy_state *state) {
	memset(state, 0, sizeof(*state));

	for (int i = 0; i < 4; i++) {
		state->pov[i] = DINPUT_POV_CENTERED;
	}
}

static void apply_direction_state(
	di_joy_state *state,
	bool dir_up,
	bool dir_down,
	bool dir_left,
	bool dir_right
) {
	if (dir_up) {
		state->y = -DTTR_DINPUT_AXIS_FULL_DEFLECTION;
	}

	if (dir_down) {
		state->y = DTTR_DINPUT_AXIS_FULL_DEFLECTION;
	}

	if (dir_left) {
		state->x = -DTTR_DINPUT_AXIS_FULL_DEFLECTION;
	}

	if (dir_right) {
		state->x = DTTR_DINPUT_AXIS_FULL_DEFLECTION;
	}
}

static LONG read_axis(int axis_idx) {
	const LONG value = dttr_inputs_scale_dinput_axis(
		dttr_inputs_read_raw_axis(axis_idx),
		dttr_config.gamepad_axis_sensitivity[axis_idx]
	);
	const LONG deadzone = dttr_config.gamepad_axis_deadzone[axis_idx];

	return (value > -deadzone && value < deadzone) ? 0 : value;
}

void *__cdecl dttr_inputs_hook_dinput_poll_callback(void *device) {
	di_joy_state *state = NULL;
	if (!REQUIRE_PCDOGS_CALL(DTTR_PCDOGS_F_Mem_MallocCRT->Call(
			dttr_sidecar_runtime_context(),
			sizeof(di_joy_state),
			(void **)&state
		))
		|| !state) {
		DTTR_LOG_ERROR("Failed to allocate joystick poll state");
		return NULL;
	}

	init_poll_state(state);

	if (!dttr_inputs_gamepad || !dttr_config.gamepad_enabled) {
		return state;
	}

	state->x = read_axis(DTTR_GAMEPAD_AXIS_IDX_STICK_X);
	state->y = read_axis(DTTR_GAMEPAD_AXIS_IDX_STICK_Y);
	state->rz = read_axis(DTTR_GAMEPAD_AXIS_IDX_CAMERA_RZ);

	bool dir_up = false;
	bool dir_down = false;
	bool dir_left = false;
	bool dir_right = false;

	for (int src = 0; src < DTTR_GAMEPAD_SOURCE_COUNT; src++) {
		const int action = dttr_config.gamepad_button_map[src];

		if (action == DTTR_GAMEPAD_MAPPING_NONE || !dttr_inputs_source_pressed(src)) {
			continue;
		}

		if (action >= PCDOGS_GAMEPAD_IDX_BTN_0 && action <= PCDOGS_GAMEPAD_IDX_BTN_12) {
			state->buttons[action - PCDOGS_GAMEPAD_IDX_BTN_0] = DINPUT_BUTTON_PRESSED;
			continue;
		}

		switch (action) {
		case PCDOGS_GAMEPAD_IDX_UP:
			dir_up = true;
			break;
		case PCDOGS_GAMEPAD_IDX_DOWN:
			dir_down = true;
			break;
		case PCDOGS_GAMEPAD_IDX_LEFT:
			dir_left = true;
			break;
		case PCDOGS_GAMEPAD_IDX_RIGHT:
			dir_right = true;
			break;
		default:
			break;
		}
	}

	apply_direction_state(state, dir_up, dir_down, dir_left, dir_right);

	return state;
}
