#include <SDL3/SDL.h>
#include <dttr_pcdogs.h>

#include "hooks_private.h"
#include "inputs_private.h"
#include "sidecar_hook_sigs.h"
#include "sidecar_private.h"

#include <dttr_config.h>
#include <dttr_log.h>

SDL_Gamepad *dttr_inputs_gamepad;

static DTTR_Core_PatchGroup *inputs_targets;

bool dttr_inputs_source_pressed(int source) {
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

static void close_gamepad() {
	set_joystick_available(0);

	if (!dttr_inputs_gamepad) {
		return;
	}

	SDL_RumbleGamepad(dttr_inputs_gamepad, 0, 0, 0);
	SDL_CloseGamepad(dttr_inputs_gamepad);
	dttr_inputs_gamepad = NULL;
}

void dttr_inputs_init() {
	if (!SDL_InitSubSystem(SDL_INIT_GAMEPAD)) {
		DTTR_LOG_ERROR("SDL_InitSubSystem(GAMEPAD) failed: %s", SDL_GetError());
	}

	if (dttr_config.gamepad_enabled) {
		try_open_configured_gamepad();
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

	DTTR_PCDOGS_T_Patch_Spec inputs_patches[4];
	size_t patch_count = 0;

	inputs_patches[patch_count++] = (DTTR_PCDOGS_T_Patch_Spec)
		DTTR_PCDOGS_PATCH_SPEC_AOB_REL32_JMP(
			true,
			DTTR_SIDECAR_AOB_DINPUT_POLL,
			0,
			dttr_inputs_hook_dinput_poll_callback
		);
	inputs_patches[patch_count++] = DTTR_PCDOGS_D_Video_PlayMovieLoop_GetAsyncKeyStateThunk
										->PatchSpec(
											true,
											dttr_inputs_hook_get_async_key_state_callback,
											NULL
										);

	if (dttr_inputs_hook_rumble_prepare(ctx)) {
		inputs_patches[patch_count++] = DTTR_PCDOGS_F_Input_TriggerRumbleIfAllowed
											->PatchSpec(
												false,
												dttr_inputs_hook_rumble_callback,
												&dttr_inputs_hook_rumble_original
											);
	} else {
		DTTR_LOG_INFO("SDL rumble hook unavailable; deferring to original vibration");
	}

	if (dttr_config.gamepad_analog_remap) {
		if (dttr_inputs_hook_read_gamepad_prepare(ctx)) {
			inputs_patches[patch_count++] = DTTR_PCDOGS_F_Input_ReadGamepad->PatchSpec(
				true,
				dttr_inputs_hook_read_gamepad_callback,
				NULL
			);
		} else {
			DTTR_LOG_WARN(
				"Analog remap is unavailable for this PCDOGS build; using current "
				"gamepad mapping"
			);
		}
	}

	return dttr_sidecar_install_pcdogs_patch_group(
		ctx,
		"sidecar/inputs",
		inputs_patches,
		patch_count,
		&inputs_targets
	);
}

// Tracks SDL gamepad hotplug events and updates the game joystick-available flag.
void dttr_inputs_handle_device_event(const SDL_Event *event) {
	switch (event->type) {
	case SDL_EVENT_GAMEPAD_ADDED:
		if (dttr_inputs_gamepad) {
			return;
		}

		if (!dttr_config.gamepad_enabled) {
			return;
		}

		if (!try_open_configured_gamepad()) {
			return;
		}

		int32_t game_initialized = 0;
		if (!REQUIRE_PCDOGS_CALL(
				DTTR_PCDOGS_D_Window_ProcessGameProc_Initialized->Read(&game_initialized)
			)) {
			close_gamepad();
			return;
		}

		if (game_initialized == 1 && !set_joystick_available(1)) {
			close_gamepad();
		}

		return;
	case SDL_EVENT_GAMEPAD_REMOVED:
		if (!dttr_inputs_gamepad
			|| SDL_GetGamepadID(dttr_inputs_gamepad) != event->gdevice.which) {
			return;
		}

		DTTR_LOG_INFO("Gamepad disconnected: %s", SDL_GetGamepadName(dttr_inputs_gamepad));
		close_gamepad();
		return;
	default:
		return;
	}
}

// Publishes joystick availability after the game has initialized its input globals.
bool dttr_inputs_late_init() {
	if (!dttr_config.gamepad_enabled || !dttr_inputs_gamepad) {
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
	dttr_inputs_hook_read_gamepad_reset();
	dttr_inputs_hook_rumble_reset();
}

void dttr_inputs_cleanup() {
	close_gamepad();
}
