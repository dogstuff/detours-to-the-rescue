#include "gui_internal.h"

#include <dttr_config_window_icon.h>

#include <stdlib.h>

#ifndef DTTR_VERSION
#define DTTR_VERSION "unknown"
#endif

#define CONFIG_WINDOW_TITLE "DttR Configuration - " DTTR_VERSION

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

static bool toolbar_button(const char *label) {
	igPushStyleColor_Vec4(ImGuiCol_Text, DTTR_CONFIG_UI_SELECTED_TAB_TEXT_COLOR);
	igPushStyleColor_Vec4(ImGuiCol_Button, DTTR_CONFIG_UI_TAB_BG);
	igPushStyleColor_Vec4(ImGuiCol_ButtonHovered, DTTR_CONFIG_UI_TAB_HOVERED_BG);
	igPushStyleColor_Vec4(ImGuiCol_ButtonActive, DTTR_CONFIG_UI_SELECTED_TAB_BG);

	const bool clicked = igButton(label, (ImVec2_c){0.0f, 0.0f});
	igPopStyleColor(4);

	return clicked;
}

static void draw_toolbar(const DTTR_ImGuiDialogContext *ctx, config_ui_state *state) {
	const ImVec2_c padding = {
		DTTR_ImGuiDialog_ScaledFloat(ctx, DTTR_CONFIG_UI_TOOLBAR_PADDING_X),
		DTTR_ImGuiDialog_ScaledFloat(ctx, DTTR_CONFIG_UI_TOOLBAR_PADDING_Y),
	};

	const ImVec2_c item_spacing = {
		DTTR_ImGuiDialog_ScaledFloat(ctx, DTTR_CONFIG_UI_TOOLBAR_BUTTON_SPACING_X),
		0.0f,
	};

	const ImVec2_c button_padding = {
		DTTR_ImGuiDialog_ScaledFloat(ctx, DTTR_CONFIG_UI_TOOLBAR_BUTTON_PADDING_X),
		igGetStyle()->FramePadding.y,
	};

	const ImGuiWindowFlags toolbar_flags = ImGuiWindowFlags_NoScrollbar
										   | ImGuiWindowFlags_NoScrollWithMouse;


	igPushStyleColor_Vec4(ImGuiCol_ChildBg, DTTR_CONFIG_UI_TOP_BAR_BG);
	igPushStyleVar_Vec2(ImGuiStyleVar_WindowPadding, padding);
	igPushStyleVar_Vec2(ImGuiStyleVar_ItemSpacing, item_spacing);
	igPushStyleVar_Vec2(ImGuiStyleVar_FramePadding, button_padding);
	igPushStyleVar_Float(
		ImGuiStyleVar_FrameRounding,
		DTTR_ImGuiDialog_ScaledFloat(ctx, DTTR_CONFIG_UI_TOOLBAR_BUTTON_ROUNDING)
	);

	if (igBeginChild_Str(
			"##config_toolbar",
			(ImVec2_c){0.0f, 0.0f},
			ImGuiChildFlags_AlwaysUseWindowPadding | ImGuiChildFlags_AutoResizeY,
			toolbar_flags
		)) {
		if (toolbar_button("Save")) {
			save_config(state);
		}

		igSameLine(0.0f, -1.0f);
		if (toolbar_button("Load")) {
			if (confirm_discard_changes(ctx, state, "reload the file from disk")) {
				load_config(state);
			}
		}

		igSameLine(0.0f, -1.0f);
		if (toolbar_button("Reset to Defaults")) {
			request_reset_defaults(ctx, state);
		}
	}

	igEndChild();
	igPopStyleVar(4);
	igPopStyleColor(1);
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
	load_config(state);
	return true;
}

static void draw_ui(const DTTR_ImGuiDialogContext *ctx, config_ui_state *state) {
	push_config_theme();
	handle_shortcuts(ctx, state);

	const bool panel_open = begin_padded_panel(ctx);
	if (panel_open) {
		const bool content_open = begin_config_content_region(ctx, state);
		if (content_open) {
			draw_tabs(ctx, state);
		}

		end_config_content_region();
		draw_footer_text(state);
		draw_toolbar(ctx, state);
	}

	end_padded_panel();

	draw_shortcut_debug_window(state);
	pop_config_theme();
}

// Capture owns keyboard/mouse events; gamepad stays forwarded because ImGui ignores it.
static bool consume_input_binding_capture(config_ui_state *state, const SDL_Event *event) {
	if (!state->input_binding_capturing) {
		return false;
	}

	if (event_cancels_binding(event)) {
		cancel_input_binding_capture(state);
		return true;
	}

	if (capture_input_binding_event(state, event)) {
		return true;
	}

	return event->type == SDL_EVENT_KEY_DOWN
		   || event->type == SDL_EVENT_MOUSE_BUTTON_DOWN;
}

static void process_events(
	const DTTR_ImGuiDialogContext *ctx,
	config_ui_state *state,
	bool *running
) {
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		if (consume_input_binding_capture(state, &event)) {
			continue;
		}

		DTTR_ImGuiDialog_ProcessEvent(ctx, &event, running);
		if (!*running) {
			if (confirm_discard_changes(ctx, state, "close the configuration tool")) {
				break;
			}

			*running = true;
			continue;
		}

		if (capture_input_binding_event(state, &event)) {
			continue;
		}
	}
}

__declspec(dllexport) int dttr_config_main(int argc, char **argv) {
	config_ui_state state = {0};

	if (!init_state_from_args(&state, argc, argv)) {
		return 1;
	}

	DTTR_ImGuiDialog_SetWindowIconPNG(
		DTTR_CONFIG_WINDOW_ICON_PNG,
		sizeof(DTTR_CONFIG_WINDOW_ICON_PNG)
	);

	DTTR_ImGuiDialogContext ctx;
	const int window_width = config_window_width();
	if (!DTTR_ImGuiDialog_BeginResizable(
			&ctx,
			CONFIG_WINDOW_TITLE,
			window_width,
			DTTR_CONFIG_UI_WINDOW_H,
			DTTR_CONFIG_UI_MIN_RESIZABLE_WINDOW_W,
			DTTR_CONFIG_UI_MIN_RESIZABLE_WINDOW_H
		)) {
		return 1;
	}

	if (!SDL_InitSubSystem(SDL_INIT_GAMEPAD | SDL_INIT_JOYSTICK)) {
		char status[sizeof(state.status)];
		snprintf(
			status,
			sizeof(status),
			"Failed to initialize gamepad support: %s",
			SDL_GetError()
		);
		set_status(&state, status);
	}
	SDL_SetGamepadEventsEnabled(true);
	SDL_SetJoystickEventsEnabled(true);

	bool running = true;
	while (running) {
		process_events(&ctx, &state, &running);
		DTTR_ImGuiDialog_RefreshScale(&ctx);
		DTTR_ImGuiDialog_NewFrame(&ctx);

		if (DTTR_ImGuiDialog_BeginRoot(
				&ctx,
				"DttR Configuration",
				ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse
			)) {
			draw_ui(&ctx, &state);
		}

		DTTR_ImGuiDialog_EndRoot();
		DTTR_ImGuiDialog_Render(&ctx);
	}

	close_gamepad_preview(&state);
	SDL_QuitSubSystem(SDL_INIT_GAMEPAD | SDL_INIT_JOYSTICK);
	DTTR_ImGuiDialog_End(&ctx);
	DTTR_ImGuiDialog_Shutdown();
	return 0;
}
