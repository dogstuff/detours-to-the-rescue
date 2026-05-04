#include "gui_internal.h"

static const char *const S_GAME_ACTION_TOOLTIPS[] = {
	"Disables this gamepad action.",
	"Moves up.",
	"Moves down.",
	"Moves left.",
	"Moves right.",
	"Activates POV up.",
	"Activates POV down.",
	"Acts as joy_1. In menus, joy_1 is the confirm button.",
	"Acts as joy_2. In menus, joy_2 is the back button.",
	"Acts as joy_3.",
	"Acts as joy_4.",
	"Acts as joy_5.",
	"Acts as joy_6.",
	"Acts as joy_7.",
	"Acts as joy_8.",
	"Acts as joy_9. joy_9 is the start/pause button.",
	"Acts as joy_10.",
	"Acts as joy_11.",
	"Acts as joy_12.",
	"Acts as joy_13.",
};

const char *s_game_action_tooltip(int action) {
	const int index = action - DTTR_GAMEPAD_MAPPING_NONE;
	if (index >= 0 && index < (int)SDL_arraysize(S_GAME_ACTION_TOOLTIPS)) {
		return S_GAME_ACTION_TOOLTIPS[index];
	}

	return "Bind each PCDogs game action to the controller input you want to press.";
}

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

static bool s_button_action_in_range(int action) {
	return action >= PCDOGS_GAMEPAD_IDX_UP && action <= PCDOGS_GAMEPAD_IDX_BTN_12;
}

static int s_source_for_action(const int *map, int action) {
	if (!map || !s_button_action_in_range(action)) {
		return DTTR_GAMEPAD_MAPPING_NONE;
	}

	for (int source = 0; source < DTTR_GAMEPAD_SOURCE_COUNT; source++) {
		if (map[source] == action) {
			return source;
		}
	}

	return DTTR_GAMEPAD_MAPPING_NONE;
}

int s_gamepad_button_row_count(void) {
	const int choice_count = dttr_config_choice_count(DTTR_CONFIG_CHOICES_GAME_ACTION);
	return choice_count > 0 ? choice_count - 1 : 0;
}

static const DTTR_ConfigChoice *s_gamepad_button_row_choice(int row) {
	if (row < 0 || row >= s_gamepad_button_row_count()) {
		return NULL;
	}

	return dttr_config_choice_get(DTTR_CONFIG_CHOICES_GAME_ACTION, row + 1);
}

int s_gamepad_button_row_action(int row) {
	const DTTR_ConfigChoice *choice = s_gamepad_button_row_choice(row);
	return choice ? choice->value : DTTR_GAMEPAD_MAPPING_NONE;
}

const char *s_gamepad_button_row_label(int row) {
	const DTTR_ConfigChoice *choice = s_gamepad_button_row_choice(row);
	return choice ? choice->label : "unknown";
}

int s_gamepad_default_source_for_action(const S_ConfigUIState *state, int action) {
	if (!state) {
		return DTTR_GAMEPAD_MAPPING_NONE;
	}

	return s_source_for_action(state->m_defaults.m_gamepad_button_map, action);
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
	const int row_count = s_gamepad_button_row_count();
	for (int i = 0; i < DTTR_GAMEPAD_SOURCE_COUNT; i++) {
		state->m_button_actions[i] = DTTR_GAMEPAD_MAPPING_NONE;
		state->m_button_sources[i] = DTTR_GAMEPAD_MAPPING_NONE;
	}

	for (int row = 0; row < row_count; row++) {
		const int action = s_gamepad_button_row_action(row);
		state->m_button_actions[row] = action;
		state->m_button_sources[row] = s_source_for_action(
			state->m_config.m_gamepad_button_map,
			action
		);
	}
}

bool s_event_cancels_binding(const SDL_Event *event) {
	return event && event->type == SDL_EVENT_KEY_DOWN
		   && event->key.scancode == SDL_SCANCODE_ESCAPE;
}

void s_cancel_binding(S_ConfigUIState *state) {
	if (state->m_binding_row < 0) {
		return;
	}

	state->m_binding_row = -1;
	s_set_status(state, "Cancelled controller input capture.");
}

void s_capture_source(S_ConfigUIState *state, int new_source) {
	const int binding_row = state->m_binding_row;
	const int row_count = s_gamepad_button_row_count();
	if (binding_row < 0 || binding_row >= row_count
		|| !s_button_source_in_range(new_source)) {
		return;
	}

	const int old_source = state->m_button_sources[binding_row];
	for (int row = 0; row < row_count; row++) {
		if (row != binding_row && state->m_button_sources[row] == new_source) {
			state->m_button_sources[row] = old_source;
			break;
		}
	}

	state->m_button_sources[binding_row] = new_source;
	state->m_binding_row = -1;
	s_set_status(state, "Captured controller input.");
}

void s_sync_config_from_rows(S_ConfigUIState *state) {
	dttr_config_clear_gamepad_button_map(state->m_config.m_gamepad_button_map);
	const int row_count = s_gamepad_button_row_count();
	for (int row = 0; row < row_count; row++) {
		const int source = state->m_button_sources[row];
		if (s_button_source_in_range(source)) {
			state->m_config.m_gamepad_button_map[source] = state->m_button_actions[row];
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
	const int row_count = s_gamepad_button_row_count();
	for (int row = 0; row < row_count; row++) {
		const int action = state->m_button_actions[row];
		if (state->m_button_sources[row]
			!= s_source_for_action(state->m_saved_config.m_gamepad_button_map, action)) {
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
	return s_config_label_state(
		source != s_source_for_action(state->m_saved_config.m_gamepad_button_map, action),
		source != s_source_for_action(state->m_defaults.m_gamepad_button_map, action)
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
