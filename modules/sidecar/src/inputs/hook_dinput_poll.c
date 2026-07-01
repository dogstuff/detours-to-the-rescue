#include <SDL3/SDL.h>
#include <dttr_pcdogs.h>
#include <stdint.h>
#include <windows.h>

#include <dttr_config.h>
#include <dttr_gamepad_mapping.h>
#include <dttr_log.h>

#include "context_private.h"
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

#define DINPUT_POV_CENTERED 0xFFFFFFFF

static void init_poll_state(di_joy_state *state) {
	SDL_zerop(state);

	for (int i = 0; i < 4; i++) {
		state->pov[i] = DINPUT_POV_CENTERED;
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

	return state;
}
