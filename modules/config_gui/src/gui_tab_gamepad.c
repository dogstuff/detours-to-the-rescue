#include "gui_internal.h"

#include <dttr_gamepad_mapping.h>
#include <dttr_input_names.h>
#include <float.h>
#include <math.h>

static const char *const GAMEPAD_AXIS_TOOLTIPS[] = {
	"Disables this gamepad axis mapping.",
	"Left stick horizontal axis.",
	"Left stick vertical axis.",
	"Right stick horizontal axis.",
	"Right stick vertical axis.",
	"Left trigger axis.",
	"Right trigger axis.",
};

static const char *TOOLTIP_GAMEPAD_ENABLED = "Enable controller input. Default: true.";
static const char *TOOLTIP_GAMEPAD_INDEX = "Controller index, starting at 0. Default: 0.";
static const char *TOOLTIP_GAMEPAD_ANALOG_REMAP = "Applies PS1-style left-stick scaling. "
												  "Default: true.";
static const char *TOOLTIP_GAMEPAD_AXIS = "SDL axis for this DttR control. Default: "
										  "movement uses the left stick; camera RZ "
										  "uses the right stick X axis.";
static const char *TOOLTIP_GAMEPAD_DEADZONE = "Per-axis deadzone in scaled axis units. "
											  "Default: sticks 333, camera 600.";
static const char
	*TOOLTIP_GAMEPAD_SENSITIVITY = "Axis sensitivity as a percentage. Lower values "
								   "soften movement; higher values reach "
								   "full input sooner.";
static const char
	*TOOLTIP_GAMEPAD_STICK_POSITION = "Live position from the configured "
									  "Stick X/Y axes with the deadzone circled in red.";
static const char *TOOLTIP_GAMEPAD_CAMERA_RZ_POSITION
	= "Live position from the configured Camera RZ / Pan axis with its deadzone circled "
	  "in red. Camera controls must still be bound manually using this axis in the game controls menu.";
static const char *TOOLTIP_CONTROL_BINDINGS
	= "Click a binding, then press a keyboard key or gamepad button. "
	  "This behavior cannot be bound from the in-game controls menu.";
static const char *TOOLTIP_CONTROL_BINDING_DEFAULT = "Uses the game's pcdogs.ini/default "
													 "binding. Click to override.";
#define GAMEPAD_AXIS_PREVIEW_SIZE 72.0f
#define GAMEPAD_AXIS_PREVIEW_DOT_RADIUS 4.0f
#define GAMEPAD_AXIS_PREVIEW_SEGMENTS 48
#define GAMEPAD_DINPUT_AXIS_SCALE 32.0f
#define GAMEPAD_SDL_AXIS_MAX 32767.0f
#define GAMEPAD_AXIS_COMPACT_COMBO_W 116.0f
#define GAMEPAD_AXIS_COMPACT_INT_W 48.0f

typedef struct {
	const char *label;
	const char *id;
	int axis_index;
} gamepad_axis_field;

static const gamepad_axis_field GAMEPAD_AXIS_FIELDS[] = {
	{"Stick X", "##axis_stick_x", DTTR_GAMEPAD_AXIS_IDX_STICK_X},
	{"Stick Y", "##axis_stick_y", DTTR_GAMEPAD_AXIS_IDX_STICK_Y},
	{"Camera RZ (Pan)", "##axis_camera_rz", DTTR_GAMEPAD_AXIS_IDX_CAMERA_RZ},
};

static void reset_preview_gamepad(config_ui_state *state) {
	state->preview_gamepad = NULL;
	state->preview_gamepad_index = -1;
}

void close_gamepad_preview(config_ui_state *state) {
	if (state->preview_gamepad) {
		SDL_CloseGamepad(state->preview_gamepad);
	}

	reset_preview_gamepad(state);
}

