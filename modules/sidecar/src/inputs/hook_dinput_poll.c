#include <SDL3/SDL.h>
#include <dttr_pcdogs.h>
#include <dttr_sidecar.h>
#include <stdint.h>
#include <string.h>
#include <windows.h>

#include <dttr_log.h>

#include "hooks_private.h"
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

/// DirectInput sentinel for a centered/neutral POV hat switch.
#define DINPUT_POV_CENTERED 0xFFFFFFFF

/// DirectInput uses this byte value to indicate a button is pressed.
#define DINPUT_BUTTON_PRESSED 0x80

// Starts each emulated DirectInput poll with neutral axes, POV hats, and buttons.
static void init_poll_state(di_joy_state *state) {
	memset(state, 0, sizeof(*state));

	for (int i = 0; i < 4; i++) {
		state->pov[i] = DINPUT_POV_CENTERED;
	}
}

// Maps digital direction bindings to DirectInput axis deflection for the game poll result.
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

// Reads one configured SDL button or trigger source for the DirectInput button map.
static bool is_source_pressed(int source) {
	if (!dttr_gamepad) {
		return false;
	}

	if (source == DTTR_GAMEPAD_SOURCE_TRIGGER_LEFT
		|| source == DTTR_GAMEPAD_SOURCE_TRIGGER_RIGHT) {
		const SDL_GamepadAxis axis = (source == DTTR_GAMEPAD_SOURCE_TRIGGER_LEFT)
										 ? SDL_GAMEPAD_AXIS_LEFT_TRIGGER
										 : SDL_GAMEPAD_AXIS_RIGHT_TRIGGER;
		return SDL_GetGamepadAxis(dttr_gamepad, axis) / DTTR_DINPUT_AXIS_SCALE
			   > DTTR_GAMEPAD_TRIGGER_THRESHOLD;
	}

	return SDL_GetGamepadButton(dttr_gamepad, (SDL_GamepadButton)source);
}

// Reads one configured SDL axis and applies the per-axis deadzone.
static LONG read_axis(int axis_idx) {
	const int sdl_axis = dttr_config.gamepad_axes[axis_idx];

	if (!dttr_gamepad || sdl_axis == DTTR_GAMEPAD_MAPPING_NONE) {
		return 0;
	}

	const LONG value = SDL_GetGamepadAxis(dttr_gamepad, sdl_axis)
					   / DTTR_DINPUT_AXIS_SCALE;
	const LONG deadzone = dttr_config.gamepad_axis_deadzone[axis_idx];

	return (value > -deadzone && value < deadzone) ? 0 : value;
}

// Fills the joystick state block expected by the game from SDL gamepad input.
void *__cdecl dttr_inputs_hook_dinput_poll_callback(void *device) {
	di_joy_state *state = DTTR_PCDOGS_F_CRTMalloc->Call(
		dttr_sidecar_runtime_context(),
		sizeof(di_joy_state),
		NULL
	);

	if (!state) {
		DTTR_LOG_ERROR("Failed to allocate joystick poll state");
		return NULL;
	}

	init_poll_state(state);

	if (!dttr_gamepad || !dttr_config.gamepad_enabled) {
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
