#include <SDL3/SDL.h>
#include <windows.h>

#include "hooks_private.h"
#include "inputs_private.h"

enum {
	DTTR_INPUTS_NO_PRESSED_BUTTON = -1,
};

static bool remapping_seen;
static bool held_scancodes[SDL_SCANCODE_COUNT];
static bool held_gamepad_buttons[DTTR_INPUTS_SDL_BUTTON_COUNT];
static bool pending_gamepad_button_events[DTTR_INPUTS_SDL_BUTTON_COUNT];
static int32_t held_native_key_code = DTTR_INPUTS_NO_PRESSED_BUTTON;

void dttr_inputs_controls_menu_reset() {
	remapping_seen = false;
	SDL_zeroa(held_scancodes);
	SDL_zeroa(held_gamepad_buttons);
	SDL_zeroa(pending_gamepad_button_events);
	held_native_key_code = DTTR_INPUTS_NO_PRESSED_BUTTON;
}

void dttr_inputs_controls_menu_handle_event(const SDL_Event *event) {
	if (!event) {
		return;
	}

	int button = SDL_GAMEPAD_BUTTON_INVALID;
	switch (event->type) {
	case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
		button = event->gbutton.button;
		break;
	case SDL_EVENT_JOYSTICK_BUTTON_DOWN:
		if (dttr_inputs_gamepad) {
			return;
		}

		button = event->jbutton.button;
		break;
	default:
		return;
	}

	if (button >= 0 && button < DTTR_INPUTS_SDL_BUTTON_COUNT) {
		pending_gamepad_button_events[button] = true;
	}
}

static int32_t remap_return_key(
	int32_t pressed_button,
	const bool *keyboard_state,
	int keyboard_state_count
) {
	const int action = DTTR_Config_ControlActionIndex("menu_confirm");
	if (pressed_button != VK_RETURN
		&& !dttr_inputs_control_action_pressed(
			action,
			keyboard_state,
			keyboard_state_count
		)) {
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

static int32_t update_held_gamepad_buttons(
	const bool *gamepad_button_state,
	int gamepad_button_count,
	const bool *joystick_button_state,
	int joystick_button_count,
	bool seed_held
) {
	int32_t key_code = DTTR_INPUTS_NO_PRESSED_BUTTON;

	for (int button = 0; button < DTTR_INPUTS_SDL_BUTTON_COUNT; button++) {
		const bool pressed = (gamepad_button_state && button < gamepad_button_count
							  && gamepad_button_state[button])
							 || (joystick_button_state && button < joystick_button_count
								 && joystick_button_state[button])
							 || pending_gamepad_button_events[button];

		if (seed_held) {
			held_gamepad_buttons[button] = pressed;
			continue;
		}

		if (!pressed) {
			held_gamepad_buttons[button] = false;
			continue;
		}

		if (held_gamepad_buttons[button] || key_code != DTTR_INPUTS_NO_PRESSED_BUTTON) {
			pending_gamepad_button_events[button] = false;
			continue;
		}

		key_code = dttr_inputs_key_code_from_sdl_button(button);
		pending_gamepad_button_events[button] = false;
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

static bool escape_requested(
	int32_t pressed_button,
	const bool *keyboard_state,
	int keyboard_state_count
) {
	return pressed_button == VK_ESCAPE
		   || dttr_inputs_keyboard_scancode_pressed(
			   keyboard_state,
			   keyboard_state_count,
			   SDL_SCANCODE_ESCAPE
		   );
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

int32_t dttr_inputs_controls_menu_pressed_keyboard_controller_button(
	int32_t pressed_button,
	int32_t remapping_active,
	const bool *keyboard_state,
	int keyboard_state_count,
	const bool *gamepad_button_state,
	int gamepad_button_count,
	const bool *joystick_button_state,
	int joystick_button_count
) {
	if (remapping_active == 0) {
		if (remapping_seen) {
			dttr_inputs_controls_menu_reset();
		}
		return remap_return_key(pressed_button, keyboard_state, keyboard_state_count);
	}

	if (escape_requested(pressed_button, keyboard_state, keyboard_state_count)) {
		dttr_inputs_controls_menu_reset();
		return VK_ESCAPE;
	}

	const DTTR_Input_KeyCodeKind pressed_kind = dttr_inputs_key_code_kind(pressed_button);
	const int32_t native_key_code = native_keyboard_key_code(pressed_button, pressed_kind);
	const bool first_remap_frame = !remapping_seen;

	if (first_remap_frame) {
		remapping_seen = true;
		SDL_zeroa(pending_gamepad_button_events);
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

	const int32_t gamepad_code = update_held_gamepad_buttons(
		gamepad_button_state,
		gamepad_button_count,
		joystick_button_state,
		joystick_button_count,
		first_remap_frame
	);
	if (gamepad_code != DTTR_INPUTS_NO_PRESSED_BUTTON) {
		return gamepad_code;
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

static void read_gamepad_buttons(bool *out_buttons, int out_count) {
	if (!out_buttons || out_count <= 0 || !dttr_inputs_gamepad) {
		return;
	}

	const int count = out_count < SDL_GAMEPAD_BUTTON_COUNT ? out_count
														   : SDL_GAMEPAD_BUTTON_COUNT;
	for (int button = SDL_GAMEPAD_BUTTON_SOUTH; button < count; button++) {
		out_buttons[button] = SDL_GetGamepadButton(
			dttr_inputs_gamepad,
			(SDL_GamepadButton)button
		);
	}
}

static void read_joystick_buttons(bool *out_buttons, int out_count) {
	if (!out_buttons || out_count <= 0 || dttr_inputs_gamepad) {
		return;
	}

	SDL_Joystick *joystick = dttr_inputs_raw_joystick();
	if (!joystick) {
		return;
	}

	int count = SDL_GetNumJoystickButtons(joystick);
	if (count <= 0) {
		return;
	}

	count = count < out_count ? count : out_count;
	for (int button = 0; button < count; button++) {
		out_buttons[button] = SDL_GetJoystickButton(joystick, button);
	}
}

int32_t dttr_inputs_controls_menu_pressed_button(
	int32_t pressed_button,
	int32_t remapping_active
) {
	SDL_PumpEvents();
	SDL_UpdateGamepads();
	SDL_UpdateJoysticks();

	int keyboard_state_count = 0;
	const bool *keyboard_state = SDL_GetKeyboardState(&keyboard_state_count);
	bool gamepad_button_state[SDL_GAMEPAD_BUTTON_COUNT] = {0};
	bool joystick_button_state[DTTR_INPUTS_SDL_BUTTON_COUNT] = {0};
	read_gamepad_buttons(gamepad_button_state, (int)SDL_arraysize(gamepad_button_state));
	read_joystick_buttons(
		joystick_button_state,
		(int)SDL_arraysize(joystick_button_state)
	);

	return dttr_inputs_controls_menu_pressed_keyboard_controller_button(
		pressed_button,
		remapping_active,
		keyboard_state,
		keyboard_state_count,
		gamepad_button_state,
		(int)SDL_arraysize(gamepad_button_state),
		joystick_button_state,
		(int)SDL_arraysize(joystick_button_state)
	);
}
