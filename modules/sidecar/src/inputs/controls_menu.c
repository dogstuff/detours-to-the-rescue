#include <windows.h>

#include "hooks_private.h"

enum {
	DTTR_INPUTS_NO_PRESSED_BUTTON = -1,
};

int32_t dttr_inputs_controls_menu_pressed_button(
	int32_t pressed_button,
	int32_t remapping_active,
	int32_t menu_state
) {
	if (pressed_button == VK_RETURN && remapping_active != 0 && menu_state != 0) {
		return DTTR_INPUTS_NO_PRESSED_BUTTON;
	}

	return pressed_button;
}
