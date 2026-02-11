#include "config_internal.h"
#include "log.h"

#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#define DTTR_PARSE_TOKEN(token, enum_value)                                                        \
	if (strcmp(value, token) == 0) {                                                               \
		*out_value = enum_value;                                                                   \
		return true;                                                                               \
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

const char *s_config_format_scaling_fit(DTTR_ScalingMode mode) {
	switch (mode) {
	case DTTR_SCALING_MODE_STRETCH:
		return "stretch";
	case DTTR_SCALING_MODE_INTEGER:
		return "integer";
	default:
		return "letterbox";
	}
}

const char *s_config_format_scaling_method(DTTR_ScalingMethod method) {
	switch (method) {
	case DTTR_SCALING_METHOD_LOGICAL:
		return "logical";
	default:
		return "present";
	}
}

const char *s_config_format_precision_mode(DTTR_PrecisionMode mode) {
	switch (mode) {
	case DTTR_PRECISION_MODE_RAW:
		return "raw";
	default:
		return "stabilized";
	}
}

const char *s_config_format_graphics_api(DTTR_GraphicsApi api) {
	switch (api) {
	case DTTR_GRAPHICS_API_VULKAN:
		return "vulkan";
	case DTTR_GRAPHICS_API_DIRECT3D12:
		return "direct3d12";
	case DTTR_GRAPHICS_API_METAL:
		return "metal";
	default:
		return "auto";
	}
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

	DTTR_PARSE_TOKEN(
		"left_trigger", DTTR_GAMEPAD_MAPPING_TRIGGER_BASE + SDL_GAMEPAD_AXIS_LEFT_TRIGGER
	);
	DTTR_PARSE_TOKEN(
		"right_trigger", DTTR_GAMEPAD_MAPPING_TRIGGER_BASE + SDL_GAMEPAD_AXIS_RIGHT_TRIGGER
	);

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

#undef DTTR_PARSE_TOKEN
