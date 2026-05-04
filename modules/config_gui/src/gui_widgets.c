#include "gui_internal.h"

void s_same_path_button_row(const DTTR_ImGuiDialogContext *ctx) {
	igSameLine(
		0.0f,
		dttr_imgui_dialog_scaled_float(ctx, DTTR_CONFIG_UI_PATH_BUTTON_SPACING)
	);
}

void s_add_scaled_vertical_spacing(const DTTR_ImGuiDialogContext *ctx, float height) {
	igDummy((ImVec2_c){0.0f, dttr_imgui_dialog_scaled_float(ctx, height)});
}

void s_align_next_item_right(float item_width) {
	const float available_width = igGetContentRegionAvail().x;
	if (available_width <= item_width) {
		return;
	}

	igSetCursorPosX(igGetCursorPosX() + available_width - item_width);
}

static float s_config_max_float(float a, float b) { return a > b ? a : b; }

typedef void (*S_ConfigPathDialogFn)(
	const DTTR_ImGuiDialogContext *ctx,
	S_ConfigUIState *state
);

typedef struct {
	const char *m_id;
	const char *m_label;
	const char *m_tooltip;
	S_ConfigPathDialogFn m_open_dialog;
} S_ConfigPathPickerButton;

static const ImGuiTableFlags S_CONFIG_TABLE_FLAGS = ImGuiTableFlags_BordersInnerH
													| ImGuiTableFlags_BordersOuterH
													| ImGuiTableFlags_SizingStretchProp
													| ImGuiTableFlags_NoSavedSettings
													| ImGuiTableFlags_NoPadOuterX;

static float s_config_path_control_width(void) {
	return DTTR_CONFIG_UI_PATH_INPUT_W + DTTR_CONFIG_UI_PATH_BUTTON_SPACING
		   + DTTR_CONFIG_UI_PATH_BUTTON_W + DTTR_CONFIG_UI_PATH_BUTTON_SPACING
		   + DTTR_CONFIG_UI_PATH_BUTTON_W;
}

float s_config_standard_input_width(void) {
	return s_config_max_float(DTTR_CONFIG_UI_INPUT_W, s_config_path_control_width());
}

static float s_config_standard_content_width(void) {
	return DTTR_CONFIG_UI_ROW_MARGIN_X * 2.0f + DTTR_CONFIG_UI_LABEL_W
		   + s_config_standard_input_width();
}

static float s_config_gamepad_content_width(void) {
	return DTTR_CONFIG_UI_ROW_MARGIN_X * 8.0f + DTTR_CONFIG_UI_GAMEPAD_SOURCE_W
		   + s_config_standard_input_width()
		   + (DTTR_CONFIG_UI_GAMEPAD_BUTTON_W + DTTR_CONFIG_UI_PATH_BUTTON_SPACING)
				 * 3.0f;
}

int s_config_window_width(void) {
	float content_width = s_config_max_float(
		s_config_standard_content_width(),
		s_config_gamepad_content_width()
	);
	return (int)((DTTR_CONFIG_UI_PANEL_PADDING_X * 2.0f + content_width) * 0.833333f);
}

static int s_choice_index(const DTTR_ConfigChoice *choices, int choice_count, int value) {
	for (int i = 0; i < choice_count; i++) {
		if (choices[i].value == value) {
			return i;
		}
	}

	return 0;
}

static const char *s_choice_label(
	const DTTR_ConfigChoice *choices,
	int choice_count,
	int value
) {
	for (int i = 0; i < choice_count; i++) {
		if (choices[i].value == value) {
			return choices[i].label;
		}
	}

	return "Unknown";
}

