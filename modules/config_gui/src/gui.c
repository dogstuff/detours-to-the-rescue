#include "gui_internal.h"

#include <stdlib.h>

static const char *const CONFIG_WINDOW_TITLE = "DttR Configuration";
static const char *const CONFIG_DEBUG_SHORTCUTS_ENV = "DTTR_CONFIG_DEBUG_SHORTCUTS";

static sds config_path_from_args(int argc, char **argv) {
	if (argc > 1 && argv[1] && argv[1][0]) {
		return sdsnew(argv[1]);
	}

	return DTTR_Path_ModuleSibling(NULL, DTTR_CONFIG_FILENAME);
}

static bool confirm_discard_changes(
	const DTTR_ImGuiDialogContext *ctx,
	const config_ui_state *state,
	const char *action
) {
	if (!config_has_unsaved_changes(state)) {
		return true;
	}

	char message[256];
	snprintf(
		message,
		sizeof(message),
		"This will discard unsaved configuration changes and %s. Continue?",
		action
	);

	const SDL_MessageBoxButtonData buttons[] = {
		{SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 1, "Discard"},
		{SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 0, "Cancel"},
	};

	const SDL_MessageBoxData message_box = {
		SDL_MESSAGEBOX_WARNING,
		ctx ? ctx->window : NULL,
		"Discard unsaved changes?",
		message,
		(int)SDL_arraysize(buttons),
		buttons,
		NULL,
	};

	int button_id = 0;
	return DTTR_SDL_ShowMessageBox(&message_box, &button_id) && button_id == 1;
}

static void draw_toolbar(const DTTR_ImGuiDialogContext *ctx, config_ui_state *state) {
	if (!igBeginMenuBar()) {
		return;
	}

	if (igMenuItem_Bool("Save", "Ctrl+S", false, true)) {
		save_config(state);
	}

	if (igMenuItem_Bool("Load", "Ctrl+O", false, true)
		&& confirm_discard_changes(ctx, state, "reload the file from disk")) {
		load_config(state);
	}

	if (igMenuItem_Bool("Reset to Defaults", NULL, false, true)) {
		request_reset_defaults(ctx, state);
	}

	igEndMenuBar();
}

static void handle_shortcuts(const DTTR_ImGuiDialogContext *ctx, config_ui_state *state) {
	if (igShortcut_Nil(ImGuiMod_Ctrl | ImGuiKey_S, ImGuiInputFlags_RouteGlobal)) {
		save_config(state);
	}

	if (igShortcut_Nil(ImGuiMod_Ctrl | ImGuiKey_O, ImGuiInputFlags_RouteGlobal)
		&& confirm_discard_changes(ctx, state, "reload the file from disk")) {
		load_config(state);
	}
}

static bool env_flag_enabled(const char *name) {
	const char *value = getenv(name);
	return value && value[0] && strcmp(value, "0") != 0;
}

static void draw_shortcut_debug_row(const char *label, ImGuiKeyChord chord) {
	const ImGuiKey key = (ImGuiKey)(chord & ~ImGuiMod_Mask_);
	const ImGuiKeyRoutingData *route = igGetShortcutRoutingData(chord);
	const ImGuiKeyData *key_data = igGetKeyData_Key(key);

	igTableNextRow(ImGuiTableRowFlags_None, 0.0f);
	igTableNextColumn();
	igTextUnformatted(label, NULL);
	igTableNextColumn();
	igTextUnformatted(igGetKeyChordName(chord), NULL);
	igTableNextColumn();

	if (route) {
		igText("0x%08X / %u", route->RoutingCurr, route->RoutingCurrScore);
	} else {
		igTextDisabled("none");
	}

	igTableNextColumn();
	igText(
		"down=%d pressed=%d chord=%d duration=%.02f",
		igIsKeyDown_ID(key, 0),
		igIsKeyPressed_InputFlags(key, ImGuiInputFlags_None, 0),
		igIsKeyChordPressed_InputFlags(chord, ImGuiInputFlags_None, 0),
		key_data ? key_data->DownDuration : -1.0f
	);
}