static void open_preview_gamepad(config_ui_state *state) {
	int count = 0;
	SDL_JoystickID *joysticks = SDL_GetGamepads(&count);
	const int index = state->config.gamepad_index;

	if (!joysticks || index < 0 || index >= count) {
		SDL_free(joysticks);
		return;
	}

	state->preview_gamepad = SDL_OpenGamepad(joysticks[index]);
	state->preview_gamepad_index = index;
	SDL_free(joysticks);
}

static SDL_Gamepad *configured_preview_gamepad(config_ui_state *state) {
	const bool stale = state->preview_gamepad
					   && (state->preview_gamepad_index != state->config.gamepad_index
						   || !SDL_GamepadConnected(state->preview_gamepad));

	if (stale) {
		close_gamepad_preview(state);
	}

	if (!state->preview_gamepad) {
		open_preview_gamepad(state);
	}

	return state->preview_gamepad;
}

static float clampf(float value, float lo, float hi) {
	if (value < lo) {
		return lo;
	}

	if (value > hi) {
		return hi;
	}

	return value;
}

static float clamp_unit_float(float value) {
	return clampf(value, -1.0f, 1.0f);
}

static float clamp_radius(float value) {
	return clampf(value, 0.0f, 1.0f);
}

static int32_t preview_axis_dinput(SDL_Gamepad *gamepad, int axis) {
	if (!gamepad || axis == DTTR_GAMEPAD_MAPPING_NONE) {
		return 0;
	}

	return SDL_GetGamepadAxis(gamepad, (SDL_GamepadAxis)axis)
		   / (int32_t)GAMEPAD_DINPUT_AXIS_SCALE;
}

static float dinput_axis_unit(int32_t axis) {
	return clamp_unit_float((float)axis / (float)DTTR_INPUTS_DINPUT_RANGE);
}

static float ps1_axis_unit(int32_t axis) {
	return clamp_unit_float((float)axis / (float)DTTR_INPUTS_PS1_FULL_SCALE);
}

static int32_t normal_dinput_axis_from_config(int32_t axis, int deadzone, int sensitivity) {
	const int32_t value = dttr_inputs_scale_dinput_axis(axis, sensitivity);
	return (value > -deadzone && value < deadzone) ? 0 : value;
}

static float configured_deadzone_radius(const config_ui_state *state, int axis_index) {
	return clamp_radius(
		(float)state->config.gamepad_axis_deadzone[axis_index] * GAMEPAD_DINPUT_AXIS_SCALE
		/ GAMEPAD_SDL_AXIS_MAX
	);
}

static float configured_stick_deadzone_radius(const config_ui_state *state) {
	const float x = configured_deadzone_radius(state, DTTR_GAMEPAD_AXIS_IDX_STICK_X);
	const float y = configured_deadzone_radius(state, DTTR_GAMEPAD_AXIS_IDX_STICK_Y);
	return x > y ? x : y;
}

static ImU32 axis_position_dot_color(float x, float y, float deadzone_radius) {
	const float magnitude = clamp_radius(sqrtf(x * x + y * y));
	const float deadzone = clamp_radius(deadzone_radius);
	const float active_range = 1.0f - deadzone;
	const float active_percent = active_range > 0.0f
									 ? clamp_radius((magnitude - deadzone) / active_range)
									 : (magnitude > deadzone ? 1.0f : 0.0f);
	const ImVec4_c inactive = {0.18f, 0.18f, 0.18f, 1.0f};
	const ImVec4_c active_start = {1.0f, 0.88f, 0.18f, 1.0f};
	const ImVec4_c active_end = {0.25f, 0.95f, 0.35f, 1.0f};
	const ImVec4_c active = {
		active_start.x + (active_end.x - active_start.x) * active_percent,
		active_start.y + (active_end.y - active_start.y) * active_percent,
		active_start.z + (active_end.z - active_start.z) * active_percent,
		1.0f,
	};
	const ImVec4_c color = active_percent > 0.0f ? active : inactive;

	return igGetColorU32_Vec4(color);
}

