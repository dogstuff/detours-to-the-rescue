#include "gui_internal.h"

static const char *const S_GAMEPAD_AXIS_TOOLTIPS[] = {
	"Disables this gamepad axis mapping.",
	"Uses the left stick horizontal axis.",
	"Uses the left stick vertical axis.",
	"Uses the right stick horizontal axis.",
	"Uses the right stick vertical axis.",
	"Uses the left trigger axis.",
	"Uses the right trigger axis.",
};

static const char *const S_GAME_ACTION_TOOLTIPS[] = {
	"Disables this gamepad input source.",
	"Maps this input to up.",
	"Maps this input to down.",
	"Maps this input to left.",
	"Maps this input to right.",
	"Maps this input to POV up.",
	"Maps this input to POV down.",
	"Maps this input to joy button 1.",
	"Maps this input to joy button 2.",
	"Maps this input to joy button 3.",
	"Maps this input to joy button 4.",
	"Maps this input to joy button 5.",
	"Maps this input to joy button 6.",
	"Maps this input to joy button 7.",
	"Maps this input to joy button 8.",
	"Maps this input to joy button 9.",
	"Maps this input to joy button 10.",
	"Maps this input to joy button 11.",
	"Maps this input to joy button 12.",
	"Maps this input to joy button 13.",
};

static const char *S_TOOLTIP_GAMEPAD_ENABLED = "Whether to enable gamepad input. Set to "
											   "false to disable all gamepad support. "
											   "Default: true.";
static const char *S_TOOLTIP_GAMEPAD_INDEX = "The index of the gamepad to use (starting "
											 "at 0). Default: 0.";
static const char *S_TOOLTIP_GAMEPAD_AXIS
	= "The SDL gamepad axis used for this DttR control axis. Default: stick axes use the "
	  "left stick; camera RZ uses the right stick horizontal axis.";
static const char *S_TOOLTIP_GAMEPAD_DEADZONE = "Per-axis deadzone (scaled axis units, "
												"default 700). Default: 700.";
static const char *S_TOOLTIP_GAMEPAD_BUTTONS = "SDL gamepad to PCDogs joypad mappings. "
											   "Multiple SDL inputs can map to the same "
											   "game action.";
static const char *S_TOOLTIP_GAMEPAD_SOUTH = "joy_1 is always used as the menu confirm "
											 "button.";
static const char *S_TOOLTIP_GAMEPAD_EAST = "joy_2 is always used as the menu back "
											"button.";
static const char *S_TOOLTIP_GAMEPAD_START = "joy_9 is always used as the start/pause "
											 "button.";
static const char *S_TOOLTIP_BIND_BUTTON = "Click, then press a gamepad button or "
										   "trigger to remap this SDL source row.";
static const char *S_TOOLTIP_CLEAR_BUTTON = "Set this SDL input mapping to none.";

typedef struct {
	const char *m_label;
	const char *m_id;
	int m_axis_index;
} S_GamepadAxisField;

static const S_GamepadAxisField S_GAMEPAD_AXIS_FIELDS[] = {
	{"Stick X axis", "##axis_stick_x", DTTR_GAMEPAD_AXIS_IDX_STICK_X},
	{"Stick Y axis", "##axis_stick_y", DTTR_GAMEPAD_AXIS_IDX_STICK_Y},
	{"Camera RZ axis", "##axis_camera_rz", DTTR_GAMEPAD_AXIS_IDX_CAMERA_RZ},
};

static const S_GamepadAxisField S_GAMEPAD_DEADZONE_FIELDS[] = {
	{"Stick X deadzone", "##deadzone_stick_x", DTTR_GAMEPAD_AXIS_IDX_STICK_X},
	{"Stick Y deadzone", "##deadzone_stick_y", DTTR_GAMEPAD_AXIS_IDX_STICK_Y},
	{"Camera RZ deadzone", "##deadzone_camera_rz", DTTR_GAMEPAD_AXIS_IDX_CAMERA_RZ},
};

static void s_draw_gamepad_axis_choice(
	const DTTR_ImGuiDialogContext *ctx,
	S_ConfigUIState *state,
	const char *label,
	const char *id,
	int axis_index
) {
	s_labeled_choice_combo(
		ctx,
		label,
		id,
		&state->m_config.m_gamepad_axes[axis_index],
		DTTR_CONFIG_CHOICES_GAMEPAD_AXIS,
		S_GAMEPAD_AXIS_TOOLTIPS,
		S_TOOLTIP_GAMEPAD_AXIS,
		S_FIELD_LABEL_STATE(state, m_gamepad_axes)
	);
}

