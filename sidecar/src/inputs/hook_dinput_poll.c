#include <SDL3/SDL.h>
#include <dttr_sidecar.h>
#include <stdint.h>
#include <string.h>
#include <windows.h>

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
	DTTR_DINPUT_BUTTON_COUNT = 13,
};

/// This is the DirectInput sentinel for a centered/neutral POV hat switch
#define DINPUT_POV_CENTERED 0xFFFFFFFF
/// DirectInput uses this byte value to indicate a button is pressed
#define DINPUT_BUTTON_PRESSED 0x80

/// Triggers are encoded as axis indices offset by a base value and compared against a threshold
static bool s_is_mapping_pressed(int mapping) {
	if (!g_dttr_gamepad) {
		return false;
	}

	if (mapping >= DTTR_GAMEPAD_MAPPING_TRIGGER_BASE) {
		const int32_t value
			= SDL_GetGamepadAxis(g_dttr_gamepad, mapping - DTTR_GAMEPAD_MAPPING_TRIGGER_BASE)
			  / DTTR_DINPUT_AXIS_SCALE;
		return value > DTTR_GAMEPAD_TRIGGER_THRESHOLD;
	}

	return SDL_GetGamepadButton(g_dttr_gamepad, (SDL_GamepadButton)mapping);
}

static LONG s_read_axis(int axis_idx) {
	const int sdl_axis = g_dttr_config.m_gamepad_axes[axis_idx];

	if (!g_dttr_gamepad || sdl_axis == DTTR_GAMEPAD_MAPPING_NONE) {
		return 0;
	}

	const LONG value = SDL_GetGamepadAxis(g_dttr_gamepad, sdl_axis) / DTTR_DINPUT_AXIS_SCALE;
	const LONG deadzone = g_dttr_config.m_gamepad_axis_deadzone[axis_idx];

	return (value > -deadzone && value < deadzone) ? 0 : value;
}

/// Slams the analog axis to full deflection when a digital direction button is held
static void s_apply_direction_overrides(S_DIJoyState *state) {
	static const struct {
		int m_mapping_idx;
		int m_sign;
	} overrides[] = {
		{PCDOGS_GAMEPAD_IDX_UP, -1},
		{PCDOGS_GAMEPAD_IDX_DOWN, +1},
		{PCDOGS_GAMEPAD_IDX_LEFT, -1},
		{PCDOGS_GAMEPAD_IDX_RIGHT, +1},
	};

	for (int i = 0; i < (int)SDL_arraysize(overrides); i++) {
		const int mapping = g_dttr_config.m_gamepad_mappings[overrides[i].m_mapping_idx];

		if (mapping == DTTR_GAMEPAD_MAPPING_NONE || !s_is_mapping_pressed(mapping)) {
			continue;
		}

		LONG *axis = (i < 2) ? &state->m_y : &state->m_x;
		*axis = overrides[i].m_sign * DTTR_DINPUT_AXIS_FULL_DEFLECTION;
	}
}

void *__cdecl dttr_inputs_hook_dinput_poll_callback(void *device) {
	(void)device;

	S_DIJoyState *state = (S_DIJoyState *)pcdogs_malloc(sizeof(S_DIJoyState));

	if (!state) {
		return NULL;
	}

	memset(state, 0, sizeof(S_DIJoyState));

	for (int i = 0; i < 4; i++) {
		state->m_pov[i] = DINPUT_POV_CENTERED;
	}

	if (!g_dttr_gamepad || !g_dttr_config.m_gamepad_enabled) {
		return state;
	}

	state->m_x = s_read_axis(DTTR_GAMEPAD_AXIS_IDX_STICK_X);
	state->m_y = s_read_axis(DTTR_GAMEPAD_AXIS_IDX_STICK_Y);
	state->m_rz = s_read_axis(DTTR_GAMEPAD_AXIS_IDX_CAMERA_RZ);

	s_apply_direction_overrides(state);

	for (int i = 0; i < DTTR_DINPUT_BUTTON_COUNT; i++) {
		const int mapping = g_dttr_config.m_gamepad_mappings[PCDOGS_GAMEPAD_IDX_BTN_0 + i];

		if (mapping != DTTR_GAMEPAD_MAPPING_NONE && s_is_mapping_pressed(mapping)) {
			state->m_buttons[i] = DINPUT_BUTTON_PRESSED;
		}
	}

	return state;
}
