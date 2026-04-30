#include "config_internal.h"
#include <dttr_log.h>

#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define S_CONFIG_BOOL_TOKENS(X)                                                          \
	X("true", true)                                                                      \
	X("false", false)

#define S_CONFIG_SCALING_FIT_TOKENS(X)                                                   \
	X("letterbox", DTTR_SCALING_MODE_LETTERBOX)                                          \
	X("stretch", DTTR_SCALING_MODE_STRETCH)                                              \
	X("integer", DTTR_SCALING_MODE_INTEGER)

#define S_CONFIG_SCALING_METHOD_TOKENS(X)                                                \
	X("present", DTTR_SCALING_METHOD_PRESENT)                                            \
	X("logical", DTTR_SCALING_METHOD_LOGICAL)

#define S_CONFIG_GRAPHICS_API_PARSE_TOKENS(X)                                            \
	X(DTTR_DRIVER_AUTO, DTTR_GRAPHICS_API_AUTO)                                          \
	X(DTTR_DRIVER_VULKAN, DTTR_GRAPHICS_API_VULKAN)                                      \
	X(DTTR_DRIVER_DIRECT3D12, DTTR_GRAPHICS_API_DIRECT3D12)                              \
	X(DTTR_DRIVER_DIRECT3D12_SHORT, DTTR_GRAPHICS_API_DIRECT3D12)                        \
	X(DTTR_DRIVER_OPENGL, DTTR_GRAPHICS_API_OPENGL)

#define S_CONFIG_GRAPHICS_API_FORMAT_TOKENS(X)                                           \
	X(DTTR_DRIVER_AUTO, DTTR_GRAPHICS_API_AUTO)                                          \
	X(DTTR_DRIVER_VULKAN, DTTR_GRAPHICS_API_VULKAN)                                      \
	X(DTTR_DRIVER_DIRECT3D12, DTTR_GRAPHICS_API_DIRECT3D12)                              \
	X(DTTR_DRIVER_OPENGL, DTTR_GRAPHICS_API_OPENGL)

#define S_CONFIG_PRESENT_FILTER_TOKENS(X)                                                \
	X("nearest", SDL_GPU_FILTER_NEAREST)                                                 \
	X("linear", SDL_GPU_FILTER_LINEAR)

#define S_CONFIG_GAMEPAD_SOURCE_TOKENS(X)                                                \
	X("south", SDL_GAMEPAD_BUTTON_SOUTH)                                                 \
	X("east", SDL_GAMEPAD_BUTTON_EAST)                                                   \
	X("west", SDL_GAMEPAD_BUTTON_WEST)                                                   \
	X("north", SDL_GAMEPAD_BUTTON_NORTH)                                                 \
	X("back", SDL_GAMEPAD_BUTTON_BACK)                                                   \
	X("guide", SDL_GAMEPAD_BUTTON_GUIDE)                                                 \
	X("start", SDL_GAMEPAD_BUTTON_START)                                                 \
	X("left_stick_click", SDL_GAMEPAD_BUTTON_LEFT_STICK)                                 \
	X("right_stick_click", SDL_GAMEPAD_BUTTON_RIGHT_STICK)                               \
	X("left_shoulder", SDL_GAMEPAD_BUTTON_LEFT_SHOULDER)                                 \
	X("right_shoulder", SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER)                               \
	X("dpad_up", SDL_GAMEPAD_BUTTON_DPAD_UP)                                             \
	X("dpad_down", SDL_GAMEPAD_BUTTON_DPAD_DOWN)                                         \
	X("dpad_left", SDL_GAMEPAD_BUTTON_DPAD_LEFT)                                         \
	X("dpad_right", SDL_GAMEPAD_BUTTON_DPAD_RIGHT)                                       \
	X("misc1", SDL_GAMEPAD_BUTTON_MISC1)                                                 \
	X("right_paddle1", SDL_GAMEPAD_BUTTON_RIGHT_PADDLE1)                                 \
	X("left_paddle1", SDL_GAMEPAD_BUTTON_LEFT_PADDLE1)                                   \
	X("right_paddle2", SDL_GAMEPAD_BUTTON_RIGHT_PADDLE2)                                 \
	X("left_paddle2", SDL_GAMEPAD_BUTTON_LEFT_PADDLE2)                                   \
	X("touchpad", SDL_GAMEPAD_BUTTON_TOUCHPAD)                                           \
	X("left_trigger", DTTR_GAMEPAD_SOURCE_TRIGGER_LEFT)                                  \
	X("right_trigger", DTTR_GAMEPAD_SOURCE_TRIGGER_RIGHT)

