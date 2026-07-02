#include "gui_internal.h"

#include <float.h>
#include <math.h>

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

static const char *const FOOTER_HINT_TEXT = "Ctrl+S to save your changes.";

static float config_path_control_width() {
	return DTTR_CONFIG_UI_PATH_INPUT_W + (DTTR_CONFIG_UI_PATH_BUTTON_W * 2.0f);
}

float config_standard_input_width() {
	return fmaxf(DTTR_CONFIG_UI_INPUT_W, config_path_control_width());
}

static float config_standard_content_width() {
	return DTTR_CONFIG_UI_LABEL_W + config_standard_input_width();
}

int config_window_width() {
	const float content_width = config_standard_content_width();
	const int content_window_width = (int)(DTTR_CONFIG_UI_PANEL_PADDING_X * 2.0f
										   + content_width);
	return SDL_max(content_window_width, DTTR_CONFIG_UI_MIN_WINDOW_W);
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
	const bool open = igBeginCombo(label, preview, ImGuiComboFlags_None);

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
		if (selected) {
			igSetItemDefaultFocus();
		}
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
	const char *release_suffix = strstr(default_value_start, " (release), ");

	if (release_suffix && release_suffix < default_value_end) {
		const char *debug_start = release_suffix + strlen(" (release), ");
		const char *debug_suffix = strstr(debug_start, " (debug)");

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
		0.0f,
	};

	igPushID_Str(id);
	igPushStyleColor_Vec4(ImGuiCol_Text, DTTR_IMGUI_COLOR_BUTTON_TEXT);
	igPushStyleColor_Vec4(ImGuiCol_Button, DTTR_IMGUI_COLOR_BUTTON_BG);
	igPushStyleColor_Vec4(ImGuiCol_ButtonHovered, DTTR_CONFIG_UI_BUTTON_HOVERED_BG);
	igPushStyleColor_Vec4(ImGuiCol_ButtonActive, DTTR_CONFIG_UI_BUTTON_ACTIVE_BG);
	igPushStyleVar_Float(
		ImGuiStyleVar_FrameRounding,
		DTTR_ImGuiDialog_ScaledFloat(ctx, 2.0f)
	);
	const bool clicked = igButton(label, size);
	igPopStyleVar(1);
	igPopStyleColor(4);
	igPopID();
	return clicked;
}

typedef struct {
	ImGuiCol target;
	ImVec4_c color;
} config_theme_color;

static const config_theme_color CONFIG_THEME_COLORS[] = {
	{ImGuiCol_FrameBg, DTTR_IMGUI_COLOR_STACK_FRAME_BG},
	{ImGuiCol_FrameBgHovered, DTTR_IMGUI_COLOR_BUTTON_BG_HOVERED},
	{ImGuiCol_FrameBgActive, DTTR_CONFIG_UI_BUTTON_ACTIVE_BG},
	{ImGuiCol_Button, DTTR_IMGUI_COLOR_BUTTON_BG},
	{ImGuiCol_ButtonHovered, DTTR_CONFIG_UI_BUTTON_HOVERED_BG},
	{ImGuiCol_ButtonActive, DTTR_CONFIG_UI_BUTTON_ACTIVE_BG},
	{ImGuiCol_Header, DTTR_IMGUI_COLOR_BUTTON_BG},
	{ImGuiCol_HeaderHovered, DTTR_CONFIG_UI_BUTTON_HOVERED_BG},
	{ImGuiCol_HeaderActive, DTTR_CONFIG_UI_BUTTON_ACTIVE_BG},
	{ImGuiCol_Tab, DTTR_CONFIG_UI_TAB_BG},
	{ImGuiCol_TabHovered, DTTR_CONFIG_UI_TAB_HOVERED_BG},
	{ImGuiCol_TabSelected, DTTR_CONFIG_UI_SELECTED_TAB_BG},
	{ImGuiCol_TabDimmed, DTTR_CONFIG_UI_TAB_BG},
	{ImGuiCol_TabDimmedSelected, DTTR_CONFIG_UI_SELECTED_TAB_BG},
	{ImGuiCol_MenuBarBg, DTTR_CONFIG_UI_TOP_BAR_BG},
	{ImGuiCol_PopupBg, DTTR_IMGUI_COLOR_STACK_FRAME_BG},
	{ImGuiCol_Border, DTTR_CONFIG_UI_BORDER_COLOR},
	{ImGuiCol_Separator, DTTR_CONFIG_UI_SEPARATOR_COLOR},
	{ImGuiCol_SeparatorHovered, DTTR_CONFIG_UI_SEPARATOR_COLOR},
	{ImGuiCol_SeparatorActive, DTTR_CONFIG_UI_SEPARATOR_COLOR},
	{ImGuiCol_TableHeaderBg, DTTR_CONFIG_UI_TABLE_HEADER_BG},
	{ImGuiCol_TableBorderStrong, DTTR_CONFIG_UI_TABLE_BORDER_COLOR},
	{ImGuiCol_TableBorderLight, DTTR_CONFIG_UI_BORDER_COLOR},
};

