#include "config_internal.h"
#include <dttr_log.h>

#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CONFIG_SCALING_FIT_TOKENS(X)                                                     \
	X("letterbox", DTTR_SCALING_MODE_LETTERBOX)                                          \
	X("stretch", DTTR_SCALING_MODE_STRETCH)                                              \
	X("integer", DTTR_SCALING_MODE_INTEGER)

#define CONFIG_SCALING_METHOD_TOKENS(X)                                                  \
	X("present", DTTR_SCALING_METHOD_PRESENT)                                            \
	X("logical", DTTR_SCALING_METHOD_LOGICAL)

#define CONFIG_GRAPHICS_API_FORMAT_TOKENS(X)                                             \
	X(DTTR_DRIVER_AUTO, DTTR_GRAPHICS_API_AUTO)                                          \
	X(DTTR_DRIVER_VULKAN, DTTR_GRAPHICS_API_VULKAN)                                      \
	X(DTTR_DRIVER_DIRECT3D12, DTTR_GRAPHICS_API_DIRECT3D12)                              \
	X(DTTR_DRIVER_OPENGL, DTTR_GRAPHICS_API_OPENGL)

#define CONFIG_PRESENT_FILTER_TOKENS(X)                                                  \
	X("nearest", SDL_GPU_FILTER_NEAREST)                                                 \
	X("linear", SDL_GPU_FILTER_LINEAR)

#define CONFIG_GAMEPAD_MISC_SOURCE_TOKENS(X)                                             \
	X("misc1", SDL_GAMEPAD_BUTTON_MISC1)                                                 \
	X("misc2", SDL_GAMEPAD_BUTTON_MISC2)                                                 \
	X("misc3", SDL_GAMEPAD_BUTTON_MISC3)                                                 \
	X("misc4", SDL_GAMEPAD_BUTTON_MISC4)                                                 \
	X("misc5", SDL_GAMEPAD_BUTTON_MISC5)                                                 \
	X("misc6", SDL_GAMEPAD_BUTTON_MISC6)

#define CONFIG_GAMEPAD_PADDLE_SOURCE_TOKENS(X)                                           \
	X("right_paddle1", SDL_GAMEPAD_BUTTON_RIGHT_PADDLE1)                                 \
	X("left_paddle1", SDL_GAMEPAD_BUTTON_LEFT_PADDLE1)                                   \
	X("right_paddle2", SDL_GAMEPAD_BUTTON_RIGHT_PADDLE2)                                 \
	X("left_paddle2", SDL_GAMEPAD_BUTTON_LEFT_PADDLE2)

#define CONFIG_GAMEPAD_SOURCE_TOKENS(X)                                                  \
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
	CONFIG_GAMEPAD_MISC_SOURCE_TOKENS(X)                                                 \
	CONFIG_GAMEPAD_PADDLE_SOURCE_TOKENS(X)                                               \
	X("touchpad", SDL_GAMEPAD_BUTTON_TOUCHPAD)                                           \
	X("left_trigger", DTTR_GAMEPAD_SOURCE_TRIGGER_LEFT)                                  \
	X("right_trigger", DTTR_GAMEPAD_SOURCE_TRIGGER_RIGHT)

#define CONFIG_GAME_ACTION_TOKENS(X)                                                     \
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

#define CONFIG_GAMEPAD_AXIS_TOKENS(X)                                                    \
	X("none", DTTR_GAMEPAD_MAPPING_NONE)                                                 \
	X("axis_left_x", SDL_GAMEPAD_AXIS_LEFTX)                                             \
	X("axis_left_y", SDL_GAMEPAD_AXIS_LEFTY)                                             \
	X("axis_right_x", SDL_GAMEPAD_AXIS_RIGHTX)                                           \
	X("axis_right_y", SDL_GAMEPAD_AXIS_RIGHTY)                                           \
	X("axis_left_trigger", SDL_GAMEPAD_AXIS_LEFT_TRIGGER)                                \
	X("axis_right_trigger", SDL_GAMEPAD_AXIS_RIGHT_TRIGGER)

#define CONFIG_LOG_LEVEL_TOKENS(X)                                                       \
	X("trace", LOG_TRACE)                                                                \
	X("debug", LOG_DEBUG)                                                                \
	X("info", LOG_INFO)                                                                  \
	X("warn", LOG_WARN)                                                                  \
	X("error", LOG_ERROR)                                                                \
	X("fatal", LOG_FATAL)

