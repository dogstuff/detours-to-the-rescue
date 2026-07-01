#include <SDL3/SDL.h>
#include <dttr_pcdogs.h>

#include "hooks_private.h"
#include "inputs_private.h"
#include "sidecar_hook_sigs.h"
#include "sidecar_private.h"

#include <dttr_config.h>
#include <dttr_log.h>
#include <kvec.h>

typedef kvec_t(DTTR_PCDOGS_T_Patch_Spec) DTTR_InputPatchVector;

SDL_Gamepad *dttr_inputs_gamepad;
static SDL_Joystick *dttr_inputs_joystick;

static DTTR_Core_PatchGroup *inputs_targets;

SDL_Joystick *dttr_inputs_raw_joystick() {
	if (dttr_inputs_gamepad) {
		return SDL_GetGamepadJoystick(dttr_inputs_gamepad);
	}

	return dttr_inputs_joystick;
}

static bool controller_open() {
	return dttr_inputs_gamepad || dttr_inputs_joystick;
}

int32_t dttr_inputs_read_raw_axis(int axis_idx) {
	const int sdl_axis = dttr_config.gamepad_axes[axis_idx];

	if (!dttr_inputs_gamepad || sdl_axis == DTTR_GAMEPAD_MAPPING_NONE) {
		return 0;
	}

	return SDL_GetGamepadAxis(dttr_inputs_gamepad, (SDL_GamepadAxis)sdl_axis)
		   / DTTR_DINPUT_AXIS_SCALE;
}

static bool set_joystick_available(int32_t available) {
	return REQUIRE_PCDOGS_CALL(
		DTTR_PCDOGS_D_Input_GetPressedButton_JoystickAvailable->Write(available)
	);
}

static bool try_open_configured_gamepad() {
	int count = 0;
	SDL_JoystickID *joysticks = SDL_GetGamepads(&count);
	const int index = dttr_config.gamepad_index;

	if (!joysticks || index < 0 || index >= count) {
		if (count > 0) {
			DTTR_LOG_WARN("Gamepad index %d out of range (%d connected)", index, count);
		}

		SDL_free(joysticks);
		return false;
	}

	dttr_inputs_gamepad = SDL_OpenGamepad(joysticks[index]);

	if (dttr_inputs_gamepad) {
		DTTR_LOG_INFO("Gamepad connected: %s", SDL_GetGamepadName(dttr_inputs_gamepad));
	} else {
		DTTR_LOG_ERROR("Failed to open gamepad: %s", SDL_GetError());
	}

	SDL_free(joysticks);
	return dttr_inputs_gamepad != NULL;
}

static bool try_open_configured_joystick() {
	int count = 0;
	SDL_JoystickID *joysticks = SDL_GetJoysticks(&count);
	const int index = dttr_config.gamepad_index;

	if (!joysticks || index < 0 || index >= count) {
		if (count > 0) {
			DTTR_LOG_WARN("Joystick index %d out of range (%d connected)", index, count);
		}

		SDL_free(joysticks);
		return false;
	}

	dttr_inputs_joystick = SDL_OpenJoystick(joysticks[index]);

	if (dttr_inputs_joystick) {
		DTTR_LOG_INFO("Joystick connected: %s", SDL_GetJoystickName(dttr_inputs_joystick));
	} else {
		DTTR_LOG_ERROR("Failed to open joystick: %s", SDL_GetError());
	}

	SDL_free(joysticks);
	return dttr_inputs_joystick != NULL;
}

static bool try_open_configured_controller() {
	return try_open_configured_gamepad() || try_open_configured_joystick();
}

static void close_gamepad() {
	if (!dttr_inputs_gamepad) {
		return;
	}

	SDL_RumbleGamepad(dttr_inputs_gamepad, 0, 0, 0);
	SDL_CloseGamepad(dttr_inputs_gamepad);
	dttr_inputs_gamepad = NULL;
}

static void close_joystick() {
	if (!dttr_inputs_joystick) {
		return;
	}

	SDL_CloseJoystick(dttr_inputs_joystick);
	dttr_inputs_joystick = NULL;
}

static void close_controller() {
	set_joystick_available(0);
	close_gamepad();
	close_joystick();
}

void dttr_inputs_init() {
	if (!SDL_InitSubSystem(SDL_INIT_GAMEPAD | SDL_INIT_JOYSTICK)) {
		DTTR_LOG_ERROR("SDL_InitSubSystem(GAMEPAD|JOYSTICK) failed: %s", SDL_GetError());
	}

	SDL_SetGamepadEventsEnabled(true);
	SDL_SetJoystickEventsEnabled(true);

	if (dttr_config.gamepad_enabled) {
		try_open_configured_controller();
	}
}

