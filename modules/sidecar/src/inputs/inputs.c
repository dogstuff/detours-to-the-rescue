#include <SDL3/SDL.h>
#include <dttr_pcdogs.h>
#include <dttr_sidecar.h>

#include "hooks_private.h"
#include "sidecar_private.h"

#include <dttr_log.h>

SDL_Gamepad *dttr_gamepad;

static const DTTR_PCDOGS_T_Patch_Spec inputs_patches[] = {
	DTTR_PCDOGS_PATCH_SPEC_AOB_REL32_JMP(
		true,
		"56 8B 74 24 ?? 56 8B 06",
		0,
		dttr_inputs_hook_dinput_poll_callback
	),
	{
		.kind = DTTR_PCDOGS_PATCH_DATA_POINTER_HOOK,
		.required = true,
		.global = DTTR_PCDOGS_DATA_VIDEO_PLAY_MOVIE_LOOP_INPUT_GET_ASYNC_KEY_STATE,
		.new_value = dttr_inputs_hook_get_async_key_state_callback,
		.out_original = NULL,
	},
};

static DTTR_Core_PatchGroup *inputs_targets;

static void try_open_configured_gamepad() {
	int count = 0;
	SDL_JoystickID *const joysticks = SDL_GetGamepads(&count);
	const int index = dttr_config.gamepad_index;

	if (!joysticks || index < 0 || index >= count) {
		if (count > 0) {
			DTTR_LOG_WARN("Gamepad index %d out of range (%d connected)", index, count);
		}

		SDL_free(joysticks);
		return;
	}

	dttr_gamepad = SDL_OpenGamepad(joysticks[index]);

	if (dttr_gamepad) {
		DTTR_LOG_INFO("Gamepad connected: %s", SDL_GetGamepadName(dttr_gamepad));
	} else {
		DTTR_LOG_ERROR("Failed to open gamepad: %s", SDL_GetError());
	}

	SDL_free(joysticks);
}

static void close_gamepad() {
	DTTR_PCDOGS_D_Input_GetPressedButton_JoystickAvailable->Write(0);
	if (!dttr_gamepad) {
		return;
	}

	SDL_CloseGamepad(dttr_gamepad);
	dttr_gamepad = NULL;
}

void DTTR_Inputs_Init() {
	if (!SDL_InitSubSystem(SDL_INIT_GAMEPAD)) {
		DTTR_LOG_ERROR("SDL_InitSubSystem(GAMEPAD) failed: %s", SDL_GetError());
	}

	if (dttr_config.gamepad_enabled) {
		try_open_configured_gamepad();
	}
}

bool dttr_inputs_hooks_init(const DTTR_Mods_Context *ctx) {
	return dttr_sidecar_install_pcdogs_patch_group(
		ctx,
		"sidecar/inputs",
		inputs_patches,
		DTTR_ARRAY_COUNT(inputs_patches),
		&inputs_targets
	);
}

// Tracks SDL gamepad hotplug events and updates the game joystick-available flag.
void DTTR_Inputs_HandleDeviceEvent(const SDL_Event *event) {
	switch (event->type) {
	case SDL_EVENT_GAMEPAD_ADDED:
		if (dttr_gamepad) {
			return;
		}

		if (!dttr_config.gamepad_enabled) {
			return;
		}

		try_open_configured_gamepad();
		int32_t game_initialized = 0;
		DTTR_PCDOGS_D_Window_ProcessGameProc_Initialized->Read(&game_initialized);

		if (dttr_gamepad && game_initialized == 1) {
			DTTR_PCDOGS_D_Input_GetPressedButton_JoystickAvailable->Write(1);
		}

		return;
	case SDL_EVENT_GAMEPAD_REMOVED:
		if (!dttr_gamepad || SDL_GetGamepadID(dttr_gamepad) != event->gdevice.which) {
			return;
		}

		DTTR_LOG_INFO("Gamepad disconnected: %s", SDL_GetGamepadName(dttr_gamepad));
		close_gamepad();
		DTTR_PCDOGS_D_Input_GetPressedButton_JoystickAvailable->Write(0);
		return;
	default:
		return;
	}
}

// Publishes joystick availability after the game has initialized its input globals.
void DTTR_Inputs_LateInit() {
	if (!dttr_config.gamepad_enabled || !dttr_gamepad) {
		DTTR_PCDOGS_D_Input_GetPressedButton_JoystickAvailable->Write(0);
		return;
	}

	DTTR_PCDOGS_D_Input_GetPressedButton_JoystickAvailable->Write(1);
	DTTR_LOG_DEBUG("Joystick is available");
}

void dttr_inputs_hooks_cleanup(const DTTR_Mods_Context *ctx) {
	DTTR_Core_PatchGroupRelease(&inputs_targets);
}

void DTTR_Inputs_Cleanup() { close_gamepad(); }
