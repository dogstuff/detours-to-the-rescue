#include <SDL3/SDL.h>
#include <dttr_pcdogs.h>
#include <windows.h>

#include "hooks_private.h"

DTTR_PCDOGS_F_Input_IsKeyPressedAsync_proto dttr_inputs_hook_is_key_pressed_async_original;
DTTR_PCDOGS_F_Input_GetButtonString_proto dttr_inputs_hook_get_button_string_original;

bool dttr_inputs_hook_key_state_prepare(const DTTR_Mods_Context *ctx) {
	return ctx && DTTR_PCDOGS_F_Input_IsKeyPressedAsync->IsCallable(&ctx->runtime)
		   && DTTR_PCDOGS_F_Input_GetButtonString->IsCallable(&ctx->runtime);
}

void dttr_inputs_hook_key_state_reset() {
	dttr_inputs_hook_is_key_pressed_async_original = NULL;
	dttr_inputs_hook_get_button_string_original = NULL;
}

static bool current_key_state_pressed(int key_code) {
	int keyboard_state_count = 0;
	const bool *keyboard_state = SDL_GetKeyboardState(&keyboard_state_count);

	return dttr_inputs_key_code_pressed(key_code, keyboard_state, keyboard_state_count);
}

BOOL __cdecl dttr_inputs_hook_is_key_pressed_async_callback(int32_t virtual_key) {
	if (virtual_key == VK_RETURN || virtual_key == DTTR_INPUTS_KEY_KEYPAD_ENTER) {
		return current_key_state_pressed(virtual_key) ? TRUE : FALSE;
	}

	return dttr_inputs_hook_is_key_pressed_async_original
			   ? dttr_inputs_hook_is_key_pressed_async_original(virtual_key)
			   : FALSE;
}

char *__cdecl dttr_inputs_hook_get_button_string_callback(int32_t button_code) {
	const char *name = dttr_inputs_key_code_name(button_code);
	if (name) {
		return (char *)name;
	}

	return dttr_inputs_hook_get_button_string_original
			   ? dttr_inputs_hook_get_button_string_original(button_code)
			   : NULL;
}
