#include <SDL3/SDL.h>
#include <dttr_pcdogs.h>
#include <stdint.h>
#include <string.h>
#include <windows.h>

#include <dttr_config.h>
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
	DTTR_DINPUT_AXIS_SCALE = 32,
	DTTR_DINPUT_AXIS_FULL_DEFLECTION = 1000,
};

#define DINPUT_POV_CENTERED 0xFFFFFFFF

#define DINPUT_BUTTON_PRESSED 0x80

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

static bool is_source_pressed(int source) {
	if (!dttr_inputs_gamepad) {
		return false;
	}

	if (source == DTTR_GAMEPAD_SOURCE_TRIGGER_LEFT
		|| source == DTTR_GAMEPAD_SOURCE_TRIGGER_RIGHT) {
		const SDL_GamepadAxis axis = (source == DTTR_GAMEPAD_SOURCE_TRIGGER_LEFT)
										 ? SDL_GAMEPAD_AXIS_LEFT_TRIGGER
										 : SDL_GAMEPAD_AXIS_RIGHT_TRIGGER;
		return SDL_GetGamepadAxis(dttr_inputs_gamepad, axis) / DTTR_DINPUT_AXIS_SCALE
			   > DTTR_GAMEPAD_TRIGGER_THRESHOLD;
	}

	return SDL_GetGamepadButton(dttr_inputs_gamepad, (SDL_GamepadButton)source);
}

static LONG read_axis(int axis_idx) {
	const int sdl_axis = dttr_config.gamepad_axes[axis_idx];

	if (!dttr_inputs_gamepad || sdl_axis == DTTR_GAMEPAD_MAPPING_NONE) {
		return 0;
	}

	const LONG value = SDL_GetGamepadAxis(dttr_inputs_gamepad, sdl_axis)
					   / DTTR_DINPUT_AXIS_SCALE;
	const LONG deadzone = dttr_config.gamepad_axis_deadzone[axis_idx];

	return (value > -deadzone && value < deadzone) ? 0 : value;
}

void *__cdecl dttr_inputs_hook_dinput_poll_callback(void *device) {
	di_joy_state *state = NULL;
	DTTR_Result alloc_result = DTTR_PCDOGS_F_Mem_MallocCRT->Call(
		dttr_sidecar_runtime_context(),
		sizeof(di_joy_state),
		(void **)&state
	);

	if (!DTTR_ResultOK(alloc_result) || !state) {
		DTTR_LOG_ERROR(
			"Failed to allocate joystick poll state: %s",
			dttr_sidecar_result_detail(alloc_result)
		);
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

		if (action == DTTR_GAMEPAD_MAPPING_NONE || !is_source_pressed(src)) {
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