void push_config_theme() {
	for (size_t i = 0; i < SDL_arraysize(CONFIG_THEME_COLORS); i++) {
		igPushStyleColor_Vec4(CONFIG_THEME_COLORS[i].target, CONFIG_THEME_COLORS[i].color);
	}
}

void pop_config_theme() {
	igPopStyleColor((int)SDL_arraysize(CONFIG_THEME_COLORS));
}

static bool status_visible(const config_ui_state *state) {
	return state->status[0] && SDL_GetTicks() < state->status_expires_at_ms;
}

static float status_text_height(const char *status_text, float wrap_width) {
	if (!status_text || !status_text[0]) {
		return 0.0f;
	}

	const ImVec2_c size = igCalcTextSize(status_text, NULL, false, wrap_width);
	return size.y > 0.0f ? size.y : igGetTextLineHeight();
}

static float footer_action_height(const DTTR_ImGuiDialogContext *ctx) {
	const ImGuiStyle *style = igGetStyle();
	const float spacing_y = style ? style->ItemSpacing.y : 0.0f;
	return spacing_y
		   + DTTR_ImGuiDialog_ScaledFloat(ctx, DTTR_CONFIG_UI_TOOLBAR_PADDING_Y * 2.0f)
		   + igGetFrameHeight();
}

static float config_footer_height(
	const DTTR_ImGuiDialogContext *ctx,
	const config_ui_state *state
) {
	float height = igGetTextLineHeight() + footer_action_height(ctx);
	const float wrap_width = igGetContentRegionAvail().x;

	if (status_visible(state)) {
		height += igGetStyle()->ItemSpacing.y
				  + status_text_height(state->status, wrap_width);
	}

	if (config_has_unsaved_changes(state)) {
		height += igGetStyle()->ItemSpacing.y
				  + status_text_height("Unsaved changes.", wrap_width);
	}

	return height;
}

bool begin_config_content_region(
	const DTTR_ImGuiDialogContext *ctx,
	const config_ui_state *state
) {
	const float footer_height = config_footer_height(ctx, state);
	return igBeginChild_Str(
		"##config_content",
		(ImVec2_c){0.0f, -footer_height},
		ImGuiChildFlags_None,
		ImGuiWindowFlags_None
	);
}

void end_config_content_region() {
	igEndChild();
}

static void draw_status_text(const char *text) {
	igPushStyleColor_Vec4(ImGuiCol_Text, DTTR_CONFIG_UI_STATUS_TEXT_COLOR);
	igTextWrapped("%s", text);
	igPopStyleColor(1);
}

