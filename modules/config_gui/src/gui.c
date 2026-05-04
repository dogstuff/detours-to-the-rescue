#include "gui_internal.h"

#include <stdlib.h>

static const char *const S_CONFIG_WINDOW_TITLE = "DttR Configuration";
static const char *const S_CONFIG_DEBUG_SHORTCUTS_ENV = "DTTR_CONFIG_DEBUG_SHORTCUTS";

static const char *s_config_path_from_args(int argc, char **argv) {
	return argc > 1 && argv[1] && argv[1][0] ? argv[1] : DTTR_CONFIG_FILENAME;
}

static void s_draw_toolbar(const DTTR_ImGuiDialogContext *ctx, S_ConfigUIState *state) {
	if (!igBeginMenuBar()) {
		return;
	}

	if (igMenuItem_Bool("Save", "Ctrl+S", false, true)) {
		s_save_config(state);
	}

	if (igMenuItem_Bool("Load", "Ctrl+O", false, true)) {
		s_load_config(state);
	}

	const ImGuiStyle *style = igGetStyle();
	const ImVec2_c reset_size = igCalcTextSize("Reset to Defaults", NULL, false, -1.0f);
	const float reset_width = reset_size.x + style->FramePadding.x * 2.0f;
	const float reset_x = igGetCursorPosX() + igGetContentRegionAvail().x - reset_width
						  - style->ItemSpacing.x;
	if (reset_x > igGetCursorPosX()) {
		igSetCursorPosX(reset_x);
	}

	if (igMenuItem_Bool("Reset to Defaults", NULL, false, true)) {
		s_request_reset_defaults(ctx, state);
	}

	igEndMenuBar();
}

static void s_handle_shortcuts(S_ConfigUIState *state) {
	if (igShortcut_Nil(ImGuiMod_Ctrl | ImGuiKey_S, ImGuiInputFlags_RouteGlobal)) {
		s_save_config(state);
	}

	if (igShortcut_Nil(ImGuiMod_Ctrl | ImGuiKey_O, ImGuiInputFlags_RouteGlobal)) {
		s_load_config(state);
	}
}

static bool s_env_flag_enabled(const char *name) {
	const char *value = getenv(name);
	return value && value[0] && strcmp(value, "0") != 0;
}

static void s_draw_shortcut_debug_row(const char *label, ImGuiKeyChord chord) {
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

static void s_draw_shortcut_debug_window(const S_ConfigUIState *state) {
	if (!state->m_show_shortcut_debug) {
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
		s_draw_shortcut_debug_row("Save", ImGuiMod_Ctrl | ImGuiKey_S);
		s_draw_shortcut_debug_row("Load", ImGuiMod_Ctrl | ImGuiKey_O);
		igEndTable();
	}

	igEnd();
}

static bool s_init_state_from_args(S_ConfigUIState *state, int argc, char **argv) {
	const char *config_path = s_config_path_from_args(argc, argv);
	if (!dttr_path_copy_string(state->m_path, sizeof(state->m_path), config_path)) {
		dttr_sdl_show_simple_message_box(
			SDL_MESSAGEBOX_ERROR,
			S_CONFIG_WINDOW_TITLE,
			"Config path is too long.",
			NULL
		);
		return false;
	}

	state->m_show_shortcut_debug = s_env_flag_enabled(S_CONFIG_DEBUG_SHORTCUTS_ENV);
	s_set_components_dir_from_config_path(state);
	dttr_config_set_defaults(&state->m_defaults);
	state->m_config = state->m_defaults;
	state->m_saved_config = state->m_config;
	s_sync_rows_from_config(state);
	s_load_config(state);
	return true;
}

static void s_draw_ui(const DTTR_ImGuiDialogContext *ctx, S_ConfigUIState *state) {
	s_push_config_theme();
	s_handle_shortcuts(state);
	s_draw_toolbar(ctx, state);
	s_add_scaled_vertical_spacing(ctx, DTTR_CONFIG_UI_HEADER_TOP_SPACING);

	dttr_imgui_dialog_draw_header(ctx, S_CONFIG_WINDOW_TITLE, DTTR_VERSION);
	igSeparator();

	float panel_width = igGetContentRegionAvail().x;
	if (panel_width < 1.0f) {
		panel_width = 1.0f;
	}

	const bool panel_open = s_begin_padded_panel(ctx, panel_width);
	if (panel_open) {
		const float content_margin_x = dttr_imgui_dialog_scaled_float(
			ctx,
			DTTR_CONFIG_UI_ROW_MARGIN_X
		);
		igIndent(content_margin_x);
		s_draw_tabs(ctx, state);
		igUnindent(content_margin_x);
	}

	s_end_padded_panel();
	if (panel_open) {
		s_draw_bottom_status_text(ctx, state);
	}

	s_draw_shortcut_debug_window(state);
	s_pop_config_theme();
}

static void s_process_events(
	const DTTR_ImGuiDialogContext *ctx,
	S_ConfigUIState *state,
	bool *running
) {
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		dttr_imgui_dialog_process_event(ctx, &event, running);
		if (!*running) {
			break;
		}

		if (s_event_cancels_binding(&event)) {
			s_cancel_binding(state);
			continue;
		}

		const int source = s_source_from_event(&event);
		if (source >= 0) {
			s_capture_source(state, source);
		}
	}
}

__declspec(dllexport) int dttr_config_main(int argc, char **argv) {
	S_ConfigUIState state = {
		.m_binding_row = -1,
	};

	if (!s_init_state_from_args(&state, argc, argv)) {
		return 1;
	}

	DTTR_ImGuiDialogContext ctx;
	if (!dttr_imgui_dialog_begin(
			&ctx,
			S_CONFIG_WINDOW_TITLE,
			s_config_window_width(),
			DTTR_CONFIG_UI_WINDOW_H
		)) {
		return 1;
	}

	SDL_InitSubSystem(SDL_INIT_GAMEPAD);

	bool running = true;
	while (running) {
		s_process_events(&ctx, &state, &running);
		dttr_imgui_dialog_refresh_scale(&ctx);
		dttr_imgui_dialog_new_frame(&ctx);

		if (dttr_imgui_dialog_begin_root(
				&ctx,
				S_CONFIG_WINDOW_TITLE,
				ImGuiWindowFlags_MenuBar
			)) {
			s_draw_ui(&ctx, &state);
		}

		dttr_imgui_dialog_end_root();
		dttr_imgui_dialog_render(&ctx);
	}

	SDL_QuitSubSystem(SDL_INIT_GAMEPAD);
	dttr_imgui_dialog_end(&ctx);
	dttr_imgui_dialog_shutdown();
	return 0;
}
