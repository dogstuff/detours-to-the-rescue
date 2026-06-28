#include <SDL3/SDL.h>
#include <windows.h>

#include "hooks_private.h"

enum {
	DTTR_INPUTS_NO_PRESSED_BUTTON = -1,
};

static bool remapping_seen;
static bool held_scancodes[SDL_SCANCODE_COUNT];
static int32_t held_native_key_code = DTTR_INPUTS_NO_PRESSED_BUTTON;

void dttr_inputs_controls_menu_reset() {
	remapping_seen = false;
	SDL_zeroa(held_scancodes);
	held_native_key_code = DTTR_INPUTS_NO_PRESSED_BUTTON;
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

static int32_t update_held_scancodes(
	const bool *keyboard_state,
	int keyboard_state_count,
	bool seed_held
) {
	int32_t key_code = DTTR_INPUTS_NO_PRESSED_BUTTON;

	for (int scancode = SDL_SCANCODE_UNKNOWN + 1; scancode < SDL_SCANCODE_COUNT;
		 scancode++) {
		const bool pressed = dttr_inputs_keyboard_scancode_pressed(
			keyboard_state,
			keyboard_state_count,
			scancode
		);

		if (seed_held) {
			held_scancodes[scancode] = pressed;
			continue;
		}

		if (!pressed) {
			held_scancodes[scancode] = false;
			continue;
		}

		if (held_scancodes[scancode] || key_code != DTTR_INPUTS_NO_PRESSED_BUTTON) {
			continue;
		}

		key_code = dttr_inputs_key_code_from_scancode(scancode);
	}

	return key_code;
}

static int32_t native_keyboard_key_code(
	int32_t pressed_button,
	DTTR_Input_KeyCodeKind kind
) {
	if (kind == DTTR_INPUTS_KEY_CODE_SCANCODE) {
		const int scancode = dttr_inputs_key_code_scancode(pressed_button);
		return dttr_inputs_key_code_from_scancode(scancode);
	}

	if (kind != DTTR_INPUTS_KEY_CODE_VKEY) {
		return DTTR_INPUTS_NO_PRESSED_BUTTON;
	}

	const int scancode = dttr_inputs_vkey_scancode(pressed_button);
	return scancode == SDL_SCANCODE_UNKNOWN
			   ? DTTR_INPUTS_NO_PRESSED_BUTTON
			   : dttr_inputs_key_code_from_scancode(scancode);
}

static void clear_released_native_key_code(int32_t native_key_code) {
	if (held_native_key_code == DTTR_INPUTS_NO_PRESSED_BUTTON) {
		return;
	}

	if (held_native_key_code == native_key_code) {
		return;
	}

	held_native_key_code = DTTR_INPUTS_NO_PRESSED_BUTTON;
}

int32_t dttr_inputs_controls_menu_pressed_keyboard_button(
	int32_t pressed_button,
	int32_t remapping_active,
	const bool *keyboard_state,
	int keyboard_state_count
) {
	if (remapping_active == 0) {
		if (remapping_seen) {
			dttr_inputs_controls_menu_reset();
		}
		return remap_return_key(pressed_button, keyboard_state, keyboard_state_count);
	}

	const DTTR_Input_KeyCodeKind pressed_kind = dttr_inputs_key_code_kind(pressed_button);
	const int32_t native_key_code = native_keyboard_key_code(pressed_button, pressed_kind);
	const bool first_remap_frame = !remapping_seen;

	if (first_remap_frame) {
		remapping_seen = true;
		held_native_key_code = native_key_code;
	}

	clear_released_native_key_code(native_key_code);

	const int32_t key_code = update_held_scancodes(
		keyboard_state,
		keyboard_state_count,
		first_remap_frame
	);
	if (key_code != DTTR_INPUTS_NO_PRESSED_BUTTON) {
		return key_code;
	}

	if (native_key_code != DTTR_INPUTS_NO_PRESSED_BUTTON
		&& native_key_code != held_native_key_code) {
		return native_key_code;
	}

	if (pressed_kind == DTTR_INPUTS_KEY_CODE_GAMEPAD) {
		return pressed_button;
	}

	return DTTR_INPUTS_NO_PRESSED_BUTTON;
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