static bool gamepad_axis_mapping_changed(
	const DTTR_Config *left,
	const DTTR_Config *right,
	int axis_index
) {
	return left->gamepad_axes[axis_index] != right->gamepad_axes[axis_index]
		   || left->gamepad_axis_deadzone[axis_index]
				  != right->gamepad_axis_deadzone[axis_index]
		   || left->gamepad_axis_sensitivity[axis_index]
				  != right->gamepad_axis_sensitivity[axis_index];
}

static config_label_state gamepad_axis_mapping_label_state(
	const config_ui_state *state,
	int axis_index
) {
	return make_config_label_state(
		gamepad_axis_mapping_changed(&state->config, &state->saved_config, axis_index),
		gamepad_axis_mapping_changed(&state->config, &state->defaults, axis_index)
	);
}

static void draw_axis_field_label(const char *label, const char *tooltip) {
	igAlignTextToFramePadding();
	igTextUnformatted(label, NULL);
	show_tooltip(tooltip);
}

static void draw_compact_axis_int(int *value, const char *id, const char *tooltip) {
	igSetNextItemWidth(-FLT_MIN);
	igInputInt(id, value, 0, 0, ImGuiInputTextFlags_None);
	show_tooltip(tooltip);
}

static bool begin_axis_mapping_fields(const DTTR_ImGuiDialogContext *ctx) {
	const ImGuiTableFlags flags = ImGuiTableFlags_SizingFixedFit
								  | ImGuiTableFlags_NoSavedSettings
								  | ImGuiTableFlags_NoPadOuterX;
	if (!igBeginTable("##axis_mapping_fields", 5, flags, (ImVec2_c){0.0f, 0.0f}, 0.0f)) {
		return false;
	}

	igTableSetupColumn(
		"Axis",
		ImGuiTableColumnFlags_WidthFixed,
		DTTR_ImGuiDialog_ScaledFloat(ctx, GAMEPAD_AXIS_COMPACT_COMBO_W),
		0
	);

	igTableSetupColumn("Deadzone Label", ImGuiTableColumnFlags_WidthFixed, 0.0f, 0);

	igTableSetupColumn(
		"Deadzone",
		ImGuiTableColumnFlags_WidthFixed,
		DTTR_ImGuiDialog_ScaledFloat(ctx, GAMEPAD_AXIS_COMPACT_INT_W),
		0
	);

	igTableSetupColumn("Sensitivity Label", ImGuiTableColumnFlags_WidthFixed, 0.0f, 0);

	igTableSetupColumn(
		"Sensitivity",
		ImGuiTableColumnFlags_WidthFixed,
		DTTR_ImGuiDialog_ScaledFloat(ctx, GAMEPAD_AXIS_COMPACT_INT_W),
		0
	);

	igTableNextRow(ImGuiTableRowFlags_None, 0.0f);

	return true;
}

static void draw_gamepad_axis_mapping_row(
	const DTTR_ImGuiDialogContext *ctx,
	config_ui_state *state,
	const gamepad_axis_field *axis
) {
	begin_setting_row();
	igAlignTextToFramePadding();
	draw_config_label(
		axis->label,
		TOOLTIP_GAMEPAD_AXIS,
		gamepad_axis_mapping_label_state(state, axis->axis_index)
	);

	igTableNextColumn();
	if (!begin_axis_mapping_fields(ctx)) {
		return;
	}

	igTableNextColumn();
	igSetNextItemWidth(-FLT_MIN);
	choice_combo(
		axis->id,
		&state->config.gamepad_axes[axis->axis_index],
		DTTR_CONFIG_CHOICES_GAMEPAD_AXIS,
		GAMEPAD_AXIS_TOOLTIPS
	);
	show_tooltip(TOOLTIP_GAMEPAD_AXIS);

	igTableNextColumn();
	draw_axis_field_label("Deadzone", TOOLTIP_GAMEPAD_DEADZONE);

	igTableNextColumn();
	draw_compact_axis_int(
		&state->config.gamepad_axis_deadzone[axis->axis_index],
		"##deadzone",
		TOOLTIP_GAMEPAD_DEADZONE
	);

	igTableNextColumn();
	draw_axis_field_label("Sensitivity", TOOLTIP_GAMEPAD_SENSITIVITY);

	igTableNextColumn();
	draw_compact_axis_int(
		&state->config.gamepad_axis_sensitivity[axis->axis_index],
		"##sensitivity",
		TOOLTIP_GAMEPAD_SENSITIVITY
	);
	igEndTable();
}

