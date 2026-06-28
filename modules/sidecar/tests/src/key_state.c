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

static void return_keys_bind_when_pressed_after_remap_opens(void **) {
	bool keyboard_state[SDL_SCANCODE_COUNT] = {0};

	dttr_inputs_controls_menu_reset();
	assert_int_equal(menu_button(-1, 1, keyboard_state), -1);
	keyboard_state[SDL_SCANCODE_RETURN] = true;
	assert_int_equal(menu_button(VK_RETURN, 1, keyboard_state), VK_RETURN);

	dttr_inputs_controls_menu_reset();
	keyboard_state[SDL_SCANCODE_RETURN] = false;
	assert_int_equal(menu_button(-1, 1, keyboard_state), -1);
	keyboard_state[SDL_SCANCODE_KP_ENTER] = true;
	assert_int_equal(
		menu_button(VK_RETURN, 1, keyboard_state),
		DTTR_INPUTS_KEY_KEYPAD_ENTER
	);
}

static void held_return_keys_wait_for_release_before_binding(void **) {
	bool keyboard_state[SDL_SCANCODE_COUNT] = {0};

	dttr_inputs_controls_menu_reset();

	keyboard_state[SDL_SCANCODE_RETURN] = true;

	assert_int_equal(menu_button(VK_RETURN, 1, keyboard_state), -1);

	keyboard_state[SDL_SCANCODE_RETURN] = false;

	assert_int_equal(menu_button(-1, 1, keyboard_state), -1);

	keyboard_state[SDL_SCANCODE_RETURN] = true;

	assert_int_equal(menu_button(VK_RETURN, 1, keyboard_state), VK_RETURN);

	dttr_inputs_controls_menu_reset();

	assert_int_equal(menu_button(VK_RETURN, 0, keyboard_state), VK_RETURN);
	assert_int_equal(menu_button(VK_RETURN, 1, keyboard_state), -1);

	keyboard_state[SDL_SCANCODE_RETURN] = false;

	assert_int_equal(menu_button(-1, 1, keyboard_state), -1);

	keyboard_state[SDL_SCANCODE_RETURN] = true;

	assert_int_equal(menu_button(VK_RETURN, 1, keyboard_state), VK_RETURN);

	dttr_inputs_controls_menu_reset();

	keyboard_state[SDL_SCANCODE_RETURN] = false;
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
		DTTR_INPUTS_KEY_KEYPAD_ENTER
	);
}

static const DTTR_TestCase TEST_CASES[] = {
	{"return-keys-bind-when-pressed-after-remap-opens",
	 return_keys_bind_when_pressed_after_remap_opens},
	{"held-return-keys-wait-for-release-before-binding",
	 held_return_keys_wait_for_release_before_binding},
};

DTTR_TEST_MAIN(TEST_CASES)
