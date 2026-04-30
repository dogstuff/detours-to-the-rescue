#include "config_internal.h"
#include <dttr_log.h>

#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DTTR_PARSE_TOKEN(token, enum_value)                                              \
	if (strcmp(value, token) == 0) {                                                     \
		*out_value = enum_value;                                                         \
		return true;                                                                     \
	}

bool s_config_parse_bool(const char *value, bool *out_value) {
	if (!value || !out_value) {
		return false;
	}

	DTTR_PARSE_TOKEN("true", true);
	DTTR_PARSE_TOKEN("false", false);

	return false;
}

bool s_config_parse_scaling_fit(const char *value, DTTR_ScalingMode *out_value) {
	if (!value || !out_value) {
		return false;
	}

	DTTR_PARSE_TOKEN("letterbox", DTTR_SCALING_MODE_LETTERBOX);
	DTTR_PARSE_TOKEN("stretch", DTTR_SCALING_MODE_STRETCH);
	DTTR_PARSE_TOKEN("integer", DTTR_SCALING_MODE_INTEGER);

	return false;
}

bool s_config_parse_scaling_method(const char *value, DTTR_ScalingMethod *out_value) {
	if (!value || !out_value) {
		return false;
	}

	DTTR_PARSE_TOKEN("present", DTTR_SCALING_METHOD_PRESENT);
	DTTR_PARSE_TOKEN("logical", DTTR_SCALING_METHOD_LOGICAL);

	return false;
}

bool s_config_parse_graphics_api(const char *value, DTTR_GraphicsApi *out_value) {
	if (!value || !out_value) {
		return false;
	}

	DTTR_PARSE_TOKEN(DTTR_DRIVER_AUTO, DTTR_GRAPHICS_API_AUTO);
	DTTR_PARSE_TOKEN(DTTR_DRIVER_VULKAN, DTTR_GRAPHICS_API_VULKAN);
	DTTR_PARSE_TOKEN(DTTR_DRIVER_DIRECT3D12, DTTR_GRAPHICS_API_DIRECT3D12);
	DTTR_PARSE_TOKEN(DTTR_DRIVER_DIRECT3D12_SHORT, DTTR_GRAPHICS_API_DIRECT3D12);
	DTTR_PARSE_TOKEN(DTTR_DRIVER_OPENGL, DTTR_GRAPHICS_API_OPENGL);

	return false;
}

bool s_config_parse_int(const char *value, int *out_value) {
	if (!value || !out_value) {
		return false;
	}

	char *end = NULL;
	errno = 0;

	const long parsed = strtol(value, &end, 10);

	if (errno != 0 || !end || *end != '\0') {
		return false;
	}

	if (parsed < INT_MIN || parsed > INT_MAX) {
		return false;
	}

	*out_value = (int)parsed;
	return true;
}

bool s_config_parse_float(const char *value, float *out_value) {
	if (!value || !out_value) {
		return false;
	}

	char *end = NULL;
	errno = 0;

	const float parsed = strtof(value, &end);
	if (errno != 0 || !end || *end != '\0' || !isfinite(parsed)) {
		return false;
	}

	*out_value = parsed;
	return true;
}

bool s_config_parse_present_filter(const char *value, SDL_GPUFilter *out_value) {
	if (!value || !out_value) {
		return false;
	}

	DTTR_PARSE_TOKEN("nearest", SDL_GPU_FILTER_NEAREST);
	DTTR_PARSE_TOKEN("linear", SDL_GPU_FILTER_LINEAR);

	return false;
}

