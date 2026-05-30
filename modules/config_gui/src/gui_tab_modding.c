#include "gui_internal.h"

static const char *TOOLTIP_HOT_RELOAD = "Hot-reload mod DLLs while the game runs. "
										"Default: false.";
static const char *TOOLTIP_MOD_ENABLE = "Load this mod DLL on the next game launch. "
										"Default: enabled.";
static const char *MODDING_WARNING_TEXT = "Experimental modding API. It may change "
										  "without warning.";

#define DTTR_CONFIG_UI_MOD_ENABLE_W 4.0f

typedef struct {
	int count;
	char names[DTTR_CONFIG_DISABLED_MODS_MAX][MAX_PATH];
} config_mod_dll_list;

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
	if (!igBeginTable("##modding_mod_table", 2, flags, (ImVec2_c){0.0f, 0.0f}, 0.0f)) {
		return false;
	}

	igTableSetupColumn(
		"On",
		ImGuiTableColumnFlags_WidthFixed,
		mod_enable_column_width(ctx),
		0
	);
	igTableSetupColumn("DLL", ImGuiTableColumnFlags_WidthStretch, 0.0f, 0);
	igTableHeadersRow();
	return true;
}

static void draw_mod_toggle_row(
	const DTTR_ImGuiDialogContext *ctx,
	config_ui_state *state,
	const char *mod_name,
	int row_index
) {
	begin_config_table_row();
	igTableNextColumn();

	bool enabled = !DTTR_Config_IsModDisabled(&state->config, mod_name);
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
	add_scaled_vertical_spacing(ctx, DTTR_CONFIG_UI_SECTION_SPACING);
	igSeparatorText("Mods");
	show_tooltip(TOOLTIP_MOD_ENABLE);
}

void draw_modding_tab(const DTTR_ImGuiDialogContext *ctx, config_ui_state *state) {
	igPushStyleColor_Vec4(ImGuiCol_Text, DTTR_CONFIG_UI_WARNING_TEXT_COLOR);
	igTextWrapped("%s", MODDING_WARNING_TEXT);
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
		"Hot Reload",
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
		draw_mod_toggle_row(ctx, state, mods.names[i], i);
	}

	igEndTable();
}
