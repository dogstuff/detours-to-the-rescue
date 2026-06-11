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
static uint32_t *dpad_up_mask;
static uint32_t *dpad_down_mask;
static uint32_t *dpad_left_mask;
static uint32_t *dpad_right_mask;
static uint32_t *rz_negative_mask;
static uint32_t *rz_positive_mask;
static uint32_t *button_masks[PCDOGS_GAMEPAD_IDX_BTN_12 - PCDOGS_GAMEPAD_IDX_BTN_0 + 1];

enum {
	DTTR_DINPUT_RZ_THRESHOLD = 600,
};

static bool required_read_gamepad_symbols_available(const DTTR_Mods_Context *ctx) {
	return DTTR_PCDOGS_F_Input_ReadGamepad->IsCallable(&ctx->runtime)
		   && DTTR_PCDOGS_F_Mem_FreeCRT->IsCallable(&ctx->runtime)
		   && DTTR_PCDOGS_D_Input_ReadGamepad_JoystickState->IsResolved()
		   && DTTR_PCDOGS_D_Input_GetPressedButton_JoystickAvailable->IsResolved();
}

static bool cache_mask_cells() {
	dpad_up_mask = DTTR_PCDOGS_D_Input_ReadGamepad_AxisYNegativeMask->Ptr();
	dpad_down_mask = DTTR_PCDOGS_D_Input_ReadGamepad_AxisYPositiveMask->Ptr();
	dpad_left_mask = DTTR_PCDOGS_D_Input_ReadGamepad_AxisXNegativeMask->Ptr();
	dpad_right_mask = DTTR_PCDOGS_D_Input_ReadGamepad_AxisXPositiveMask->Ptr();
	rz_negative_mask = DTTR_PCDOGS_D_Input_ReadGamepad_AxisRzNegativeMask->Ptr();
	rz_positive_mask = DTTR_PCDOGS_D_Input_ReadGamepad_AxisRzPositiveMask->Ptr();
	button_masks[0] = DTTR_PCDOGS_D_Input_ReadGamepad_Button0Mask->Ptr();
	button_masks[1] = DTTR_PCDOGS_D_Input_ReadGamepad_Button1Mask->Ptr();
	button_masks[2] = DTTR_PCDOGS_D_Input_ReadGamepad_Button2Mask->Ptr();
	button_masks[3] = DTTR_PCDOGS_D_Input_ReadGamepad_Button3Mask->Ptr();
	button_masks[4] = DTTR_PCDOGS_D_Input_ReadGamepad_Button4Mask->Ptr();
	button_masks[5] = DTTR_PCDOGS_D_Input_ReadGamepad_Button5Mask->Ptr();
	button_masks[6] = DTTR_PCDOGS_D_Input_ReadGamepad_Button6Mask->Ptr();
	button_masks[7] = DTTR_PCDOGS_D_Input_ReadGamepad_Button7Mask->Ptr();
	button_masks[8] = DTTR_PCDOGS_D_Input_ReadGamepad_Button8Mask->Ptr();
	button_masks[9] = DTTR_PCDOGS_D_Input_ReadGamepad_Button9Mask->Ptr();
	button_masks[10] = DTTR_PCDOGS_D_Input_ReadGamepad_Button10Mask->Ptr();
	button_masks[11] = DTTR_PCDOGS_D_Input_ReadGamepad_Button11Mask->Ptr();
	button_masks[12] = DTTR_PCDOGS_D_Input_ReadGamepad_Button12Mask->Ptr();

	if (!dpad_up_mask || !dpad_down_mask || !dpad_left_mask || !dpad_right_mask
		|| !rz_negative_mask || !rz_positive_mask) {
		return false;
	}

	for (size_t i = 0; i < DTTR_ARRAY_COUNT(button_masks); i++) {
		if (!button_masks[i]) {
			return false;
		}
	}

	return true;
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
	dpad_up_mask = NULL;
	dpad_down_mask = NULL;
	dpad_left_mask = NULL;
	dpad_right_mask = NULL;
	rz_negative_mask = NULL;
	rz_positive_mask = NULL;
	for (size_t i = 0; i < DTTR_ARRAY_COUNT(button_masks); i++) {
		button_masks[i] = NULL;
	}
}

static int32_t ps1_deadzone_from_config(int axis_idx) {
	return dttr_inputs_ps1_deadzone_from_dinput(
		dttr_config.gamepad_axis_deadzone[axis_idx]
	);
}

static void apply_mapped_dpad(uint32_t *button_bits) {
	const DTTR_DirectionMasks masks = {
		.up = *dpad_up_mask,
		.down = *dpad_down_mask,
		.left = *dpad_left_mask,
		.right = *dpad_right_mask,
	};

	for (int src = 0; src < DTTR_GAMEPAD_SOURCE_COUNT; src++) {
		const int action = dttr_config.gamepad_button_map[src];
		if (action == DTTR_GAMEPAD_MAPPING_NONE || !dttr_inputs_source_pressed(src)) {
			continue;
		}

		dttr_inputs_apply_dpad_action(button_bits, &masks, action);
	}
}

static void apply_rz_and_buttons(
	DTTR_PCDOGS_T_Input_State *state,
	const DTTR_PCDOGS_T_Input_JoystickState *joystick
) {
	const int32_t rz = joystick->rot.z;
	if (rz < -DTTR_DINPUT_RZ_THRESHOLD) {
		state->button_bits |= *rz_negative_mask;
	} else if (rz > DTTR_DINPUT_RZ_THRESHOLD) {
		state->button_bits |= *rz_positive_mask;
	}

	for (size_t i = 0; i < DTTR_ARRAY_COUNT(button_masks); i++) {
		if (joystick->rgb_buttons[i] & DINPUT_BUTTON_PRESSED) {
			state->button_bits |= *button_masks[i];
		}
	}
}

static void release_joystick_state(struct DIJOYSTATE *state) {
	struct DIJOYSTATE **slot = DTTR_PCDOGS_D_Input_ReadGamepad_JoystickState->Ptr();
	REQUIRE_PCDOGS_CALL(
		DTTR_PCDOGS_F_Mem_FreeCRT->Call(&read_gamepad_ctx->runtime, state)
	);
	*slot = NULL;
}

void __cdecl dttr_inputs_hook_read_gamepad_callback(DTTR_PCDOGS_T_Input_State *state) {
	if (!state || !read_gamepad_ctx) {
		return;
	}

	uint8_t available = 0;
	if (!REQUIRE_PCDOGS_CALL(
			DTTR_PCDOGS_D_Input_GetPressedButton_JoystickAvailable->Read(&available)
		)
		|| !available) {
		return;
	}

	struct DIJOYSTATE *polled = dttr_inputs_hook_dinput_poll_callback(NULL);
	if (!polled) {
		DTTR_LOG_WARN("Analog remap poll state allocation failed");
		return;
	}

	const DTTR_PCDOGS_T_Input_JoystickState *joystick = (const void *)polled;
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

	apply_mapped_dpad(&state->button_bits);
	apply_rz_and_buttons(state, joystick);
	release_joystick_state(polled);
}
