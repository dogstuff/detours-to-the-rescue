#include "gui_internal.h"

static const char *const GAMEPAD_AXIS_TOOLTIPS[] = {
	"Disables this gamepad axis mapping.",
	"Uses the left stick horizontal axis.",
	"Uses the left stick vertical axis.",
	"Uses the right stick horizontal axis.",
	"Uses the right stick vertical axis.",
	"Uses the left trigger axis.",
	"Uses the right trigger axis.",
};

static const char *TOOLTIP_GAMEPAD_ENABLED = "Enable controller input. Default: true.";
static const char *TOOLTIP_GAMEPAD_INDEX = "Controller index, starting at 0. Default: 0.";
static const char *TOOLTIP_GAMEPAD_AXIS = "SDL axis for this DttR control. Default: "
										  "movement uses the left stick; camera RZ "
										  "uses the right stick X axis.";
static const char *TOOLTIP_GAMEPAD_DEADZONE = "Per-axis deadzone in scaled axis units. "
											  "Default: 700.";
static const char *TOOLTIP_GAMEPAD_BUTTONS = "Map controller inputs to game actions. "
											 "Bind waits for a button; Clear leaves the "
											 "action unused.";
static const char *TOOLTIP_GAMEPAD_SOUTH = "Default menu confirm action.";
static const char *TOOLTIP_GAMEPAD_EAST = "Default menu back action.";
static const char *TOOLTIP_GAMEPAD_START = "Default start/pause action.";
static const char *TOOLTIP_BIND_BUTTON = "Click, then press a gamepad button or "
										 "trigger to remap this SDL source row.";
static const char *TOOLTIP_CLEAR_BUTTON = "Set this SDL input mapping to none.";
static const char *TOOLTIP_RESET_BUTTON = "Reset this game action to its default "
										  "controller input.";
static const char *const GAMEPAD_SOURCE_UNKNOWN = "Unknown";

typedef struct {
	const char *label;
	const char *id;
	int axis_index;
} gamepad_axis_field;

static const gamepad_axis_field GAMEPAD_AXIS_FIELDS[] = {
	{"Stick X axis", "##axis_stick_x", DTTR_GAMEPAD_AXIS_IDX_STICK_X},
	{"Stick Y axis", "##axis_stick_y", DTTR_GAMEPAD_AXIS_IDX_STICK_Y},
	{"Camera RZ axis", "##axis_camera_rz", DTTR_GAMEPAD_AXIS_IDX_CAMERA_RZ},
};

static const gamepad_axis_field GAMEPAD_DEADZONE_FIELDS[] = {
	{"Stick X deadzone", "##deadzone_stick_x", DTTR_GAMEPAD_AXIS_IDX_STICK_X},
	{"Stick Y deadzone", "##deadzone_stick_y", DTTR_GAMEPAD_AXIS_IDX_STICK_Y},
	{"Camera RZ deadzone", "##deadzone_camera_rz", DTTR_GAMEPAD_AXIS_IDX_CAMERA_RZ},
};

static config_label_state gamepad_axis_label_state(
	const config_ui_state *state,
	const int *saved_values,
	const int *default_values,
	int axis_index
) {
	return make_config_label_state(
		state->config.gamepad_axes[axis_index] != saved_values[axis_index],
		state->config.gamepad_axes[axis_index] != default_values[axis_index]
	);
}

static config_label_state gamepad_deadzone_label_state(
	const config_ui_state *state,
	const int *saved_values,
	const int *default_values,
	int axis_index
) {
	return make_config_label_state(
		state->config.gamepad_axis_deadzone[axis_index] != saved_values[axis_index],
		state->config.gamepad_axis_deadzone[axis_index] != default_values[axis_index]
	);
}

static void draw_gamepad_axis_choice(
	const DTTR_ImGuiDialogContext *ctx,
	config_ui_state *state,
	const char *label,
	const char *id,
	int axis_index
) {
	labeled_choice_combo(
		ctx,
		label,
		id,
		&state->config.gamepad_axes[axis_index],
		DTTR_CONFIG_CHOICES_GAMEPAD_AXIS,
		GAMEPAD_AXIS_TOOLTIPS,
		TOOLTIP_GAMEPAD_AXIS,
		gamepad_axis_label_state(
			state,
			state->saved_config.gamepad_axes,
			state->defaults.gamepad_axes,
			axis_index
		)
	);
}

static void draw_gamepad_deadzone_input(
	const DTTR_ImGuiDialogContext *ctx,
	config_ui_state *state,
	const char *label,
	const char *id,
	int axis_index
) {
	labeled_input_int(
		ctx,
		label,
		id,
		&state->config.gamepad_axis_deadzone[axis_index],
		10,
		100,
		TOOLTIP_GAMEPAD_DEADZONE,
		gamepad_deadzone_label_state(
			state,
			state->saved_config.gamepad_axis_deadzone,
			state->defaults.gamepad_axis_deadzone,
			axis_index
		)
	);
}

const char *source_label(int source) {
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
		return GAMEPAD_SOURCE_UNKNOWN;
	}

	const char *label = SDL_GetGamepadStringForButton((SDL_GamepadButton)source);
	return label ? label : GAMEPAD_SOURCE_UNKNOWN;
}

