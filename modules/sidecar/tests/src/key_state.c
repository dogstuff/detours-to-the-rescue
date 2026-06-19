#include <dttr_test_support.h>

#include <SDL3/SDL.h>
#include <windows.h>

#include "inputs/hooks_private.h"

static void assert_vkey_pressed_by_scancode(int vkey, SDL_Scancode scancode) {
	bool keyboard_state[SDL_SCANCODE_COUNT] = {0};
	keyboard_state[scancode] = true;
	assert_true(dttr_inputs_vkey_pressed(vkey, keyboard_state, SDL_SCANCODE_COUNT));
}

static void return_virtual_key_accepts_main_and_keypad_enter(void **) {
	assert_vkey_pressed_by_scancode(VK_RETURN, SDL_SCANCODE_RETURN);
	assert_vkey_pressed_by_scancode(VK_RETURN, SDL_SCANCODE_KP_ENTER);
}

static void return_virtual_key_ignores_unpressed_enter_keys(void **) {
	bool keyboard_state[SDL_SCANCODE_COUNT] = {0};
	keyboard_state[SDL_SCANCODE_SPACE] = true;
	assert_false(dttr_inputs_vkey_pressed(VK_RETURN, keyboard_state, SDL_SCANCODE_COUNT));
}

static void held_enter_that_opened_remap_is_debounced(void **) {
	assert_int_equal(dttr_inputs_controls_menu_pressed_button(VK_RETURN, 1, 1), -1);
}

static void released_then_pressed_enter_stays_bindable(void **) {
	assert_int_equal(dttr_inputs_controls_menu_pressed_button(VK_RETURN, 1, 0), VK_RETURN);
}

static void controls_menu_filter_leaves_escape_cancel_available(void **) {
	assert_int_equal(dttr_inputs_controls_menu_pressed_button(VK_ESCAPE, 1, 1), VK_ESCAPE);
}

static const DTTR_TestCase TEST_CASES[] = {
	{"return-vkey-accepts-main-and-keypad-enter",
	 return_virtual_key_accepts_main_and_keypad_enter},
	{"return-vkey-ignores-unpressed-enter-keys",
	 return_virtual_key_ignores_unpressed_enter_keys},
	{"held-enter-that-opened-remap-is-debounced",
	 held_enter_that_opened_remap_is_debounced},
	{"released-then-pressed-enter-stays-bindable",
	 released_then_pressed_enter_stays_bindable},
	{"controls-menu-filter-leaves-escape-cancel-available",
	 controls_menu_filter_leaves_escape_cancel_available},
};

DTTR_TEST_MAIN(TEST_CASES)
