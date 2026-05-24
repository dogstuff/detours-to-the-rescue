#include "gui_internal.h"

static const char *const GAME_ACTION_TOOLTIPS[] = {
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

const char *game_action_tooltip(int action) {
	const int index = action - DTTR_GAMEPAD_MAPPING_NONE;

	if (index >= 0 && index < (int)SDL_arraysize(GAME_ACTION_TOOLTIPS)) {
		return GAME_ACTION_TOOLTIPS[index];
	}

	return "Bind each PCDOGS game action to the controller input you want to press.";
}

static void clear_mods_dir(config_ui_state *state) { state->mods_dir[0] = '\0'; }

static bool button_source_in_range(int source) {
	return source >= 0 && source < DTTR_GAMEPAD_SOURCE_COUNT;
}

static void refresh_button_rows(config_ui_state *state) {
	sync_rows_from_config(state);
	state->binding_row = -1;
}

static bool button_action_in_range(int action) {
	return action >= PCDOGS_GAMEPAD_IDX_UP && action <= PCDOGS_GAMEPAD_IDX_BTN_12;
}

static int source_for_action(const int *map, int action) {
	if (!map || !button_action_in_range(action)) {
		return DTTR_GAMEPAD_MAPPING_NONE;
	}

	for (int source = 0; source < DTTR_GAMEPAD_SOURCE_COUNT; source++) {
		if (map[source] == action) {
			return source;
		}
	}

	return DTTR_GAMEPAD_MAPPING_NONE;
}

int gamepad_button_row_count() {
	const int choice_count = DTTR_Config_ChoiceCount(DTTR_CONFIG_CHOICES_GAME_ACTION);
	return choice_count > 0 ? choice_count - 1 : 0;
}

static const DTTR_ConfigChoice *gamepad_button_row_choice(int row) {
	if (row < 0 || row >= gamepad_button_row_count()) {
		return NULL;
	}

	return DTTR_Config_ChoiceGet(DTTR_CONFIG_CHOICES_GAME_ACTION, row + 1);
}

int gamepad_button_row_action(int row) {
	const DTTR_ConfigChoice *choice = gamepad_button_row_choice(row);
	return choice ? choice->value : DTTR_GAMEPAD_MAPPING_NONE;
}

const char *gamepad_button_row_label(int row) {
	const DTTR_ConfigChoice *choice = gamepad_button_row_choice(row);
	return choice ? choice->label : "unknown";
}

int gamepad_default_source_for_action(const config_ui_state *state, int action) {
	if (!state) {
		return DTTR_GAMEPAD_MAPPING_NONE;
	}

	return source_for_action(state->defaults.gamepad_button_map, action);
}

void set_status(config_ui_state *state, const char *status) {
	snprintf(state->status, sizeof(state->status), "%s", status ? status : "");
	state->status_expires_at_ms = SDL_GetTicks() + DTTR_CONFIG_UI_STATUS_TIMEOUT_MS;
}

void set_mods_dir_from_config_path(config_ui_state *state) {
	if (!state) {
		return;
	}

	char exe_path[MAX_PATH];
	const DWORD len = GetModuleFileNameA(NULL, exe_path, (DWORD)sizeof(exe_path));

	if (len == 0 || len >= sizeof(exe_path)) {
		clear_mods_dir(state);
		return;
	}

	char *last_sep = strrchr(exe_path, '\\');

	if (!last_sep) {
		clear_mods_dir(state);
		return;
	}

	last_sep[1] = '\0';
	const int written = snprintf(
		state->mods_dir,
		sizeof(state->mods_dir),
		"%smods",
		exe_path
	);

	if (written <= 0 || (size_t)written >= sizeof(state->mods_dir)) {
		clear_mods_dir(state);
	}
}

void sync_rows_from_config(config_ui_state *state) {
	const int row_count = gamepad_button_row_count();

	for (int i = 0; i < DTTR_GAMEPAD_SOURCE_COUNT; i++) {
		state->button_actions[i] = DTTR_GAMEPAD_MAPPING_NONE;
		state->button_sources[i] = DTTR_GAMEPAD_MAPPING_NONE;
	}

	for (int row = 0; row < row_count; row++) {
		const int action = gamepad_button_row_action(row);
		state->button_actions[row] = action;
		state->button_sources[row] = source_for_action(
			state->config.gamepad_button_map,
			action
		);
	}
}

bool event_cancels_binding(const SDL_Event *event) {
	return event && event->type == SDL_EVENT_KEY_DOWN
		   && event->key.scancode == SDL_SCANCODE_ESCAPE;
}

void cancel_binding(config_ui_state *state) {
	if (state->binding_row < 0) {
		return;
	}

	state->binding_row = -1;
	set_status(state, "Cancelled controller input capture.");
}

void capture_source(config_ui_state *state, int new_source) {
	const int binding_row = state->binding_row;
	const int row_count = gamepad_button_row_count();

	if (binding_row < 0 || binding_row >= row_count
		|| !button_source_in_range(new_source)) {
		return;
	}

	const int old_source = state->button_sources[binding_row];

	for (int row = 0; row < row_count; row++) {
		if (row != binding_row && state->button_sources[row] == new_source) {
			state->button_sources[row] = old_source;
			break;
		}
	}

	state->button_sources[binding_row] = new_source;
	state->binding_row = -1;
	set_status(state, "Captured controller input.");
}

void sync_config_from_rows(config_ui_state *state) {
	DTTR_Config_ClearGamepadButtonMap(state->config.gamepad_button_map);
	const int row_count = gamepad_button_row_count();

	for (int row = 0; row < row_count; row++) {
		const int source = state->button_sources[row];

		if (button_source_in_range(source)) {
			state->config.gamepad_button_map[source] = state->button_actions[row];
		}
	}
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
	refresh_button_rows(state);
	set_status(state, "Loaded config.");
}

void save_config(config_ui_state *state) {
	sync_config_from_rows(state);

	if (!DTTR_Config_Save(state->path, &state->config)) {
		set_status(state, "Failed to save config.");
		return;
	}

	state->saved_config = state->config;
	set_status(state, "Saved config.");
}

void reset_defaults(config_ui_state *state) {
	DTTR_Config_SetDefaults(&state->config);
	refresh_button_rows(state);
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

bool gamepad_button_rows_have_unsaved_changes(const config_ui_state *state) {
	const int row_count = gamepad_button_row_count();

	for (int row = 0; row < row_count; row++) {
		const int action = state->button_actions[row];

		if (state->button_sources[row]
			!= source_for_action(state->saved_config.gamepad_button_map, action)) {
			return true;
		}
	}

	return false;
}

config_label_state gamepad_button_label_state(
	const config_ui_state *state,
	int source,
	int action
) {
	const int saved_source = source_for_action(
		state->saved_config.gamepad_button_map,
		action
	);
	const int default_source = source_for_action(
		state->defaults.gamepad_button_map,
		action
	);

	return make_config_label_state(source != saved_source, source != default_source);
}

bool config_has_unsaved_changes(const config_ui_state *state) {
	return DTTR_Config_SchemaChanged(&state->config, &state->saved_config)
		   || DTTR_Config_DisabledModsChanged(&state->config, &state->saved_config)
		   || gamepad_button_rows_have_unsaved_changes(state);
}
