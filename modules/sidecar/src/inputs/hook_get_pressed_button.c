#include <dttr_pcdogs.h>

#include "hooks_private.h"
#include "sidecar_private.h"

DTTR_PCDOGS_F_Input_GetPressedButton_proto dttr_inputs_hook_get_pressed_button_original;

bool dttr_inputs_hook_get_pressed_button_prepare(const DTTR_Mods_Context *ctx) {
	return ctx && DTTR_PCDOGS_F_Input_GetPressedButton->IsCallable(&ctx->runtime)
		   && DTTR_PCDOGS_D_Menu_HandleOptionsLogic_InputMenuButtonRemappingActive
				  ->IsResolved();
}

int32_t __cdecl dttr_inputs_hook_get_pressed_button_callback() {
	if (!dttr_inputs_hook_get_pressed_button_original) {
		return -1;
	}

	const int32_t pressed_button = dttr_inputs_hook_get_pressed_button_original();

	int32_t remapping_active = 0;
	if (!REQUIRE_PCDOGS_CALL(
			DTTR_PCDOGS_D_Menu_HandleOptionsLogic_InputMenuButtonRemappingActive->Read(
				&remapping_active
			)
		)) {
		return pressed_button;
	}

	return dttr_inputs_controls_menu_pressed_button(pressed_button, remapping_active);
}

void dttr_inputs_hook_get_pressed_button_reset() {
	dttr_inputs_hook_get_pressed_button_original = NULL;
	dttr_inputs_controls_menu_reset();
}
