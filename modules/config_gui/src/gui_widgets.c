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
												  | ImGuiTableFlags_PadOuterX;
static const char *const FOOTER_HINT_TEXT = "Ctrl+S to save your changes.";

static float config_path_control_width() {
	return DTTR_CONFIG_UI_PATH_INPUT_W + DTTR_CONFIG_UI_PATH_BUTTON_SPACING
		   + DTTR_CONFIG_UI_PATH_BUTTON_W + DTTR_CONFIG_UI_PATH_BUTTON_SPACING
		   + DTTR_CONFIG_UI_PATH_BUTTON_W;
}

float config_standard_input_width() {
	return config_max_float(DTTR_CONFIG_UI_INPUT_W, config_path_control_width());
}

static float config_standard_content_width() {
	return DTTR_CONFIG_UI_LABEL_W + config_standard_input_width();
}

static float config_gamepad_content_width() {
	return DTTR_CONFIG_UI_GAMEPAD_SOURCE_W + config_standard_input_width()
		   + (DTTR_CONFIG_UI_GAMEPAD_BUTTON_W + DTTR_CONFIG_UI_PATH_BUTTON_SPACING)
				 * 3.0f;
}

int config_window_width() {
	const float content_width = config_max_float(
		config_standard_content_width(),
		config_gamepad_content_width()
	);
	return (int)(DTTR_CONFIG_UI_PANEL_PADDING_X * 2.0f + content_width);
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
	const ImVec2_c combo_popup_padding = {
		igGetStyle()->WindowPadding.x,
		DTTR_CONFIG_UI_COMBO_POPUP_PADDING_Y,
	};

	igPushStyleVar_Vec2(ImGuiStyleVar_WindowPadding, combo_popup_padding);
	const bool open = igBeginCombo(label, preview, ImGuiComboFlags_None);
	igPopStyleVar(1);

	if (!open) {
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

static void draw_wrapped_tooltip_segment(const char *start, const char *end) {
	if (!start || start == end) {
		return;
	}

	igTextWrapped("%.*s", (int)(end - start), start);
}

static void draw_inline_tooltip_segment(const char *start, const char *end) {
	if (!start || start == end) {
		return;
	}

	igSameLine(0.0f, 0.0f);
	igTextUnformatted(start, end);
}

static void draw_default_inline_text(const char *start, const char *end) {
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
			draw_default_inline_text(default_value_start, release_suffix);
			draw_inline_tooltip_segment(release_suffix, debug_start);
			draw_default_inline_text(debug_start, debug_suffix);
			draw_inline_tooltip_segment(debug_suffix, default_value_end);
			return;
		}
	}

	draw_default_inline_text(default_value_start, default_value_end);
}