bool s_config_parse_gamepad_source(const char *value, int *out_value) {
	if (!value || !out_value) {
		return false;
	}

	DTTR_PARSE_TOKEN("south", SDL_GAMEPAD_BUTTON_SOUTH);
	DTTR_PARSE_TOKEN("east", SDL_GAMEPAD_BUTTON_EAST);
	DTTR_PARSE_TOKEN("west", SDL_GAMEPAD_BUTTON_WEST);
	DTTR_PARSE_TOKEN("north", SDL_GAMEPAD_BUTTON_NORTH);
	DTTR_PARSE_TOKEN("back", SDL_GAMEPAD_BUTTON_BACK);
	DTTR_PARSE_TOKEN("guide", SDL_GAMEPAD_BUTTON_GUIDE);
	DTTR_PARSE_TOKEN("start", SDL_GAMEPAD_BUTTON_START);
	DTTR_PARSE_TOKEN("left_stick_click", SDL_GAMEPAD_BUTTON_LEFT_STICK);
	DTTR_PARSE_TOKEN("right_stick_click", SDL_GAMEPAD_BUTTON_RIGHT_STICK);
	DTTR_PARSE_TOKEN("left_shoulder", SDL_GAMEPAD_BUTTON_LEFT_SHOULDER);
	DTTR_PARSE_TOKEN("right_shoulder", SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER);
	DTTR_PARSE_TOKEN("dpad_up", SDL_GAMEPAD_BUTTON_DPAD_UP);
	DTTR_PARSE_TOKEN("dpad_down", SDL_GAMEPAD_BUTTON_DPAD_DOWN);
	DTTR_PARSE_TOKEN("dpad_left", SDL_GAMEPAD_BUTTON_DPAD_LEFT);
	DTTR_PARSE_TOKEN("dpad_right", SDL_GAMEPAD_BUTTON_DPAD_RIGHT);
	DTTR_PARSE_TOKEN("misc1", SDL_GAMEPAD_BUTTON_MISC1);
	DTTR_PARSE_TOKEN("right_paddle1", SDL_GAMEPAD_BUTTON_RIGHT_PADDLE1);
	DTTR_PARSE_TOKEN("left_paddle1", SDL_GAMEPAD_BUTTON_LEFT_PADDLE1);
	DTTR_PARSE_TOKEN("right_paddle2", SDL_GAMEPAD_BUTTON_RIGHT_PADDLE2);
	DTTR_PARSE_TOKEN("left_paddle2", SDL_GAMEPAD_BUTTON_LEFT_PADDLE2);
	DTTR_PARSE_TOKEN("touchpad", SDL_GAMEPAD_BUTTON_TOUCHPAD);
	DTTR_PARSE_TOKEN("left_trigger", DTTR_GAMEPAD_SOURCE_TRIGGER_LEFT);
	DTTR_PARSE_TOKEN("right_trigger", DTTR_GAMEPAD_SOURCE_TRIGGER_RIGHT);

	return false;
}

bool s_config_parse_game_action(const char *value, int *out_value) {
	if (!value || !out_value) {
		return false;
	}

	DTTR_PARSE_TOKEN("none", DTTR_GAMEPAD_MAPPING_NONE);
	DTTR_PARSE_TOKEN("up", PCDOGS_GAMEPAD_IDX_UP);
	DTTR_PARSE_TOKEN("down", PCDOGS_GAMEPAD_IDX_DOWN);
	DTTR_PARSE_TOKEN("left", PCDOGS_GAMEPAD_IDX_LEFT);
	DTTR_PARSE_TOKEN("right", PCDOGS_GAMEPAD_IDX_RIGHT);
	DTTR_PARSE_TOKEN("pov_up", PCDOGS_GAMEPAD_IDX_POV_UP);
	DTTR_PARSE_TOKEN("pov_down", PCDOGS_GAMEPAD_IDX_POV_DOWN);
	DTTR_PARSE_TOKEN("joy_1", PCDOGS_GAMEPAD_IDX_BTN_0);
	DTTR_PARSE_TOKEN("joy_2", PCDOGS_GAMEPAD_IDX_BTN_1);
	DTTR_PARSE_TOKEN("joy_3", PCDOGS_GAMEPAD_IDX_BTN_2);
	DTTR_PARSE_TOKEN("joy_4", PCDOGS_GAMEPAD_IDX_BTN_3);
	DTTR_PARSE_TOKEN("joy_5", PCDOGS_GAMEPAD_IDX_BTN_4);
	DTTR_PARSE_TOKEN("joy_6", PCDOGS_GAMEPAD_IDX_BTN_5);
	DTTR_PARSE_TOKEN("joy_7", PCDOGS_GAMEPAD_IDX_BTN_6);
	DTTR_PARSE_TOKEN("joy_8", PCDOGS_GAMEPAD_IDX_BTN_7);
	DTTR_PARSE_TOKEN("joy_9", PCDOGS_GAMEPAD_IDX_BTN_8);
	DTTR_PARSE_TOKEN("joy_10", PCDOGS_GAMEPAD_IDX_BTN_9);
	DTTR_PARSE_TOKEN("joy_11", PCDOGS_GAMEPAD_IDX_BTN_10);
	DTTR_PARSE_TOKEN("joy_12", PCDOGS_GAMEPAD_IDX_BTN_11);
	DTTR_PARSE_TOKEN("joy_13", PCDOGS_GAMEPAD_IDX_BTN_12);

	return false;
}

