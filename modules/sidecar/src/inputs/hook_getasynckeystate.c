#include <SDL3/SDL.h>
#include <dttr_input_names.h>
#include <windows.h>

#include "hooks_private.h"
#include "inputs_private.h"

#define GETASYNCKEYSTATE_KEY_PRESSED 0x8000

static const SDL_Scancode vk_to_scancode[256] = {
	['A'] = SDL_SCANCODE_A,
	['B'] = SDL_SCANCODE_B,
	['C'] = SDL_SCANCODE_C,
	['D'] = SDL_SCANCODE_D,
	['E'] = SDL_SCANCODE_E,
	['F'] = SDL_SCANCODE_F,
	['G'] = SDL_SCANCODE_G,
	['H'] = SDL_SCANCODE_H,
	['I'] = SDL_SCANCODE_I,
	['J'] = SDL_SCANCODE_J,
	['K'] = SDL_SCANCODE_K,
	['L'] = SDL_SCANCODE_L,
	['M'] = SDL_SCANCODE_M,
	['N'] = SDL_SCANCODE_N,
	['O'] = SDL_SCANCODE_O,
	['P'] = SDL_SCANCODE_P,
	['Q'] = SDL_SCANCODE_Q,
	['R'] = SDL_SCANCODE_R,
	['S'] = SDL_SCANCODE_S,
	['T'] = SDL_SCANCODE_T,
	['U'] = SDL_SCANCODE_U,
	['V'] = SDL_SCANCODE_V,
	['W'] = SDL_SCANCODE_W,
	['X'] = SDL_SCANCODE_X,
	['Y'] = SDL_SCANCODE_Y,
	['Z'] = SDL_SCANCODE_Z,
	['0'] = SDL_SCANCODE_0,
	['1'] = SDL_SCANCODE_1,
	['2'] = SDL_SCANCODE_2,
	['3'] = SDL_SCANCODE_3,
	['4'] = SDL_SCANCODE_4,
	['5'] = SDL_SCANCODE_5,
	['6'] = SDL_SCANCODE_6,
	['7'] = SDL_SCANCODE_7,
	['8'] = SDL_SCANCODE_8,
	['9'] = SDL_SCANCODE_9,
	[VK_OEM_1] = SDL_SCANCODE_SEMICOLON,
	[VK_OEM_PLUS] = SDL_SCANCODE_EQUALS,
	[VK_OEM_COMMA] = SDL_SCANCODE_COMMA,
	[VK_OEM_MINUS] = SDL_SCANCODE_MINUS,
	[VK_OEM_PERIOD] = SDL_SCANCODE_PERIOD,
	[VK_OEM_2] = SDL_SCANCODE_SLASH,
	[VK_OEM_3] = SDL_SCANCODE_GRAVE,
	[VK_OEM_4] = SDL_SCANCODE_LEFTBRACKET,
	[VK_OEM_5] = SDL_SCANCODE_BACKSLASH,
	[VK_OEM_6] = SDL_SCANCODE_RIGHTBRACKET,
	[VK_OEM_7] = SDL_SCANCODE_APOSTROPHE,
	[VK_OEM_102] = SDL_SCANCODE_NONUSBACKSLASH,
	[VK_CANCEL] = SDL_SCANCODE_CANCEL,
	[VK_RETURN] = SDL_SCANCODE_RETURN,
	[VK_ESCAPE] = SDL_SCANCODE_ESCAPE,
	[VK_BACK] = SDL_SCANCODE_BACKSPACE,
	[VK_TAB] = SDL_SCANCODE_TAB,
	[VK_CLEAR] = SDL_SCANCODE_CLEAR,
	[VK_SPACE] = SDL_SCANCODE_SPACE,
	[VK_PAUSE] = SDL_SCANCODE_PAUSE,
	[VK_CAPITAL] = SDL_SCANCODE_CAPSLOCK,
	[VK_F1] = SDL_SCANCODE_F1,
	[VK_F2] = SDL_SCANCODE_F2,
	[VK_F3] = SDL_SCANCODE_F3,
	[VK_F4] = SDL_SCANCODE_F4,
	[VK_F5] = SDL_SCANCODE_F5,
	[VK_F6] = SDL_SCANCODE_F6,
	[VK_F7] = SDL_SCANCODE_F7,
	[VK_F8] = SDL_SCANCODE_F8,
	[VK_F9] = SDL_SCANCODE_F9,
	[VK_F10] = SDL_SCANCODE_F10,
	[VK_F11] = SDL_SCANCODE_F11,
	[VK_F12] = SDL_SCANCODE_F12,
	[VK_F13] = SDL_SCANCODE_F13,
	[VK_F14] = SDL_SCANCODE_F14,
	[VK_F15] = SDL_SCANCODE_F15,
	[VK_F16] = SDL_SCANCODE_F16,
	[VK_F17] = SDL_SCANCODE_F17,
	[VK_F18] = SDL_SCANCODE_F18,
	[VK_F19] = SDL_SCANCODE_F19,
	[VK_F20] = SDL_SCANCODE_F20,
	[VK_F21] = SDL_SCANCODE_F21,
	[VK_F22] = SDL_SCANCODE_F22,
	[VK_F23] = SDL_SCANCODE_F23,
	[VK_F24] = SDL_SCANCODE_F24,
	[VK_INSERT] = SDL_SCANCODE_INSERT,
	[VK_DELETE] = SDL_SCANCODE_DELETE,
	[VK_HOME] = SDL_SCANCODE_HOME,
	[VK_END] = SDL_SCANCODE_END,
	[VK_PRIOR] = SDL_SCANCODE_PAGEUP,
	[VK_NEXT] = SDL_SCANCODE_PAGEDOWN,
	[VK_RIGHT] = SDL_SCANCODE_RIGHT,
	[VK_LEFT] = SDL_SCANCODE_LEFT,
	[VK_DOWN] = SDL_SCANCODE_DOWN,
	[VK_UP] = SDL_SCANCODE_UP,
	[VK_SELECT] = SDL_SCANCODE_SELECT,
	[VK_PRINT] = SDL_SCANCODE_PRINTSCREEN,
	[VK_EXECUTE] = SDL_SCANCODE_EXECUTE,
	[VK_SNAPSHOT] = SDL_SCANCODE_PRINTSCREEN,
	[VK_HELP] = SDL_SCANCODE_HELP,
	[VK_CONTROL] = SDL_SCANCODE_LCTRL,
	[VK_LCONTROL] = SDL_SCANCODE_LCTRL,
	[VK_RCONTROL] = SDL_SCANCODE_RCTRL,
	[VK_SHIFT] = SDL_SCANCODE_LSHIFT,
	[VK_LSHIFT] = SDL_SCANCODE_LSHIFT,
	[VK_RSHIFT] = SDL_SCANCODE_RSHIFT,
	[VK_MENU] = SDL_SCANCODE_LALT,
	[VK_LMENU] = SDL_SCANCODE_LALT,
	[VK_RMENU] = SDL_SCANCODE_RALT,
	[VK_LWIN] = SDL_SCANCODE_LGUI,
	[VK_RWIN] = SDL_SCANCODE_RGUI,
	[VK_APPS] = SDL_SCANCODE_APPLICATION,
	[VK_SLEEP] = SDL_SCANCODE_SLEEP,
	[VK_NUMPAD0] = SDL_SCANCODE_KP_0,
	[VK_NUMPAD1] = SDL_SCANCODE_KP_1,
	[VK_NUMPAD2] = SDL_SCANCODE_KP_2,
	[VK_NUMPAD3] = SDL_SCANCODE_KP_3,
	[VK_NUMPAD4] = SDL_SCANCODE_KP_4,
	[VK_NUMPAD5] = SDL_SCANCODE_KP_5,
	[VK_NUMPAD6] = SDL_SCANCODE_KP_6,
	[VK_NUMPAD7] = SDL_SCANCODE_KP_7,
	[VK_NUMPAD8] = SDL_SCANCODE_KP_8,
	[VK_NUMPAD9] = SDL_SCANCODE_KP_9,
	[VK_MULTIPLY] = SDL_SCANCODE_KP_MULTIPLY,
	[VK_ADD] = SDL_SCANCODE_KP_PLUS,
	[VK_SEPARATOR] = SDL_SCANCODE_SEPARATOR,
	[VK_SUBTRACT] = SDL_SCANCODE_KP_MINUS,
	[VK_DECIMAL] = SDL_SCANCODE_KP_PERIOD,
	[VK_DIVIDE] = SDL_SCANCODE_KP_DIVIDE,
	[VK_NUMLOCK] = SDL_SCANCODE_NUMLOCKCLEAR,
	[VK_SCROLL] = SDL_SCANCODE_SCROLLLOCK,
	[VK_OEM_NEC_EQUAL] = SDL_SCANCODE_KP_EQUALS,
	[VK_BROWSER_BACK] = SDL_SCANCODE_AC_BACK,
	[VK_BROWSER_FORWARD] = SDL_SCANCODE_AC_FORWARD,
	[VK_BROWSER_REFRESH] = SDL_SCANCODE_AC_REFRESH,
	[VK_BROWSER_STOP] = SDL_SCANCODE_AC_STOP,
	[VK_BROWSER_SEARCH] = SDL_SCANCODE_AC_SEARCH,
	[VK_BROWSER_FAVORITES] = SDL_SCANCODE_AC_BOOKMARKS,
	[VK_BROWSER_HOME] = SDL_SCANCODE_AC_HOME,
	[VK_VOLUME_MUTE] = SDL_SCANCODE_MUTE,
	[VK_VOLUME_DOWN] = SDL_SCANCODE_VOLUMEDOWN,
	[VK_VOLUME_UP] = SDL_SCANCODE_VOLUMEUP,
	[VK_MEDIA_NEXT_TRACK] = SDL_SCANCODE_MEDIA_NEXT_TRACK,
	[VK_MEDIA_PREV_TRACK] = SDL_SCANCODE_MEDIA_PREVIOUS_TRACK,
	[VK_MEDIA_STOP] = SDL_SCANCODE_MEDIA_STOP,
	[VK_MEDIA_PLAY_PAUSE] = SDL_SCANCODE_MEDIA_PLAY_PAUSE,
	[VK_LAUNCH_MEDIA_SELECT] = SDL_SCANCODE_MEDIA_SELECT,
	[VK_CRSEL] = SDL_SCANCODE_CRSEL,
	[VK_EXSEL] = SDL_SCANCODE_EXSEL,
	[VK_PLAY] = SDL_SCANCODE_MEDIA_PLAY,
	[VK_OEM_CLEAR] = SDL_SCANCODE_CLEAR,
};

