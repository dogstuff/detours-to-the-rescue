#include <SDL3/SDL.h>
#include <dttr_sidecar.h>

#include "dttr_hooks_inputs.h"
#include "dttr_interop_pcdogs.h"
#include <dttr_log.h>

SDL_Gamepad *g_dttr_gamepad;

static void s_try_open_configured_gamepad(void) {
	int count = 0;
	SDL_JoystickID *const joysticks = SDL_GetGamepads(&count);
	const int index = g_dttr_config.m_gamepad_index;

	if (joysticks && index < count) {
		g_dttr_gamepad = SDL_OpenGamepad(joysticks[index]);
		if (g_dttr_gamepad) {
			DTTR_LOG_INFO("Gamepad connected: %s", SDL_GetGamepadName(g_dttr_gamepad));
		} else {
			DTTR_LOG_ERROR("Failed to open gamepad: %s", SDL_GetError());
		}
	} else if (count > 0) {
		DTTR_LOG_WARN("Gamepad index %d out of range (%d connected)", index, count);
	}

	SDL_free(joysticks);
}

static void s_close_gamepad(void) {
	if (!g_dttr_gamepad) {
		return;
	}

	SDL_CloseGamepad(g_dttr_gamepad);
	g_dttr_gamepad = NULL;
}

void dttr_inputs_init(void) {
	if (!SDL_InitSubSystem(SDL_INIT_GAMEPAD)) {
		DTTR_LOG_ERROR("SDL_InitSubSystem(GAMEPAD) failed: %s", SDL_GetError());
	}

	s_try_open_configured_gamepad();
}

void dttr_inputs_hooks_init(const DTTR_ComponentContext *ctx) {
	DTTR_INSTALL_JMP(
		dttr_inputs_hook_dinput_poll,
		ctx,
		"\x56\x8B\x74\x24?\x56\x8B\x06",
		"xxxx?xxx"
	);

	DTTR_INSTALL_POINTER(
		dttr_inputs_hook_get_async_key_state,
		ctx,
		"\x8B\x1D????\x56\x33\xF6",
		"xx????xxx",
		*(uint32_t *)(match_ + 2)
	);
}

void dttr_inputs_handle_device_event(const SDL_Event *event) {
	switch (event->type) {
	case SDL_EVENT_GAMEPAD_ADDED:
		if (g_dttr_gamepad) {
			return;
		}

		s_try_open_configured_gamepad();
		if (g_dttr_gamepad && g_pcdogs_game_initialized_get() == 1) {
			g_pcdogs_joystick_available_set(1);
		}
		return;
	case SDL_EVENT_GAMEPAD_REMOVED:
		if (!g_dttr_gamepad || SDL_GetGamepadID(g_dttr_gamepad) != event->gdevice.which) {
			return;
		}

		DTTR_LOG_INFO("Gamepad disconnected: %s", SDL_GetGamepadName(g_dttr_gamepad));
		s_close_gamepad();
		g_pcdogs_joystick_available_set(0);
		return;
	default:
		return;
	}
}

void dttr_inputs_late_init(void) {
	if (!g_dttr_gamepad) {
		return;
	}

	g_pcdogs_joystick_available_set(1);
	DTTR_LOG_DEBUG("Joystick is available");
}

void dttr_inputs_hooks_cleanup(const DTTR_ComponentContext *ctx) {
	DTTR_UNINSTALL(dttr_inputs_hook_dinput_poll, ctx);
	DTTR_UNINSTALL(dttr_inputs_hook_get_async_key_state, ctx);
}

void dttr_inputs_cleanup(void) { s_close_gamepad(); }
