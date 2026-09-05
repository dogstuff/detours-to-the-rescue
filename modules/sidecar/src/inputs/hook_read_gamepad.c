#include <SDL3/SDL.h>
#include <dttr_pcdogs.h>
#include <stdint.h>
#include <windows.h>

#include <dttr_config.h>
#include <dttr_gamepad_mapping.h>
#include <dttr_log.h>

#include "hooks_private.h"
#include "inputs_private.h"
#include "sidecar_private.h"

static const DTTR_Mods_Context *read_gamepad_ctx;
static uint32_t *rz_negative_mask;
static uint32_t *rz_positive_mask;
DTTR_PCDOGS_F_Input_ReadGamepad_proto dttr_inputs_hook_read_gamepad_original;

enum {
	DTTR_DINPUT_RZ_THRESHOLD = 600,
};

static bool required_read_gamepad_symbols_available(const DTTR_Mods_Context *ctx) {
	return DTTR_PCDOGS_F_Input_ReadGamepad->IsCallable(&ctx->runtime)
		   && DTTR_PCDOGS_F_Mem_FreeMemoryExCRT->IsCallable(&ctx->runtime)
		   && DTTR_PCDOGS_D_Input_ReadGamepad_JoystickState->IsResolved()
		   && DTTR_PCDOGS_D_Input_GetPressedButton_JoystickAvailable->IsResolved();
}

static bool cache_mask_cells() {
	rz_negative_mask = DTTR_PCDOGS_D_Input_ReadGamepad_AxisRzNegativeMask->Ptr();
	rz_positive_mask = DTTR_PCDOGS_D_Input_ReadGamepad_AxisRzPositiveMask->Ptr();
	return rz_negative_mask && rz_positive_mask;
}

bool dttr_inputs_hook_read_gamepad_prepare(const DTTR_Mods_Context *ctx) {
	if (!ctx || !required_read_gamepad_symbols_available(ctx)) {
		return false;
	}

	if (!cache_mask_cells()) {
		return false;
	}

	read_gamepad_ctx = ctx;
	return true;
}

void dttr_inputs_hook_read_gamepad_reset() {
	read_gamepad_ctx = NULL;
	rz_negative_mask = NULL;
	rz_positive_mask = NULL;
	dttr_inputs_hook_dinput_neutralize_left_stick(false);
	dttr_inputs_hook_read_gamepad_original = NULL;
}

static int32_t ps1_deadzone_from_config(int axis_idx) {
	return dttr_inputs_ps1_deadzone_from_dinput(
		dttr_config.gamepad_axis_deadzone[axis_idx]
	);
}

static void apply_rz_buttons(
	DTTR_PCDOGS_T_Input_State *state,
	const DTTR_PCDOGS_T_Input_JoystickState *joystick
) {
	const int32_t rz = joystick->rot.z;
	if (rz < -DTTR_DINPUT_RZ_THRESHOLD) {
		state->button_bits |= *rz_negative_mask;
	} else if (rz > DTTR_DINPUT_RZ_THRESHOLD) {
		state->button_bits |= *rz_positive_mask;
	}
}

static void release_joystick_state(DTTR_PCDOGS_T_Input_JoystickState *state) {
	DTTR_PCDOGS_T_Input_JoystickState *
		*slot = DTTR_PCDOGS_D_Input_ReadGamepad_JoystickState->Ptr();
	REQUIRE_PCDOGS_CALL(
		DTTR_PCDOGS_F_Mem_FreeMemoryExCRT->Call(&read_gamepad_ctx->runtime, state)
	);
	*slot = NULL;
}

void __cdecl dttr_inputs_hook_read_gamepad_callback(DTTR_PCDOGS_T_Input_State *state) {
	if (!state || !read_gamepad_ctx) {
		return;
	}

	if (dttr_inputs_hook_read_gamepad_original) {
		const bool neutralize = dttr_config.gamepad_analog_remap;
		dttr_inputs_hook_dinput_neutralize_left_stick(neutralize);
		dttr_inputs_hook_read_gamepad_original(state);
		dttr_inputs_hook_dinput_neutralize_left_stick(false);
	}

	uint8_t available = 0;
	if (!REQUIRE_PCDOGS_CALL(
			DTTR_PCDOGS_D_Input_GetPressedButton_JoystickAvailable->Read(&available)
		)
		|| !available) {
		dttr_inputs_apply_custom_button_mappings(state);
		return;
	}

	if (!dttr_config.gamepad_analog_remap) {
		dttr_inputs_apply_custom_button_mappings(state);
		return;
	}

	DTTR_PCDOGS_T_Input_JoystickState *polled = dttr_inputs_hook_dinput_poll_callback(
		NULL
	);
	if (!polled) {
		DTTR_LOG_WARN("Analog remap poll state allocation failed");
		dttr_inputs_apply_custom_button_mappings(state);
		return;
	}

	const DTTR_PCDOGS_T_Input_JoystickState *joystick = polled;
	DTTR_StickAxes axes = {0};
	dttr_inputs_apply_ps1_stick_axes(
		&axes,
		dttr_inputs_read_raw_axis(DTTR_GAMEPAD_AXIS_IDX_STICK_X),
		dttr_inputs_read_raw_axis(DTTR_GAMEPAD_AXIS_IDX_STICK_Y),
		ps1_deadzone_from_config(DTTR_GAMEPAD_AXIS_IDX_STICK_X),
		ps1_deadzone_from_config(DTTR_GAMEPAD_AXIS_IDX_STICK_Y),
		dttr_config.gamepad_axis_sensitivity[DTTR_GAMEPAD_AXIS_IDX_STICK_X],
		dttr_config.gamepad_axis_sensitivity[DTTR_GAMEPAD_AXIS_IDX_STICK_Y]
	);
	state->axis.x = axes.axis_x;
	state->axis.y = axes.axis_y;

	apply_rz_buttons(state, joystick);
	release_joystick_state(polled);
	dttr_inputs_apply_custom_button_mappings(state);
}