static void draw_axis_position_preview(
	const DTTR_ImGuiDialogContext *ctx,
	float x,
	float y,
	const char *id,
	const char *tooltip,
	float deadzone_radius,
	bool has_gamepad
) {
	const float size = DTTR_ImGuiDialog_ScaledFloat(ctx, GAMEPAD_AXIS_PREVIEW_SIZE);
	const float radius = size * 0.5f;
	const float dot_radius = DTTR_ImGuiDialog_ScaledFloat(
		ctx,
		GAMEPAD_AXIS_PREVIEW_DOT_RADIUS
	);
	const float thickness = DTTR_ImGuiDialog_ScaledFloat(ctx, 1.5f);
	const ImVec2_c cursor = igGetCursorScreenPos();
	const ImVec2_c center = {
		cursor.x + radius,
		cursor.y + radius,
	};
	const ImVec2_c outer_max = {
		cursor.x + size,
		cursor.y + size,
	};
	const ImVec2_c dot = {
		center.x + x * (radius - dot_radius),
		center.y + y * (radius - dot_radius),
	};
	ImDrawList *draw_list = igGetWindowDrawList();
	const ImU32 outline_color = igGetColorU32_Col(ImGuiCol_Border, 1.0f);
	const ImU32 deadzone_color = igGetColorU32_Vec4((ImVec4_c){1.0f, 0.18f, 0.18f, 1.0f});
	const ImU32 dot_color = axis_position_dot_color(x, y, deadzone_radius);

	igInvisibleButton(id, (ImVec2_c){size, size}, ImGuiButtonFlags_None);
	ImDrawList_AddRect(
		draw_list,
		cursor,
		outer_max,
		outline_color,
		0.0f,
		ImDrawFlags_None,
		thickness
	);
	ImDrawList_AddCircle(
		draw_list,
		center,
		radius * clamp_radius(deadzone_radius),
		deadzone_color,
		GAMEPAD_AXIS_PREVIEW_SEGMENTS,
		thickness
	);
	ImDrawList_AddCircleFilled(
		draw_list,
		dot,
		dot_radius,
		dot_color,
		GAMEPAD_AXIS_PREVIEW_SEGMENTS
	);

	if (!has_gamepad) {
		show_tooltip("No configured gamepad is connected.");
		return;
	}

	show_tooltip(tooltip);
}

typedef struct {
	float raw_x;
	float raw_y;
	float preview_x;
	float preview_y;
	float mapped_x;
	float mapped_y;
} axis_preview_values;

static void draw_axis_position_readout(const axis_preview_values *values) {
	const float raw_distance = clamp_radius(
		sqrtf(values->raw_x * values->raw_x + values->raw_y * values->raw_y)
	);
	const float mapped_distance = clamp_radius(
		sqrtf(values->mapped_x * values->mapped_x + values->mapped_y * values->mapped_y)
	);

	igText("Raw: % .2f, % .2f", values->raw_x, values->raw_y);
	igText("Raw Distance: %.2f", raw_distance);
	igText("Mapped: % .2f, % .2f", values->mapped_x, values->mapped_y);
	igText("Mapped Distance: %.2f", mapped_distance);
}