bool s_config_parse_gamepad_axis(const char *value, int *out_value) {
	if (!value || !out_value) {
		return false;
	}

	DTTR_PARSE_TOKEN("none", DTTR_GAMEPAD_MAPPING_NONE);
	DTTR_PARSE_TOKEN("axis_left_x", SDL_GAMEPAD_AXIS_LEFTX);
	DTTR_PARSE_TOKEN("axis_left_y", SDL_GAMEPAD_AXIS_LEFTY);
	DTTR_PARSE_TOKEN("axis_right_x", SDL_GAMEPAD_AXIS_RIGHTX);
	DTTR_PARSE_TOKEN("axis_right_y", SDL_GAMEPAD_AXIS_RIGHTY);
	DTTR_PARSE_TOKEN("axis_left_trigger", SDL_GAMEPAD_AXIS_LEFT_TRIGGER);
	DTTR_PARSE_TOKEN("axis_right_trigger", SDL_GAMEPAD_AXIS_RIGHT_TRIGGER);

	return false;
}

bool s_config_parse_log_level(const char *value, int *out_value) {
	if (!value || !out_value) {
		return false;
	}

	DTTR_PARSE_TOKEN("trace", LOG_TRACE);
	DTTR_PARSE_TOKEN("debug", LOG_DEBUG);
	DTTR_PARSE_TOKEN("info", LOG_INFO);
	DTTR_PARSE_TOKEN("warn", LOG_WARN);
	DTTR_PARSE_TOKEN("error", LOG_ERROR);
	DTTR_PARSE_TOKEN("fatal", LOG_FATAL);

	return false;
}

bool s_config_parse_minidump_type(const char *value, DTTR_MinidumpType *out_value) {
	if (!value || !out_value) {
		return false;
	}

	DTTR_PARSE_TOKEN("normal", DTTR_MINIDUMP_NORMAL);
	DTTR_PARSE_TOKEN("detailed", DTTR_MINIDUMP_DETAILED);

	return false;
}

bool s_config_parse_vertex_precision(const char *value, DTTR_VertexPrecision *out_value) {
	if (!value || !out_value) {
		return false;
	}

	DTTR_PARSE_TOKEN("native", DTTR_VERTEX_PRECISION_NATIVE);
	DTTR_PARSE_TOKEN("subpixel", DTTR_VERTEX_PRECISION_SUBPIXEL);

	return false;
}

bool s_config_parse_string(const char *value, char *out_value, size_t out_size) {
	if (!value || !out_value || out_size == 0) {
		return false;
	}

	strncpy(out_value, value, out_size - 1);
	out_value[out_size - 1] = '\0';
	return true;
}

#undef DTTR_PARSE_TOKEN

#define DTTR_FORMAT_TOKEN(enum_value, token)                                             \
	case (enum_value):                                                                   \
		return (token);

const char *s_config_format_bool(bool value) { return value ? "true" : "false"; }

void s_config_format_int(int value, char *buf, size_t buf_size) {
	snprintf(buf, buf_size, "%d", value);
}

void s_config_format_float(float value, char *buf, size_t buf_size) {
	snprintf(buf, buf_size, "%.9g", value);
}

const char *s_config_format_scaling_fit(DTTR_ScalingMode mode) {
	switch (mode) {
		DTTR_FORMAT_TOKEN(DTTR_SCALING_MODE_STRETCH, "stretch")
		DTTR_FORMAT_TOKEN(DTTR_SCALING_MODE_INTEGER, "integer")
	default:
		return "letterbox";
	}
}

const char *s_config_format_scaling_method(DTTR_ScalingMethod method) {
	switch (method) {
		DTTR_FORMAT_TOKEN(DTTR_SCALING_METHOD_LOGICAL, "logical")
	default:
		return "present";
	}
}