#define S_CONFIG_GAME_ACTION_TOKENS(X)                                                   \
	X("none", DTTR_GAMEPAD_MAPPING_NONE)                                                 \
	X("up", PCDOGS_GAMEPAD_IDX_UP)                                                       \
	X("down", PCDOGS_GAMEPAD_IDX_DOWN)                                                   \
	X("left", PCDOGS_GAMEPAD_IDX_LEFT)                                                   \
	X("right", PCDOGS_GAMEPAD_IDX_RIGHT)                                                 \
	X("pov_up", PCDOGS_GAMEPAD_IDX_POV_UP)                                               \
	X("pov_down", PCDOGS_GAMEPAD_IDX_POV_DOWN)                                           \
	X("joy_1", PCDOGS_GAMEPAD_IDX_BTN_0)                                                 \
	X("joy_2", PCDOGS_GAMEPAD_IDX_BTN_1)                                                 \
	X("joy_3", PCDOGS_GAMEPAD_IDX_BTN_2)                                                 \
	X("joy_4", PCDOGS_GAMEPAD_IDX_BTN_3)                                                 \
	X("joy_5", PCDOGS_GAMEPAD_IDX_BTN_4)                                                 \
	X("joy_6", PCDOGS_GAMEPAD_IDX_BTN_5)                                                 \
	X("joy_7", PCDOGS_GAMEPAD_IDX_BTN_6)                                                 \
	X("joy_8", PCDOGS_GAMEPAD_IDX_BTN_7)                                                 \
	X("joy_9", PCDOGS_GAMEPAD_IDX_BTN_8)                                                 \
	X("joy_10", PCDOGS_GAMEPAD_IDX_BTN_9)                                                \
	X("joy_11", PCDOGS_GAMEPAD_IDX_BTN_10)                                               \
	X("joy_12", PCDOGS_GAMEPAD_IDX_BTN_11)                                               \
	X("joy_13", PCDOGS_GAMEPAD_IDX_BTN_12)

#define S_CONFIG_GAMEPAD_AXIS_TOKENS(X)                                                  \
	X("none", DTTR_GAMEPAD_MAPPING_NONE)                                                 \
	X("axis_left_x", SDL_GAMEPAD_AXIS_LEFTX)                                             \
	X("axis_left_y", SDL_GAMEPAD_AXIS_LEFTY)                                             \
	X("axis_right_x", SDL_GAMEPAD_AXIS_RIGHTX)                                           \
	X("axis_right_y", SDL_GAMEPAD_AXIS_RIGHTY)                                           \
	X("axis_left_trigger", SDL_GAMEPAD_AXIS_LEFT_TRIGGER)                                \
	X("axis_right_trigger", SDL_GAMEPAD_AXIS_RIGHT_TRIGGER)

#define S_CONFIG_LOG_LEVEL_TOKENS(X)                                                     \
	X("trace", LOG_TRACE)                                                                \
	X("debug", LOG_DEBUG)                                                                \
	X("info", LOG_INFO)                                                                  \
	X("warn", LOG_WARN)                                                                  \
	X("error", LOG_ERROR)                                                                \
	X("fatal", LOG_FATAL)

#define S_CONFIG_MINIDUMP_TYPE_TOKENS(X)                                                 \
	X("normal", DTTR_MINIDUMP_NORMAL)                                                    \
	X("detailed", DTTR_MINIDUMP_DETAILED)

#define S_CONFIG_VERTEX_PRECISION_TOKENS(X)                                              \
	X("native", DTTR_VERTEX_PRECISION_NATIVE)                                            \
	X("subpixel", DTTR_VERTEX_PRECISION_SUBPIXEL)

#define S_CONFIG_PARSE_TOKEN(token, enum_value)                                          \
	if (strcmp(value, token) == 0) {                                                     \
		*out_value = enum_value;                                                         \
		return true;                                                                     \
	}

#define S_CONFIG_PARSE_FN(fn_name, type, token_list)                                     \
	bool fn_name(const char *value, type *out_value) {                                   \
		if (!value || !out_value) {                                                      \
			return false;                                                                \
		}                                                                                \
		token_list(S_CONFIG_PARSE_TOKEN) return false;                                   \
	}

