#include <dttr_test_support.h>

#include <SDL3/SDL.h>
#include <windows.h>

#include "inputs/hooks_private.h"

static int32_t menu_button(int32_t pressed, int32_t remapping, const bool *keyboard_state) {
	return dttr_inputs_controls_menu_pressed_keyboard_button(
		pressed,
		remapping,
		keyboard_state,
		SDL_SCANCODE_COUNT
	);
}

static int32_t key_code(SDL_Scancode scancode) {
	return dttr_inputs_key_code_from_scancode(scancode);
}

static void open_remap_menu(bool *keyboard_state) {
	dttr_inputs_controls_menu_reset();
	assert_int_equal(menu_button(-1, 1, keyboard_state), -1);
}

static void assert_new_key_binds(int32_t pressed_button, SDL_Scancode scancode) {
	bool keyboard_state[SDL_SCANCODE_COUNT] = {0};

	open_remap_menu(keyboard_state);
	keyboard_state[scancode] = true;

	assert_int_equal(menu_button(pressed_button, 1, keyboard_state), key_code(scancode));
}

static void assert_held_key_waits_for_release(
	int32_t pressed_button,
	SDL_Scancode scancode
) {
	bool keyboard_state[SDL_SCANCODE_COUNT] = {0};

	dttr_inputs_controls_menu_reset();
	keyboard_state[scancode] = true;
	assert_int_equal(menu_button(pressed_button, 1, keyboard_state), -1);
	assert_int_equal(menu_button(pressed_button, 1, keyboard_state), -1);

	keyboard_state[scancode] = false;
	assert_int_equal(menu_button(-1, 1, keyboard_state), -1);

	keyboard_state[scancode] = true;
	assert_int_equal(menu_button(pressed_button, 1, keyboard_state), key_code(scancode));
}

static void key_codes_read_keyboard_state(void **) {
	bool keyboard_state[SDL_SCANCODE_COUNT] = {0};
	const int32_t grave = key_code(SDL_SCANCODE_GRAVE);

	assert_true(key_code(SDL_SCANCODE_COUNT - 1) < DTTR_INPUTS_GAMEPAD_BUTTON_BASE);
	assert_false(dttr_inputs_key_code_pressed(grave, keyboard_state, SDL_SCANCODE_COUNT));

	keyboard_state[SDL_SCANCODE_GRAVE] = true;
	keyboard_state[SDL_SCANCODE_LEFTBRACKET] = true;
	keyboard_state[SDL_SCANCODE_RSHIFT] = true;
	keyboard_state[SDL_SCANCODE_KP_ENTER] = true;

	assert_true(dttr_inputs_key_code_pressed(grave, keyboard_state, SDL_SCANCODE_COUNT));
	assert_false(dttr_inputs_key_code_pressed(
		dttr_inputs_key_code_from_scancode(SDL_SCANCODE_COUNT),
		keyboard_state,
		SDL_SCANCODE_COUNT
	));
	assert_true(dttr_inputs_vkey_pressed(VK_OEM_4, keyboard_state, SDL_SCANCODE_COUNT));
	assert_true(dttr_inputs_vkey_pressed(VK_SHIFT, keyboard_state, SDL_SCANCODE_COUNT));
	assert_true(dttr_inputs_vkey_pressed(VK_RETURN, keyboard_state, SDL_SCANCODE_COUNT));
}

static void controls_menu_binds_new_keyboard_keys(void **) {
	assert_new_key_binds('A', SDL_SCANCODE_A);
	assert_new_key_binds(-1, SDL_SCANCODE_GRAVE);
	assert_new_key_binds(VK_RETURN, SDL_SCANCODE_KP_ENTER);
}

static void controls_menu_waits_for_held_keys_to_release(void **) {
	bool keyboard_state[SDL_SCANCODE_COUNT] = {0};

	assert_held_key_waits_for_release('A', SDL_SCANCODE_A);
	assert_held_key_waits_for_release(-1, SDL_SCANCODE_GRAVE);

	dttr_inputs_controls_menu_reset();
	keyboard_state[SDL_SCANCODE_KP_ENTER] = true;
	assert_int_equal(
		menu_button(VK_RETURN, 0, keyboard_state),
		DTTR_INPUTS_KEY_KEYPAD_ENTER
	);
	assert_int_equal(menu_button(VK_RETURN, 1, keyboard_state), -1);

	keyboard_state[SDL_SCANCODE_KP_ENTER] = false;
	assert_int_equal(menu_button(-1, 1, keyboard_state), -1);

	keyboard_state[SDL_SCANCODE_KP_ENTER] = true;
	assert_int_equal(
		menu_button(VK_RETURN, 1, keyboard_state),
		key_code(SDL_SCANCODE_KP_ENTER)
	);
}

static void key_code_names_use_game_labels(void **) {
	assert_string_equal(
		dttr_inputs_key_code_name(key_code(SDL_SCANCODE_LEFTBRACKET)),
		"Left Bracket"
	);
	assert_string_equal(
		dttr_inputs_key_code_name(key_code(SDL_SCANCODE_KP_DIVIDE)),
		"Keypad Divide"
	);
	assert_string_equal(
		dttr_inputs_key_code_name(DTTR_INPUTS_KEY_KEYPAD_ENTER),
		"Keypad Enter"
	);
	assert_null(
		dttr_inputs_key_code_name(dttr_inputs_key_code_from_scancode(SDL_SCANCODE_COUNT))
	);
}

static const DTTR_TestCase TEST_CASES[] = {
	{"key-codes-read-keyboard-state", key_codes_read_keyboard_state},
	{"controls-menu-binds-new-keyboard-keys", controls_menu_binds_new_keyboard_keys},
	{"controls-menu-waits-for-held-keys-to-release",
	 controls_menu_waits_for_held_keys_to_release},
	{"key-code-names-use-game-labels", key_code_names_use_game_labels},
};

DTTR_TEST_MAIN(TEST_CASES)