bool s_choice_combo(
	const char *label,
	int *value,
	DTTR_ConfigChoiceList choices,
	const char *const *tooltips
) {
	int choice_count = 0;
	const DTTR_ConfigChoice *choice_list = dttr_config_choices(choices, &choice_count);
	const int current = s_choice_index(choice_list, choice_count, *value);
	const char *preview = s_choice_label(choice_list, choice_count, *value);
	if (!igBeginCombo(label, preview, ImGuiComboFlags_None)) {
		return false;
	}

	bool changed = false;
	for (int i = 0; i < choice_count; i++) {
		const bool selected = i == current;
		if (igSelectable_Bool(
				choice_list[i].label,
				selected,
				ImGuiSelectableFlags_None,
				(ImVec2_c){0.0f, 0.0f}
			)) {
			*value = choice_list[i].value;
			changed = true;
		}

		s_show_tooltip(tooltips ? tooltips[i] : NULL);
	}

	igEndCombo();
	return changed;
}

static void s_draw_tooltip_text_segment(
	const char *start,
	const char *end,
	bool same_line
) {
	if (!start || start == end) {
		return;
	}

	if (same_line) {
		igSameLine(0.0f, 0.0f);
	}

	igTextUnformatted(start, end);
}

static void s_draw_inline_text(const char *start, const char *end) {
	s_draw_tooltip_text_segment(start, end, true);
}

static void s_draw_green_inline_text(const char *start, const char *end) {
	if (!start || start == end) {
		return;
	}

	igSameLine(0.0f, 0.0f);
	igTextColored(
		DTTR_CONFIG_UI_TOOLTIP_DEFAULT_TEXT_COLOR,
		"%.*s",
		(int)(end - start),
		start
	);
}

static void s_draw_default_tooltip_value(
	const char *default_value_start,
	const char *default_value_end
) {
	const char *release_suffix = strstr(default_value_start, " (Release), ");
	if (release_suffix && release_suffix < default_value_end) {
		const char *debug_start = release_suffix + strlen(" (Release), ");
		const char *debug_suffix = strstr(debug_start, " (Debug)");
		if (debug_suffix && debug_suffix < default_value_end) {
			s_draw_green_inline_text(default_value_start, release_suffix);
			s_draw_inline_text(release_suffix, debug_start);
			s_draw_green_inline_text(debug_start, debug_suffix);
			s_draw_inline_text(debug_suffix, default_value_end);
			return;
		}
	}

	s_draw_green_inline_text(default_value_start, default_value_end);
}

void s_show_tooltip(const char *text) {
	if (!text || !text[0]) {
		return;
	}

	if (!igBeginItemTooltip()) {
		return;
	}

	igPushTextWrapPos(DTTR_CONFIG_UI_TOOLTIP_WRAP_W);
	const char *default_text = strstr(text, "Default:");
	if (default_text) {
		const char *default_value_start = default_text + strlen("Default:");
		while (*default_value_start == ' ') {
			default_value_start++;
		}

		const char *default_value_end = default_value_start + strlen(default_value_start);
		if (default_value_end > default_value_start && default_value_end[-1] == '.') {
			default_value_end--;
		}

		s_draw_tooltip_text_segment(text, default_value_start, false);
		s_draw_default_tooltip_value(default_value_start, default_value_end);
		if (*default_value_end) {
			igSameLine(0.0f, 0.0f);
			igTextUnformatted(default_value_end, NULL);
		}
	} else {
		igTextWrapped("%s", text);
	}

	igPopTextWrapPos();
	igEndTooltip();
}

bool s_themed_row_button(
	const DTTR_ImGuiDialogContext *ctx,
	const char *id,
	const char *label,
	float width
) {
	const ImVec2_c size = {
		dttr_imgui_dialog_scaled_float(ctx, width),
		igGetFrameHeight(),
	};

	return dttr_imgui_dialog_button(ctx, id, label, size);
}

