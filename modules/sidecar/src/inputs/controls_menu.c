#include <SDL3/SDL.h>
#include <windows.h>

#include "hooks_private.h"

enum {
	DTTR_INPUTS_NO_PRESSED_BUTTON = -1,
};

static bool remapping_seen;
static bool waiting_for_enter_release;

void dttr_inputs_controls_menu_reset() {
	remapping_seen = false;
	waiting_for_enter_release = false;
}

static int32_t remap_return_key(
	int32_t pressed_button,
	const bool *keyboard_state,
	int keyboard_state_count
) {
	if (pressed_button != VK_RETURN) {
		return pressed_button;
	}

	if (dttr_inputs_key_code_pressed(
			DTTR_INPUTS_KEY_KEYPAD_ENTER,
			keyboard_state,
			keyboard_state_count
		)) {
		return DTTR_INPUTS_KEY_KEYPAD_ENTER;
	}

	return VK_RETURN;
}

static bool return_or_keypad_enter_down(
	int32_t pressed_button,
	const bool *keyboard_state,
	int keyboard_state_count
) {
	return pressed_button == VK_RETURN
		   || dttr_inputs_vkey_or_keypad_enter_pressed(
			   VK_RETURN,
			   keyboard_state,
			   keyboard_state_count
		   );
}

int32_t dttr_inputs_controls_menu_pressed_keyboard_button(
	int32_t pressed_button,
	int32_t remapping_active,
	const bool *keyboard_state,
	int keyboard_state_count
) {
	const bool enter_down = return_or_keypad_enter_down(
		pressed_button,
		keyboard_state,
		keyboard_state_count
	);

	if (remapping_active == 0) {
		remapping_seen = false;
		waiting_for_enter_release = false;
		return remap_return_key(pressed_button, keyboard_state, keyboard_state_count);
	}

	if (!remapping_seen) {
		remapping_seen = true;
		waiting_for_enter_release = enter_down;
	}

	if (waiting_for_enter_release) {
		if (!enter_down) {
			waiting_for_enter_release = false;
		}
		return DTTR_INPUTS_NO_PRESSED_BUTTON;
	}

	return remap_return_key(pressed_button, keyboard_state, keyboard_state_count);
}

int32_t dttr_inputs_controls_menu_pressed_button(
	int32_t pressed_button,
	int32_t remapping_active
) {
	int keyboard_state_count = 0;
	const bool *keyboard_state = SDL_GetKeyboardState(&keyboard_state_count);

	return dttr_inputs_controls_menu_pressed_keyboard_button(
		pressed_button,
		remapping_active,
		keyboard_state,
		keyboard_state_count
	);
}