S_CONFIG_PARSE_FN(s_config_parse_bool, bool, S_CONFIG_BOOL_TOKENS)
S_CONFIG_PARSE_FN(
	s_config_parse_scaling_fit,
	DTTR_ScalingMode,
	S_CONFIG_SCALING_FIT_TOKENS
)
S_CONFIG_PARSE_FN(
	s_config_parse_scaling_method,
	DTTR_ScalingMethod,
	S_CONFIG_SCALING_METHOD_TOKENS
)
S_CONFIG_PARSE_FN(
	s_config_parse_graphics_api,
	DTTR_GraphicsApi,
	S_CONFIG_GRAPHICS_API_PARSE_TOKENS
)
S_CONFIG_PARSE_FN(
	s_config_parse_present_filter,
	SDL_GPUFilter,
	S_CONFIG_PRESENT_FILTER_TOKENS
)
S_CONFIG_PARSE_FN(s_config_parse_gamepad_source, int, S_CONFIG_GAMEPAD_SOURCE_TOKENS)
S_CONFIG_PARSE_FN(s_config_parse_game_action, int, S_CONFIG_GAME_ACTION_TOKENS)
S_CONFIG_PARSE_FN(s_config_parse_gamepad_axis, int, S_CONFIG_GAMEPAD_AXIS_TOKENS)
S_CONFIG_PARSE_FN(s_config_parse_log_level, int, S_CONFIG_LOG_LEVEL_TOKENS)
S_CONFIG_PARSE_FN(
	s_config_parse_minidump_type,
	DTTR_MinidumpType,
	S_CONFIG_MINIDUMP_TYPE_TOKENS
)
S_CONFIG_PARSE_FN(
	s_config_parse_vertex_precision,
	DTTR_VertexPrecision,
	S_CONFIG_VERTEX_PRECISION_TOKENS
)

#undef S_CONFIG_PARSE_FN
#undef S_CONFIG_PARSE_TOKEN

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

bool s_config_parse_string(const char *value, char *out_value, size_t out_size) {
	if (!value || !out_value || out_size == 0) {
		return false;
	}

	strncpy(out_value, value, out_size - 1);
	out_value[out_size - 1] = '\0';
	return true;
}

#define S_CONFIG_FORMAT_TOKEN(token, enum_value)                                         \
	case (enum_value):                                                                   \
		return (token);

const char *s_config_format_bool(bool value) { return value ? "true" : "false"; }

void s_config_format_int(int value, char *buf, size_t buf_size) {
	snprintf(buf, buf_size, "%d", value);
}

void s_config_format_float(float value, char *buf, size_t buf_size) {
	snprintf(buf, buf_size, "%.9g", value);
}

#define S_CONFIG_FORMAT_FN(fn_name, type, arg_name, token_list, default_token)           \
	const char *fn_name(type arg_name) {                                                 \
		switch (arg_name) {                                                              \
			token_list(S_CONFIG_FORMAT_TOKEN) default : return (default_token);          \
		}                                                                                \
	}

S_CONFIG_FORMAT_FN(
	s_config_format_scaling_fit,
	DTTR_ScalingMode,
	mode,
	S_CONFIG_SCALING_FIT_TOKENS,
	"letterbox"
)
S_CONFIG_FORMAT_FN(
	s_config_format_scaling_method,
	DTTR_ScalingMethod,
	method,
	S_CONFIG_SCALING_METHOD_TOKENS,
	"present"
)
S_CONFIG_FORMAT_FN(
	s_config_format_graphics_api,
	DTTR_GraphicsApi,
	api,
	S_CONFIG_GRAPHICS_API_FORMAT_TOKENS,
	DTTR_DRIVER_AUTO
)
S_CONFIG_FORMAT_FN(
	s_config_format_present_filter,
	SDL_GPUFilter,
	filter,
	S_CONFIG_PRESENT_FILTER_TOKENS,
	"linear"
)
S_CONFIG_FORMAT_FN(s_config_format_log_level, int, level, S_CONFIG_LOG_LEVEL_TOKENS, "info")
S_CONFIG_FORMAT_FN(
	s_config_format_minidump_type,
	DTTR_MinidumpType,
	type,
	S_CONFIG_MINIDUMP_TYPE_TOKENS,
	"normal"
)
S_CONFIG_FORMAT_FN(
	s_config_format_gamepad_source,
	int,
	source,
	S_CONFIG_GAMEPAD_SOURCE_TOKENS,
	NULL
)
S_CONFIG_FORMAT_FN(
	s_config_format_game_action,
	int,
	action,
	S_CONFIG_GAME_ACTION_TOKENS,
	"none"
)
S_CONFIG_FORMAT_FN(
	s_config_format_gamepad_axis,
	int,
	axis,
	S_CONFIG_GAMEPAD_AXIS_TOKENS,
	"none"
)
S_CONFIG_FORMAT_FN(
	s_config_format_vertex_precision,
	DTTR_VertexPrecision,
	precision,
	S_CONFIG_VERTEX_PRECISION_TOKENS,
	"subpixel"
)

#undef S_CONFIG_FORMAT_FN
#undef S_CONFIG_FORMAT_TOKEN

const char *s_config_format_string(const char *value) { return value; }
