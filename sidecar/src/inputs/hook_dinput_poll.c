#include <SDL3/SDL.h>
#include <dttr_sidecar.h>
#include <stdint.h>
#include <string.h>
#include <windows.h>

#include "log.h"

typedef struct {
	LONG m_x;
	LONG m_y;
	LONG m_z;
	LONG m_rx;
	LONG m_ry;
	LONG m_rz;
	LONG m_sliders[2];
	DWORD m_pov[4];
	BYTE m_buttons[32];
} S_DIJoyState;

enum {
	DTTR_DINPUT_AXIS_SCALE = 32,
	DTTR_DINPUT_AXIS_FULL_DEFLECTION = 1000,
};

/// DirectInput sentinel for a centered/neutral POV hat switch
#define DINPUT_POV_CENTERED 0xFFFFFFFF

/// DirectInput uses this byte value to indicate a button is pressed
#define DINPUT_BUTTON_PRESSED 0x80

static void s_init_poll_state(S_DIJoyState *state) {
	memset(state, 0, sizeof(*state));

	for (int i = 0; i < 4; i++) {
		state->m_pov[i] = DINPUT_POV_CENTERED;
	}
}

static void s_apply_direction_state(
	S_DIJoyState *state,
	bool dir_up,
	bool dir_down,
	bool dir_left,
	bool dir_right
) {
	if (dir_up) {
		state->m_y = -DTTR_DINPUT_AXIS_FULL_DEFLECTION;
	}
	if (dir_down) {
		state->m_y = DTTR_DINPUT_AXIS_FULL_DEFLECTION;
	}
	if (dir_left) {
		state->m_x = -DTTR_DINPUT_AXIS_FULL_DEFLECTION;
	}
	if (dir_right) {
		state->m_x = DTTR_DINPUT_AXIS_FULL_DEFLECTION;
	}
}

static bool s_is_source_pressed(int source) {
	if (!g_dttr_gamepad) {
		return false;
	}

	if (source == DTTR_GAMEPAD_SOURCE_TRIGGER_LEFT
		|| source == DTTR_GAMEPAD_SOURCE_TRIGGER_RIGHT) {
		const SDL_GamepadAxis axis = (source == DTTR_GAMEPAD_SOURCE_TRIGGER_LEFT)
										 ? SDL_GAMEPAD_AXIS_LEFT_TRIGGER
										 : SDL_GAMEPAD_AXIS_RIGHT_TRIGGER;
		return SDL_GetGamepadAxis(g_dttr_gamepad, axis) / DTTR_DINPUT_AXIS_SCALE
			   > DTTR_GAMEPAD_TRIGGER_THRESHOLD;
	}

	return SDL_GetGamepadButton(g_dttr_gamepad, (SDL_GamepadButton)source);
}

static LONG s_read_axis(int axis_idx) {
	const int sdl_axis = g_dttr_config.m_gamepad_axes[axis_idx];

	if (!g_dttr_gamepad || sdl_axis == DTTR_GAMEPAD_MAPPING_NONE) {
		return 0;
	}

	const LONG value = SDL_GetGamepadAxis(g_dttr_gamepad, sdl_axis)
					   / DTTR_DINPUT_AXIS_SCALE;
	const LONG deadzone = g_dttr_config.m_gamepad_axis_deadzone[axis_idx];

	return (value > -deadzone && value < deadzone) ? 0 : value;
}

void *__cdecl dttr_inputs_hook_dinput_poll_callback(void *device) {
	S_DIJoyState *state = (S_DIJoyState *)pcdogs_malloc(sizeof(*state));

	if (!state) {
		log_error("Failed to allocate joystick poll state");
		return NULL;
	}

	s_init_poll_state(state);

	if (!g_dttr_gamepad || !g_dttr_config.m_gamepad_enabled) {
		return state;
	}

	state->m_x = s_read_axis(DTTR_GAMEPAD_AXIS_IDX_STICK_X);
	state->m_y = s_read_axis(DTTR_GAMEPAD_AXIS_IDX_STICK_Y);
	state->m_rz = s_read_axis(DTTR_GAMEPAD_AXIS_IDX_CAMERA_RZ);

	bool dir_up = false, dir_down = false;
	bool dir_left = false, dir_right = false;

	for (int src = 0; src < DTTR_GAMEPAD_SOURCE_COUNT; src++) {
		const int action = g_dttr_config.m_gamepad_button_map[src];

		if (action == DTTR_GAMEPAD_MAPPING_NONE || !s_is_source_pressed(src)) {
			continue;
		}

		if (action >= PCDOGS_GAMEPAD_IDX_BTN_0 && action <= PCDOGS_GAMEPAD_IDX_BTN_12) {
			state->m_buttons[action - PCDOGS_GAMEPAD_IDX_BTN_0] = DINPUT_BUTTON_PRESSED;
		} else if (action == PCDOGS_GAMEPAD_IDX_UP) {
			dir_up = true;
		} else if (action == PCDOGS_GAMEPAD_IDX_DOWN) {
			dir_down = true;
		} else if (action == PCDOGS_GAMEPAD_IDX_LEFT) {
			dir_left = true;
		} else if (action == PCDOGS_GAMEPAD_IDX_RIGHT) {
			dir_right = true;
		}
	}

	s_apply_direction_state(state, dir_up, dir_down, dir_left, dir_right);

	return state;
}