void s_push_config_theme(void) {
	igPushStyleVar_Float(
		ImGuiStyleVar_ScrollbarSize,
		igGetStyle()->ScrollbarSize * DTTR_CONFIG_UI_SCROLLBAR_WIDTH_SCALE
	);
	igPushStyleColor_Vec4(ImGuiCol_FrameBg, DTTR_IMGUI_COLOR_STACK_FRAME_BG);
	igPushStyleColor_Vec4(ImGuiCol_FrameBgHovered, DTTR_IMGUI_COLOR_BUTTON_BG_HOVERED);
	igPushStyleColor_Vec4(ImGuiCol_FrameBgActive, DTTR_IMGUI_COLOR_BUTTON_BG_ACTIVE);
	igPushStyleColor_Vec4(ImGuiCol_Button, DTTR_IMGUI_COLOR_BUTTON_BG);
	igPushStyleColor_Vec4(ImGuiCol_ButtonHovered, DTTR_IMGUI_COLOR_BUTTON_BG_HOVERED);
	igPushStyleColor_Vec4(ImGuiCol_ButtonActive, DTTR_IMGUI_COLOR_BUTTON_BG_ACTIVE);
	igPushStyleColor_Vec4(ImGuiCol_Header, DTTR_IMGUI_COLOR_BUTTON_BG);
	igPushStyleColor_Vec4(ImGuiCol_HeaderHovered, DTTR_IMGUI_COLOR_BUTTON_BG_HOVERED);
	igPushStyleColor_Vec4(ImGuiCol_HeaderActive, DTTR_IMGUI_COLOR_BUTTON_BG_ACTIVE);
	igPushStyleColor_Vec4(ImGuiCol_Tab, DTTR_IMGUI_COLOR_BUTTON_BG);
	igPushStyleColor_Vec4(ImGuiCol_TabHovered, DTTR_IMGUI_COLOR_BUTTON_BG_HOVERED);
	igPushStyleColor_Vec4(ImGuiCol_TabSelected, DTTR_CONFIG_UI_SELECTED_TAB_BG);
	igPushStyleColor_Vec4(ImGuiCol_TabDimmed, DTTR_IMGUI_COLOR_STACK_FRAME_BG);
	igPushStyleColor_Vec4(ImGuiCol_TabDimmedSelected, DTTR_CONFIG_UI_SELECTED_TAB_BG);
	igPushStyleColor_Vec4(ImGuiCol_MenuBarBg, DTTR_IMGUI_COLOR_STACK_FRAME_BG);
	igPushStyleColor_Vec4(ImGuiCol_PopupBg, DTTR_IMGUI_COLOR_STACK_FRAME_BG);
}

void s_pop_config_theme(void) {
	igPopStyleColor(16);
	igPopStyleVar(1);
}

static bool s_format_status_text(
	const S_ConfigUIState *state,
	char *buffer,
	size_t buffer_size
) {
	const bool changed = s_config_has_unsaved_changes(state);
	const bool status_visible = state->m_status[0]
								&& SDL_GetTicks() < state->m_status_expires_at_ms;
	if (!changed && !status_visible) {
		if (buffer_size > 0) {
			buffer[0] = '\0';
		}

		return false;
	}

	if (changed && status_visible) {
		snprintf(buffer, buffer_size, "Unsaved changes.\n%s", state->m_status);
	} else if (changed) {
		snprintf(buffer, buffer_size, "Unsaved changes.");
	} else {
		snprintf(buffer, buffer_size, "%s", state->m_status);
	}

	return true;
}

void s_draw_bottom_status_text(
	const DTTR_ImGuiDialogContext *ctx,
	const S_ConfigUIState *state
) {
	char status_text[sizeof(state->m_status) + sizeof("Unsaved changes.\n")];
	if (!s_format_status_text(state, status_text, sizeof(status_text))) {
		return;
	}

	const int line_count = strchr(status_text, '\n') ? 2 : 1;
	const float status_height = igGetTextLineHeight()
								+ (float)(line_count - 1)
									  * igGetTextLineHeightWithSpacing();
	const float bottom_margin = dttr_imgui_dialog_scaled_float(
		ctx,
		DTTR_CONFIG_UI_STATUS_BOTTOM_MARGIN
	);
	const ImVec2_c window_pos = igGetWindowPos();
	const ImVec2_c text_pos = {
		window_pos.x + dttr_imgui_dialog_scaled_float(ctx, DTTR_CONFIG_UI_STATUS_X),
		window_pos.y + igGetWindowHeight() - status_height - bottom_margin,
	};

	ImDrawList *draw_list = igGetForegroundDrawList_ViewportPtr(igGetMainViewport());
	ImDrawList_AddText_Vec2(
		draw_list,
		text_pos,
		igGetColorU32_Vec4(DTTR_CONFIG_UI_STATUS_TEXT_COLOR),
		status_text,
		NULL
	);
}