static void draw_axis_position_group(
	const DTTR_ImGuiDialogContext *ctx,
	const char *label,
	const axis_preview_values *values,
	const char *id,
	const char *tooltip,
	float deadzone_radius,
	bool has_gamepad
) {
	igPushID_Str(id);
	igBeginGroup();
	igTextUnformatted(label, NULL);
	show_tooltip(tooltip);

	if (igBeginTable(
			"##axis_position",
			2,
			ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_NoSavedSettings
				| ImGuiTableFlags_NoPadOuterX,
			(ImVec2_c){0.0f, 0.0f},
			0.0f
		)) {
		igTableSetupColumn(
			"Preview",
			ImGuiTableColumnFlags_WidthFixed,
			DTTR_ImGuiDialog_ScaledFloat(ctx, GAMEPAD_AXIS_PREVIEW_SIZE),
			0
		);

		igTableSetupColumn("Readout", ImGuiTableColumnFlags_WidthStretch, 1.0f, 0);

		igTableNextRow(ImGuiTableRowFlags_None, 0.0f);

		igTableNextColumn();

		draw_axis_position_preview(
			ctx,
			values->preview_x,
			values->preview_y,
			"##preview",
			tooltip,
			deadzone_radius,
			has_gamepad
		);

		igTableNextColumn();

		draw_axis_position_readout(values);

		igEndTable();
	}
	igEndGroup();
	igPopID();
}