void draw_footer_text(const config_ui_state *state) {
	if (status_visible(state)) {
		draw_status_text(state->status);
	}

	if (config_has_unsaved_changes(state)) {
		draw_status_text("Unsaved changes.");
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

static ImVec2_c table_cell_padding(const DTTR_ImGuiDialogContext *ctx) {
	const ImGuiStyle *style = igGetStyle();
	return (ImVec2_c){
		DTTR_ImGuiDialog_ScaledFloat(ctx, DTTR_CONFIG_UI_TABLE_CELL_PADDING_X),
		style ? style->CellPadding.y : 0.0f,
	};
}

static bool begin_config_table(
	const DTTR_ImGuiDialogContext *ctx,
	const char *id,
	int column_count,
	float table_width
) {
	igPushStyleVar_Vec2(ImGuiStyleVar_CellPadding, table_cell_padding(ctx));

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

bool begin_settings_table(
	const DTTR_ImGuiDialogContext *ctx,
	const char *id,
	float label_width,
	float input_width
) {
	if (!begin_config_table(ctx, id, 2, 0.0f)) {
		return false;
	}

	setup_scaled_table_column(
		ctx,
		"Setting",
		ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_IndentDisable,
		label_width
	);
	setup_scaled_table_column(
		ctx,
		"Value",
		ImGuiTableColumnFlags_WidthStretch,
		input_width
	);
	igTableHeadersRow();
	return true;
}

void end_settings_table() {
	igEndTable();
	igPopStyleVar(1);
}

void begin_config_table_row() {
	igTableNextRow(ImGuiTableRowFlags_None, 0.0f);
}

void begin_setting_row() {
	begin_config_table_row();
	igTableNextColumn();
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

static void draw_labeled_control_label(
	const char *label,
	const char *tooltip,
	config_label_state label_state
) {
	igAlignTextToFramePadding();
	draw_config_label(label, tooltip, label_state);
	igTableNextColumn();
}

static void begin_labeled_control(
	const char *label,
	const char *tooltip,
	config_label_state label_state
) {
	draw_labeled_control_label(label, tooltip, label_state);
	igSetNextItemWidth(-FLT_MIN);
}

static float trailing_button_reserve(
	const DTTR_ImGuiDialogContext *ctx,
	int button_count,
	float button_width
) {
	if (button_count <= 0) {
		return FLT_MIN;
	}

	const ImGuiStyle *style = igGetStyle();
	const float spacing = style ? style->ItemSpacing.x : 0.0f;
	return (float)button_count * DTTR_ImGuiDialog_ScaledFloat(ctx, button_width)
		   + (float)button_count * spacing;
}

static bool binding_row_button(
	const DTTR_ImGuiDialogContext *ctx,
	const char *id,
	const char *label,
	const char *tooltip
) {
	igSameLine(0.0f, -1.0f);
	const bool
		clicked = themed_row_button(ctx, id, label, DTTR_CONFIG_UI_MOD_BINDING_BUTTON_W);
	show_tooltip(tooltip);
	return clicked;
}

static bool binding_value_field(
	const DTTR_ImGuiDialogContext *ctx,
	const char *text,
	float field_width,
	bool capturing,
	const char *tooltip
) {
	const ImVec2_c size = {
		field_width,
		0.0f,
	};

	const ImVec4_c bg = capturing ? DTTR_CONFIG_UI_BUTTON_HOVERED_BG
								  : DTTR_IMGUI_COLOR_STACK_FRAME_BG;
	const ImVec4_c text_color = capturing ? DTTR_CONFIG_UI_CHANGED_LABEL_TEXT_COLOR
										  : DTTR_IMGUI_COLOR_BUTTON_TEXT;
	const ImVec4_c border = capturing ? DTTR_IMGUI_COLOR_LINK
									  : DTTR_CONFIG_UI_TABLE_BORDER_COLOR;

	igPushID_Str("##bindvalue");
	igPushStyleColor_Vec4(ImGuiCol_Text, text_color);
	igPushStyleColor_Vec4(ImGuiCol_Button, bg);
	igPushStyleColor_Vec4(ImGuiCol_ButtonHovered, DTTR_CONFIG_UI_BUTTON_HOVERED_BG);
	igPushStyleColor_Vec4(ImGuiCol_ButtonActive, DTTR_CONFIG_UI_BUTTON_ACTIVE_BG);
	igPushStyleColor_Vec4(ImGuiCol_Border, border);
	igPushStyleVar_Float(
		ImGuiStyleVar_FrameRounding,
		DTTR_ImGuiDialog_ScaledFloat(ctx, 2.0f)
	);
	igPushStyleVar_Float(ImGuiStyleVar_FrameBorderSize, 1.0f);
	igPushStyleVar_Vec2(ImGuiStyleVar_ButtonTextAlign, (ImVec2_c){0.0f, 0.5f});

	const bool clicked = igButton(text, size);

	igPopStyleVar(3);
	igPopStyleColor(5);
	igPopID();
	show_tooltip(tooltip);
	return clicked;
}

config_binding_row_result draw_config_binding_row(
	const DTTR_ImGuiDialogContext *ctx,
	const config_binding_row_spec *spec
) {
	config_binding_row_result result = {0};
	if (!spec) {
		return result;
	}

	begin_setting_row();
	igAlignTextToFramePadding();
	draw_config_label(spec->label, spec->tooltip, spec->label_state);
	igTableNextColumn();

	const int trailing_buttons = (spec->show_clear_button ? 1 : 0)
								 + (spec->show_reset_button ? 1 : 0);
	const float reserve = trailing_button_reserve(
		ctx,
		trailing_buttons,
		DTTR_CONFIG_UI_MOD_BINDING_BUTTON_W
	);

	result.bind_clicked = binding_value_field(
		ctx,
		spec->capturing
			? (spec->capture_display ? spec->capture_display : "Press any input...")
			: (spec->display ? spec->display : "Unbound"),
		-reserve,
		spec->capturing,
		spec->bind_tooltip ? spec->bind_tooltip
						   : "Click, then press a key, mouse button, or gamepad button."
	);

	if (spec->show_clear_button) {
		result.clear_clicked = binding_row_button(
			ctx,
			"##clear",
			"Clear",
			spec->clear_tooltip ? spec->clear_tooltip : "Leave this binding unbound."
		);
	}

	if (spec->show_reset_button) {
		result.reset_clicked = binding_row_button(
			ctx,
			"##reset",
			"Reset",
			spec->reset_tooltip ? spec->reset_tooltip : "Reset to the default binding."
		);
	}

	return result;
}

static void draw_path_picker_button(
	const DTTR_ImGuiDialogContext *ctx,
	config_ui_state *state,
	const char *id,
	const char *label,
	const char *tooltip,
	config_path_dialog_fn open_dialog
) {
	igSameLine(0.0f, -1.0f);

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

	draw_labeled_control_label(label, tooltip, label_state);
	igPushID_Str(id);

	const float reserve = trailing_button_reserve(
		ctx,
		button_count,
		DTTR_CONFIG_UI_PATH_BUTTON_W
	);

	igSetNextItemWidth(-reserve);
	const bool edited = igInputText(
		"##value",
		buf,
		buf_size,
		ImGuiInputTextFlags_None,
		NULL,
		NULL
	);
	show_tooltip(tooltip);
	draw_path_picker_buttons(ctx, state, buttons, button_count);
	igPopID();
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
	begin_labeled_control(label, tooltip, label_state);
	const bool
		edited = igInputText(id, buf, buf_size, ImGuiInputTextFlags_None, NULL, NULL);
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
	begin_labeled_control(label, tooltip, label_state);
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
	begin_labeled_control(label, tooltip, label_state);
	const bool
		edited = igInputFloat(id, value, 0.05f, 0.25f, "%.3f", ImGuiInputTextFlags_None);
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
	begin_labeled_control(label, tooltip, label_state);
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
	begin_labeled_control(label, tooltip, label_state);
	const bool edited = choice_combo(id, value, choices, tooltips);
	show_tooltip(tooltip);
	return edited;
}