bool s_begin_padded_panel(const DTTR_ImGuiDialogContext *ctx, float width) {
	const ImVec2_c padding = {
		dttr_imgui_dialog_scaled_float(ctx, DTTR_CONFIG_UI_PANEL_PADDING_X),
		dttr_imgui_dialog_scaled_float(ctx, DTTR_CONFIG_UI_PANEL_PADDING_Y),
	};

	igPushStyleVar_Vec2(ImGuiStyleVar_WindowPadding, padding);
	return igBeginChild_Str(
		"##config_panel",
		(ImVec2_c){width, 0.0f},
		ImGuiChildFlags_None,
		ImGuiWindowFlags_None
	);
}

void s_end_padded_panel(void) {
	igEndChild();
	igPopStyleVar(1);
}

static ImVec2_c s_table_cell_padding(const DTTR_ImGuiDialogContext *ctx, float padding_x) {
	return (ImVec2_c){
		dttr_imgui_dialog_scaled_float(ctx, padding_x),
		dttr_imgui_dialog_scaled_float(ctx, DTTR_CONFIG_UI_TABLE_CELL_PADDING_Y),
	};
}

static bool s_begin_config_table(
	const DTTR_ImGuiDialogContext *ctx,
	const char *id,
	int column_count,
	float cell_padding_x,
	float table_width
) {
	igPushStyleVar_Vec2(
		ImGuiStyleVar_CellPadding,
		s_table_cell_padding(ctx, cell_padding_x)
	);
	if (igBeginTable(
			id,
			column_count,
			S_CONFIG_TABLE_FLAGS,
			(ImVec2_c){table_width, 0.0f},
			0.0f
		)) {
		return true;
	}

	igPopStyleVar(1);
	return false;
}

static void s_setup_scaled_table_column(
	const DTTR_ImGuiDialogContext *ctx,
	const char *id,
	ImGuiTableColumnFlags flags,
	float width
) {
	igTableSetupColumn(id, flags, dttr_imgui_dialog_scaled_float(ctx, width), 0);
}

bool s_begin_settings_table_with_cell_padding_and_margins(
	const DTTR_ImGuiDialogContext *ctx,
	const char *id,
	float label_width,
	float input_width,
	float table_width,
	float cell_padding_x,
	float left_margin_width,
	float right_margin_width
) {
	if (!s_begin_config_table(ctx, id, 4, cell_padding_x, table_width)) {
		return false;
	}

	s_setup_scaled_table_column(
		ctx,
		"##left_margin",
		ImGuiTableColumnFlags_WidthFixed,
		left_margin_width
	);
	s_setup_scaled_table_column(
		ctx,
		"##label",
		ImGuiTableColumnFlags_WidthFixed,
		label_width
	);
	s_setup_scaled_table_column(
		ctx,
		"##input",
		ImGuiTableColumnFlags_WidthStretch,
		input_width
	);
	s_setup_scaled_table_column(
		ctx,
		"##right_margin",
		ImGuiTableColumnFlags_WidthFixed,
		right_margin_width
	);
	return true;
}

bool s_begin_settings_table_with_width(
	const DTTR_ImGuiDialogContext *ctx,
	const char *id,
	float label_width,
	float input_width,
	float table_width
) {
	return s_begin_settings_table_with_cell_padding_and_margins(
		ctx,
		id,
		label_width,
		input_width,
		table_width,
		DTTR_CONFIG_UI_PATH_BUTTON_SPACING * 0.5f,
		0.0f,
		DTTR_CONFIG_UI_ROW_MARGIN_X * 4.0f
	);
}

