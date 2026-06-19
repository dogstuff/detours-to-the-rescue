#include <dttr_pcdogs.h>
#include <dttr_result.h>

#include "hooks_private.h"

DTTR_PCDOGS_F_Input_GetPressedButton_proto dttr_inputs_hook_get_pressed_button_original;

bool dttr_inputs_hook_get_pressed_button_prepare(const DTTR_Mods_Context *ctx) {
	return ctx && DTTR_PCDOGS_F_Input_GetPressedButton->IsCallable(&ctx->runtime)
		   && DTTR_PCDOGS_D_Menu_HandleOptionsLogic_InputMenuButtonRemappingActive
				  ->IsResolved()
		   && DTTR_PCDOGS_D_Menu_HandleOptionsLogic_State->IsResolved();
}

static bool read_controls_menu_state(int32_t *remapping_active, int32_t *menu_state) {
	if (!DTTR_ResultOK(DTTR_PCDOGS_D_Menu_HandleOptionsLogic_InputMenuButtonRemappingActive
						   ->Read(remapping_active))) {
		return false;
	}

	return DTTR_ResultOK(DTTR_PCDOGS_D_Menu_HandleOptionsLogic_State->Read(menu_state));
}

int32_t __cdecl dttr_inputs_hook_get_pressed_button_callback() {
	if (!dttr_inputs_hook_get_pressed_button_original) {
		return -1;
	}

	const int32_t pressed_button = dttr_inputs_hook_get_pressed_button_original();
	if (pressed_button != VK_RETURN) {
		return pressed_button;
	}

	int32_t remapping_active = 0;
	int32_t menu_state = 0;
	if (!read_controls_menu_state(&remapping_active, &menu_state)) {
		return pressed_button;
	}

	return dttr_inputs_controls_menu_pressed_button(
		pressed_button,
		remapping_active,
		menu_state
	);
}

void dttr_inputs_hook_get_pressed_button_reset() {
	dttr_inputs_hook_get_pressed_button_original = NULL;
}
