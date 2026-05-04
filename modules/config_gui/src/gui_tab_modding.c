#include "gui_internal.h"

static const char *S_TOOLTIP_HOT_RELOAD = "Hot reload component DLLs while the game is "
										  "running. Default: false.";
static const char *S_TOOLTIP_COMPONENT_ENABLE = "Controls whether this component DLL is "
												"loaded on the next game launch. "
												"Default: enabled.";
static const char *S_MODDING_WARNING_TEXT = "The components/modding API is experimental. "
											"It is incomplete and breaking changes may "
											"be made without warning.";

#define S_CONFIG_COMPONENT_SHADOW_PREFIX "_dttr_hot_"
#define DTTR_CONFIG_UI_COMPONENT_ENABLE_W 4.0f

typedef struct {
	int m_count;
	char m_names[DTTR_CONFIG_DISABLED_COMPONENTS_MAX][MAX_PATH];
} S_ConfigComponentDllList;

static float s_component_margin_x(void) { return DTTR_CONFIG_UI_ROW_MARGIN_X * 4.0f; }

static float s_component_margin_width(const DTTR_ImGuiDialogContext *ctx) {
	return dttr_imgui_dialog_scaled_float(ctx, s_component_margin_x());
}

static float s_component_enable_column_width(const DTTR_ImGuiDialogContext *ctx) {
	return igGetFrameHeight()
		   + dttr_imgui_dialog_scaled_float(ctx, DTTR_CONFIG_UI_COMPONENT_ENABLE_W);
}

static bool s_is_shadow_component_dll(const char *filename) {
	return strncmp(
			   filename,
			   S_CONFIG_COMPONENT_SHADOW_PREFIX,
			   sizeof(S_CONFIG_COMPONENT_SHADOW_PREFIX) - 1
		   )
		   == 0;
}

static void s_scan_component_dlls(
	const S_ConfigUIState *state,
	S_ConfigComponentDllList *out
) {
	memset(out, 0, sizeof(*out));
	if (!state || !state->m_components_dir[0]) {
		return;
	}

	char search_pattern[MAX_PATH];
	const int written = snprintf(
		search_pattern,
		sizeof(search_pattern),
		"%s\\*.dll",
		state->m_components_dir
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
		if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			continue;
		}

		if (s_is_shadow_component_dll(find_data.cFileName)) {
			continue;
		}

		if (out->m_count >= DTTR_CONFIG_DISABLED_COMPONENTS_MAX) {
			break;
		}

		if (dttr_path_copy_string(
				out->m_names[out->m_count],
				sizeof(out->m_names[out->m_count]),
				find_data.cFileName
			)) {
			out->m_count++;
		}
	} while (FindNextFileA(find_handle, &find_data));

	FindClose(find_handle);
}

static bool s_begin_component_table(const DTTR_ImGuiDialogContext *ctx) {
	const ImGuiTableFlags flags = ImGuiTableFlags_BordersInnerH
								  | ImGuiTableFlags_BordersOuterH
								  | ImGuiTableFlags_SizingStretchProp
								  | ImGuiTableFlags_NoSavedSettings
								  | ImGuiTableFlags_NoPadOuterX;
	const float component_margin_width = s_component_margin_width(ctx);
	if (!igBeginTable(
			"##modding_component_table",
			4,
			flags,
			(ImVec2_c){s_table_width_ignoring_scrollbar(), 0.0f},
			0.0f
		)) {
		return false;
	}

	igTableSetupColumn(
		"##left_margin",
		ImGuiTableColumnFlags_WidthFixed,
		component_margin_width,
		0
	);
	igTableSetupColumn(
		"##enabled",
		ImGuiTableColumnFlags_WidthFixed,
		s_component_enable_column_width(ctx),
		0
	);
	igTableSetupColumn("DLL", ImGuiTableColumnFlags_WidthStretch, 0.0f, 0);
	igTableSetupColumn(
		"##right_margin",
		ImGuiTableColumnFlags_WidthFixed,
		component_margin_width,
		0
	);
	igTableHeadersRow();
	return true;
}

static void s_center_component_checkbox(void) {
	const float checkbox_width = igGetFrameHeight();
	const float checkbox_cell_width = igGetContentRegionAvail().x;
	if (checkbox_cell_width <= checkbox_width) {
		return;
	}

	igSetCursorPosX(igGetCursorPosX() + (checkbox_cell_width - checkbox_width) * 0.5f);
}

static void s_draw_component_toggle_row(
	S_ConfigUIState *state,
	const char *component_name,
	int row_index
) {
	igTableNextRow(ImGuiTableRowFlags_None, 0.0f);
	igTableNextColumn();
	igTableNextColumn();

	bool enabled = !dttr_config_is_component_disabled(&state->m_config, component_name);
	s_center_component_checkbox();
	igPushID_Int(row_index);
	const bool changed = igCheckbox("##component_enabled", &enabled);
	igPopID();
	if (changed
		&& !dttr_config_set_component_enabled(&state->m_config, component_name, enabled)) {
		s_set_status(state, "Could not update component toggle.");
	}

	s_show_tooltip(S_TOOLTIP_COMPONENT_ENABLE);

	igTableNextColumn();
	igBeginDisabled(!enabled);
	igTextUnformatted(component_name, NULL);
	igEndDisabled();
}

static void s_draw_component_section_header(const DTTR_ImGuiDialogContext *ctx) {
	const float component_margin_width = s_component_margin_width(ctx);

	s_add_scaled_vertical_spacing(ctx, 6.0f);
	igIndent(component_margin_width);
	igSeparatorText("Components");
	s_show_tooltip(S_TOOLTIP_COMPONENT_ENABLE);
	igUnindent(component_margin_width);
}

void s_draw_modding_tab(const DTTR_ImGuiDialogContext *ctx, S_ConfigUIState *state) {
	igPushStyleColor_Vec4(ImGuiCol_Text, DTTR_CONFIG_UI_WARNING_TEXT_COLOR);
	dttr_imgui_dialog_draw_padded_text(
		ctx,
		S_MODDING_WARNING_TEXT,
		s_component_margin_x(),
		0.0f
	);
	igPopStyleColor(1);
	s_add_scaled_vertical_spacing(ctx, 4.0f);

	if (!s_begin_tab_settings_table(
			ctx,
			"##modding_settings_table",
			DTTR_CONFIG_UI_INPUT_W
		)) {
		return;
	}

	s_labeled_checkbox(
		ctx,
		"Hot reload",
		"##hot_reload",
		&state->m_config.m_hot_reload,
		S_TOOLTIP_HOT_RELOAD,
		S_FIELD_LABEL_STATE(state, m_hot_reload)
	);
	s_end_settings_table();

	S_ConfigComponentDllList components;
	s_scan_component_dlls(state, &components);

	s_draw_component_section_header(ctx);

	if (components.m_count == 0) {
		igTextWrapped(
			"No component DLLs found in %s",
			state->m_components_dir[0] ? state->m_components_dir : "components"
		);
		return;
	}

	if (!s_begin_component_table(ctx)) {
		return;
	}

	for (int i = 0; i < components.m_count; i++) {
		s_draw_component_toggle_row(state, components.m_names[i], i);
	}

	igEndTable();
}