static void s_draw_gamepad_deadzone_input(
	const DTTR_ImGuiDialogContext *ctx,
	S_ConfigUIState *state,
	const char *label,
	const char *id,
	int axis_index
) {
	s_labeled_input_int(
		ctx,
		label,
		id,
		&state->m_config.m_gamepad_axis_deadzone[axis_index],
		10,
		100,
		S_TOOLTIP_GAMEPAD_DEADZONE,
		S_FIELD_LABEL_STATE(state, m_gamepad_axis_deadzone)
	);
}

const char *s_source_label(int source) {
	if (source == DTTR_GAMEPAD_SOURCE_TRIGGER_LEFT) {
		return "left_trigger";
	}

	if (source == DTTR_GAMEPAD_SOURCE_TRIGGER_RIGHT) {
		return "right_trigger";
	}

	if (source < 0 || source >= SDL_GAMEPAD_BUTTON_COUNT) {
		return "Unknown";
	}

	const char *label = SDL_GetGamepadStringForButton((SDL_GamepadButton)source);
	return label ? label : "Unknown";
}

const char *s_source_tooltip(int source) {
	switch (source) {
	case SDL_GAMEPAD_BUTTON_SOUTH:
		return S_TOOLTIP_GAMEPAD_SOUTH;
	case SDL_GAMEPAD_BUTTON_EAST:
		return S_TOOLTIP_GAMEPAD_EAST;
	case SDL_GAMEPAD_BUTTON_START:
		return S_TOOLTIP_GAMEPAD_START;
	default:
		return S_TOOLTIP_GAMEPAD_BUTTONS;
	}
}

int s_source_from_event(const SDL_Event *event) {
	switch (event->type) {
	case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
		return event->gbutton.button;
	case SDL_EVENT_GAMEPAD_AXIS_MOTION:
		if (event->gaxis.value < DTTR_GAMEPAD_TRIGGER_THRESHOLD) {
			return -1;
		}

		switch (event->gaxis.axis) {
		case SDL_GAMEPAD_AXIS_LEFT_TRIGGER:
			return DTTR_GAMEPAD_SOURCE_TRIGGER_LEFT;
		case SDL_GAMEPAD_AXIS_RIGHT_TRIGGER:
			return DTTR_GAMEPAD_SOURCE_TRIGGER_RIGHT;
		default:
			return -1;
		}

	default:
		return -1;
	}
}

bool s_event_cancels_binding(const SDL_Event *event) {
	return event->type == SDL_EVENT_KEY_DOWN
		   && event->key.scancode == SDL_SCANCODE_ESCAPE;
}

void s_cancel_binding(S_ConfigUIState *state) {
	if (state->m_binding_row < 0) {
		return;
	}

	state->m_binding_row = -1;
	s_set_status(state, "Cancelled controller input capture.");
}

void s_capture_source(S_ConfigUIState *state, int new_source) {
	const int binding_row = state->m_binding_row;
	if (binding_row < 0 || binding_row >= DTTR_GAMEPAD_SOURCE_COUNT || new_source < 0
		|| new_source >= DTTR_GAMEPAD_SOURCE_COUNT) {
		return;
	}

	const int old_source = state->m_button_sources[binding_row];
	for (int i = 0; i < DTTR_GAMEPAD_SOURCE_COUNT; i++) {
		if (i != binding_row && state->m_button_sources[i] == new_source) {
			state->m_button_sources[i] = old_source;
			break;
		}
	}

	state->m_button_sources[binding_row] = new_source;
	state->m_binding_row = -1;
	s_set_status(state, "Captured controller input.");
}