static void draw_shortcut_debug_window(const config_ui_state *state) {
	if (!state->show_shortcut_debug) {
		return;
	}

	if (!igBegin(
			"DttR Shortcut Debug",
			NULL,
			ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings
		)) {
		igEnd();
		return;
	}

	if (igBeginTable(
			"##shortcut_debug_routes",
			4,
			ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_BordersOuterH,
			(ImVec2_c){0.0f, 0.0f},
			0.0f
		)) {
		igTableSetupColumn("Action", ImGuiTableColumnFlags_None, 0.0f, 0);
		igTableSetupColumn("Chord", ImGuiTableColumnFlags_None, 0.0f, 0);
		igTableSetupColumn("Route", ImGuiTableColumnFlags_None, 0.0f, 0);
		igTableSetupColumn("Input", ImGuiTableColumnFlags_None, 0.0f, 0);
		igTableHeadersRow();
		draw_shortcut_debug_row("Save", ImGuiMod_Ctrl | ImGuiKey_S);
		draw_shortcut_debug_row("Load", ImGuiMod_Ctrl | ImGuiKey_O);
		igEndTable();
	}

	igEnd();
}

static bool init_state_from_args(config_ui_state *state, int argc, char **argv) {
	sds config_path = config_path_from_args(argc, argv);
	if (!DTTR_Path_CopySds(state->path, sizeof(state->path), config_path)) {
		sdsfree(config_path);
		DTTR_SDL_ShowSimpleMessageBox(
			SDL_MESSAGEBOX_ERROR,
			CONFIG_WINDOW_TITLE,
			"Config path is too long.",
			NULL
		);
		return false;
	}

	sdsfree(config_path);

	state->show_shortcut_debug = env_flag_enabled(CONFIG_DEBUG_SHORTCUTS_ENV);
	set_mods_dir_from_config_path(state);
	DTTR_Config_SetDefaults(&state->defaults);
	state->config = state->defaults;
	state->saved_config = state->config;
	sync_rows_from_config(state);
	load_config(state);
	return true;
}

static void draw_ui(const DTTR_ImGuiDialogContext *ctx, config_ui_state *state) {
	push_config_theme();
	handle_shortcuts(ctx, state);
	draw_toolbar(ctx, state);
	add_scaled_vertical_spacing(ctx, DTTR_CONFIG_UI_HEADER_TOP_SPACING);

	DTTR_ImGuiDialog_DrawHeader(ctx, CONFIG_WINDOW_TITLE, DTTR_VERSION);
	igSeparator();

	const bool panel_open = begin_padded_panel(ctx);
	if (panel_open) {
		const bool content_open = begin_config_content_region(ctx, state);
		if (content_open) {
			draw_tabs(ctx, state);
		}

		end_config_content_region();
		draw_footer_text(ctx, state);
	}

	end_padded_panel();

	draw_shortcut_debug_window(state);
	pop_config_theme();
}

static void process_events(
	const DTTR_ImGuiDialogContext *ctx,
	config_ui_state *state,
	bool *running
) {
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		DTTR_ImGuiDialog_ProcessEvent(ctx, &event, running);
		if (!*running) {
			if (confirm_discard_changes(ctx, state, "close the configuration tool")) {
				break;
			}

			*running = true;
			continue;
		}

		if (event_cancels_binding(&event)) {
			cancel_binding(state);
			continue;
		}

		const int source = source_from_event(&event);
		if (source >= 0) {
			capture_source(state, source);
		}
	}
}

__declspec(dllexport) int dttr_config_main(int argc, char **argv) {
	config_ui_state state = {
		.binding_row = -1,
	};

	if (!init_state_from_args(&state, argc, argv)) {
		return 1;
	}

	DTTR_ImGuiDialogContext ctx;
	if (!DTTR_ImGuiDialog_Begin(
			&ctx,
			CONFIG_WINDOW_TITLE,
			config_window_width(),
			DTTR_CONFIG_UI_WINDOW_H
		)) {
		return 1;
	}

	if (!SDL_InitSubSystem(SDL_INIT_GAMEPAD)) {
		char status[sizeof(state.status)];
		snprintf(
			status,
			sizeof(status),
			"Failed to initialize gamepad support: %s",
			SDL_GetError()
		);
		set_status(&state, status);
	}

	bool running = true;
	while (running) {
		process_events(&ctx, &state, &running);
		DTTR_ImGuiDialog_RefreshScale(&ctx);
		DTTR_ImGuiDialog_NewFrame(&ctx);

		if (DTTR_ImGuiDialog_BeginRoot(
				&ctx,
				CONFIG_WINDOW_TITLE,
				ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoScrollbar
					| ImGuiWindowFlags_NoScrollWithMouse
			)) {
			draw_ui(&ctx, &state);
		}

		DTTR_ImGuiDialog_EndRoot();
		DTTR_ImGuiDialog_Render(&ctx);
	}

	close_gamepad_preview(&state);
	SDL_QuitSubSystem(SDL_INIT_GAMEPAD);
	DTTR_ImGuiDialog_End(&ctx);
	DTTR_ImGuiDialog_Shutdown();
	return 0;
}
