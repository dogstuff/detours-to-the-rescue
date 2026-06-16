#include <dttr_input_binding.h>

#include <SDL3/SDL.h>

/// Compacts SDL's display names into a single token.
static bool dttr_input_binding_normalize(const char *name, char *out, size_t out_size) {
	if (!name || !out || out_size == 0) {
		return false;
	}

	size_t i = 0;
	for (; name[i] != '\0'; i++) {
		if (i + 1 >= out_size) {
			return false;
		}

		const char c = name[i];
		out[i] = (c >= 'A' && c <= 'Z') ? (char)(c + ('a' - 'A')) : (c == ' ') ? '_' : c;
	}

	out[i] = '\0';
	return i > 0;
}

static bool dttr_input_binding_match_prefix(
	const char *token,
	const char *prefix,
	const char **rest
) {
	const size_t len = SDL_strlen(prefix);
	if (SDL_strncmp(token, prefix, len) != 0) {
		return false;
	}

	*rest = token + len;
	return true;
}

typedef struct {
	int button;
	const char *name;
	const char *label;
} dttr_input_binding_mouse_entry;

static const dttr_input_binding_mouse_entry dttr_input_binding_mouse_names[] = {
	{SDL_BUTTON_LEFT, "left", "Left Mouse"},
	{SDL_BUTTON_MIDDLE, "middle", "Middle Mouse"},
	{SDL_BUTTON_RIGHT, "right", "Right Mouse"},
	{SDL_BUTTON_X1, "x1", "Mouse X1"},
	{SDL_BUTTON_X2, "x2", "Mouse X2"},
};

static const dttr_input_binding_mouse_entry *dttr_input_binding_mouse_find(
	int button,
	const char *name
) {
	for (size_t i = 0; i < SDL_arraysize(dttr_input_binding_mouse_names); i++) {
		const dttr_input_binding_mouse_entry *entry = &dttr_input_binding_mouse_names[i];
		if ((name && SDL_strcmp(entry->name, name) == 0)
			|| (!name && entry->button == button)) {
			return entry;
		}
	}

	return NULL;
}

static DTTR_Result dttr_input_binding_result(DTTR_Status status, const char *message) {
	return (DTTR_Result){
		.status = status,
		.message = message,
	};
}

static DTTR_Result dttr_input_binding_copy_out(
	char *out,
	size_t out_size,
	const char *value
) {
	if (!out || out_size == 0) {
		return dttr_input_binding_result(
			DTTR_ERR_INVALID_ARGUMENT,
			"Output buffer is required."
		);
	}

	if (!value) {
		return dttr_input_binding_result(
			DTTR_ERR_INVALID_ARGUMENT,
			"Output value is required."
		);
	}

	if (SDL_strlcpy(out, value, out_size) >= out_size) {
		return dttr_input_binding_result(
			DTTR_ERR_INVALID_ARGUMENT,
			"Output buffer is too small."
		);
	}

	return dttr_input_binding_result(DTTR_OK, NULL);
}

static DTTR_Result dttr_input_binding_emit_token(
	char *out,
	size_t out_size,
	const char *prefix,
	const char *name
) {
	if (!out || out_size == 0) {
		return dttr_input_binding_result(
			DTTR_ERR_INVALID_ARGUMENT,
			"Output buffer is required."
		);
	}

	const int written = SDL_snprintf(out, out_size, "%s%s", prefix, name);
	if (written <= 0 || (size_t)written >= out_size) {
		return dttr_input_binding_result(
			DTTR_ERR_UNSUPPORTED_CONTRACT,
			"Input binding token is too long."
		);
	}

	return dttr_input_binding_result(DTTR_OK, NULL);
}

static DTTR_Result dttr_input_binding_format_named(
	char *out,
	size_t out_size,
	const char *prefix,
	const char *name,
	const char *error_message
) {
	char normalized[64];
	if (!name || !name[0]
		|| !dttr_input_binding_normalize(name, normalized, sizeof(normalized))) {
		return dttr_input_binding_result(DTTR_ERR_UNSUPPORTED_CONTRACT, error_message);
	}

	return dttr_input_binding_emit_token(out, out_size, prefix, normalized);
}

