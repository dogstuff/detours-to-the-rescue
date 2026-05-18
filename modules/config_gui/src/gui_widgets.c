#include "gui_internal.h"

void same_path_button_row(const DTTR_ImGuiDialogContext *ctx) {
	igSameLine(
		0.0f,
		DTTR_ImGuiDialog_ScaledFloat(ctx, DTTR_CONFIG_UI_PATH_BUTTON_SPACING)
	);
}

void add_scaled_vertical_spacing(const DTTR_ImGuiDialogContext *ctx, float height) {
	igDummy((ImVec2_c){0.0f, DTTR_ImGuiDialog_ScaledFloat(ctx, height)});
}

void align_next_item_right(float item_width) {
	const float available_width = igGetContentRegionAvail().x;

	if (available_width <= item_width) {
		return;
	}

	igSetCursorPosX(igGetCursorPosX() + available_width - item_width);
}

static float config_max_float(float a, float b) { return a > b ? a : b; }

typedef void (*config_path_dialog_fn)(
	const DTTR_ImGuiDialogContext *ctx,
	config_ui_state *state
);

typedef struct {
	const char *id;
	const char *label;
	const char *tooltip;
	config_path_dialog_fn open_dialog;
} config_path_picker_button;

static const ImGuiTableFlags CONFIG_TABLE_FLAGS = ImGuiTableFlags_BordersInnerH
												  | ImGuiTableFlags_BordersOuterH
												  | ImGuiTableFlags_SizingStretchProp
												  | ImGuiTableFlags_NoSavedSettings
												  | ImGuiTableFlags_NoPadOuterX;

static float config_path_control_width() {
	return DTTR_CONFIG_UI_PATH_INPUT_W + DTTR_CONFIG_UI_PATH_BUTTON_SPACING
		   + DTTR_CONFIG_UI_PATH_BUTTON_W + DTTR_CONFIG_UI_PATH_BUTTON_SPACING
		   + DTTR_CONFIG_UI_PATH_BUTTON_W;
}

float config_standard_input_width() {
	return config_max_float(DTTR_CONFIG_UI_INPUT_W, config_path_control_width());
}

static float config_standard_content_width() {
	return DTTR_CONFIG_UI_ROW_MARGIN_X * 2.0f + DTTR_CONFIG_UI_LABEL_W
		   + config_standard_input_width();
}

static float config_gamepad_content_width() {
	return DTTR_CONFIG_UI_ROW_MARGIN_X * 8.0f + DTTR_CONFIG_UI_GAMEPAD_SOURCE_W
		   + config_standard_input_width()
		   + (DTTR_CONFIG_UI_GAMEPAD_BUTTON_W + DTTR_CONFIG_UI_PATH_BUTTON_SPACING)
				 * 3.0f;
}

int config_window_width() {
	float content_width = config_max_float(
		config_standard_content_width(),
		config_gamepad_content_width()
	);
	return (int)((DTTR_CONFIG_UI_PANEL_PADDING_X * 2.0f + content_width) * 0.833333f);
}

static int choice_index(const DTTR_ConfigChoice *choices, int choice_count, int value) {
	for (int i = 0; i < choice_count; i++) {
		if (choices[i].value == value) {
			return i;
		}
	}

	return 0;
}

bool choice_combo(
	const char *label,
	int *value,
	DTTR_ConfigChoiceList choices,
	const char *const *tooltips
) {
	int choice_count = 0;
	const DTTR_ConfigChoice *choice_list = DTTR_Config_Choices(choices, &choice_count);
	const int current = choice_index(choice_list, choice_count, *value);
	const char *preview = choice_count > 0 ? choice_list[current].label : "Unknown";

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

		show_tooltip(tooltips ? tooltips[i] : NULL);
	}

	igEndCombo();
	return changed;
}

static void draw_tooltip_text_segment(const char *start, const char *end, bool same_line) {
	if (!start || start == end) {
		return;
	}

	if (same_line) {
		igSameLine(0.0f, 0.0f);
	}

	igTextUnformatted(start, end);
}