bool s_begin_settings_table(
	const DTTR_ImGuiDialogContext *ctx,
	const char *id,
	float label_width,
	float input_width
) {
	return s_begin_settings_table_with_width(ctx, id, label_width, input_width, 0.0f);
}

bool s_begin_full_width_settings_table(
	const DTTR_ImGuiDialogContext *ctx,
	const char *id,
	float label_width,
	float input_width
) {
	return s_begin_settings_table_with_width(
		ctx,
		id,
		label_width,
		input_width,
		s_table_width_ignoring_scrollbar()
	);
}

void s_end_settings_table(void) {
	igEndTable();
	igPopStyleVar(1);
}

float s_table_width_ignoring_scrollbar(void) {
	const ImGuiStyle *style = igGetStyle();
	const float width = igGetContentRegionAvail().x + style->ScrollbarSize;
	return width > 1.0f ? width : 1.0f;
}

bool s_begin_gamepad_button_table(const DTTR_ImGuiDialogContext *ctx) {
	if (!s_begin_config_table(
			ctx,
			"##gamepad_button_table",
			7,
			DTTR_CONFIG_UI_PATH_BUTTON_SPACING * 0.5f,
			s_table_width_ignoring_scrollbar()
		)) {
		return false;
	}

	s_setup_scaled_table_column(
		ctx,
		"##left_margin",
		ImGuiTableColumnFlags_WidthFixed,
		DTTR_CONFIG_UI_ROW_MARGIN_X * 4.0f
	);
	s_setup_scaled_table_column(
		ctx,
		"##source",
		ImGuiTableColumnFlags_WidthFixed,
		DTTR_CONFIG_UI_GAMEPAD_SOURCE_W
	);
	s_setup_scaled_table_column(
		ctx,
		"##action",
		ImGuiTableColumnFlags_WidthStretch,
		s_config_standard_input_width()
	);
	s_setup_scaled_table_column(
		ctx,
		"##bind",
		ImGuiTableColumnFlags_WidthFixed,
		DTTR_CONFIG_UI_GAMEPAD_BUTTON_W
	);
	s_setup_scaled_table_column(
		ctx,
		"##clear",
		ImGuiTableColumnFlags_WidthFixed,
		DTTR_CONFIG_UI_GAMEPAD_BUTTON_W
	);
	s_setup_scaled_table_column(
		ctx,
		"##reset",
		ImGuiTableColumnFlags_WidthFixed,
		DTTR_CONFIG_UI_GAMEPAD_BUTTON_W
	);
	s_setup_scaled_table_column(
		ctx,
		"##right_margin",
		ImGuiTableColumnFlags_WidthFixed,
		DTTR_CONFIG_UI_ROW_MARGIN_X * 4.0f
	);
	return true;
}

void s_begin_setting_row(void) {
	igTableNextRow(ImGuiTableRowFlags_None, 0.0f);
	igTableNextColumn();
	igTableNextColumn();
}

float s_table_input_width(const DTTR_ImGuiDialogContext *ctx, float input_width) {
	const float available = igGetContentRegionAvail().x;
	if (available > 1.0f) {
		return available;
	}

	return dttr_imgui_dialog_scaled_float(ctx, input_width);
}

float s_path_text_input_width(const DTTR_ImGuiDialogContext *ctx, int button_count) {
	float width = s_table_input_width(ctx, DTTR_CONFIG_UI_PATH_INPUT_W);
	const float trailing_width = dttr_imgui_dialog_scaled_float(
		ctx,
		(float)button_count * DTTR_CONFIG_UI_PATH_BUTTON_W
			+ (float)button_count * DTTR_CONFIG_UI_PATH_BUTTON_SPACING
	);
	if (width > trailing_width + 1.0f) {
		width -= trailing_width;
	}

	return width > 1.0f ? width : 1.0f;
}

static ImVec4_c s_config_label_text_color(S_ConfigLabelState label_state) {
	switch (label_state) {
	case S_CONFIG_LABEL_UNSAVED:
		return DTTR_CONFIG_UI_CHANGED_LABEL_TEXT_COLOR;
	case S_CONFIG_LABEL_SAVED_CHANGED:
		return DTTR_CONFIG_UI_SAVED_CHANGED_LABEL_TEXT_COLOR;
	case S_CONFIG_LABEL_DEFAULT:
	default:
		return DTTR_CONFIG_UI_LABEL_TEXT_COLOR;
	}
}