#define CONFIG_MINIDUMP_TYPE_TOKENS(X)                                                   \
	X("normal", DTTR_MINIDUMP_NORMAL)                                                    \
	X("detailed", DTTR_MINIDUMP_DETAILED)

#define CONFIG_VERTEX_PRECISION_TOKENS(X)                                                \
	X("native", DTTR_VERTEX_PRECISION_NATIVE)                                            \
	X("subpixel", DTTR_VERTEX_PRECISION_SUBPIXEL)

#define CONFIG_CHOICE(token, enum_value) {(token), (int)(enum_value)},

static const DTTR_ConfigChoice CONFIG_SCALING_FIT_CHOICES[] = {
	CONFIG_SCALING_FIT_TOKENS(CONFIG_CHOICE)
};

static const DTTR_ConfigChoice CONFIG_SCALING_METHOD_CHOICES[] = {
	CONFIG_SCALING_METHOD_TOKENS(CONFIG_CHOICE)
};

static const DTTR_ConfigChoice CONFIG_GRAPHICS_API_CHOICES[] = {
	CONFIG_GRAPHICS_API_FORMAT_TOKENS(CONFIG_CHOICE)
};

static const DTTR_ConfigChoice CONFIG_PRESENT_FILTER_CHOICES[] = {
	CONFIG_PRESENT_FILTER_TOKENS(CONFIG_CHOICE)
};

static const DTTR_ConfigChoice CONFIG_GAME_ACTION_CHOICES[] = {
	CONFIG_GAME_ACTION_TOKENS(CONFIG_CHOICE)
};

static const DTTR_ConfigChoice CONFIG_GAMEPAD_AXIS_CHOICES[] = {
	CONFIG_GAMEPAD_AXIS_TOKENS(CONFIG_CHOICE)
};

static const DTTR_ConfigChoice CONFIG_LOG_LEVEL_CHOICES[] = {
	CONFIG_LOG_LEVEL_TOKENS(CONFIG_CHOICE)
};

static const DTTR_ConfigChoice CONFIG_MINIDUMP_TYPE_CHOICES[] = {
	CONFIG_MINIDUMP_TYPE_TOKENS(CONFIG_CHOICE)
};

static const DTTR_ConfigChoice CONFIG_VERTEX_PRECISION_CHOICES[] = {
	CONFIG_VERTEX_PRECISION_TOKENS(CONFIG_CHOICE)
};

#undef CONFIG_CHOICE

typedef struct {
	const DTTR_ConfigChoice *choices;
	int count;
} config_choice_list_data;

#define CONFIG_CHOICE_LIST_DATA(array) {(array), (int)SDL_arraysize(array)}

static const config_choice_list_data CONFIG_CHOICE_LISTS[] = {
	[DTTR_CONFIG_CHOICES_LOG_LEVEL] = CONFIG_CHOICE_LIST_DATA(CONFIG_LOG_LEVEL_CHOICES),
	[DTTR_CONFIG_CHOICES_MINIDUMP_TYPE] = CONFIG_CHOICE_LIST_DATA(
		CONFIG_MINIDUMP_TYPE_CHOICES
	),
	[DTTR_CONFIG_CHOICES_GRAPHICS_API] = CONFIG_CHOICE_LIST_DATA(
		CONFIG_GRAPHICS_API_CHOICES
	),
	[DTTR_CONFIG_CHOICES_SCALING_FIT] = CONFIG_CHOICE_LIST_DATA(
		CONFIG_SCALING_FIT_CHOICES
	),
	[DTTR_CONFIG_CHOICES_SCALING_METHOD] = CONFIG_CHOICE_LIST_DATA(
		CONFIG_SCALING_METHOD_CHOICES
	),
	[DTTR_CONFIG_CHOICES_PRESENT_FILTER] = CONFIG_CHOICE_LIST_DATA(
		CONFIG_PRESENT_FILTER_CHOICES
	),
	[DTTR_CONFIG_CHOICES_VERTEX_PRECISION] = CONFIG_CHOICE_LIST_DATA(
		CONFIG_VERTEX_PRECISION_CHOICES
	),
	[DTTR_CONFIG_CHOICES_GAMEPAD_AXIS] = CONFIG_CHOICE_LIST_DATA(
		CONFIG_GAMEPAD_AXIS_CHOICES
	),
	[DTTR_CONFIG_CHOICES_GAME_ACTION] = CONFIG_CHOICE_LIST_DATA(
		CONFIG_GAME_ACTION_CHOICES
	),
};

