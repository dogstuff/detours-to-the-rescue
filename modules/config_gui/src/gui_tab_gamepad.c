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
static const char *S_TOOLTIP_GAMEPAD_BUTTONS
	= "Each row maps what you press to what the game does. Click Bind, press a "
	  "controller button, or Clear to leave that action unused.";
static const char *S_TOOLTIP_GAMEPAD_SOUTH = "joy_1 is always used as the menu confirm "
											 "button.";
static const char *S_TOOLTIP_GAMEPAD_EAST = "joy_2 is always used as the menu back "
											"button.";
static const char *S_TOOLTIP_GAMEPAD_START = "joy_9 is always used as the start/pause "
											 "button.";
static const char *S_TOOLTIP_BIND_BUTTON = "Click, then press a gamepad button or "
										   "trigger to remap this SDL source row.";
static const char *S_TOOLTIP_CLEAR_BUTTON = "Set this SDL input mapping to none.";
static const char *S_TOOLTIP_RESET_BUTTON = "Reset this game action to its default "
											"controller input.";

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
	switch (source) {
	case DTTR_GAMEPAD_MAPPING_NONE:
		return "none";
	case DTTR_GAMEPAD_SOURCE_TRIGGER_LEFT:
		return "left_trigger";
	case DTTR_GAMEPAD_SOURCE_TRIGGER_RIGHT:
		return "right_trigger";
	default:
		break;
	}

	if (source < 0 || source >= SDL_GAMEPAD_BUTTON_COUNT) {
		return "Unknown";
	}

	const char *label = SDL_GetGamepadStringForButton((SDL_GamepadButton)source);
	return label ? label : "Unknown";
}

const char *s_source_tooltip(int source) {
	switch (source) {
	case DTTR_GAMEPAD_MAPPING_NONE:
		return "No controller input is bound to this game action.";
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

	const int action = state->m_button_actions[row];
	const int source = state->m_button_sources[row];

	igTableNextRow(ImGuiTableRowFlags_None, 0.0f);
	igTableNextColumn();
	igTableNextColumn();
	igAlignTextToFramePadding();
	s_draw_config_label(
		s_gamepad_button_row_label(row),
		s_game_action_tooltip(action),
		s_gamepad_button_label_state(state, source, action)
	);

	igTableNextColumn();
	igAlignTextToFramePadding();
	igText("%s", s_source_label(source));
	s_show_tooltip(s_source_tooltip(source));

	igTableNextColumn();
	if (s_themed_row_button(ctx, "##bind", "Bind", DTTR_CONFIG_UI_GAMEPAD_BUTTON_W)) {
		state->m_binding_row = row;
		s_set_status(state, "Press a gamepad button or trigger. Press Esc to cancel.");
	}

	s_show_tooltip(S_TOOLTIP_BIND_BUTTON);

	igTableNextColumn();
	if (s_themed_row_button(ctx, "##clear", "Clear", DTTR_CONFIG_UI_GAMEPAD_BUTTON_W)) {
		state->m_button_sources[row] = DTTR_GAMEPAD_MAPPING_NONE;
	}

	s_show_tooltip(S_TOOLTIP_CLEAR_BUTTON);

	igTableNextColumn();
	if (s_themed_row_button(ctx, "##reset", "Reset", DTTR_CONFIG_UI_GAMEPAD_BUTTON_W)) {
		state->m_button_sources[row] = s_gamepad_default_source_for_action(state, action);
	}

	s_show_tooltip(S_TOOLTIP_RESET_BUTTON);

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
		igText(
			"Waiting for input for %s...",
			s_gamepad_button_row_label(state->m_binding_row)
		);
	}

	if (!s_begin_gamepad_button_table(ctx)) {
		return;
	}

	for (int i = 0; i < s_gamepad_button_row_count(); i++) {
		s_draw_gamepad_button_row(ctx, state, i);
	}

	s_end_settings_table();
}

void s_draw_gamepad_tab(const DTTR_ImGuiDialogContext *ctx, S_ConfigUIState *state) {
	s_draw_gamepad_axes(ctx, state);
	s_draw_gamepad_buttons(ctx, state);
}
