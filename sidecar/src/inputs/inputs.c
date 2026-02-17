#include <SDL3/SDL.h>
#include <dttr_sidecar.h>
#include <windows.h>

#include "dttr_hooks_inputs.h"
#include "dttr_interop_pcdogs.h"
#include "log.h"

SDL_Gamepad *g_dttr_gamepad;

static void s_try_open_configured_gamepad(void) {
	int count = 0;
	SDL_JoystickID *const joysticks = SDL_GetGamepads(&count);
	const int index = g_dttr_config.m_gamepad_index;

	if (joysticks && index < count) {
		g_dttr_gamepad = SDL_OpenGamepad(joysticks[index]);
		if (g_dttr_gamepad) {
			log_info("Gamepad connected: %s", SDL_GetGamepadName(g_dttr_gamepad));
		} else {
			log_error("Failed to open gamepad: %s", SDL_GetError());
		}
	} else if (count > 0) {
		log_warn("Gamepad index %d out of range (%d connected)", index, count);
	}

	SDL_free(joysticks);
}

void dttr_inputs_init(void) {
	if (!SDL_InitSubSystem(SDL_INIT_GAMEPAD)) {
		log_error("SDL_InitSubSystem(GAMEPAD) failed: %s", SDL_GetError());
	}

	s_try_open_configured_gamepad();
}

void dttr_inputs_hook_init(HMODULE module) {
	DTTR_INTEROP_HOOK_FUNC_LOG(dttr_inputs_hook_dinput_poll, module);
	DTTR_INTEROP_PATCH_PTR_LOG(dttr_inputs_hook_get_async_key_state, module);
}

void dttr_inputs_handle_device_event(const SDL_Event *event) {
	if (event->type == SDL_EVENT_GAMEPAD_ADDED && !g_dttr_gamepad) {
		s_try_open_configured_gamepad();
		if (g_dttr_gamepad && g_pcdogs_game_initialized_get() == 1) {
			g_pcdogs_joystick_available_set(1);
		}
		return;
	}

	if (event->type == SDL_EVENT_GAMEPAD_REMOVED && g_dttr_gamepad
		&& SDL_GetGamepadID(g_dttr_gamepad) == event->gdevice.which) {
		log_info("Gamepad disconnected: %s", SDL_GetGamepadName(g_dttr_gamepad));
		SDL_CloseGamepad(g_dttr_gamepad);
		g_dttr_gamepad = NULL;
		g_pcdogs_joystick_available_set(0);
	}
}

void dttr_inputs_late_init(void) {
	if (!g_dttr_gamepad) {
		return;
	}

	g_pcdogs_joystick_available_set(1);
	log_debug("Joystick is available");
}

void dttr_inputs_hook_cleanup(void) {
	DTTR_INTEROP_UNHOOK_LOG(dttr_inputs_hook_dinput_poll);
	DTTR_INTEROP_UNHOOK_LOG(dttr_inputs_hook_get_async_key_state);
}

void dttr_inputs_cleanup(void) {
	if (g_dttr_gamepad) {
		SDL_CloseGamepad(g_dttr_gamepad);
		g_dttr_gamepad = NULL;
	}
}