const char *s_config_format_graphics_api(DTTR_GraphicsApi api) {
	switch (api) {
		DTTR_FORMAT_TOKEN(DTTR_GRAPHICS_API_VULKAN, DTTR_DRIVER_VULKAN)
		DTTR_FORMAT_TOKEN(DTTR_GRAPHICS_API_DIRECT3D12, DTTR_DRIVER_DIRECT3D12)
		DTTR_FORMAT_TOKEN(DTTR_GRAPHICS_API_OPENGL, DTTR_DRIVER_OPENGL)
	default:
		return DTTR_DRIVER_AUTO;
	}
}

const char *s_config_format_present_filter(SDL_GPUFilter filter) {
	switch (filter) {
		DTTR_FORMAT_TOKEN(SDL_GPU_FILTER_NEAREST, "nearest")
	default:
		return "linear";
	}
}

const char *s_config_format_log_level(int level) {
	switch (level) {
		DTTR_FORMAT_TOKEN(LOG_TRACE, "trace")
		DTTR_FORMAT_TOKEN(LOG_DEBUG, "debug")
		DTTR_FORMAT_TOKEN(LOG_INFO, "info")
		DTTR_FORMAT_TOKEN(LOG_WARN, "warn")
		DTTR_FORMAT_TOKEN(LOG_ERROR, "error")
		DTTR_FORMAT_TOKEN(LOG_FATAL, "fatal")
	default:
		return "info";
	}
}

const char *s_config_format_minidump_type(DTTR_MinidumpType type) {
	switch (type) {
		DTTR_FORMAT_TOKEN(DTTR_MINIDUMP_DETAILED, "detailed")
	default:
		return "normal";
	}
}

const char *s_config_format_gamepad_source(int source) {
	switch (source) {
		DTTR_FORMAT_TOKEN(SDL_GAMEPAD_BUTTON_SOUTH, "south")
		DTTR_FORMAT_TOKEN(SDL_GAMEPAD_BUTTON_EAST, "east")
		DTTR_FORMAT_TOKEN(SDL_GAMEPAD_BUTTON_WEST, "west")
		DTTR_FORMAT_TOKEN(SDL_GAMEPAD_BUTTON_NORTH, "north")
		DTTR_FORMAT_TOKEN(SDL_GAMEPAD_BUTTON_BACK, "back")
		DTTR_FORMAT_TOKEN(SDL_GAMEPAD_BUTTON_GUIDE, "guide")
		DTTR_FORMAT_TOKEN(SDL_GAMEPAD_BUTTON_START, "start")
		DTTR_FORMAT_TOKEN(SDL_GAMEPAD_BUTTON_LEFT_STICK, "left_stick_click")
		DTTR_FORMAT_TOKEN(SDL_GAMEPAD_BUTTON_RIGHT_STICK, "right_stick_click")
		DTTR_FORMAT_TOKEN(SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, "left_shoulder")
		DTTR_FORMAT_TOKEN(SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, "right_shoulder")
		DTTR_FORMAT_TOKEN(SDL_GAMEPAD_BUTTON_DPAD_UP, "dpad_up")
		DTTR_FORMAT_TOKEN(SDL_GAMEPAD_BUTTON_DPAD_DOWN, "dpad_down")
		DTTR_FORMAT_TOKEN(SDL_GAMEPAD_BUTTON_DPAD_LEFT, "dpad_left")
		DTTR_FORMAT_TOKEN(SDL_GAMEPAD_BUTTON_DPAD_RIGHT, "dpad_right")
		DTTR_FORMAT_TOKEN(SDL_GAMEPAD_BUTTON_MISC1, "misc1")
		DTTR_FORMAT_TOKEN(SDL_GAMEPAD_BUTTON_RIGHT_PADDLE1, "right_paddle1")
		DTTR_FORMAT_TOKEN(SDL_GAMEPAD_BUTTON_LEFT_PADDLE1, "left_paddle1")
		DTTR_FORMAT_TOKEN(SDL_GAMEPAD_BUTTON_RIGHT_PADDLE2, "right_paddle2")
		DTTR_FORMAT_TOKEN(SDL_GAMEPAD_BUTTON_LEFT_PADDLE2, "left_paddle2")
		DTTR_FORMAT_TOKEN(SDL_GAMEPAD_BUTTON_TOUCHPAD, "touchpad")
		DTTR_FORMAT_TOKEN(DTTR_GAMEPAD_SOURCE_TRIGGER_LEFT, "left_trigger")
		DTTR_FORMAT_TOKEN(DTTR_GAMEPAD_SOURCE_TRIGGER_RIGHT, "right_trigger")
	default:
		return NULL;
	}
}