const char *source_tooltip(int source) {
	switch (source) {
	case DTTR_GAMEPAD_MAPPING_NONE:
		return "No controller input is bound to this game action.";
	case SDL_GAMEPAD_BUTTON_SOUTH:
		return TOOLTIP_GAMEPAD_SOUTH;
	case SDL_GAMEPAD_BUTTON_EAST:
		return TOOLTIP_GAMEPAD_EAST;
	case SDL_GAMEPAD_BUTTON_START:
		return TOOLTIP_GAMEPAD_START;
	default:
		return TOOLTIP_GAMEPAD_BUTTONS;
	}
}

int source_from_event(const SDL_Event *event) {
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

static void draw_gamepad_axes(const DTTR_ImGuiDialogContext *ctx, config_ui_state *state) {
	if (!begin_settings_table(
			ctx,
			"##gamepad_axes_table",
			DTTR_CONFIG_UI_LABEL_W,
			DTTR_CONFIG_UI_INPUT_W
		)) {
		return;
	}

	labeled_checkbox(
		ctx,
		"Enable gamepad",
		"##gamepad_enabled",
		&state->config.gamepad_enabled,
		TOOLTIP_GAMEPAD_ENABLED,
		FIELD_LABEL_STATE(state, gamepad_enabled)
	);
	labeled_input_int(
		ctx,
		"Gamepad index",
		"##gamepad_index",
		&state->config.gamepad_index,
		1,
		1,
		TOOLTIP_GAMEPAD_INDEX,
		FIELD_LABEL_STATE(state, gamepad_index)
	);

	for (int i = 0; i < (int)SDL_arraysize(GAMEPAD_AXIS_FIELDS); i++) {
		draw_gamepad_axis_choice(
			ctx,
			state,
			GAMEPAD_AXIS_FIELDS[i].label,
			GAMEPAD_AXIS_FIELDS[i].id,
			GAMEPAD_AXIS_FIELDS[i].axis_index
		);
	}

	for (int i = 0; i < (int)SDL_arraysize(GAMEPAD_DEADZONE_FIELDS); i++) {
		draw_gamepad_deadzone_input(
			ctx,
			state,
			GAMEPAD_DEADZONE_FIELDS[i].label,
			GAMEPAD_DEADZONE_FIELDS[i].id,
			GAMEPAD_DEADZONE_FIELDS[i].axis_index
		);
	}

	end_settings_table();
}

static void draw_gamepad_button_row(
	const DTTR_ImGuiDialogContext *ctx,
	config_ui_state *state,
	int row
) {
	igPushID_Int(row);

	const int action = state->button_actions[row];
	const int source = state->button_sources[row];

	begin_config_table_row();
	igTableNextColumn();
	igAlignTextToFramePadding();
	draw_config_label(
		gamepad_button_row_label(row),
		game_action_tooltip(action),
		gamepad_button_label_state(state, source, action)
	);

	igTableNextColumn();
	igAlignTextToFramePadding();
	igText("%s", source_label(source));
	show_tooltip(source_tooltip(source));

	igTableNextColumn();

	if (themed_row_button(ctx, "##bind", "Bind", DTTR_CONFIG_UI_GAMEPAD_BUTTON_W)) {
		state->binding_row = row;
		set_status(state, "Press a gamepad button or trigger. Press Esc to cancel.");
	}

	show_tooltip(TOOLTIP_BIND_BUTTON);

	igTableNextColumn();

	if (themed_row_button(ctx, "##clear", "Clear", DTTR_CONFIG_UI_GAMEPAD_BUTTON_W)) {
		state->button_sources[row] = DTTR_GAMEPAD_MAPPING_NONE;
	}

	show_tooltip(TOOLTIP_CLEAR_BUTTON);

	igTableNextColumn();

	if (themed_row_button(ctx, "##reset", "Reset", DTTR_CONFIG_UI_GAMEPAD_BUTTON_W)) {
		state->button_sources[row] = gamepad_default_source_for_action(state, action);
	}

	show_tooltip(TOOLTIP_RESET_BUTTON);

	igPopID();
}

static void draw_gamepad_buttons(
	const DTTR_ImGuiDialogContext *ctx,
	config_ui_state *state
) {
	add_scaled_vertical_spacing(ctx, DTTR_CONFIG_UI_SECTION_SPACING);
	igSeparatorText("Button mappings");
	show_tooltip(TOOLTIP_GAMEPAD_BUTTONS);

	if (state->binding_row >= 0) {
		igText(
			"Waiting for input for %s...",
			gamepad_button_row_label(state->binding_row)
		);
	}

	if (!begin_gamepad_button_table(ctx)) {
		return;
	}

	for (int i = 0; i < gamepad_button_row_count(); i++) {
		draw_gamepad_button_row(ctx, state, i);
	}

	end_settings_table();
}

void draw_gamepad_tab(const DTTR_ImGuiDialogContext *ctx, config_ui_state *state) {
	if (!igBeginChild_Str(
			"##gamepad_scroll",
			(ImVec2_c){0.0f, 0.0f},
			ImGuiChildFlags_None,
			ImGuiWindowFlags_None
		)) {
		igEndChild();
		return;
	}

	draw_gamepad_axes(ctx, state);
	draw_gamepad_buttons(ctx, state);
	igEndChild();
}
