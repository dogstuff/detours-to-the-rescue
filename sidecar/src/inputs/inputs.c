#include <SDL3/SDL.h>
#include <dttr_sidecar.h>
#include <windows.h>

#include "dttr_hooks.h"
#include "dttr_interop_pcdogs.h"
#include "log.h"

SDL_Gamepad *g_dttr_gamepad;

void dttr_inputs_init(HMODULE module) {
	SDL_InitSubSystem(SDL_INIT_GAMEPAD);

	int count = 0;
	SDL_JoystickID *const joysticks = SDL_GetGamepads(&count);

	if (joysticks && count > 0) {
		g_dttr_gamepad = SDL_OpenGamepad(joysticks[0]);
		log_info("Gamepad connected: %s", SDL_GetGamepadName(g_dttr_gamepad));
	}

	SDL_free(joysticks);

	DTTR_INTEROP_HOOK_FUNC_LOG(dttr_inputs_hook_dinput_poll, module);

	DTTR_INTEROP_PATCH_PTR_LOG(dttr_inputs_hook_get_async_key_state, module);
}

void dttr_inputs_late_init(void) {
	if (g_dttr_gamepad) {
		g_pcdogs_joystick_available_set(1);
		log_info("Joystick available flag set");
	}
}

void dttr_inputs_cleanup(void) {
	DTTR_INTEROP_UNHOOK_LOG(dttr_inputs_hook_dinput_poll);

	DTTR_INTEROP_UNHOOK_LOG(dttr_inputs_hook_get_async_key_state);

	if (g_dttr_gamepad) {
		SDL_CloseGamepad(g_dttr_gamepad);
		g_dttr_gamepad = NULL;
	}
}

void dttr_inputs_tick(void) {}