bool dttr_inputs_keyboard_scancode_pressed(
	const bool *keyboard_state,
	int keyboard_state_count,
	int scancode
) {
	return keyboard_state && scancode > SDL_SCANCODE_UNKNOWN
		   && scancode < keyboard_state_count && keyboard_state[scancode];
}

int32_t dttr_inputs_key_code_from_scancode(int scancode) {
	if (scancode <= SDL_SCANCODE_UNKNOWN || scancode >= SDL_SCANCODE_COUNT) {
		return -1;
	}

	return DTTR_INPUTS_KEY_SCANCODE_BASE + scancode;
}

int32_t dttr_inputs_key_code_from_sdl_button(int button) {
	if (button < 0 || button >= DTTR_INPUTS_SDL_BUTTON_COUNT) {
		return -1;
	}

	return DTTR_INPUTS_SDL_GAMEPAD_BUTTON_BASE + button;
}

int32_t dttr_inputs_key_code_from_gamepad_button(SDL_GamepadButton button) {
	if (button < SDL_GAMEPAD_BUTTON_SOUTH || button >= SDL_GAMEPAD_BUTTON_COUNT) {
		return -1;
	}

	return dttr_inputs_key_code_from_sdl_button((int)button);
}

int dttr_inputs_key_code_scancode(int32_t key_code) {
	if (key_code == DTTR_INPUTS_KEY_KEYPAD_ENTER) {
		return SDL_SCANCODE_KP_ENTER;
	}

	if (key_code < DTTR_INPUTS_KEY_SCANCODE_BASE) {
		return SDL_SCANCODE_UNKNOWN;
	}

	const int scancode = key_code - DTTR_INPUTS_KEY_SCANCODE_BASE;
	if (scancode <= SDL_SCANCODE_UNKNOWN || scancode >= SDL_SCANCODE_COUNT) {
		return SDL_SCANCODE_UNKNOWN;
	}

	return scancode;
}