static void draw_axis_positions(
	const DTTR_ImGuiDialogContext *ctx,
	config_ui_state *state
) {
	SDL_Gamepad *gamepad = configured_preview_gamepad(state);
	const bool has_gamepad = gamepad != NULL;
	const int *axes = state->config.gamepad_axes;
	const int *deadzone = state->config.gamepad_axis_deadzone;
	const int *sensitivity = state->config.gamepad_axis_sensitivity;
	const int32_t raw_stick_x_dinput = preview_axis_dinput(
		gamepad,
		axes[DTTR_GAMEPAD_AXIS_IDX_STICK_X]
	);
	const int32_t raw_stick_y_dinput = preview_axis_dinput(
		gamepad,
		axes[DTTR_GAMEPAD_AXIS_IDX_STICK_Y]
	);
	const int32_t raw_camera_rz_dinput = preview_axis_dinput(
		gamepad,
		axes[DTTR_GAMEPAD_AXIS_IDX_CAMERA_RZ]
	);
	const int32_t stick_x_dinput = dttr_inputs_scale_dinput_axis(
		raw_stick_x_dinput,
		sensitivity[DTTR_GAMEPAD_AXIS_IDX_STICK_X]
	);
	const int32_t stick_y_dinput = dttr_inputs_scale_dinput_axis(
		raw_stick_y_dinput,
		sensitivity[DTTR_GAMEPAD_AXIS_IDX_STICK_Y]
	);
	const int32_t camera_rz_dinput = normal_dinput_axis_from_config(
		raw_camera_rz_dinput,
		deadzone[DTTR_GAMEPAD_AXIS_IDX_CAMERA_RZ],
		sensitivity[DTTR_GAMEPAD_AXIS_IDX_CAMERA_RZ]
	);
	axis_preview_values stick = {
		.raw_x = dinput_axis_unit(raw_stick_x_dinput),
		.raw_y = dinput_axis_unit(raw_stick_y_dinput),
		.preview_x = dinput_axis_unit(stick_x_dinput),
		.preview_y = dinput_axis_unit(stick_y_dinput),
	};
	axis_preview_values camera = {
		.raw_x = dinput_axis_unit(raw_camera_rz_dinput),
		.preview_x = dinput_axis_unit(camera_rz_dinput),
		.mapped_x = dinput_axis_unit(camera_rz_dinput),
	};
	const float stick_deadzone = configured_stick_deadzone_radius(state);
	const float camera_rz_deadzone = configured_deadzone_radius(
		state,
		DTTR_GAMEPAD_AXIS_IDX_CAMERA_RZ
	);

	if (state->config.gamepad_analog_remap) {
		DTTR_StickAxes ps1_axes = {0};
		dttr_inputs_apply_ps1_stick_axes(
			&ps1_axes,
			raw_stick_x_dinput,
			raw_stick_y_dinput,
			dttr_inputs_ps1_deadzone_from_dinput(deadzone[DTTR_GAMEPAD_AXIS_IDX_STICK_X]),
			dttr_inputs_ps1_deadzone_from_dinput(deadzone[DTTR_GAMEPAD_AXIS_IDX_STICK_Y]),
			sensitivity[DTTR_GAMEPAD_AXIS_IDX_STICK_X],
			sensitivity[DTTR_GAMEPAD_AXIS_IDX_STICK_Y]
		);
		stick.mapped_x = ps1_axis_unit(ps1_axes.axis_x);
		stick.mapped_y = ps1_axis_unit(ps1_axes.axis_y);
	} else {
		stick.mapped_x = dinput_axis_unit(normal_dinput_axis_from_config(
			raw_stick_x_dinput,
			deadzone[DTTR_GAMEPAD_AXIS_IDX_STICK_X],
			sensitivity[DTTR_GAMEPAD_AXIS_IDX_STICK_X]
		));
		stick.mapped_y = dinput_axis_unit(normal_dinput_axis_from_config(
			raw_stick_y_dinput,
			deadzone[DTTR_GAMEPAD_AXIS_IDX_STICK_Y],
			sensitivity[DTTR_GAMEPAD_AXIS_IDX_STICK_Y]
		));
	}

	if (!igBeginTable(
			"##axis_position_groups",
			2,
			ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_NoSavedSettings
				| ImGuiTableFlags_NoPadOuterX,
			(ImVec2_c){0.0f, 0.0f},
			0.0f
		)) {
		return;
	}

	igTableSetupColumn("Stick", ImGuiTableColumnFlags_WidthStretch, 1.0f, 0);
	igTableSetupColumn("Camera", ImGuiTableColumnFlags_WidthStretch, 1.0f, 0);
	igTableNextRow(ImGuiTableRowFlags_None, 0.0f);

	igTableNextColumn();
	draw_axis_position_group(
		ctx,
		"Stick",
		&stick,
		"##stick_position_preview",
		TOOLTIP_GAMEPAD_STICK_POSITION,
		stick_deadzone,
		has_gamepad
	);
	igTableNextColumn();
	draw_axis_position_group(
		ctx,
		"Camera",
		&camera,
		"##camera_rz_position_preview",
		TOOLTIP_GAMEPAD_CAMERA_RZ_POSITION,
		camera_rz_deadzone,
		has_gamepad
	);
	igEndTable();
}

static void draw_gamepad_section(
	const DTTR_ImGuiDialogContext *ctx,
	config_ui_state *state
) {
	igSpacing();
	igSeparatorText("Gamepad");

	if (!begin_settings_table(
			ctx,
			"##gamepad_settings_table",
			DTTR_CONFIG_UI_LABEL_W,
			DTTR_CONFIG_UI_INPUT_W
		)) {
		return;
	}

	labeled_checkbox(
		ctx,
		"Enable Gamepad",
		"##gamepad_enabled",
		&state->config.gamepad_enabled,
		TOOLTIP_GAMEPAD_ENABLED,
		FIELD_LABEL_STATE(state, gamepad_enabled)
	);

	labeled_input_int(
		ctx,
		"Gamepad Index",
		"##gamepad_index",
		&state->config.gamepad_index,
		1,
		1,
		TOOLTIP_GAMEPAD_INDEX,
		FIELD_LABEL_STATE(state, gamepad_index)
	);

	labeled_checkbox(
		ctx,
		"Preserve Analog Inputs",
		"##gamepad_analog_remap",
		&state->config.gamepad_analog_remap,
		TOOLTIP_GAMEPAD_ANALOG_REMAP,
		FIELD_LABEL_STATE(state, gamepad_analog_remap)
	);

	for (int i = 0; i < (int)SDL_arraysize(GAMEPAD_AXIS_FIELDS); i++) {
		igPushID_Int(GAMEPAD_AXIS_FIELDS[i].axis_index);
		draw_gamepad_axis_mapping_row(ctx, state, &GAMEPAD_AXIS_FIELDS[i]);
		igPopID();
	}

	end_settings_table();
	igSpacing();
	draw_axis_positions(ctx, state);
}

