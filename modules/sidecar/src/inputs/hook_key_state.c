#include <SDL3/SDL.h>
#include <dttr_pcdogs.h>
#include <windows.h>

#include "context_private.h"
#include "hooks_private.h"
#include "sidecar_private.h"

enum {
	DTTR_INPUTS_BUTTON_NAME_SLOT_COUNT = 13,
	DTTR_INPUTS_BUTTON_NAME_BUFFER_SIZE = 0x14,
};

DTTR_PCDOGS_F_Input_IsKeyPressedAsync_proto dttr_inputs_hook_is_key_pressed_async_original;
DTTR_PCDOGS_F_Input_GetButtonString_proto dttr_inputs_hook_get_button_string_original;
DTTR_PCDOGS_F_Input_FormatButtonName_proto dttr_inputs_hook_format_button_name_original;

bool dttr_inputs_hook_key_state_prepare(const DTTR_Mods_Context *ctx) {
	return ctx && DTTR_PCDOGS_F_Input_IsKeyPressedAsync->IsCallable(&ctx->runtime)
		   && DTTR_PCDOGS_F_Input_GetButtonString->IsCallable(&ctx->runtime);
}

bool dttr_inputs_hook_format_button_name_prepare(const DTTR_Mods_Context *ctx) {
	return ctx && DTTR_PCDOGS_F_Input_FormatButtonName->IsCallable(&ctx->runtime)
		   && DTTR_PCDOGS_F_Input_GetButtonIndex->IsCallable(&ctx->runtime)
		   && DTTR_PCDOGS_D_G_InputButtonNameBuffers->IsResolved();
}

void dttr_inputs_hook_key_state_reset() {
	dttr_inputs_hook_is_key_pressed_async_original = NULL;
	dttr_inputs_hook_get_button_string_original = NULL;
	dttr_inputs_hook_format_button_name_original = NULL;
}

BOOL __cdecl dttr_inputs_hook_is_key_pressed_async_callback(int32_t virtual_key) {
	int keyboard_state_count = 0;
	const bool *keyboard_state = SDL_GetKeyboardState(&keyboard_state_count);

	if (dttr_inputs_key_state_uses_live_state(virtual_key)) {
		return dttr_inputs_global_vkey_pressed(
				   virtual_key,
				   keyboard_state,
				   keyboard_state_count
			   )
				   ? TRUE
				   : FALSE;
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

static char *format_custom_button_name(const char *name, uint32_t button_mask) {
	int32_t button_index = 0;
	if (!REQUIRE_PCDOGS_CALL(DTTR_PCDOGS_F_Input_GetButtonIndex->Call(
			dttr_sidecar_runtime_context(),
			button_mask,
			&button_index
		))) {
		return (char *)name;
	}

	if (button_index < 0 || button_index >= DTTR_INPUTS_BUTTON_NAME_SLOT_COUNT) {
		return (char *)name;
	}

	char *(
		*button_name_slots
	)[DTTR_INPUTS_BUTTON_NAME_SLOT_COUNT] = DTTR_PCDOGS_D_G_InputButtonNameBuffers->Ptr();
	if (!button_name_slots) {
		return (char *)name;
	}
	char **button_names = *button_name_slots;

	if (!button_names[button_index]) {
		void *buffer = NULL;
		if (!REQUIRE_PCDOGS_CALL(DTTR_PCDOGS_F_Mem_MallocCRT->Call(
				dttr_sidecar_runtime_context(),
				DTTR_INPUTS_BUTTON_NAME_BUFFER_SIZE,
				&buffer
			))
			|| !buffer) {
			return (char *)name;
		}
		button_names[button_index] = (char *)buffer;
	}

	SDL_strlcpy(button_names[button_index], name, DTTR_INPUTS_BUTTON_NAME_BUFFER_SIZE);
	return button_names[button_index];
}

char *__cdecl dttr_inputs_hook_format_button_name_callback(
	int32_t control_code,
	uint32_t button_mask
) {
	const char *name = dttr_inputs_key_code_name(control_code);
	if (name) {
		return format_custom_button_name(name, button_mask);
	}

	return dttr_inputs_hook_format_button_name_original
			   ? dttr_inputs_hook_format_button_name_original(control_code, button_mask)
			   : NULL;
}