int dttr_inputs_key_code_sdl_button(int32_t key_code) {
	if (key_code < DTTR_INPUTS_SDL_GAMEPAD_BUTTON_BASE
		|| key_code
			   >= DTTR_INPUTS_SDL_GAMEPAD_BUTTON_BASE + DTTR_INPUTS_SDL_BUTTON_COUNT) {
		return -1;
	}

	const int button = key_code - DTTR_INPUTS_SDL_GAMEPAD_BUTTON_BASE;
	return button >= 0 && button < DTTR_INPUTS_SDL_BUTTON_COUNT ? button : -1;
}

DTTR_Input_KeyCodeKind dttr_inputs_key_code_kind(int32_t key_code) {
	if (key_code < 0) {
		return DTTR_INPUTS_KEY_CODE_NONE;
	}

	if (key_code >= DTTR_INPUTS_SDL_GAMEPAD_BUTTON_BASE) {
		return dttr_inputs_key_code_sdl_button(key_code) >= 0
				   ? DTTR_INPUTS_KEY_CODE_SDL_GAMEPAD
				   : DTTR_INPUTS_KEY_CODE_NONE;
	}

	if (key_code >= DTTR_INPUTS_GAMEPAD_BUTTON_BASE) {
		return key_code < DTTR_INPUTS_GAMEPAD_BUTTON_BASE
							  + DTTR_INPUTS_GAMEPAD_BUTTON_COUNT
				   ? DTTR_INPUTS_KEY_CODE_GAMEPAD
				   : DTTR_INPUTS_KEY_CODE_NONE;
	}

	if (key_code >= DTTR_INPUTS_KEY_SCANCODE_BASE
		|| key_code == DTTR_INPUTS_KEY_KEYPAD_ENTER) {
		return dttr_inputs_key_code_scancode(key_code) == SDL_SCANCODE_UNKNOWN
				   ? DTTR_INPUTS_KEY_CODE_NONE
				   : DTTR_INPUTS_KEY_CODE_SCANCODE;
	}

	return DTTR_INPUTS_KEY_CODE_VKEY;
}