static config_label_state control_binding_label_state(
	const config_ui_state *state,
	int action
) {
	return make_config_label_state(
		state->config.control_bindings[action]
			!= state->saved_config.control_bindings[action],
		state->config.control_bindings[action] != state->defaults.control_bindings[action]
	);
}

static const char *control_binding_code_name(int code) {
	enum {
		BUFFER_COUNT = 8,
		BUFFER_SIZE = 32,
	};
	static char buffers[BUFFER_COUNT][BUFFER_SIZE];
	static int buffer_index;

	if (code == DTTR_CONFIG_CONTROL_BINDING_NONE) {
		return "Game Default";
	}

	char *buffer = buffers[buffer_index];
	buffer_index = (buffer_index + 1) % BUFFER_COUNT;
	if (DTTR_InputNames_ControlCode(code, buffer, BUFFER_SIZE)) {
		return buffer;
	}

	SDL_snprintf(buffer, BUFFER_SIZE, "Code %d", code);
	return buffer;
}

static void draw_control_binding_row(
	const DTTR_ImGuiDialogContext *ctx,
	config_ui_state *state,
	int action
) {
	const int code = state->config.control_bindings[action];
	const bool capturing = control_binding_field_capturing(state, action);

	igPushID_Int(action);
	const config_binding_row_result row = draw_config_binding_row(
		ctx,
		&(config_binding_row_spec){
			.label = DTTR_Config_ControlActionLabel(action),
			.tooltip = TOOLTIP_CONTROL_BINDINGS,
			.label_state = control_binding_label_state(state, action),
			.display = control_binding_code_name(code),
			.capture_display = "Press any input...",
			.capturing = capturing,
			.show_clear_button = false,
			.show_reset_button = true,
			.bind_tooltip = "Click, then press a keyboard key or gamepad button.",
			.reset_tooltip = TOOLTIP_CONTROL_BINDING_DEFAULT,
		}
	);

	if (row.bind_clicked) {
		begin_control_binding_capture(state, action);
	}

	if (row.reset_clicked) {
		state->config.control_bindings[action] = DTTR_CONFIG_CONTROL_BINDING_NONE;
	}

	igPopID();
}

static void draw_control_bindings(
	const DTTR_ImGuiDialogContext *ctx,
	config_ui_state *state
) {
	if (!begin_settings_table(
			ctx,
			"##control_bindings_table",
			DTTR_CONFIG_UI_LABEL_W,
			config_standard_input_width()
		)) {
		return;
	}

	for (int action = 0; action < DTTR_CONFIG_CONTROL_ACTION_COUNT; action++) {
		if (!DTTR_Config_ControlActionInGameBindable(action)) {
			draw_control_binding_row(ctx, state, action);
		}
	}

	end_settings_table();
}

void draw_controls_tab(const DTTR_ImGuiDialogContext *ctx, config_ui_state *state) {
	if (!igBeginChild_Str(
			"##gamepad_scroll",
			(ImVec2_c){0.0f, 0.0f},
			ImGuiChildFlags_None,
			ImGuiWindowFlags_None
		)) {
		igEndChild();
		return;
	}

	draw_control_bindings(ctx, state);
	draw_gamepad_section(ctx, state);
	igEndChild();
}
