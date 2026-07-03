#include <dttr_test_support.h>

#include <SDL3/SDL.h>
#include <windows.h>

#include "inputs/hooks_private.h"

SDL_Gamepad *dttr_inputs_gamepad;

SDL_Joystick *dttr_inputs_raw_joystick() {
	return NULL;
}

static int register_original_calls;
static int32_t register_original_code;
static uint32_t register_original_mask;
static int set_rumble_suppress_original_calls;
static int32_t set_rumble_suppress_original_value;
static int sdl_rumble_calls;
static SDL_Gamepad *sdl_rumble_gamepad;
static uint16_t sdl_rumble_low_frequency;
static uint16_t sdl_rumble_high_frequency;
static uint32_t sdl_rumble_duration_ms;
static bool pressed_gamepad_buttons[SDL_GAMEPAD_BUTTON_COUNT];

static int32_t __cdecl register_button_mapping_original_stub(
	int32_t control_code,
	uint32_t button_mask
) {
	register_original_calls++;
	register_original_code = control_code;
	register_original_mask = button_mask;
	return 123;
}

static int32_t __cdecl set_rumble_suppress_original_stub(char suppress_rumble) {
	set_rumble_suppress_original_calls++;
	set_rumble_suppress_original_value = suppress_rumble;
	return suppress_rumble;
}

static void __cdecl read_devices_original_stub(
	int32_t player_index,
	DTTR_PCDOGS_T_Input_State *state
) {
	if (state) {
		state->button_bits = 0x40;
	}
}

bool SDLCALL __wrap_SDL_RumbleGamepad(
	SDL_Gamepad *gamepad,
	uint16_t low_frequency_rumble,
	uint16_t high_frequency_rumble,
	uint32_t duration_ms
) {
	sdl_rumble_calls++;
	sdl_rumble_gamepad = gamepad;
	sdl_rumble_low_frequency = low_frequency_rumble;
	sdl_rumble_high_frequency = high_frequency_rumble;
	sdl_rumble_duration_ms = duration_ms;
	return true;
}

bool SDLCALL __wrap_SDL_GetGamepadButton(SDL_Gamepad *gamepad, SDL_GamepadButton button) {
	return gamepad && button >= 0 && button < SDL_GAMEPAD_BUTTON_COUNT
		   && pressed_gamepad_buttons[button];
}

typedef struct {
	bool keyboard[SDL_SCANCODE_COUNT];
	bool gamepad[SDL_GAMEPAD_BUTTON_COUNT];
	bool joystick[DTTR_INPUTS_SDL_BUTTON_COUNT];
} menu_state;

static int32_t menu_press(menu_state *state, int32_t pressed, int32_t remapping) {
	return dttr_inputs_controls_menu_pressed_keyboard_controller_button(
		pressed,
		remapping,
		state->keyboard,
		SDL_SCANCODE_COUNT,
		state->gamepad,
		SDL_GAMEPAD_BUTTON_COUNT,
		state->joystick,
		DTTR_INPUTS_SDL_BUTTON_COUNT
	);
}

static int32_t key_code(SDL_Scancode scancode) {
	return dttr_inputs_key_code_from_scancode(scancode);
}

static int32_t gamepad_button_code(SDL_GamepadButton button) {
	return dttr_inputs_key_code_from_gamepad_button(button);
}

static int32_t sdl_button_code(int button) {
	return dttr_inputs_key_code_from_sdl_button(button);
}

static void bind_control_action(const char *key, int code) {
	const int action = DTTR_Config_ControlActionIndex(key);
	assert_true(action >= 0);
	dttr_config.control_bindings[action] = code;
}

static void open_remap_menu(menu_state *state) {
	dttr_inputs_controls_menu_reset();
	assert_int_equal(menu_press(state, -1, 1), -1);
}

static void assert_new_key_binds(int32_t pressed_button, SDL_Scancode scancode) {
	menu_state state = {0};

	open_remap_menu(&state);
	state.keyboard[scancode] = true;

	assert_int_equal(menu_press(&state, pressed_button, 1), key_code(scancode));
}