bool dttr_inputs_key_state_uses_live_state(int32_t key_code) {
	const DTTR_Input_KeyCodeKind kind = dttr_inputs_key_code_kind(key_code);
	return key_code == VK_RETURN || kind == DTTR_INPUTS_KEY_CODE_SCANCODE
		   || kind == DTTR_INPUTS_KEY_CODE_SDL_GAMEPAD;
}

int dttr_inputs_vkey_scancode(int vkey) {
	if (vkey < 0 || vkey >= (int)SDL_arraysize(vk_to_scancode)) {
		return SDL_SCANCODE_UNKNOWN;
	}

	return vk_to_scancode[vkey];
}

const char *dttr_inputs_key_code_name(int32_t key_code) {
	enum {
		BUFFER_COUNT = 4,
		BUFFER_SIZE = 32,
	};
	static char buffers[BUFFER_COUNT][BUFFER_SIZE];
	static int buffer_index;

	char *buffer = buffers[buffer_index];
	buffer_index = (buffer_index + 1) % BUFFER_COUNT;
	return DTTR_InputNames_ControlCode(key_code, buffer, BUFFER_SIZE) ? buffer : NULL;
}

bool dttr_inputs_controller_button_pressed(int button) {
	if (button < 0 || button >= DTTR_INPUTS_SDL_BUTTON_COUNT) {
		return false;
	}

	if (dttr_inputs_gamepad && button < SDL_GAMEPAD_BUTTON_COUNT
		&& SDL_GetGamepadButton(dttr_inputs_gamepad, (SDL_GamepadButton)button)) {
		return true;
	}

	if (dttr_inputs_gamepad) {
		return false;
	}

	SDL_Joystick *joystick = dttr_inputs_raw_joystick();
	if (!joystick) {
		return false;
	}

	const int button_count = SDL_GetNumJoystickButtons(joystick);
	return button < button_count && SDL_GetJoystickButton(joystick, button);
}