bool dttr_inputs_hooks_init(const DTTR_Mods_Context *ctx) {
	DTTR_Result alloc_status = DTTR_PCDOGS_F_Mem_MallocCRT->Status(&ctx->runtime);
	if (!DTTR_ResultOK(alloc_status)) {
		DTTR_LOG_ERROR(
			"Required DInput allocator unavailable: %s",
			dttr_sidecar_result_detail(alloc_status)
		);
		return false;
	}

	DTTR_InputPatchVector inputs_patches;
	kv_init(inputs_patches);

	kv_push(
		DTTR_PCDOGS_T_Patch_Spec,
		inputs_patches,
		(DTTR_PCDOGS_T_Patch_Spec)DTTR_PCDOGS_PATCH_SPEC_AOB_REL32_JMP(
			true,
			DTTR_SIDECAR_AOB_DINPUT_POLL,
			0,
			dttr_inputs_hook_dinput_poll_callback
		)
	);

	kv_push(
		DTTR_PCDOGS_T_Patch_Spec,
		inputs_patches,
		DTTR_PCDOGS_D_Video_PlayMovieLoop_GetAsyncKeyStateThunk
			->PatchSpec(true, dttr_inputs_hook_get_async_key_state_callback, NULL)
	);

	if (!dttr_inputs_hook_key_state_prepare(ctx)) {
		DTTR_LOG_ERROR("Input key-state hook unavailable");
		kv_destroy(inputs_patches);
		return false;
	}

	kv_push(
		DTTR_PCDOGS_T_Patch_Spec,
		inputs_patches,
		DTTR_PCDOGS_F_Input_IsKeyPressedAsync->PatchSpec(
			true,
			dttr_inputs_hook_is_key_pressed_async_callback,
			&dttr_inputs_hook_is_key_pressed_async_original
		)
	);

	if (!dttr_inputs_hook_mapping_prepare(ctx)) {
		DTTR_LOG_ERROR("Input mapping hook unavailable");
		kv_destroy(inputs_patches);
		return false;
	}

	kv_push(
		DTTR_PCDOGS_T_Patch_Spec,
		inputs_patches,
		DTTR_PCDOGS_F_Input_RegisterButtonMapping->PatchSpec(
			true,
			dttr_inputs_hook_register_button_mapping_callback,
			&dttr_inputs_hook_register_button_mapping_original
		)
	);

	kv_push(
		DTTR_PCDOGS_T_Patch_Spec,
		inputs_patches,
		DTTR_PCDOGS_F_Config_ApplySettings->PatchSpec(
			true,
			dttr_inputs_hook_config_apply_settings_callback,
			&dttr_inputs_hook_config_apply_settings_original
		)
	);

	kv_push(
		DTTR_PCDOGS_T_Patch_Spec,
		inputs_patches,
		DTTR_PCDOGS_F_Input_GetButtonString->PatchSpec(
			false,
			dttr_inputs_hook_get_button_string_callback,
			&dttr_inputs_hook_get_button_string_original
		)
	);

	if (dttr_inputs_hook_format_button_name_prepare(ctx)) {
		kv_push(
			DTTR_PCDOGS_T_Patch_Spec,
			inputs_patches,
			DTTR_PCDOGS_F_Input_FormatButtonName->PatchSpec(
				true,
				dttr_inputs_hook_format_button_name_callback,
				&dttr_inputs_hook_format_button_name_original
			)
		);
	} else {
		DTTR_LOG_INFO(
			"Button display name override hook unavailable; using original control names"
		);
	}

	if (!dttr_inputs_hook_get_pressed_button_prepare(ctx)) {
		DTTR_LOG_ERROR("Controls menu return key remap hook unavailable");
		kv_destroy(inputs_patches);
		return false;
	}

	kv_push(
		DTTR_PCDOGS_T_Patch_Spec,
		inputs_patches,
		DTTR_PCDOGS_F_Input_GetPressedButton->PatchSpec(
			true,
			dttr_inputs_hook_get_pressed_button_callback,
			&dttr_inputs_hook_get_pressed_button_original
		)
	);

	for (size_t i = 0; i < dttr_sidecar_input_byte_patch_spec_count; i++) {
		kv_push(
			DTTR_PCDOGS_T_Patch_Spec,
			inputs_patches,
			dttr_sidecar_input_byte_patch_specs[i]
		);
	}

	if (dttr_inputs_hook_rumble_prepare(ctx)) {
		kv_push(
			DTTR_PCDOGS_T_Patch_Spec,
			inputs_patches,
			DTTR_PCDOGS_F_Input_TriggerRumbleIfAllowed->PatchSpec(
				false,
				dttr_inputs_hook_rumble_callback,
				&dttr_inputs_hook_rumble_original
			)
		);
	} else {
		DTTR_LOG_INFO("SDL rumble hook unavailable; deferring to original vibration");
	}

	if (dttr_inputs_hook_set_rumble_suppress_flag_prepare(ctx)) {
		kv_push(
			DTTR_PCDOGS_T_Patch_Spec,
			inputs_patches,
			DTTR_PCDOGS_F_Settings_SetRumbleSuppressFlag->PatchSpec(
				false,
				dttr_inputs_hook_set_rumble_suppress_flag_callback,
				&dttr_inputs_hook_set_rumble_suppress_flag_original
			)
		);
	}

	if (dttr_inputs_hook_read_gamepad_prepare(ctx)) {
		kv_push(
			DTTR_PCDOGS_T_Patch_Spec,
			inputs_patches,
			DTTR_PCDOGS_F_Input_ReadGamepad->PatchSpec(
				true,
				dttr_inputs_hook_read_gamepad_callback,
				&dttr_inputs_hook_read_gamepad_original
			)
		);
	} else if (dttr_config.gamepad_analog_remap) {
		DTTR_LOG_WARN(
			"Analog remap is unavailable for this PCDOGS build; using current "
			"gamepad mapping"
		);
	}

	const bool installed = dttr_sidecar_install_pcdogs_patch_group(
		ctx,
		"sidecar/inputs",
		inputs_patches.a,
		inputs_patches.n,
		&inputs_targets
	);
	kv_destroy(inputs_patches);
	return installed;
}

