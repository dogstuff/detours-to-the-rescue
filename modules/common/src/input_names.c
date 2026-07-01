#include <dttr_config.h>
#include <dttr_input_names.h>

static const char *const GAMEPAD_BUTTON_LABELS[SDL_GAMEPAD_BUTTON_COUNT] = {
	[SDL_GAMEPAD_BUTTON_SOUTH] = "A",
	[SDL_GAMEPAD_BUTTON_EAST] = "B",
	[SDL_GAMEPAD_BUTTON_WEST] = "X",
	[SDL_GAMEPAD_BUTTON_NORTH] = "Y",
	[SDL_GAMEPAD_BUTTON_BACK] = "Back",
	[SDL_GAMEPAD_BUTTON_GUIDE] = "Guide",
	[SDL_GAMEPAD_BUTTON_START] = "Start",
	[SDL_GAMEPAD_BUTTON_LEFT_STICK] = "Left Stick",
	[SDL_GAMEPAD_BUTTON_RIGHT_STICK] = "Right Stick",
	[SDL_GAMEPAD_BUTTON_LEFT_SHOULDER] = "Left Shoulder",
	[SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER] = "Right Shoulder",
	[SDL_GAMEPAD_BUTTON_DPAD_UP] = "D-Pad Up",
	[SDL_GAMEPAD_BUTTON_DPAD_DOWN] = "D-Pad Down",
	[SDL_GAMEPAD_BUTTON_DPAD_LEFT] = "D-Pad Left",
	[SDL_GAMEPAD_BUTTON_DPAD_RIGHT] = "D-Pad Right",
	[SDL_GAMEPAD_BUTTON_MISC1] = "Misc 1",
	[SDL_GAMEPAD_BUTTON_RIGHT_PADDLE1] = "Right Paddle 1",
	[SDL_GAMEPAD_BUTTON_LEFT_PADDLE1] = "Left Paddle 1",
	[SDL_GAMEPAD_BUTTON_RIGHT_PADDLE2] = "Right Paddle 2",
	[SDL_GAMEPAD_BUTTON_LEFT_PADDLE2] = "Left Paddle 2",
	[SDL_GAMEPAD_BUTTON_TOUCHPAD] = "Touchpad",
	[SDL_GAMEPAD_BUTTON_MISC2] = "Misc 2",
	[SDL_GAMEPAD_BUTTON_MISC3] = "Misc 3",
	[SDL_GAMEPAD_BUTTON_MISC4] = "Misc 4",
	[SDL_GAMEPAD_BUTTON_MISC5] = "Misc 5",
	[SDL_GAMEPAD_BUTTON_MISC6] = "Misc 6",
};

static const char *gamepad_button_label(int button) {
	return button >= 0 && button < (int)SDL_arraysize(GAMEPAD_BUTTON_LABELS)
			   ? GAMEPAD_BUTTON_LABELS[button]
			   : NULL;
}

bool DTTR_InputNames_GamepadButton(int button, char *out, size_t out_size) {
	if (!out || out_size == 0) {
		return false;
	}

	const char *label = gamepad_button_label(button);
	const int written = label ? SDL_snprintf(out, out_size, "Joy %s", label)
							  : SDL_snprintf(out, out_size, "Joy Button %d", button);
	return written >= 0 && (size_t)written < out_size;
}

static bool write_name(char *out, size_t out_size, const char *name) {
	if (!name || !name[0]) {
		return false;
	}

	const int written = SDL_snprintf(out, out_size, "%s", name);
	return written >= 0 && (size_t)written < out_size;
}

static const char *symbol_scancode_name(int scancode) {
	static const struct {
		int scancode;
		const char *name;
	} names[] = {
		{SDL_SCANCODE_LEFTBRACKET, "Left Bracket"},
		{SDL_SCANCODE_RIGHTBRACKET, "Right Bracket"},
		{SDL_SCANCODE_BACKSLASH, "Backslash"},
		{SDL_SCANCODE_SEMICOLON, "Semicolon"},
		{SDL_SCANCODE_APOSTROPHE, "Apostrophe"},
		{SDL_SCANCODE_COMMA, "Comma"},
		{SDL_SCANCODE_PERIOD, "Period"},
		{SDL_SCANCODE_MINUS, "Minus"},
		{SDL_SCANCODE_EQUALS, "Equals"},
		{SDL_SCANCODE_SLASH, "Slash"},
		{SDL_SCANCODE_GRAVE, "Grave"},
		{SDL_SCANCODE_NONUSBACKSLASH, "Non-US Backslash"},
		{SDL_SCANCODE_KP_DIVIDE, "Keypad Divide"},
		{SDL_SCANCODE_KP_MULTIPLY, "Keypad Multiply"},
		{SDL_SCANCODE_KP_MINUS, "Keypad Minus"},
		{SDL_SCANCODE_KP_PLUS, "Keypad Plus"},
		{SDL_SCANCODE_KP_PERIOD, "Keypad Period"},
		{SDL_SCANCODE_KP_EQUALS, "Keypad Equals"},
	};

	for (size_t i = 0; i < SDL_arraysize(names); i++) {
		if (names[i].scancode == scancode) {
			return names[i].name;
		}
	}

	return NULL;
}

static bool scancode_name(int scancode, char *out, size_t out_size) {
	if (scancode <= SDL_SCANCODE_UNKNOWN || scancode >= SDL_SCANCODE_COUNT) {
		return false;
	}

	const char *name = symbol_scancode_name(scancode);
	if (write_name(out, out_size, name)) {
		return true;
	}

	name = SDL_GetScancodeName((SDL_Scancode)scancode);
	if (write_name(out, out_size, name)) {
		return true;
	}

	const int written = SDL_snprintf(out, out_size, "Scancode %d", scancode);
	return written >= 0 && (size_t)written < out_size;
}

bool DTTR_InputNames_ControlCode(int code, char *out, size_t out_size) {
	if (!out || out_size == 0) {
		return false;
	}

	if (code == DTTR_CONFIG_CONTROL_CODE_KEYPAD_ENTER) {
		return write_name(out, out_size, "Keypad Enter");
	}

	if (code >= DTTR_CONFIG_CONTROL_CODE_SDL_GAMEPAD_BUTTON_BASE
		&& code < DTTR_CONFIG_CONTROL_CODE_SDL_GAMEPAD_BUTTON_BASE
					  + DTTR_CONFIG_CONTROL_CODE_SDL_GAMEPAD_BUTTON_COUNT) {
		return DTTR_InputNames_GamepadButton(
			code - DTTR_CONFIG_CONTROL_CODE_SDL_GAMEPAD_BUTTON_BASE,
			out,
			out_size
		);
	}

	if (code >= DTTR_CONFIG_CONTROL_CODE_NATIVE_GAMEPAD_BASE
		&& code < DTTR_CONFIG_CONTROL_CODE_NATIVE_GAMEPAD_BASE
					  + DTTR_CONFIG_CONTROL_CODE_NATIVE_GAMEPAD_COUNT) {
		const int written = SDL_snprintf(
			out,
			out_size,
			"Native Button %d",
			code - DTTR_CONFIG_CONTROL_CODE_NATIVE_GAMEPAD_BASE
		);
		return written >= 0 && (size_t)written < out_size;
	}

	if (code >= DTTR_CONFIG_CONTROL_CODE_SCANCODE_BASE) {
		return scancode_name(code - DTTR_CONFIG_CONTROL_CODE_SCANCODE_BASE, out, out_size);
	}

	return false;
}