SHORT __stdcall dttr_inputs_hook_get_async_key_state_callback(int vkey) {
	int keyboard_state_count = 0;
	const bool *keyboard_state = SDL_GetKeyboardState(&keyboard_state_count);

	return dttr_inputs_global_vkey_pressed(vkey, keyboard_state, keyboard_state_count)
			   ? (SHORT)GETASYNCKEYSTATE_KEY_PRESSED
			   : 0;
}

// Aggregate Windows vkeys represent paired SDL scancodes; either side counts as pressed.
static const struct {
	int vkey;
	SDL_Scancode left;
	SDL_Scancode right;
} vkey_scancode_pairs[] = {
	{VK_RETURN, SDL_SCANCODE_RETURN, SDL_SCANCODE_KP_ENTER},
	{VK_SHIFT, SDL_SCANCODE_LSHIFT, SDL_SCANCODE_RSHIFT},
	{VK_CONTROL, SDL_SCANCODE_LCTRL, SDL_SCANCODE_RCTRL},
	{VK_MENU, SDL_SCANCODE_LALT, SDL_SCANCODE_RALT},
};

bool dttr_inputs_vkey_pressed(
	int vkey,
	const bool *keyboard_state,
	int keyboard_state_count
) {
	for (size_t i = 0; i < SDL_arraysize(vkey_scancode_pairs); i++) {
		if (vkey_scancode_pairs[i].vkey == vkey) {
			return dttr_inputs_keyboard_scancode_pressed(
					   keyboard_state,
					   keyboard_state_count,
					   vkey_scancode_pairs[i].left
				   )
				   || dttr_inputs_keyboard_scancode_pressed(
					   keyboard_state,
					   keyboard_state_count,
					   vkey_scancode_pairs[i].right
				   );
		}
	}

	const SDL_Scancode scancode = (SDL_Scancode)dttr_inputs_vkey_scancode(vkey);

	if (scancode == SDL_SCANCODE_UNKNOWN) {
		return false;
	}

	return dttr_inputs_keyboard_scancode_pressed(
		keyboard_state,
		keyboard_state_count,
		scancode
	);
}

bool dttr_inputs_key_code_pressed(
	int key_code,
	const bool *keyboard_state,
	int keyboard_state_count
) {
	const DTTR_Input_KeyCodeKind kind = dttr_inputs_key_code_kind(key_code);

	if (kind == DTTR_INPUTS_KEY_CODE_SCANCODE) {
		const int scancode = dttr_inputs_key_code_scancode(key_code);
		return dttr_inputs_keyboard_scancode_pressed(
			keyboard_state,
			keyboard_state_count,
			(SDL_Scancode)scancode
		);
	}

	if (kind == DTTR_INPUTS_KEY_CODE_SDL_GAMEPAD) {
		const int button = dttr_inputs_key_code_sdl_button(key_code);
		return dttr_inputs_controller_button_pressed(button);
	}

	return kind == DTTR_INPUTS_KEY_CODE_VKEY
		   && dttr_inputs_vkey_pressed(key_code, keyboard_state, keyboard_state_count);
}

bool dttr_inputs_control_action_pressed(
	int action,
	const bool *keyboard_state,
	int keyboard_state_count
) {
	if (action < 0 || action >= DTTR_CONFIG_CONTROL_ACTION_COUNT) {
		return false;
	}

	const int key_code = dttr_config.control_bindings[action];
	return key_code != DTTR_CONFIG_CONTROL_BINDING_NONE
		   && dttr_inputs_key_code_pressed(key_code, keyboard_state, keyboard_state_count);
}

bool dttr_inputs_global_vkey_pressed(
	int vkey,
	const bool *keyboard_state,
	int keyboard_state_count
) {
	return dttr_inputs_key_code_pressed(vkey, keyboard_state, keyboard_state_count);
}
