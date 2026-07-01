#include "gui_internal.h"

void set_status(config_ui_state *state, const char *status) {
	snprintf(state->status, sizeof(state->status), "%s", status ? status : "");
	state->status_expires_at_ms = SDL_GetTicks() + DTTR_CONFIG_UI_STATUS_TIMEOUT_MS;
}

void set_mods_dir_from_config_path(config_ui_state *state) {
	if (!state) {
		return;
	}

	sds mods_dir = DTTR_Path_ModuleDir(NULL);

	if (!mods_dir
		|| !DTTR_Path_AppendSegment(&mods_dir, "mods", DTTR_PATH_NATIVE_SEPARATOR)
		|| !DTTR_Path_CopySds(state->mods_dir, sizeof(state->mods_dir), mods_dir)) {
		state->mods_dir[0] = '\0';
	}

	sdsfree(mods_dir);
}

bool event_cancels_binding(const SDL_Event *event) {
	return event && event->type == SDL_EVENT_KEY_DOWN
		   && event->key.scancode == SDL_SCANCODE_ESCAPE;
}

void begin_input_binding_capture(
	config_ui_state *state,
	const char *mod_id,
	const char *field_id
) {
	if (!state || !mod_id || !field_id) {
		return;
	}

	state->input_binding_capturing = true;
	state->control_binding_action = -1;
	snprintf(
		state->input_binding_mod_id,
		sizeof(state->input_binding_mod_id),
		"%s",
		mod_id
	);
	snprintf(
		state->input_binding_field_id,
		sizeof(state->input_binding_field_id),
		"%s",
		field_id
	);
	set_status(
		state,
		"Press a key, mouse button, or gamepad button. Press Esc to cancel."
	);
}

static void clear_input_binding_capture_state(config_ui_state *state) {
	state->input_binding_capturing = false;
	state->input_binding_mod_id[0] = '\0';
	state->input_binding_field_id[0] = '\0';
	state->control_binding_action = -1;
}

void begin_control_binding_capture(config_ui_state *state, int action) {
	if (!state || action < 0 || action >= DTTR_CONFIG_CONTROL_ACTION_COUNT) {
		return;
	}

	state->input_binding_capturing = true;
	state->input_binding_mod_id[0] = '\0';
	state->input_binding_field_id[0] = '\0';
	state->control_binding_action = action;
	set_status(state, "Press a key or gamepad button. Press Esc to cancel.");
}

void cancel_input_binding_capture(config_ui_state *state) {
	if (!state || !state->input_binding_capturing) {
		return;
	}

	clear_input_binding_capture_state(state);
	set_status(state, "Cancelled input binding capture.");
}

bool input_binding_field_capturing(
	const config_ui_state *state,
	const char *mod_id,
	const char *field_id
) {
	return state && state->input_binding_capturing && state->control_binding_action < 0
		   && mod_id && field_id && SDL_strcmp(state->input_binding_mod_id, mod_id) == 0
		   && SDL_strcmp(state->input_binding_field_id, field_id) == 0;
}

bool control_binding_field_capturing(const config_ui_state *state, int action) {
	return state && state->input_binding_capturing
		   && state->control_binding_action == action;
}

static bool input_binding_from_event(
	const SDL_Event *event,
	DTTR_Mods_ConfigInputBinding *out
) {
	out->struct_size = sizeof(*out);
	out->code = 0;

	switch (event->type) {
	case SDL_EVENT_KEY_DOWN:
		if (event->key.repeat) {
			return false;
		}

		out->device = DTTR_MODS_BINDING_KEYBOARD;
		out->code = (int)event->key.scancode;
		return true;
	case SDL_EVENT_MOUSE_BUTTON_DOWN:
		out->device = DTTR_MODS_BINDING_MOUSE;
		out->code = event->button.button;
		return true;
	case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
		out->device = DTTR_MODS_BINDING_GAMEPAD;
		out->code = event->gbutton.button;
		return true;
	default:
		return false;
	}
}

static int control_binding_button_code(int button) {
	return button >= 0 && button < DTTR_CONFIG_CONTROL_CODE_SDL_GAMEPAD_BUTTON_COUNT
			   ? DTTR_CONFIG_CONTROL_CODE_SDL_GAMEPAD_BUTTON_BASE + button
			   : DTTR_CONFIG_CONTROL_BINDING_NONE;
}