static void assert_held_key_waits_for_release(
	int32_t pressed_button,
	SDL_Scancode scancode
) {
	menu_state state = {0};

	dttr_inputs_controls_menu_reset();
	state.keyboard[scancode] = true;
	assert_int_equal(menu_press(&state, pressed_button, 1), -1);
	assert_int_equal(menu_press(&state, pressed_button, 1), -1);

	state.keyboard[scancode] = false;
	assert_int_equal(menu_press(&state, -1, 1), -1);

	state.keyboard[scancode] = true;
	assert_int_equal(menu_press(&state, pressed_button, 1), key_code(scancode));
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

static void controls_menu_binds_controller_buttons(void **) {
	menu_state state = {0};

	open_remap_menu(&state);
	state.gamepad[SDL_GAMEPAD_BUTTON_MISC2] = true;

	assert_int_equal(
		menu_press(&state, -1, 1),
		gamepad_button_code(SDL_GAMEPAD_BUTTON_MISC2)
	);

	state = (menu_state){0};

	dttr_inputs_controls_menu_reset();
	assert_int_equal(menu_press(&state, -1, 1), -1);

	state.joystick[SDL_GAMEPAD_BUTTON_EAST] = true;

	assert_int_equal(
		menu_press(&state, -1, 1),
		gamepad_button_code(SDL_GAMEPAD_BUTTON_EAST)
	);

	state.joystick[SDL_GAMEPAD_BUTTON_EAST] = false;
	const int raw_button = SDL_GAMEPAD_BUTTON_COUNT + 7;
	state.joystick[raw_button] = true;

	assert_int_equal(menu_press(&state, -1, 1), sdl_button_code(raw_button));

	state = (menu_state){0};
	SDL_Event event = {0};

	dttr_inputs_controls_menu_reset();
	assert_int_equal(menu_press(&state, -1, 1), -1);

	event.type = SDL_EVENT_GAMEPAD_BUTTON_DOWN;
	event.gbutton.button = SDL_GAMEPAD_BUTTON_NORTH;
	dttr_inputs_controls_menu_handle_event(&event);

	assert_int_equal(
		menu_press(&state, -1, 1),
		gamepad_button_code(SDL_GAMEPAD_BUTTON_NORTH)
	);

	state = (menu_state){0};
	event = (SDL_Event){0};
	const int queued_raw_button = SDL_GAMEPAD_BUTTON_COUNT + 9;

	dttr_inputs_controls_menu_reset();
	assert_int_equal(menu_press(&state, -1, 1), -1);

	event.type = SDL_EVENT_JOYSTICK_BUTTON_DOWN;
	event.jbutton.button = queued_raw_button;
	dttr_inputs_controls_menu_handle_event(&event);

	assert_int_equal(menu_press(&state, -1, 1), sdl_button_code(queued_raw_button));
}

static void controls_menu_waits_for_held_keys_to_release(void **) {
	menu_state state = {0};

	assert_held_key_waits_for_release('A', SDL_SCANCODE_A);
	assert_held_key_waits_for_release(-1, SDL_SCANCODE_GRAVE);

	dttr_inputs_controls_menu_reset();
	state.keyboard[SDL_SCANCODE_KP_ENTER] = true;
	assert_int_equal(menu_press(&state, VK_RETURN, 0), DTTR_INPUTS_KEY_KEYPAD_ENTER);
	assert_int_equal(menu_press(&state, VK_RETURN, 1), -1);

	state.keyboard[SDL_SCANCODE_KP_ENTER] = false;
	assert_int_equal(menu_press(&state, -1, 1), -1);

	state.keyboard[SDL_SCANCODE_KP_ENTER] = true;
	assert_int_equal(menu_press(&state, VK_RETURN, 1), key_code(SDL_SCANCODE_KP_ENTER));
}

static void controls_menu_special_keys(void **) {
	menu_state state = {0};

	DTTR_Config_SetDefaults(&dttr_config);
	bind_control_action("menu_confirm", key_code(SDL_SCANCODE_BACKSPACE));
	state.keyboard[SDL_SCANCODE_BACKSPACE] = true;
	assert_int_equal(menu_press(&state, -1, 0), VK_RETURN);

	state = (menu_state){0};
	dttr_inputs_controls_menu_reset();
	assert_int_equal(menu_press(&state, VK_ESCAPE, 1), VK_ESCAPE);

	state = (menu_state){0};
	open_remap_menu(&state);
	state.keyboard[SDL_SCANCODE_ESCAPE] = true;

	assert_int_equal(menu_press(&state, -1, 1), VK_ESCAPE);
	DTTR_Config_SetDefaults(&dttr_config);
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

static void custom_sdl_button_mappings_apply_direct_masks(void **) {
	register_original_calls = 0;
	register_original_code = 0;
	register_original_mask = 0;
	dttr_inputs_hook_register_button_mapping_original = register_button_mapping_original_stub;
	dttr_inputs_custom_button_mappings_clear();

	const int32_t sdl_button = sdl_button_code(SDL_GAMEPAD_BUTTON_SOUTH);
	assert_int_equal(
		dttr_inputs_hook_register_button_mapping_callback(sdl_button, 0x80),
		0
	);
	assert_int_equal(
		dttr_inputs_hook_register_button_mapping_callback(sdl_button, 0x400),
		0
	);
	assert_int_equal(register_original_calls, 0);
	assert_int_equal(
		dttr_inputs_custom_button_mapping_mask(SDL_GAMEPAD_BUTTON_SOUTH),
		0x480
	);

	dttr_inputs_gamepad = (SDL_Gamepad *)0x1234;
	dttr_inputs_hook_read_devices_original = read_devices_original_stub;
	pressed_gamepad_buttons[SDL_GAMEPAD_BUTTON_SOUTH] = true;
	DTTR_PCDOGS_T_Input_State state = {0};
	dttr_inputs_hook_read_devices_callback(0, &state);
	assert_int_equal(state.button_bits, 0x4c0);

	dttr_inputs_gamepad = NULL;
	SDL_zeroa(pressed_gamepad_buttons);
	dttr_inputs_custom_button_mappings_clear();

	DTTR_Config_SetDefaults(&dttr_config);
	bind_control_action("start_pause", sdl_button_code(SDL_GAMEPAD_BUTTON_START));
	dttr_inputs_hook_config_apply_settings_callback();
	assert_int_equal(
		dttr_inputs_custom_button_mapping_mask(SDL_GAMEPAD_BUTTON_START),
		0x8000
	);

	assert_int_equal(dttr_inputs_hook_register_button_mapping_callback('A', 0x10), 123);
	assert_int_equal(register_original_calls, 1);
	assert_int_equal(register_original_code, 'A');
	assert_int_equal(register_original_mask, 0x10);

	dttr_inputs_hook_mapping_reset();
	DTTR_Config_SetDefaults(&dttr_config);
}

static void setting_rumble_suppression_stops_active_sdl_rumble(void **) {
	set_rumble_suppress_original_calls = 0;
	set_rumble_suppress_original_value = 0;
	sdl_rumble_calls = 0;
	sdl_rumble_gamepad = NULL;
	sdl_rumble_low_frequency = 1;
	sdl_rumble_high_frequency = 1;
	sdl_rumble_duration_ms = 1;
	dttr_inputs_gamepad = (SDL_Gamepad *)0x1234;
	dttr_inputs_hook_set_rumble_suppress_flag_original = set_rumble_suppress_original_stub;

	assert_int_equal(dttr_inputs_hook_set_rumble_suppress_flag_callback(1), 1);
	assert_int_equal(set_rumble_suppress_original_calls, 1);
	assert_int_equal(set_rumble_suppress_original_value, 1);
	assert_int_equal(sdl_rumble_calls, 1);
	assert_ptr_equal(sdl_rumble_gamepad, dttr_inputs_gamepad);
	assert_int_equal(sdl_rumble_low_frequency, 0);
	assert_int_equal(sdl_rumble_high_frequency, 0);
	assert_int_equal(sdl_rumble_duration_ms, 0);

	sdl_rumble_calls = 0;
	assert_int_equal(dttr_inputs_hook_set_rumble_suppress_flag_callback(0), 0);
	assert_int_equal(set_rumble_suppress_original_calls, 2);
	assert_int_equal(set_rumble_suppress_original_value, 0);
	assert_int_equal(sdl_rumble_calls, 0);

	dttr_inputs_hook_rumble_reset();
	dttr_inputs_gamepad = NULL;
}

static const DTTR_TestCase TEST_CASES[] = {
	{"key-codes-read-keyboard-state", key_codes_read_keyboard_state},
	{"controls-menu-binds-new-keyboard-keys", controls_menu_binds_new_keyboard_keys},
	{"controls-menu-binds-controller-buttons", controls_menu_binds_controller_buttons},
	{"controls-menu-waits-for-held-keys-to-release",
	 controls_menu_waits_for_held_keys_to_release},
	{"controls-menu-special-keys", controls_menu_special_keys},
	{"key-code-names-use-game-labels", key_code_names_use_game_labels},
	{"custom-sdl-button-mappings-apply-direct-masks",
	 custom_sdl_button_mappings_apply_direct_masks},
	{"setting-rumble-suppression-stops-active-sdl-rumble",
	 setting_rumble_suppression_stops_active_sdl_rumble},
};

DTTR_TEST_MAIN(TEST_CASES)