#undef CONFIG_CHOICE_LIST_DATA

static const config_choice_list_data *config_choice_list(DTTR_ConfigChoiceList list) {
	if (list < 0 || list >= (DTTR_ConfigChoiceList)SDL_arraysize(CONFIG_CHOICE_LISTS)) {
		return NULL;
	}

	return &CONFIG_CHOICE_LISTS[list];
}

int DTTR_Config_ChoiceCount(DTTR_ConfigChoiceList list) {
	const config_choice_list_data *data = config_choice_list(list);
	return data ? data->count : 0;
}

const DTTR_ConfigChoice *DTTR_Config_ChoiceGet(DTTR_ConfigChoiceList list, int index) {
	const config_choice_list_data *data = config_choice_list(list);
	if (!data || index < 0 || index >= data->count) {
		return NULL;
	}

	return &data->choices[index];
}

const DTTR_ConfigChoice *DTTR_Config_Choices(DTTR_ConfigChoiceList list, int *count) {
	const config_choice_list_data *data = config_choice_list(list);
	if (count) {
		*count = data ? data->count : 0;
	}

	return data ? data->choices : NULL;
}

static bool config_parse_choice(
	DTTR_ConfigChoiceList list,
	const char *value,
	int *out_value
) {
	int count = 0;
	const DTTR_ConfigChoice *choices = DTTR_Config_Choices(list, &count);
	if (!value || !out_value || !choices) {
		return false;
	}

	for (int i = 0; i < count; i++) {
		if (strcmp(value, choices[i].label) == 0) {
			*out_value = choices[i].value;
			return true;
		}
	}

	return false;
}

bool config_parse_bool(const char *value, bool *out_value) {
	if (!value || !out_value) {
		return false;
	}

	if (strcmp(value, "true") == 0) {
		*out_value = true;
		return true;
	}

	if (strcmp(value, "false") == 0) {
		*out_value = false;
		return true;
	}

	return false;
}

#define CONFIG_PARSE_CHOICE_FN(fn_name, type, choice_list)                               \
	bool fn_name(const char *value, type *out_value) {                                   \
		int parsed = 0;                                                                  \
		if (!out_value || !config_parse_choice(choice_list, value, &parsed)) {           \
			return false;                                                                \
		}                                                                                \
		*out_value = (type)parsed;                                                       \
		return true;                                                                     \
	}

CONFIG_PARSE_CHOICE_FN(
	config_parse_scaling_fit,
	DTTR_ScalingMode,
	DTTR_CONFIG_CHOICES_SCALING_FIT
)
CONFIG_PARSE_CHOICE_FN(
	config_parse_scaling_method,
	DTTR_ScalingMethod,
	DTTR_CONFIG_CHOICES_SCALING_METHOD
)
CONFIG_PARSE_CHOICE_FN(
	config_parse_present_filter,
	SDL_GPUFilter,
	DTTR_CONFIG_CHOICES_PRESENT_FILTER
)

bool config_parse_graphics_api(const char *value, DTTR_GraphicsApi *out_value) {
	if (!out_value) {
		return false;
	}

	if (value && strcmp(value, DTTR_DRIVER_DIRECT3D12_SHORT) == 0) {
		*out_value = DTTR_GRAPHICS_API_DIRECT3D12;
		return true;
	}

	int parsed = 0;
	if (!config_parse_choice(DTTR_CONFIG_CHOICES_GRAPHICS_API, value, &parsed)) {
		return false;
	}

	*out_value = (DTTR_GraphicsApi)parsed;
	return true;
}

#define CONFIG_PARSE_TOKEN(token, enum_value)                                            \
	if (strcmp(value, token) == 0) {                                                     \
		*out_value = enum_value;                                                         \
		return true;                                                                     \
	}