static int control_binding_code_from_event(const SDL_Event *event) {
	if (!event) {
		return DTTR_CONFIG_CONTROL_BINDING_NONE;
	}

	switch (event->type) {
	case SDL_EVENT_KEY_DOWN:
		if (event->key.repeat || event->key.scancode <= SDL_SCANCODE_UNKNOWN
			|| event->key.scancode >= SDL_SCANCODE_COUNT) {
			return DTTR_CONFIG_CONTROL_BINDING_NONE;
		}

		return DTTR_CONFIG_CONTROL_CODE_SCANCODE_BASE + (int)event->key.scancode;
	case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
		return control_binding_button_code(event->gbutton.button);
	case SDL_EVENT_JOYSTICK_BUTTON_DOWN:
		return control_binding_button_code(event->jbutton.button);
	default:
		return DTTR_CONFIG_CONTROL_BINDING_NONE;
	}
}

static bool capture_control_binding_event(config_ui_state *state, const SDL_Event *event) {
	if (!state || state->control_binding_action < 0) {
		return false;
	}

	const int code = control_binding_code_from_event(event);
	if (code == DTTR_CONFIG_CONTROL_BINDING_NONE) {
		return false;
	}

	state->config.control_bindings[state->control_binding_action] = code;
	clear_input_binding_capture_state(state);
	set_status(state, "Captured control binding.");
	return true;
}

bool capture_input_binding_event(config_ui_state *state, const SDL_Event *event) {
	if (!state || !state->input_binding_capturing || !event) {
		return false;
	}

	if (capture_control_binding_event(state, event)) {
		return true;
	}

	if (state->control_binding_action >= 0) {
		return false;
	}

	DTTR_Mods_ConfigInputBinding binding = {0};
	if (!input_binding_from_event(event, &binding)) {
		return false;
	}

	char token[DTTR_CONFIG_MOD_STRING_MAX] = {0};
	DTTR_Result result = DTTR_InputBinding_Format(&binding, token, sizeof(token));
	if (result.status == DTTR_OK) {
		result = DTTR_Config_SetModString(
			&state->config,
			state->input_binding_mod_id,
			state->input_binding_field_id,
			token
		);
	}

	if (result.status != DTTR_OK) {
		set_status(state, "Could not capture input binding.");
	} else {
		set_status(state, "Captured input binding.");
	}

	clear_input_binding_capture_state(state);
	return true;
}

void load_config(config_ui_state *state) {
	if (!DTTR_Config_Load(state->path)) {
		const char *details = DTTR_Config_LastError();
		if (details) {
			char status[sizeof(state->status)];
			snprintf(status, sizeof(status), "Failed to load config: %s", details);
			set_status(state, status);
		} else {
			set_status(state, "Failed to load config.");
		}

		return;
	}

	state->config = dttr_config;
	state->saved_config = state->config;
	reload_mod_config_specs(state);
	set_status(state, "Loaded config.");
}

void save_config(config_ui_state *state) {
	if (!DTTR_Config_Save(state->path, &state->config)) {
		set_status(state, "Failed to save config.");
		return;
	}

	state->saved_config = state->config;
	set_status(state, "Saved config.");
}

void reset_defaults(config_ui_state *state) {
	state->config = state->defaults;
	set_status(state, "Reset to built-in defaults. Save to write changes.");
}

void request_reset_defaults(const DTTR_ImGuiDialogContext *ctx, config_ui_state *state) {
	const SDL_MessageBoxButtonData buttons[] = {
		{SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 1, "Reset"},
		{SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 0, "Cancel"},
	};

	const SDL_MessageBoxData message_box = {
		SDL_MESSAGEBOX_WARNING,
		ctx ? ctx->window : NULL,
		"Reset to defaults?",
		"This will reset every configuration value to its default. Continue?",
		(int)SDL_arraysize(buttons),
		buttons,
		NULL,
	};

	int button_id = 0;

	if (DTTR_SDL_ShowMessageBox(&message_box, &button_id) && button_id == 1) {
		reset_defaults(state);
	}
}

config_label_state make_config_label_state(bool unsaved_changed, bool default_changed) {
	if (unsaved_changed) {
		return CONFIG_LABEL_UNSAVED;
	}

	return default_changed ? CONFIG_LABEL_SAVED_CHANGED : CONFIG_LABEL_DEFAULT;
}

bool config_has_unsaved_changes(const config_ui_state *state) {
	return DTTR_Config_SchemaChanged(&state->config, &state->saved_config)
		   || DTTR_Config_ControlBindingsChanged(&state->config, &state->saved_config)
		   || DTTR_Config_DisabledModsChanged(&state->config, &state->saved_config)
		   || DTTR_Config_ModConfigsChanged(&state->config, &state->saved_config);
}