// Tracks SDL gamepad hotplug events and updates the game joystick-available flag.
void dttr_inputs_handle_device_event(const SDL_Event *event) {
	switch (event->type) {
	case SDL_EVENT_GAMEPAD_ADDED:
	case SDL_EVENT_JOYSTICK_ADDED:
		if (controller_open()) {
			return;
		}

		if (!dttr_config.gamepad_enabled) {
			return;
		}

		if (!try_open_configured_controller()) {
			return;
		}

		int32_t game_initialized = 0;
		if (!REQUIRE_PCDOGS_CALL(
				DTTR_PCDOGS_D_Window_ProcessGameProc_Initialized->Read(&game_initialized)
			)) {
			close_controller();
			return;
		}

		if (game_initialized == 1 && !set_joystick_available(1)) {
			close_controller();
		}

		return;
	case SDL_EVENT_GAMEPAD_REMOVED:
		if (!dttr_inputs_gamepad
			|| SDL_GetGamepadID(dttr_inputs_gamepad) != event->gdevice.which) {
			return;
		}

		DTTR_LOG_INFO("Gamepad disconnected: %s", SDL_GetGamepadName(dttr_inputs_gamepad));
		close_controller();
		return;
	case SDL_EVENT_JOYSTICK_REMOVED:
		if (!dttr_inputs_joystick
			|| SDL_GetJoystickID(dttr_inputs_joystick) != event->jdevice.which) {
			return;
		}

		DTTR_LOG_INFO(
			"Joystick disconnected: %s",
			SDL_GetJoystickName(dttr_inputs_joystick)
		);
		close_controller();
		return;
	default:
		return;
	}
}

// Publishes joystick availability after the game has initialized its input globals.
bool dttr_inputs_late_init() {
	if (!dttr_config.gamepad_enabled || !controller_open()) {
		return set_joystick_available(0);
	}

	if (!set_joystick_available(1)) {
		return false;
	}

	DTTR_LOG_DEBUG("Joystick is available");
	return true;
}

void dttr_inputs_hooks_cleanup(const DTTR_Mods_Context *ctx) {
	DTTR_Core_PatchGroupRelease(&inputs_targets);
	dttr_inputs_hook_mapping_reset();
	dttr_inputs_hook_get_pressed_button_reset();
	dttr_inputs_hook_key_state_reset();
	dttr_inputs_hook_read_gamepad_reset();
	dttr_inputs_hook_rumble_reset();
}

void dttr_inputs_cleanup() {
	close_controller();
}