static void s_draw_gamepad_axes(
	const DTTR_ImGuiDialogContext *ctx,
	S_ConfigUIState *state
) {
	if (!s_begin_full_width_settings_table(
			ctx,
			"##gamepad_axes_table",
			DTTR_CONFIG_UI_LABEL_W,
			DTTR_CONFIG_UI_INPUT_W
		)) {
		return;
	}

	s_labeled_checkbox(
		ctx,
		"Enable gamepad",
		"##gamepad_enabled",
		&state->m_config.m_gamepad_enabled,
		S_TOOLTIP_GAMEPAD_ENABLED,
		S_FIELD_LABEL_STATE(state, m_gamepad_enabled)
	);
	s_labeled_input_int(
		ctx,
		"Gamepad index",
		"##gamepad_index",
		&state->m_config.m_gamepad_index,
		1,
		1,
		S_TOOLTIP_GAMEPAD_INDEX,
		S_FIELD_LABEL_STATE(state, m_gamepad_index)
	);

	for (int i = 0; i < (int)SDL_arraysize(S_GAMEPAD_AXIS_FIELDS); i++) {
		s_draw_gamepad_axis_choice(
			ctx,
			state,
			S_GAMEPAD_AXIS_FIELDS[i].m_label,
			S_GAMEPAD_AXIS_FIELDS[i].m_id,
			S_GAMEPAD_AXIS_FIELDS[i].m_axis_index
		);
	}

	for (int i = 0; i < (int)SDL_arraysize(S_GAMEPAD_DEADZONE_FIELDS); i++) {
		s_draw_gamepad_deadzone_input(
			ctx,
			state,
			S_GAMEPAD_DEADZONE_FIELDS[i].m_label,
			S_GAMEPAD_DEADZONE_FIELDS[i].m_id,
			S_GAMEPAD_DEADZONE_FIELDS[i].m_axis_index
		);
	}

	s_end_settings_table();
}

static void s_draw_gamepad_button_row(
	const DTTR_ImGuiDialogContext *ctx,
	S_ConfigUIState *state,
	int row
) {
	igPushID_Int(row);

	igTableNextRow(ImGuiTableRowFlags_None, 0.0f);
	igTableNextColumn();
	igTableNextColumn();
	igAlignTextToFramePadding();
	const int source = state->m_button_sources[row];
	s_draw_config_label(
		s_source_label(source),
		s_source_tooltip(source),
		s_gamepad_button_label_state(state, source, state->m_button_actions[row])
	);

	igTableNextColumn();
	igSetNextItemWidth(s_table_input_width(ctx, DTTR_CONFIG_UI_GAMEPAD_ACTION_W));
	s_choice_combo(
		"##action",
		&state->m_button_actions[row],
		DTTR_CONFIG_CHOICES_GAME_ACTION,
		S_GAME_ACTION_TOOLTIPS
	);
	s_show_tooltip(s_source_tooltip(source));

	igTableNextColumn();
	if (s_themed_row_button(ctx, "##bind", "Bind", DTTR_CONFIG_UI_GAMEPAD_BUTTON_W)) {
		state->m_binding_row = row;
		s_set_status(state, "Press a gamepad button or trigger. Press Esc to cancel.");
	}

	s_show_tooltip(S_TOOLTIP_BIND_BUTTON);

	igTableNextColumn();
	if (s_themed_row_button(ctx, "##clear", "Clear", DTTR_CONFIG_UI_GAMEPAD_BUTTON_W)) {
		state->m_button_actions[row] = DTTR_GAMEPAD_MAPPING_NONE;
	}

	s_show_tooltip(S_TOOLTIP_CLEAR_BUTTON);

	igTableNextColumn();
	if (s_themed_row_button(ctx, "##reset", "Reset", DTTR_CONFIG_UI_GAMEPAD_BUTTON_W)) {
		state->m_button_actions[row] = state->m_defaults.m_gamepad_button_map[source];
	}

	s_show_tooltip("Reset this binding to its default.");

	igPopID();
}

static void s_draw_gamepad_buttons(
	const DTTR_ImGuiDialogContext *ctx,
	S_ConfigUIState *state
) {
	s_add_scaled_vertical_spacing(ctx, 6.0f);
	igSeparatorText("Button mappings");
	s_show_tooltip(S_TOOLTIP_GAMEPAD_BUTTONS);
	if (state->m_binding_row >= 0) {
		igText("Waiting for input for row %d...", state->m_binding_row + 1);
	}

	if (!s_begin_gamepad_button_table(ctx)) {
		return;
	}

	for (int i = 0; i < DTTR_GAMEPAD_SOURCE_COUNT; i++) {
		s_draw_gamepad_button_row(ctx, state, i);
	}

	s_end_settings_table();
}

void s_draw_gamepad_tab(const DTTR_ImGuiDialogContext *ctx, S_ConfigUIState *state) {
	s_draw_gamepad_axes(ctx, state);
	s_draw_gamepad_buttons(ctx, state);
}