static void draw_green_inline_text(const char *start, const char *end) {
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

static void draw_default_tooltip_value(
	const char *default_value_start,
	const char *default_value_end
) {
	const char *release_suffix = strstr(default_value_start, " (Release), ");

	if (release_suffix && release_suffix < default_value_end) {
		const char *debug_start = release_suffix + strlen(" (Release), ");
		const char *debug_suffix = strstr(debug_start, " (Debug)");

		if (debug_suffix && debug_suffix < default_value_end) {
			draw_green_inline_text(default_value_start, release_suffix);
			draw_tooltip_text_segment(release_suffix, debug_start, true);
			draw_green_inline_text(debug_start, debug_suffix);
			draw_tooltip_text_segment(debug_suffix, default_value_end, true);
			return;
		}
	}

	draw_green_inline_text(default_value_start, default_value_end);
}

void show_tooltip(const char *text) {
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

		draw_tooltip_text_segment(text, default_value_start, false);
		draw_default_tooltip_value(default_value_start, default_value_end);

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

bool themed_row_button(
	const DTTR_ImGuiDialogContext *ctx,
	const char *id,
	const char *label,
	float width
) {
	const ImVec2_c size = {
		DTTR_ImGuiDialog_ScaledFloat(ctx, width),
		igGetFrameHeight(),
	};

	return DTTR_ImGuiDialog_Button(ctx, id, label, size);
}

void push_config_theme() {
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

void pop_config_theme() {
	igPopStyleColor(16);
	igPopStyleVar(1);
}

static bool format_status_text(
	const config_ui_state *state,
	char *buffer,
	size_t buffer_size
) {
	const bool changed = config_has_unsaved_changes(state);
	const bool status_visible = state->status[0]
								&& SDL_GetTicks() < state->status_expires_at_ms;

	if (!changed && !status_visible) {
		if (buffer_size > 0) {
			buffer[0] = '\0';
		}

		return false;
	}

	if (changed && status_visible) {
		snprintf(buffer, buffer_size, "Unsaved changes.\n%s", state->status);
	} else if (changed) {
		snprintf(buffer, buffer_size, "Unsaved changes.");
	} else {
		snprintf(buffer, buffer_size, "%s", state->status);
	}

	return true;
}

void draw_bottom_status_text(
	const DTTR_ImGuiDialogContext *ctx,
	const config_ui_state *state
) {
	char status_text[sizeof(state->status) + sizeof("Unsaved changes.\n")];

	if (!format_status_text(state, status_text, sizeof(status_text))) {
		return;
	}

	const int line_count = strchr(status_text, '\n') ? 2 : 1;
	const float status_height = igGetTextLineHeight()
								+ (float)(line_count - 1)
									  * igGetTextLineHeightWithSpacing();
	const float bottom_margin = DTTR_ImGuiDialog_ScaledFloat(
		ctx,
		DTTR_CONFIG_UI_STATUS_BOTTOM_MARGIN
	);
	const ImVec2_c window_pos = igGetWindowPos();
	const ImVec2_c text_pos = {
		window_pos.x + DTTR_ImGuiDialog_ScaledFloat(ctx, DTTR_CONFIG_UI_STATUS_X),
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

bool begin_padded_panel(const DTTR_ImGuiDialogContext *ctx, float width) {
	const ImVec2_c padding = {
		DTTR_ImGuiDialog_ScaledFloat(ctx, DTTR_CONFIG_UI_PANEL_PADDING_X),
		DTTR_ImGuiDialog_ScaledFloat(ctx, DTTR_CONFIG_UI_PANEL_PADDING_Y),
	};

	igPushStyleVar_Vec2(ImGuiStyleVar_WindowPadding, padding);
	return igBeginChild_Str(
		"##config_panel",
		(ImVec2_c){width, 0.0f},
		ImGuiChildFlags_None,
		ImGuiWindowFlags_None
	);
}

void end_padded_panel() {
	igEndChild();
	igPopStyleVar(1);
}

static ImVec2_c table_cell_padding(const DTTR_ImGuiDialogContext *ctx, float padding_x) {
	return (ImVec2_c){
		DTTR_ImGuiDialog_ScaledFloat(ctx, padding_x),
		DTTR_ImGuiDialog_ScaledFloat(ctx, DTTR_CONFIG_UI_TABLE_CELL_PADDING_Y),
	};
}

static bool begin_config_table(
	const DTTR_ImGuiDialogContext *ctx,
	const char *id,
	int column_count,
	float cell_padding_x,
	float table_width
) {
	igPushStyleVar_Vec2(
		ImGuiStyleVar_CellPadding,
		table_cell_padding(ctx, cell_padding_x)
	);

	if (igBeginTable(
			id,
			column_count,
			CONFIG_TABLE_FLAGS,
			(ImVec2_c){table_width, 0.0f},
			0.0f
		)) {
		return true;
	}

	igPopStyleVar(1);
	return false;
}

static void setup_scaled_table_column(
	const DTTR_ImGuiDialogContext *ctx,
	const char *id,
	ImGuiTableColumnFlags flags,
	float width
) {
	igTableSetupColumn(id, flags, DTTR_ImGuiDialog_ScaledFloat(ctx, width), 0);
}

bool begin_settings_table_with_cell_padding_and_margins(
	const DTTR_ImGuiDialogContext *ctx,
	const char *id,
	float label_width,
	float input_width,
	float table_width,
	float cell_padding_x,
	float left_margin_width,
	float right_margin_width
) {
	if (!begin_config_table(ctx, id, 4, cell_padding_x, table_width)) {
		return false;
	}

	setup_scaled_table_column(
		ctx,
		"##left_margin",
		ImGuiTableColumnFlags_WidthFixed,
		left_margin_width
	);
	setup_scaled_table_column(
		ctx,
		"Setting",
		ImGuiTableColumnFlags_WidthFixed,
		label_width
	);
	setup_scaled_table_column(
		ctx,
		"Value",
		ImGuiTableColumnFlags_WidthStretch,
		input_width
	);
	setup_scaled_table_column(
		ctx,
		"##right_margin",
		ImGuiTableColumnFlags_WidthFixed,
		right_margin_width
	);
	igTableHeadersRow();
	return true;
}

bool begin_settings_table_with_width(
	const DTTR_ImGuiDialogContext *ctx,
	const char *id,
	float label_width,
	float input_width,
	float table_width
) {
	return begin_settings_table_with_cell_padding_and_margins(
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

bool begin_settings_table(
	const DTTR_ImGuiDialogContext *ctx,
	const char *id,
	float label_width,
	float input_width
) {
	return begin_settings_table_with_width(ctx, id, label_width, input_width, 0.0f);
}

bool begin_full_width_settings_table(
	const DTTR_ImGuiDialogContext *ctx,
	const char *id,
	float label_width,
	float input_width
) {
	return begin_settings_table_with_width(
		ctx,
		id,
		label_width,
		input_width,
		table_width_ignoring_scrollbar()
	);
}

void end_settings_table() {
	igEndTable();
	igPopStyleVar(1);
}

float table_width_ignoring_scrollbar() {
	const ImGuiStyle *style = igGetStyle();
	const float width = igGetContentRegionAvail().x + style->ScrollbarSize;
	return width > 1.0f ? width : 1.0f;
}

bool begin_gamepad_button_table(const DTTR_ImGuiDialogContext *ctx) {
	if (!begin_config_table(
			ctx,
			"##gamepad_button_table",
			7,
			DTTR_CONFIG_UI_PATH_BUTTON_SPACING * 0.5f,
			table_width_ignoring_scrollbar()
		)) {
		return false;
	}

	setup_scaled_table_column(
		ctx,
		"##left_margin",
		ImGuiTableColumnFlags_WidthFixed,
		DTTR_CONFIG_UI_ROW_MARGIN_X * 4.0f
	);
	setup_scaled_table_column(
		ctx,
		"Game does",
		ImGuiTableColumnFlags_WidthFixed,
		DTTR_CONFIG_UI_GAMEPAD_SOURCE_W
	);
	setup_scaled_table_column(
		ctx,
		"You press",
		ImGuiTableColumnFlags_WidthStretch,
		config_standard_input_width()
	);
	setup_scaled_table_column(
		ctx,
		"Bind",
		ImGuiTableColumnFlags_WidthFixed,
		DTTR_CONFIG_UI_GAMEPAD_BUTTON_W
	);
	setup_scaled_table_column(
		ctx,
		"Clear",
		ImGuiTableColumnFlags_WidthFixed,
		DTTR_CONFIG_UI_GAMEPAD_BUTTON_W
	);
	setup_scaled_table_column(
		ctx,
		"Reset",
		ImGuiTableColumnFlags_WidthFixed,
		DTTR_CONFIG_UI_GAMEPAD_BUTTON_W
	);
	setup_scaled_table_column(
		ctx,
		"##right_margin",
		ImGuiTableColumnFlags_WidthFixed,
		DTTR_CONFIG_UI_ROW_MARGIN_X * 4.0f
	);
	igTableHeadersRow();
	return true;
}

void begin_setting_row() {
	igTableNextRow(ImGuiTableRowFlags_None, 0.0f);
	igTableNextColumn();
	igTableNextColumn();
}

float table_input_width(const DTTR_ImGuiDialogContext *ctx, float input_width) {
	const float available = igGetContentRegionAvail().x;

	if (available > 1.0f) {
		return available;
	}

	return DTTR_ImGuiDialog_ScaledFloat(ctx, input_width);
}

float path_text_input_width(const DTTR_ImGuiDialogContext *ctx, int button_count) {
	float width = table_input_width(ctx, DTTR_CONFIG_UI_PATH_INPUT_W);
	const float trailing_width = DTTR_ImGuiDialog_ScaledFloat(
		ctx,
		(float)button_count * DTTR_CONFIG_UI_PATH_BUTTON_W
			+ (float)button_count * DTTR_CONFIG_UI_PATH_BUTTON_SPACING
	);

	if (width > trailing_width + 1.0f) {
		width -= trailing_width;
	}

	return width > 1.0f ? width : 1.0f;
}

static ImVec4_c config_label_text_color(config_label_state label_state) {
	switch (label_state) {
	case CONFIG_LABEL_UNSAVED:
		return DTTR_CONFIG_UI_CHANGED_LABEL_TEXT_COLOR;
	case CONFIG_LABEL_SAVED_CHANGED:
		return DTTR_CONFIG_UI_SAVED_CHANGED_LABEL_TEXT_COLOR;
	case CONFIG_LABEL_DEFAULT:
	default:
		return DTTR_CONFIG_UI_LABEL_TEXT_COLOR;
	}
}

void draw_config_label(
	const char *label,
	const char *tooltip,
	config_label_state label_state
) {
	igTextColored(
		config_label_text_color(label_state),
		"%s%s",
		label_state == CONFIG_LABEL_UNSAVED ? "* " : "",
		label
	);
	show_tooltip(tooltip);
}

static void begin_labeled_control(
	const DTTR_ImGuiDialogContext *ctx,
	const char *label,
	float input_width,
	const char *tooltip,
	config_label_state label_state
) {
	igAlignTextToFramePadding();
	draw_config_label(label, tooltip, label_state);
	igTableNextColumn();
	igSetNextItemWidth(table_input_width(ctx, input_width));
}

static void draw_path_picker_button(
	const DTTR_ImGuiDialogContext *ctx,
	config_ui_state *state,
	const char *id,
	const char *label,
	const char *tooltip,
	config_path_dialog_fn open_dialog
) {
	same_path_button_row(ctx);

	if (themed_row_button(ctx, id, label, DTTR_CONFIG_UI_PATH_BUTTON_W)) {
		open_dialog(ctx, state);
	}

	show_tooltip(tooltip);
}

static void draw_path_picker_buttons(
	const DTTR_ImGuiDialogContext *ctx,
	config_ui_state *state,
	const config_path_picker_button *buttons,
	int button_count
) {
	for (int i = 0; i < button_count; i++) {
		if (!buttons[i].open_dialog) {
			continue;
		}

		draw_path_picker_button(
			ctx,
			state,
			buttons[i].id,
			buttons[i].label,
			buttons[i].tooltip,
			buttons[i].open_dialog
		);
	}
}

static bool labeled_path_picker_with_dialog(
	const DTTR_ImGuiDialogContext *ctx,
	config_ui_state *state,
	const char *label,
	const char *id,
	char *buf,
	size_t buf_size,
	const char *tooltip,
	config_label_state label_state,
	const config_path_picker_button *buttons,
	int button_count
) {
	begin_setting_row();
	begin_labeled_control(ctx, label, DTTR_CONFIG_UI_PATH_INPUT_W, tooltip, label_state);
	igSetNextItemWidth(path_text_input_width(ctx, button_count));
	const bool edited = igInputText(
		id,
		buf,
		buf_size,
		ImGuiInputTextFlags_None,
		NULL,
		NULL
	);
	show_tooltip(tooltip);
	draw_path_picker_buttons(ctx, state, buttons, button_count);
	return edited;
}

bool labeled_input_text(
	const DTTR_ImGuiDialogContext *ctx,
	const char *label,
	const char *id,
	char *buf,
	size_t buf_size,
	const char *tooltip,
	config_label_state label_state
) {
	begin_setting_row();
	begin_labeled_control(ctx, label, DTTR_CONFIG_UI_INPUT_W, tooltip, label_state);
	const bool edited = igInputText(
		id,
		buf,
		buf_size,
		ImGuiInputTextFlags_None,
		NULL,
		NULL
	);
	show_tooltip(tooltip);
	return edited;
}

bool labeled_path_picker(
	const DTTR_ImGuiDialogContext *ctx,
	config_ui_state *state,
	const char *label,
	const char *id,
	char *buf,
	size_t buf_size,
	const char *tooltip,
	config_label_state label_state
) {
	static const config_path_picker_button buttons[] = {
		{
			"##open_dir",
			"Open Dir",
			"Select an extracted or installed game directory.",
			open_pcdogs_dir_dialog,
		},
		{
			"##open_iso",
			"Open ISO",
			"Select an original game ISO.",
			open_pcdogs_iso_dialog,
		},
	};

	return labeled_path_picker_with_dialog(
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

bool labeled_log_path_picker(
	const DTTR_ImGuiDialogContext *ctx,
	config_ui_state *state,
	const char *label,
	const char *id,
	char *buf,
	size_t buf_size,
	const char *tooltip,
	config_label_state label_state
) {
	static const config_path_picker_button buttons[] = {
		{
			"##open_log",
			"Open Log",
			"Select a DttR log file path.",
			open_log_file_dialog,
		},
	};

	return labeled_path_picker_with_dialog(
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

static void push_spin_button_spacing(const DTTR_ImGuiDialogContext *ctx) {
	const float spin_spacing = DTTR_ImGuiDialog_ScaledFloat(
		ctx,
		DTTR_CONFIG_UI_SPIN_BUTTON_SPACING
	);
	igPushStyleVar_Vec2(ImGuiStyleVar_ItemInnerSpacing, (ImVec2_c){spin_spacing, 0.0f});
}

bool labeled_input_int(
	const DTTR_ImGuiDialogContext *ctx,
	const char *label,
	const char *id,
	int *value,
	int step,
	int step_fast,
	const char *tooltip,
	config_label_state label_state
) {
	begin_setting_row();
	begin_labeled_control(ctx, label, DTTR_CONFIG_UI_INPUT_W, tooltip, label_state);
	push_spin_button_spacing(ctx);
	const bool edited = igInputInt(id, value, step, step_fast, ImGuiInputTextFlags_None);
	igPopStyleVar(1);
	show_tooltip(tooltip);
	return edited;
}

bool labeled_input_float(
	const DTTR_ImGuiDialogContext *ctx,
	const char *label,
	const char *id,
	float *value,
	const char *tooltip,
	config_label_state label_state
) {
	begin_setting_row();
	begin_labeled_control(ctx, label, DTTR_CONFIG_UI_INPUT_W, tooltip, label_state);
	push_spin_button_spacing(ctx);
	const bool edited = igInputFloat(
		id,
		value,
		0.05f,
		0.25f,
		"%.3f",
		ImGuiInputTextFlags_None
	);
	igPopStyleVar(1);
	show_tooltip(tooltip);
	return edited;
}

bool labeled_checkbox(
	const DTTR_ImGuiDialogContext *ctx,
	const char *label,
	const char *id,
	bool *value,
	const char *tooltip,
	config_label_state label_state
) {
	begin_setting_row();
	begin_labeled_control(ctx, label, DTTR_CONFIG_UI_INPUT_W, tooltip, label_state);
	const float checkbox_width = igGetFrameHeight();
	align_next_item_right(checkbox_width);
	const bool edited = igCheckbox(id, value);
	show_tooltip(tooltip);
	return edited;
}

bool labeled_choice_combo(
	const DTTR_ImGuiDialogContext *ctx,
	const char *label,
	const char *id,
	int *value,
	DTTR_ConfigChoiceList choices,
	const char *const *tooltips,
	const char *tooltip,
	config_label_state label_state
) {
	begin_setting_row();
	begin_labeled_control(ctx, label, DTTR_CONFIG_UI_INPUT_W, tooltip, label_state);
	const bool edited = choice_combo(id, value, choices, tooltips);
	show_tooltip(tooltip);
	return edited;
}