void s_draw_config_label(
	const char *label,
	const char *tooltip,
	S_ConfigLabelState label_state
) {
	igTextColored(
		s_config_label_text_color(label_state),
		"%s%s",
		label_state == S_CONFIG_LABEL_UNSAVED ? "* " : "",
		label
	);
	s_show_tooltip(tooltip);
}

static void s_begin_labeled_control(
	const DTTR_ImGuiDialogContext *ctx,
	const char *label,
	float input_width,
	const char *tooltip,
	S_ConfigLabelState label_state
) {
	igAlignTextToFramePadding();
	s_draw_config_label(label, tooltip, label_state);
	igTableNextColumn();
	igSetNextItemWidth(s_table_input_width(ctx, input_width));
}

static void s_draw_path_picker_button(
	const DTTR_ImGuiDialogContext *ctx,
	S_ConfigUIState *state,
	const char *id,
	const char *label,
	const char *tooltip,
	S_ConfigPathDialogFn open_dialog
) {
	s_same_path_button_row(ctx);
	if (s_themed_row_button(ctx, id, label, DTTR_CONFIG_UI_PATH_BUTTON_W)) {
		open_dialog(ctx, state);
	}

	s_show_tooltip(tooltip);
}

static void s_draw_path_picker_buttons(
	const DTTR_ImGuiDialogContext *ctx,
	S_ConfigUIState *state,
	const S_ConfigPathPickerButton *buttons,
	int button_count
) {
	for (int i = 0; i < button_count; i++) {
		if (!buttons[i].m_open_dialog) {
			continue;
		}

		s_draw_path_picker_button(
			ctx,
			state,
			buttons[i].m_id,
			buttons[i].m_label,
			buttons[i].m_tooltip,
			buttons[i].m_open_dialog
		);
	}
}

static bool s_labeled_path_picker_with_dialog(
	const DTTR_ImGuiDialogContext *ctx,
	S_ConfigUIState *state,
	const char *label,
	const char *id,
	char *buf,
	size_t buf_size,
	const char *tooltip,
	S_ConfigLabelState label_state,
	const S_ConfigPathPickerButton *buttons,
	int button_count
) {
	s_begin_setting_row();
	s_begin_labeled_control(ctx, label, DTTR_CONFIG_UI_PATH_INPUT_W, tooltip, label_state);
	igSetNextItemWidth(s_path_text_input_width(ctx, button_count));
	const bool edited = igInputText(
		id,
		buf,
		buf_size,
		ImGuiInputTextFlags_None,
		NULL,
		NULL
	);
	s_show_tooltip(tooltip);
	s_draw_path_picker_buttons(ctx, state, buttons, button_count);
	return edited;
}

bool s_labeled_input_text(
	const DTTR_ImGuiDialogContext *ctx,
	const char *label,
	const char *id,
	char *buf,
	size_t buf_size,
	const char *tooltip,
	S_ConfigLabelState label_state
) {
	s_begin_setting_row();
	s_begin_labeled_control(ctx, label, DTTR_CONFIG_UI_INPUT_W, tooltip, label_state);
	const bool edited = igInputText(
		id,
		buf,
		buf_size,
		ImGuiInputTextFlags_None,
		NULL,
		NULL
	);
	s_show_tooltip(tooltip);
	return edited;
}

bool s_labeled_path_picker(
	const DTTR_ImGuiDialogContext *ctx,
	S_ConfigUIState *state,
	const char *label,
	const char *id,
	char *buf,
	size_t buf_size,
	const char *tooltip,
	S_ConfigLabelState label_state
) {
	static const S_ConfigPathPickerButton buttons[] = {
		{
			"##open_dir",
			"Open Dir",
			"Select an extracted or installed game directory.",
			s_open_pcdogs_dir_dialog,
		},
		{
			"##open_iso",
			"Open ISO",
			"Select an original game ISO.",
			s_open_pcdogs_iso_dialog,
		},
	};

	return s_labeled_path_picker_with_dialog(
		ctx,
		state,
		label,
		id,
		buf,
		buf_size,
		tooltip,
		label_state,
		buttons,
		(int)SDL_arraysize(buttons)
	);
}

