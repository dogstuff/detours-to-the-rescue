#include "config_internal.h"
#include "log.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DTTR_PARSE_TOKEN(token, enum_value)                                                                            \
	if (strcmp(value, token) == 0) {                                                                                   \
		*out_value = enum_value;                                                                                       \
		return true;                                                                                                   \
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

bool s_config_parse_precision_mode(const char *value, DTTR_PrecisionMode *out_value) {
	if (!value || !out_value) {
		return false;
	}

	DTTR_PARSE_TOKEN("raw", DTTR_PRECISION_MODE_RAW);
	DTTR_PARSE_TOKEN("stabilized", DTTR_PRECISION_MODE_STABILIZED);

	return false;
}

bool s_config_parse_graphics_api(const char *value, DTTR_GraphicsApi *out_value) {
	if (!value || !out_value) {
		return false;
	}

	DTTR_PARSE_TOKEN("auto", DTTR_GRAPHICS_API_AUTO);
	DTTR_PARSE_TOKEN("vulkan", DTTR_GRAPHICS_API_VULKAN);
	DTTR_PARSE_TOKEN("direct3d12", DTTR_GRAPHICS_API_DIRECT3D12);
	DTTR_PARSE_TOKEN("d3d12", DTTR_GRAPHICS_API_DIRECT3D12);
	DTTR_PARSE_TOKEN("metal", DTTR_GRAPHICS_API_METAL);

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

bool s_config_parse_present_filter(const char *value, SDL_GPUFilter *out_value) {
	if (!value || !out_value) {
		return false;
	}

	DTTR_PARSE_TOKEN("nearest", SDL_GPU_FILTER_NEAREST);
	DTTR_PARSE_TOKEN("linear", SDL_GPU_FILTER_LINEAR);

	return false;
}

bool s_config_parse_gamepad_button(const char *value, int *out_value) {
	if (!value || !out_value) {
		return false;
	}

	DTTR_PARSE_TOKEN("none", DTTR_GAMEPAD_MAPPING_NONE);
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
	DTTR_PARSE_TOKEN("left_trigger", DTTR_GAMEPAD_MAPPING_TRIGGER_BASE + SDL_GAMEPAD_AXIS_LEFT_TRIGGER);
	DTTR_PARSE_TOKEN("right_trigger", DTTR_GAMEPAD_MAPPING_TRIGGER_BASE + SDL_GAMEPAD_AXIS_RIGHT_TRIGGER);

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

#undef DTTR_PARSE_TOKEN

#define DTTR_FORMAT_TOKEN(enum_value, token) case (enum_value): return (token);

const char *s_config_format_bool(bool value) { return value ? "true" : "false"; }

void s_config_format_int(int value, char *buf, size_t buf_size) { snprintf(buf, buf_size, "%d", value); }

const char *s_config_format_scaling_fit(DTTR_ScalingMode mode) {
	switch (mode) {
	DTTR_FORMAT_TOKEN(DTTR_SCALING_MODE_STRETCH, "stretch")
	DTTR_FORMAT_TOKEN(DTTR_SCALING_MODE_INTEGER, "integer")
	default: return "letterbox";
	}
}

const char *s_config_format_scaling_method(DTTR_ScalingMethod method) {
	switch (method) {
	DTTR_FORMAT_TOKEN(DTTR_SCALING_METHOD_LOGICAL, "logical")
	default: return "present";
	}
}

const char *s_config_format_precision_mode(DTTR_PrecisionMode mode) {
	switch (mode) {
	DTTR_FORMAT_TOKEN(DTTR_PRECISION_MODE_RAW, "raw")
	default: return "stabilized";
	}
}

const char *s_config_format_graphics_api(DTTR_GraphicsApi api) {
	switch (api) {
	DTTR_FORMAT_TOKEN(DTTR_GRAPHICS_API_VULKAN, "vulkan")
	DTTR_FORMAT_TOKEN(DTTR_GRAPHICS_API_DIRECT3D12, "direct3d12")
	DTTR_FORMAT_TOKEN(DTTR_GRAPHICS_API_METAL, "metal")
	default: return "auto";
	}
}

const char *s_config_format_present_filter(SDL_GPUFilter filter) {
	switch (filter) {
	DTTR_FORMAT_TOKEN(SDL_GPU_FILTER_NEAREST, "nearest")
	default: return "linear";
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
	default: return "info";
	}
}

const char *s_config_format_minidump_type(DTTR_MinidumpType type) {
	switch (type) {
	DTTR_FORMAT_TOKEN(DTTR_MINIDUMP_DETAILED, "detailed")
	default: return "normal";
	}
}

const char *s_config_format_gamepad_button(int button) {
	switch (button) {
	DTTR_FORMAT_TOKEN(DTTR_GAMEPAD_MAPPING_NONE, "none")
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
	DTTR_FORMAT_TOKEN(DTTR_GAMEPAD_MAPPING_TRIGGER_BASE + SDL_GAMEPAD_AXIS_LEFT_TRIGGER, "left_trigger")
	DTTR_FORMAT_TOKEN(DTTR_GAMEPAD_MAPPING_TRIGGER_BASE + SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, "right_trigger")
	default: return "none";
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
	default: return "none";
	}
}

#undef DTTR_FORMAT_TOKEN
