#include "gui_internal.h"

static void s_clear_components_dir(S_ConfigUIState *state) {
	state->m_components_dir[0] = '\0';
}

static bool s_button_source_in_range(int source) {
	return source >= 0 && source < DTTR_GAMEPAD_SOURCE_COUNT;
}

static void s_refresh_button_rows(S_ConfigUIState *state) {
	s_sync_rows_from_config(state);
	state->m_binding_row = -1;
}

void s_set_status(S_ConfigUIState *state, const char *status) {
	snprintf(state->m_status, sizeof(state->m_status), "%s", status ? status : "");
	state->m_status_expires_at_ms = SDL_GetTicks() + DTTR_CONFIG_UI_STATUS_TIMEOUT_MS;
}

void s_set_components_dir_from_config_path(S_ConfigUIState *state) {
	if (!state) {
		return;
	}

	char exe_path[MAX_PATH];
	const DWORD len = GetModuleFileNameA(NULL, exe_path, (DWORD)sizeof(exe_path));
	if (len == 0 || len >= sizeof(exe_path)) {
		s_clear_components_dir(state);
		return;
	}

	char *last_sep = strrchr(exe_path, '\\');
	if (!last_sep) {
		s_clear_components_dir(state);
		return;
	}

	last_sep[1] = '\0';
	const int written = snprintf(
		state->m_components_dir,
		sizeof(state->m_components_dir),
		"%scomponents",
		exe_path
	);
	if (written <= 0 || (size_t)written >= sizeof(state->m_components_dir)) {
		s_clear_components_dir(state);
	}
}

void s_sync_rows_from_config(S_ConfigUIState *state) {
	for (int i = 0; i < DTTR_GAMEPAD_SOURCE_COUNT; i++) {
		state->m_button_sources[i] = i;
		state->m_button_actions[i] = state->m_config.m_gamepad_button_map[i];
	}
}

void s_sync_config_from_rows(S_ConfigUIState *state) {
	dttr_config_clear_gamepad_button_map(state->m_config.m_gamepad_button_map);
	for (int i = 0; i < DTTR_GAMEPAD_SOURCE_COUNT; i++) {
		const int source = state->m_button_sources[i];
		if (s_button_source_in_range(source)) {
			state->m_config.m_gamepad_button_map[source] = state->m_button_actions[i];
		}
	}
}

void s_load_config(S_ConfigUIState *state) {
	if (!dttr_config_load(state->m_path)) {
		s_set_status(state, "Failed to load config.");
		return;
	}

	state->m_config = g_dttr_config;
	state->m_saved_config = state->m_config;
	s_refresh_button_rows(state);
	s_set_status(state, "Loaded config.");
}

void s_save_config(S_ConfigUIState *state) {
	s_sync_config_from_rows(state);
	if (!dttr_config_save(state->m_path, &state->m_config)) {
		s_set_status(state, "Failed to save config.");
		return;
	}

	state->m_saved_config = state->m_config;
	s_set_status(state, "Saved config.");
}

void s_reset_defaults(S_ConfigUIState *state) {
	dttr_config_set_defaults(&state->m_config);
	s_refresh_button_rows(state);
	s_set_status(state, "Reset to built-in defaults. Save to write changes.");
}

void s_request_reset_defaults(const DTTR_ImGuiDialogContext *ctx, S_ConfigUIState *state) {
	const SDL_MessageBoxButtonData buttons[] = {
		{SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 1, "Reset"},
		{SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 0, "Cancel"},
	};

	const SDL_MessageBoxData message_box = {
		SDL_MESSAGEBOX_WARNING,
		ctx ? ctx->m_window : NULL,
		"Reset to defaults?",
		"This will reset every configuration value to its default. Continue?",
		(int)SDL_arraysize(buttons),
		buttons,
		NULL,
	};

	int button_id = 0;
	if (dttr_sdl_show_message_box(&message_box, &button_id) && button_id == 1) {
		s_reset_defaults(state);
	}
}

S_ConfigLabelState s_config_label_state(bool unsaved_changed, bool default_changed) {
	if (unsaved_changed) {
		return S_CONFIG_LABEL_UNSAVED;
	}

	return default_changed ? S_CONFIG_LABEL_SAVED_CHANGED : S_CONFIG_LABEL_DEFAULT;
}

bool s_gamepad_button_rows_have_unsaved_changes(const S_ConfigUIState *state) {
	for (int i = 0; i < DTTR_GAMEPAD_SOURCE_COUNT; i++) {
		const int source = state->m_button_sources[i];
		if (!s_button_source_in_range(source)) {
			return true;
		}

		if (state->m_button_actions[i]
			!= state->m_saved_config.m_gamepad_button_map[source]) {
			return true;
		}
	}

	return false;
}

S_ConfigLabelState s_gamepad_button_label_state(
	const S_ConfigUIState *state,
	int source,
	int action
) {
	if (!s_button_source_in_range(source)) {
		return S_CONFIG_LABEL_UNSAVED;
	}

	return s_config_label_state(
		action != state->m_saved_config.m_gamepad_button_map[source],
		action != state->m_defaults.m_gamepad_button_map[source]
	);
}

bool s_config_has_unsaved_changes(const S_ConfigUIState *state) {
	return dttr_config_schema_changed(&state->m_config, &state->m_saved_config)
		   || dttr_config_disabled_components_changed(
			   &state->m_config,
			   &state->m_saved_config
		   )
		   || s_gamepad_button_rows_have_unsaved_changes(state);
}