bool s_labeled_log_path_picker(
	const DTTR_ImGuiDialogContext *ctx,
	S_ConfigUIState *state,
	const char *label,
	const char *id,
	char *buf,
	size_t buf_size,
	const char *tooltip,
	S_ConfigLabelState label_state
) {
	static const S_ConfigPathPickerButton buttons[] = {
		{
			"##open_log",
			"Open Log",
			"Select a DttR log file path.",
			s_open_log_file_dialog,
		},
	};

	return s_labeled_path_picker_with_dialog(
		ctx,
		state,
		label,
		id,
		buf,
		buf_size,
		tooltip,
		label_state,
		buttons,
		(int)SDL_arraysize(buttons)
	);
}

static void s_push_spin_button_spacing(const DTTR_ImGuiDialogContext *ctx) {
	const float spin_spacing = dttr_imgui_dialog_scaled_float(
		ctx,
		DTTR_CONFIG_UI_SPIN_BUTTON_SPACING
	);
	igPushStyleVar_Vec2(ImGuiStyleVar_ItemInnerSpacing, (ImVec2_c){spin_spacing, 0.0f});
}

bool s_labeled_input_int(
	const DTTR_ImGuiDialogContext *ctx,
	const char *label,
	const char *id,
	int *value,
	int step,
	int step_fast,
	const char *tooltip,
	S_ConfigLabelState label_state
) {
	s_begin_setting_row();
	s_begin_labeled_control(ctx, label, DTTR_CONFIG_UI_INPUT_W, tooltip, label_state);
	s_push_spin_button_spacing(ctx);
	const bool edited = igInputInt(id, value, step, step_fast, ImGuiInputTextFlags_None);
	igPopStyleVar(1);
	s_show_tooltip(tooltip);
	return edited;
}

bool s_labeled_input_float(
	const DTTR_ImGuiDialogContext *ctx,
	const char *label,
	const char *id,
	float *value,
	const char *tooltip,
	S_ConfigLabelState label_state
) {
	s_begin_setting_row();
	s_begin_labeled_control(ctx, label, DTTR_CONFIG_UI_INPUT_W, tooltip, label_state);
	s_push_spin_button_spacing(ctx);
	const bool edited = igInputFloat(
		id,
		value,
		0.05f,
		0.25f,
		"%.3f",
		ImGuiInputTextFlags_None
	);
	igPopStyleVar(1);
	s_show_tooltip(tooltip);
	return edited;
}

bool s_labeled_checkbox(
	const DTTR_ImGuiDialogContext *ctx,
	const char *label,
	const char *id,
	bool *value,
	const char *tooltip,
	S_ConfigLabelState label_state
) {
	s_begin_setting_row();
	s_begin_labeled_control(ctx, label, DTTR_CONFIG_UI_INPUT_W, tooltip, label_state);
	const float checkbox_width = igGetFrameHeight();
	s_align_next_item_right(checkbox_width);
	const bool edited = igCheckbox(id, value);
	s_show_tooltip(tooltip);
	return edited;
}

bool s_labeled_choice_combo(
	const DTTR_ImGuiDialogContext *ctx,
	const char *label,
	const char *id,
	int *value,
	DTTR_ConfigChoiceList choices,
	const char *const *tooltips,
	const char *tooltip,
	S_ConfigLabelState label_state
) {
	s_begin_setting_row();
	s_begin_labeled_control(ctx, label, DTTR_CONFIG_UI_INPUT_W, tooltip, label_state);
	const bool edited = s_choice_combo(id, value, choices, tooltips);
	s_show_tooltip(tooltip);
	return edited;
}