const char *s_config_format_game_action(int action) {
	switch (action) {
		DTTR_FORMAT_TOKEN(DTTR_GAMEPAD_MAPPING_NONE, "none")
		DTTR_FORMAT_TOKEN(PCDOGS_GAMEPAD_IDX_UP, "up")
		DTTR_FORMAT_TOKEN(PCDOGS_GAMEPAD_IDX_DOWN, "down")
		DTTR_FORMAT_TOKEN(PCDOGS_GAMEPAD_IDX_LEFT, "left")
		DTTR_FORMAT_TOKEN(PCDOGS_GAMEPAD_IDX_RIGHT, "right")
		DTTR_FORMAT_TOKEN(PCDOGS_GAMEPAD_IDX_POV_UP, "pov_up")
		DTTR_FORMAT_TOKEN(PCDOGS_GAMEPAD_IDX_POV_DOWN, "pov_down")
		DTTR_FORMAT_TOKEN(PCDOGS_GAMEPAD_IDX_BTN_0, "joy_1")
		DTTR_FORMAT_TOKEN(PCDOGS_GAMEPAD_IDX_BTN_1, "joy_2")
		DTTR_FORMAT_TOKEN(PCDOGS_GAMEPAD_IDX_BTN_2, "joy_3")
		DTTR_FORMAT_TOKEN(PCDOGS_GAMEPAD_IDX_BTN_3, "joy_4")
		DTTR_FORMAT_TOKEN(PCDOGS_GAMEPAD_IDX_BTN_4, "joy_5")
		DTTR_FORMAT_TOKEN(PCDOGS_GAMEPAD_IDX_BTN_5, "joy_6")
		DTTR_FORMAT_TOKEN(PCDOGS_GAMEPAD_IDX_BTN_6, "joy_7")
		DTTR_FORMAT_TOKEN(PCDOGS_GAMEPAD_IDX_BTN_7, "joy_8")
		DTTR_FORMAT_TOKEN(PCDOGS_GAMEPAD_IDX_BTN_8, "joy_9")
		DTTR_FORMAT_TOKEN(PCDOGS_GAMEPAD_IDX_BTN_9, "joy_10")
		DTTR_FORMAT_TOKEN(PCDOGS_GAMEPAD_IDX_BTN_10, "joy_11")
		DTTR_FORMAT_TOKEN(PCDOGS_GAMEPAD_IDX_BTN_11, "joy_12")
		DTTR_FORMAT_TOKEN(PCDOGS_GAMEPAD_IDX_BTN_12, "joy_13")
	default:
		return "none";
	}
}

const char *s_config_format_gamepad_axis(int axis) {
	switch (axis) {
		DTTR_FORMAT_TOKEN(DTTR_GAMEPAD_MAPPING_NONE, "none")
		DTTR_FORMAT_TOKEN(SDL_GAMEPAD_AXIS_LEFTX, "axis_left_x")
		DTTR_FORMAT_TOKEN(SDL_GAMEPAD_AXIS_LEFTY, "axis_left_y")
		DTTR_FORMAT_TOKEN(SDL_GAMEPAD_AXIS_RIGHTX, "axis_right_x")
		DTTR_FORMAT_TOKEN(SDL_GAMEPAD_AXIS_RIGHTY, "axis_right_y")
		DTTR_FORMAT_TOKEN(SDL_GAMEPAD_AXIS_LEFT_TRIGGER, "axis_left_trigger")
		DTTR_FORMAT_TOKEN(SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, "axis_right_trigger")
	default:
		return "none";
	}
}

const char *s_config_format_vertex_precision(DTTR_VertexPrecision precision) {
	switch (precision) {
		DTTR_FORMAT_TOKEN(DTTR_VERTEX_PRECISION_NATIVE, "native")
	default:
		return "subpixel";
	}
}

const char *s_config_format_string(const char *value) { return value; }

#undef DTTR_FORMAT_TOKEN