DTTR_Result DTTR_InputBinding_Format(
	const DTTR_Mods_ConfigInputBinding *binding,
	char *out,
	size_t out_size
) {
	if (!binding) {
		return dttr_input_binding_result(
			DTTR_ERR_INVALID_ARGUMENT,
			"Input binding is required."
		);
	}

	switch (binding->device) {
	case DTTR_MODS_BINDING_KEYBOARD:
		return dttr_input_binding_format_named(
			out,
			out_size,
			DTTR_INPUT_BINDING_KEY_PREFIX,
			SDL_GetScancodeName((SDL_Scancode)binding->code),
			"Keyboard binding code is unsupported."
		);
	case DTTR_MODS_BINDING_MOUSE: {
		const dttr_input_binding_mouse_entry *mouse = dttr_input_binding_mouse_find(
			binding->code,
			NULL
		);
		if (!mouse) {
			return dttr_input_binding_result(
				DTTR_ERR_UNSUPPORTED_CONTRACT,
				"Mouse binding code is unsupported."
			);
		}

		return dttr_input_binding_emit_token(
			out,
			out_size,
			DTTR_INPUT_BINDING_MOUSE_PREFIX,
			mouse->name
		);
	}
	case DTTR_MODS_BINDING_GAMEPAD:
		return dttr_input_binding_format_named(
			out,
			out_size,
			DTTR_INPUT_BINDING_GAMEPAD_PREFIX,
			SDL_GetGamepadStringForButton((SDL_GamepadButton)binding->code),
			"Gamepad binding code is unsupported."
		);
	case DTTR_MODS_BINDING_NONE:
		return dttr_input_binding_copy_out(out, out_size, "");
	default:
		return dttr_input_binding_result(
			DTTR_ERR_UNSUPPORTED_CONTRACT,
			"Input binding device is unsupported."
		);
	}
}

DTTR_Result DTTR_InputBinding_Parse(const char *token, DTTR_Mods_ConfigInputBinding *out) {
	if (!out) {
		return dttr_input_binding_result(
			DTTR_ERR_INVALID_ARGUMENT,
			"Input binding output is required."
		);
	}

	DTTR_Mods_ConfigInputBinding parsed = {
		.struct_size = sizeof(*out),
		.device = DTTR_MODS_BINDING_NONE,
		.code = 0,
	};

	if (!token || !token[0]) {
		goto done;
	}

	const char *name = NULL;

	if (dttr_input_binding_match_prefix(token, DTTR_INPUT_BINDING_KEY_PREFIX, &name)) {
		for (int sc = 1; sc < SDL_SCANCODE_COUNT; sc++) {
			const char *candidate = SDL_GetScancodeName((SDL_Scancode)sc);
			char normalized[64];
			if (candidate && candidate[0]
				&& dttr_input_binding_normalize(candidate, normalized, sizeof(normalized))
				&& SDL_strcmp(normalized, name) == 0) {
				parsed.device = DTTR_MODS_BINDING_KEYBOARD;
				parsed.code = sc;
				break;
			}
		}

		goto done;
	}

	if (dttr_input_binding_match_prefix(token, DTTR_INPUT_BINDING_MOUSE_PREFIX, &name)) {
		const dttr_input_binding_mouse_entry *mouse = dttr_input_binding_mouse_find(
			0,
			name
		);
		if (mouse) {
			parsed.device = DTTR_MODS_BINDING_MOUSE;
			parsed.code = mouse->button;
		}

		goto done;
	}

	if (dttr_input_binding_match_prefix(token, DTTR_INPUT_BINDING_GAMEPAD_PREFIX, &name)) {
		const SDL_GamepadButton button = SDL_GetGamepadButtonFromString(name);
		if (button != SDL_GAMEPAD_BUTTON_INVALID) {
			parsed.device = DTTR_MODS_BINDING_GAMEPAD;
			parsed.code = (int)button;
		}

		goto done;
	}

done:
	*out = parsed;
	return dttr_input_binding_result(DTTR_OK, NULL);
}

DTTR_Result DTTR_InputBinding_DisplayName(
	const DTTR_Mods_ConfigInputBinding *binding,
	char *out,
	size_t out_size
) {
	if (!out || out_size == 0) {
		return dttr_input_binding_result(
			DTTR_ERR_INVALID_ARGUMENT,
			"Output buffer is required."
		);
	}

	if (!binding || binding->device == DTTR_MODS_BINDING_NONE) {
		return dttr_input_binding_copy_out(out, out_size, "Unbound");
	}

	switch (binding->device) {
	case DTTR_MODS_BINDING_KEYBOARD: {
		const char *name = SDL_GetScancodeName((SDL_Scancode)binding->code);
		return dttr_input_binding_copy_out(
			out,
			out_size,
			(name && name[0]) ? name : "Unknown key"
		);
	}
	case DTTR_MODS_BINDING_MOUSE: {
		const dttr_input_binding_mouse_entry *mouse = dttr_input_binding_mouse_find(
			binding->code,
			NULL
		);

		return dttr_input_binding_copy_out(out, out_size, mouse ? mouse->label : "Mouse");
	}
	case DTTR_MODS_BINDING_GAMEPAD: {
		const char *name = SDL_GetGamepadStringForButton((SDL_GamepadButton)binding->code);

		return dttr_input_binding_copy_out(
			out,
			out_size,
			(name && name[0]) ? name : "Unknown button"
		);
	}
	default:
		return dttr_input_binding_copy_out(out, out_size, "Unbound");
	}
}