bool config_parse_gamepad_source(const char *value, int *out_value) {
	if (!value || !out_value) {
		return false;
	}

	CONFIG_GAMEPAD_SOURCE_TOKENS(CONFIG_PARSE_TOKEN)
	return false;
}

#undef CONFIG_PARSE_TOKEN

bool config_parse_game_action(const char *value, int *out_value) {
	return config_parse_choice(DTTR_CONFIG_CHOICES_GAME_ACTION, value, out_value);
}

bool config_parse_gamepad_axis(const char *value, int *out_value) {
	return config_parse_choice(DTTR_CONFIG_CHOICES_GAMEPAD_AXIS, value, out_value);
}

bool config_parse_log_level(const char *value, int *out_value) {
	return config_parse_choice(DTTR_CONFIG_CHOICES_LOG_LEVEL, value, out_value);
}

CONFIG_PARSE_CHOICE_FN(
	config_parse_minidump_type,
	DTTR_MinidumpType,
	DTTR_CONFIG_CHOICES_MINIDUMP_TYPE
)
CONFIG_PARSE_CHOICE_FN(
	config_parse_vertex_precision,
	DTTR_VertexPrecision,
	DTTR_CONFIG_CHOICES_VERTEX_PRECISION
)

#undef CONFIG_PARSE_CHOICE_FN

bool config_parse_int(const char *value, int *out_value) {
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

bool config_parse_float(const char *value, float *out_value) {
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

bool config_parse_string(const char *value, char *out_value, size_t out_size) {
	if (!value || !out_value || out_size == 0) {
		return false;
	}

	const size_t value_len = strlen(value);
	if (value_len >= out_size) {
		return false;
	}

	memcpy(out_value, value, value_len + 1);
	return true;
}

#define CONFIG_FORMAT_TOKEN(token, enum_value)                                           \
	case (enum_value):                                                                   \
		return (token);

void config_format_int(int value, char *buf, size_t buf_size) {
	snprintf(buf, buf_size, "%d", value);
}

void config_format_float(float value, char *buf, size_t buf_size) {
	snprintf(buf, buf_size, "%.9g", value);
}

#define CONFIG_FORMAT_FN(fn_name, type, arg_name, token_list, default_token)             \
	const char *fn_name(type arg_name) {                                                 \
		switch (arg_name) {                                                              \
			token_list(CONFIG_FORMAT_TOKEN) default : return (default_token);            \
		}                                                                                \
	}

CONFIG_FORMAT_FN(
	config_format_scaling_fit,
	DTTR_ScalingMode,
	mode,
	CONFIG_SCALING_FIT_TOKENS,
	"letterbox"
)
CONFIG_FORMAT_FN(
	config_format_scaling_method,
	DTTR_ScalingMethod,
	method,
	CONFIG_SCALING_METHOD_TOKENS,
	"present"
)
CONFIG_FORMAT_FN(
	config_format_graphics_api,
	DTTR_GraphicsApi,
	api,
	CONFIG_GRAPHICS_API_FORMAT_TOKENS,
	DTTR_DRIVER_AUTO
)
CONFIG_FORMAT_FN(
	config_format_present_filter,
	SDL_GPUFilter,
	filter,
	CONFIG_PRESENT_FILTER_TOKENS,
	"linear"
)
CONFIG_FORMAT_FN(config_format_log_level, int, level, CONFIG_LOG_LEVEL_TOKENS, "info")
CONFIG_FORMAT_FN(
	config_format_minidump_type,
	DTTR_MinidumpType,
	type,
	CONFIG_MINIDUMP_TYPE_TOKENS,
	"normal"
)
CONFIG_FORMAT_FN(
	config_format_gamepad_source,
	int,
	source,
	CONFIG_GAMEPAD_SOURCE_TOKENS,
	NULL
)
CONFIG_FORMAT_FN(config_format_game_action, int, action, CONFIG_GAME_ACTION_TOKENS, "none")
CONFIG_FORMAT_FN(config_format_gamepad_axis, int, axis, CONFIG_GAMEPAD_AXIS_TOKENS, "none")
CONFIG_FORMAT_FN(
	config_format_vertex_precision,
	DTTR_VertexPrecision,
	precision,
	CONFIG_VERTEX_PRECISION_TOKENS,
	"subpixel"
)

#undef CONFIG_FORMAT_FN
#undef CONFIG_FORMAT_TOKEN
