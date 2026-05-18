#include "gui_internal.h"

static const char *TOOLTIP_HOT_RELOAD = "Hot reload mod DLLs while the game is "
										"running. Default: false.";
static const char *TOOLTIP_MOD_ENABLE = "Controls whether this mod DLL is "
										"loaded on the next game launch. "
										"Default: enabled.";
static const char *MODDING_WARNING_TEXT = "The modding API is experimental. "
										  "It is incomplete and breaking changes may "
										  "be made without warning.";

#define DTTR_CONFIG_UI_MOD_ENABLE_W 4.0f

typedef struct {
	int count;
	char names[DTTR_CONFIG_DISABLED_MODS_MAX][MAX_PATH];
} config_mod_dll_list;

static float mod_margin_x() { return DTTR_CONFIG_UI_ROW_MARGIN_X * 4.0f; }

static float compute_mod_margin_width(const DTTR_ImGuiDialogContext *ctx) {
	return DTTR_ImGuiDialog_ScaledFloat(ctx, mod_margin_x());
}

static float mod_enable_column_width(const DTTR_ImGuiDialogContext *ctx) {
	return igGetFrameHeight()
		   + DTTR_ImGuiDialog_ScaledFloat(ctx, DTTR_CONFIG_UI_MOD_ENABLE_W);
}

static bool is_shadow_mod_dll(const char *filename) {
	return strncmp(filename, DTTR_MODS_SHADOW_PREFIX, sizeof(DTTR_MODS_SHADOW_PREFIX) - 1)
		   == 0;
}

static void scan_mod_dlls(const config_ui_state *state, config_mod_dll_list *out) {
	memset(out, 0, sizeof(*out));
	if (!state || !state->mods_dir[0]) {
		return;
	}

	char search_pattern[MAX_PATH];
	const int written = snprintf(
		search_pattern,
		sizeof(search_pattern),
		"%s\\*.dll",
		state->mods_dir
	);
	if (written <= 0 || (size_t)written >= sizeof(search_pattern)) {
		return;
	}

	WIN32_FIND_DATAA find_data;
	HANDLE find_handle = FindFirstFileA(search_pattern, &find_data);
	if (find_handle == INVALID_HANDLE_VALUE) {
		return;
	}

	do {
		if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			|| is_shadow_mod_dll(find_data.cFileName)) {
			continue;
		}

		if (out->count >= DTTR_CONFIG_DISABLED_MODS_MAX) {
			break;
		}

		if (!DTTR_Path_CopyString(
				out->names[out->count],
				sizeof(out->names[out->count]),
				find_data.cFileName
			)) {
			continue;
		}

		out->count++;
	} while (FindNextFileA(find_handle, &find_data));

	FindClose(find_handle);
}

static bool begin_mod_table(const DTTR_ImGuiDialogContext *ctx) {
	const ImGuiTableFlags flags = ImGuiTableFlags_BordersInnerH
								  | ImGuiTableFlags_BordersOuterH
								  | ImGuiTableFlags_SizingStretchProp
								  | ImGuiTableFlags_NoSavedSettings
								  | ImGuiTableFlags_NoPadOuterX;
	const float mod_margin_width = compute_mod_margin_width(ctx);
	if (!igBeginTable(
			"##modding_mod_table",
			4,
			flags,
			(ImVec2_c){table_width_ignoring_scrollbar(), 0.0f},
			0.0f
		)) {
		return false;
	}

	igTableSetupColumn(
		"##left_margin",
		ImGuiTableColumnFlags_WidthFixed,
		mod_margin_width,
		0
	);
	igTableSetupColumn(
		"Enabled",
		ImGuiTableColumnFlags_WidthFixed,
		mod_enable_column_width(ctx),
		0
	);
	igTableSetupColumn("DLL", ImGuiTableColumnFlags_WidthStretch, 0.0f, 0);
	igTableSetupColumn(
		"##right_margin",
		ImGuiTableColumnFlags_WidthFixed,
		mod_margin_width,
		0
	);
	igTableHeadersRow();
	return true;
}

static void center_mod_checkbox() {
	const float checkbox_width = igGetFrameHeight();
	const float checkbox_cell_width = igGetContentRegionAvail().x;
	if (checkbox_cell_width <= checkbox_width) {
		return;
	}

	igSetCursorPosX(igGetCursorPosX() + (checkbox_cell_width - checkbox_width) * 0.5f);
}

static void draw_mod_toggle_row(
	config_ui_state *state,
	const char *mod_name,
	int row_index
) {
	igTableNextRow(ImGuiTableRowFlags_None, 0.0f);
	igTableNextColumn();
	igTableNextColumn();

	bool enabled = !DTTR_Config_IsModDisabled(&state->config, mod_name);
	center_mod_checkbox();
	igPushID_Int(row_index);
	const bool changed = igCheckbox("##mod_enabled", &enabled);
	igPopID();
	if (changed && !DTTR_Config_SetModEnabled(&state->config, mod_name, enabled)) {
		set_status(state, "Could not update mod toggle.");
	}

	show_tooltip(TOOLTIP_MOD_ENABLE);

	igTableNextColumn();
	igBeginDisabled(!enabled);
	igTextUnformatted(mod_name, NULL);
	igEndDisabled();
}

static void draw_mod_section_header(const DTTR_ImGuiDialogContext *ctx) {
	const float mod_margin_width = compute_mod_margin_width(ctx);

	add_scaled_vertical_spacing(ctx, 6.0f);
	igIndent(mod_margin_width);
	igSeparatorText("Mods");
	show_tooltip(TOOLTIP_MOD_ENABLE);
	igUnindent(mod_margin_width);
}

void draw_modding_tab(const DTTR_ImGuiDialogContext *ctx, config_ui_state *state) {
	igPushStyleColor_Vec4(ImGuiCol_Text, DTTR_CONFIG_UI_WARNING_TEXT_COLOR);
	DTTR_ImGuiDialog_DrawPaddedText(ctx, MODDING_WARNING_TEXT, mod_margin_x(), 0.0f);
	igPopStyleColor(1);
	add_scaled_vertical_spacing(ctx, 4.0f);

	if (!begin_tab_settings_table(
			ctx,
			"##modding_settings_table",
			DTTR_CONFIG_UI_INPUT_W
		)) {
		return;
	}

	labeled_checkbox(
		ctx,
		"Hot reload",
		"##hot_reload",
		&state->config.hot_reload,
		TOOLTIP_HOT_RELOAD,
		FIELD_LABEL_STATE(state, hot_reload)
	);
	end_settings_table();

	config_mod_dll_list mods;
	scan_mod_dlls(state, &mods);

	draw_mod_section_header(ctx);

	if (mods.count == 0) {
		igTextWrapped(
			"No mod DLLs found in %s",
			state->mods_dir[0] ? state->mods_dir : "mods"
		);
		return;
	}

	if (!begin_mod_table(ctx)) {
		return;
	}

	for (int i = 0; i < mods.count; i++) {
		draw_mod_toggle_row(state, mods.names[i], i);
	}

	igEndTable();
}