void show_tooltip(const char *text) {
	if (!text || !text[0]) {
		return;
	}

	const ImVec2_c tooltip_padding = {
		igGetStyle()->WindowPadding.x,
		DTTR_CONFIG_UI_TOOLTIP_PADDING_Y,
	};

	igPushStyleVar_Vec2(ImGuiStyleVar_WindowPadding, tooltip_padding);

	if (!igBeginItemTooltip()) {
		igPopStyleVar(1);
		return;
	}

	igPushTextWrapPos(DTTR_CONFIG_UI_TOOLTIP_WRAP_W);
	const char *default_text = strstr(text, "Default:");

	if (default_text) {
		const char *default_value_start = default_text + strlen("Default:");

		while (*default_value_start == ' ') {
			default_value_start++;
		}

		const char *description_end = default_text;
		while (description_end > text && description_end[-1] == ' ') {
			description_end--;
		}

		const char *default_value_end = default_value_start + strlen(default_value_start);

		if (default_value_end > default_value_start && default_value_end[-1] == '.') {
			default_value_end--;
		}

		draw_wrapped_tooltip_segment(text, description_end);
		draw_wrapped_tooltip_segment(default_text, default_value_start);
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
	igPopStyleVar(1);
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
	igPushStyleColor_Vec4(ImGuiCol_Border, DTTR_CONFIG_UI_BORDER_COLOR);
	igPushStyleColor_Vec4(ImGuiCol_Separator, DTTR_CONFIG_UI_SEPARATOR_COLOR);
	igPushStyleColor_Vec4(ImGuiCol_SeparatorHovered, DTTR_CONFIG_UI_SEPARATOR_COLOR);
	igPushStyleColor_Vec4(ImGuiCol_SeparatorActive, DTTR_CONFIG_UI_SEPARATOR_COLOR);
	igPushStyleColor_Vec4(ImGuiCol_TableBorderStrong, DTTR_CONFIG_UI_TABLE_BORDER_COLOR);
	igPushStyleColor_Vec4(ImGuiCol_TableBorderLight, DTTR_CONFIG_UI_BORDER_COLOR);
}

void pop_config_theme() { igPopStyleColor(22); }

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

static float status_text_height(const char *status_text) {
	if (!status_text || !status_text[0]) {
		return 0.0f;
	}

	int line_count = 1;

	for (const char *p = status_text; *p; p++) {
		if (*p == '\n') {
			line_count++;
		}
	}

	return igGetTextLineHeight()
		   + (float)(line_count - 1) * igGetTextLineHeightWithSpacing();
}

float config_footer_height(
	const DTTR_ImGuiDialogContext *ctx,
	const config_ui_state *state
) {
	char status_text[sizeof(state->status) + sizeof("Unsaved changes.\n")];
	const bool has_status = format_status_text(state, status_text, sizeof(status_text));
	float height = igGetTextLineHeight();

	if (has_status) {
		height += igGetStyle()->ItemSpacing.y + status_text_height(status_text);
	}

	return height;
}

bool begin_config_content_region(
	const DTTR_ImGuiDialogContext *ctx,
	const config_ui_state *state
) {
	const float footer_height = config_footer_height(ctx, state);
	const ImGuiWindowFlags content_flags = ImGuiWindowFlags_NoScrollbar
										   | ImGuiWindowFlags_NoScrollWithMouse;
	return igBeginChild_Str(
		"##config_content",
		(ImVec2_c){0.0f, -footer_height},
		ImGuiChildFlags_None,
		content_flags
	);
}

void end_config_content_region() { igEndChild(); }

void draw_footer_text(const DTTR_ImGuiDialogContext *ctx, const config_ui_state *state) {
	char status_text[sizeof(state->status) + sizeof("Unsaved changes.\n")];
	if (format_status_text(state, status_text, sizeof(status_text))) {
		igPushStyleColor_Vec4(ImGuiCol_Text, DTTR_CONFIG_UI_STATUS_TEXT_COLOR);
		igTextWrapped("%s", status_text);
		igPopStyleColor(1);
	}

	igPushStyleColor_Vec4(ImGuiCol_Text, DTTR_CONFIG_UI_HINT_TEXT_COLOR);
	igTextWrapped("%s", FOOTER_HINT_TEXT);
	igPopStyleColor(1);
}

bool begin_padded_panel(const DTTR_ImGuiDialogContext *ctx) {
	const ImVec2_c padding = {
		DTTR_ImGuiDialog_ScaledFloat(ctx, DTTR_CONFIG_UI_PANEL_PADDING_X),
		DTTR_ImGuiDialog_ScaledFloat(ctx, DTTR_CONFIG_UI_PANEL_PADDING_Y),
	};

	const ImVec2_c item_spacing = {
		DTTR_ImGuiDialog_ScaledFloat(ctx, DTTR_CONFIG_UI_ITEM_SPACING_X),
		DTTR_ImGuiDialog_ScaledFloat(ctx, DTTR_CONFIG_UI_ITEM_SPACING_Y),
	};

	igPushStyleVar_Vec2(ImGuiStyleVar_WindowPadding, padding);
	igPushStyleVar_Vec2(ImGuiStyleVar_ItemSpacing, item_spacing);
	const ImGuiWindowFlags panel_flags = ImGuiWindowFlags_NoScrollbar
										 | ImGuiWindowFlags_NoScrollWithMouse;
	return igBeginChild_Str(
		"##config_panel",
		(ImVec2_c){0.0f, 0.0f},
		ImGuiChildFlags_AlwaysUseWindowPadding,
		panel_flags
	);
}

void end_padded_panel() {
	igEndChild();
	igPopStyleVar(2);
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
	igPushStyleVar_Vec2(
		ImGuiStyleVar_ItemSpacing,
		(ImVec2_c){igGetStyle()->ItemSpacing.x, 0.0f}
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

	igPopStyleVar(2);
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

static void append_table_header_text(
	const DTTR_ImGuiDialogContext *ctx,
	int column,
	const char *text
) {
	if (!igTableSetColumnIndex(column)) {
		return;
	}

	igSetCursorPosX(
		igGetCursorPosX()
		+ DTTR_ImGuiDialog_ScaledFloat(ctx, DTTR_CONFIG_UI_HEADER_TEXT_INSET_X)
	);
	igTextUnformatted(text, NULL);
}

static void begin_table_header_row() {
	igTableNextRow(ImGuiTableRowFlags_Headers, 0.0f);
	igTableSetBgColor(
		ImGuiTableBgTarget_RowBg0,
		igGetColorU32_Col(ImGuiCol_TableHeaderBg, 1.0f),
		-1
	);
}

static bool begin_settings_table_with_cell_padding(
	const DTTR_ImGuiDialogContext *ctx,
	const char *id,
	float label_width,
	float input_width,
	float table_width,
	float cell_padding_x
) {
	if (!begin_config_table(ctx, id, 2, cell_padding_x, table_width)) {
		return false;
	}

	setup_scaled_table_column(
		ctx,
		"Setting",
		ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_IndentDisable
			| ImGuiTableColumnFlags_NoHeaderLabel,
		label_width
	);
	setup_scaled_table_column(
		ctx,
		"Value",
		ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_NoHeaderLabel,
		input_width
	);
	begin_table_header_row();
	append_table_header_text(ctx, 0, "Setting");
	append_table_header_text(ctx, 1, "Value");
	return true;
}

bool begin_settings_table_with_width(
	const DTTR_ImGuiDialogContext *ctx,
	const char *id,
	float label_width,
	float input_width,
	float table_width
) {
	return begin_settings_table_with_cell_padding(
		ctx,
		id,
		label_width,
		input_width,
		table_width,
		DTTR_CONFIG_UI_TABLE_CELL_PADDING_X
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

void end_settings_table() {
	igEndTable();
	igPopStyleVar(2);
}

bool begin_gamepad_button_table(const DTTR_ImGuiDialogContext *ctx) {
	if (!begin_config_table(
			ctx,
			"##gamepad_button_table",
			5,
			DTTR_CONFIG_UI_TABLE_CELL_PADDING_X,
			0.0f
		)) {
		return false;
	}

	setup_scaled_table_column(
		ctx,
		"In-Game Action",
		ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_IndentDisable
			| ImGuiTableColumnFlags_NoHeaderLabel,
		DTTR_CONFIG_UI_GAMEPAD_SOURCE_W
	);
	setup_scaled_table_column(
		ctx,
		"Gamepad Input",
		ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_NoHeaderLabel,
		config_standard_input_width()
	);
	setup_scaled_table_column(
		ctx,
		"Bind",
		ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoHeaderLabel
			| ImGuiTableColumnFlags_NoHeaderWidth,
		DTTR_CONFIG_UI_GAMEPAD_BUTTON_W
	);
	setup_scaled_table_column(
		ctx,
		"Clear",
		ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoHeaderLabel
			| ImGuiTableColumnFlags_NoHeaderWidth,
		DTTR_CONFIG_UI_GAMEPAD_BUTTON_W
	);
	setup_scaled_table_column(
		ctx,
		"Reset",
		ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoHeaderLabel
			| ImGuiTableColumnFlags_NoHeaderWidth,
		DTTR_CONFIG_UI_GAMEPAD_BUTTON_W
	);
	begin_table_header_row();
	append_table_header_text(ctx, 0, "In-Game Action");
	append_table_header_text(ctx, 1, "Gamepad Input");
	return true;
}

void begin_config_table_row() { igTableNextRow(ImGuiTableRowFlags_None, 0.0f); }

void begin_setting_row() {
	begin_config_table_row();
	igTableNextColumn();
}

float table_input_width(const DTTR_ImGuiDialogContext *ctx, float input_width) {
	const float available = igGetContentRegionAvail().x;
	return available > 1.0f ? available : DTTR_ImGuiDialog_ScaledFloat(ctx, input_width);
}

float path_text_input_width(const DTTR_ImGuiDialogContext *ctx, int button_count) {
	float width = table_input_width(ctx, DTTR_CONFIG_UI_PATH_INPUT_W);
	const float trailing_width = DTTR_ImGuiDialog_ScaledFloat(
		ctx,
		(float)button_count * DTTR_CONFIG_UI_PATH_BUTTON_W
			+ (float)button_count * DTTR_CONFIG_UI_PATH_BUTTON_SPACING
	);

	const float text_width = width > trailing_width + 1.0f ? width - trailing_width
														   : width;
	return text_width > 1.0f ? text_width : 1.0f;
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
	const bool edited = igInputInt(id, value, step, step_fast, ImGuiInputTextFlags_None);
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
	const bool edited = igInputFloat(
		id,
		value,
		0.05f,
		0.25f,
		"%.3f",
		ImGuiInputTextFlags_None
	);
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
