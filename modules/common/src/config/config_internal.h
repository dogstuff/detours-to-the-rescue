#ifndef DTTR_CONFIG_INTERNAL_H
#define DTTR_CONFIG_INTERNAL_H

#include <dttr_config.h>

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

enum {
	CONFIG_BOOL = DTTR_CONFIG_VALUE_BOOL,
	CONFIG_SCALING_FIT = DTTR_CONFIG_VALUE_SCALING_FIT,
	CONFIG_SCALING_METHOD = DTTR_CONFIG_VALUE_SCALING_METHOD,
	CONFIG_GRAPHICS_API = DTTR_CONFIG_VALUE_GRAPHICS_API,
	CONFIG_INT = DTTR_CONFIG_VALUE_INT,
	CONFIG_FLOAT = DTTR_CONFIG_VALUE_FLOAT,
	CONFIG_PRESENT_FILTER = DTTR_CONFIG_VALUE_PRESENT_FILTER,
	CONFIG_LOG_LEVEL = DTTR_CONFIG_VALUE_LOG_LEVEL,
	CONFIG_MINIDUMP_TYPE = DTTR_CONFIG_VALUE_MINIDUMP_TYPE,
	CONFIG_STRING = DTTR_CONFIG_VALUE_STRING,
	CONFIG_VERTEX_PRECISION = DTTR_CONFIG_VALUE_VERTEX_PRECISION,
	CONFIG_GAMEPAD_AXIS = DTTR_CONFIG_VALUE_GAMEPAD_AXIS,
};

#define CONFIG_GAMEPAD_AXIS_FIELDS(X)                                                    \
	X("axis_stick_x", DTTR_GAMEPAD_AXIS_IDX_STICK_X)                                     \
	X("axis_stick_y", DTTR_GAMEPAD_AXIS_IDX_STICK_Y)                                     \
	X("axis_camera_rz", DTTR_GAMEPAD_AXIS_IDX_CAMERA_RZ)

#define CONFIG_GAMEPAD_DEADZONE_FIELDS(X)                                                \
	X("deadzone_stick_x", DTTR_GAMEPAD_AXIS_IDX_STICK_X)                                 \
	X("deadzone_stick_y", DTTR_GAMEPAD_AXIS_IDX_STICK_Y)                                 \
	X("deadzone_camera_rz", DTTR_GAMEPAD_AXIS_IDX_CAMERA_RZ)

#define CONFIG_GAMEPAD_SENSITIVITY_FIELDS(X)                                             \
	X("sensitivity_stick_x", DTTR_GAMEPAD_AXIS_IDX_STICK_X)                              \
	X("sensitivity_stick_y", DTTR_GAMEPAD_AXIS_IDX_STICK_Y)                              \
	X("sensitivity_camera_rz", DTTR_GAMEPAD_AXIS_IDX_CAMERA_RZ)

#define DTTR_CONFIG_MOD_VALUE_EQ_SCALAR(a, b) ((a) == (b))
#define DTTR_CONFIG_MOD_VALUE_EQ_STRING(a, b) (strcmp((a), (b)) == 0)

// One row per DTTR_ConfigModValueType, shared by mods.c (equality) and io.c (JSON
// writer) so a new type can't be added to one path and forgotten in the other.
// Columns: enum suffix, union member, yyjson writer, equality fn.
#define DTTR_CONFIG_MOD_VALUE_TYPES(X)                                                   \
	X(BOOL, bool_value, yyjson_mut_obj_add_bool, DTTR_CONFIG_MOD_VALUE_EQ_SCALAR)        \
	X(INT, int_value, yyjson_mut_obj_add_int, DTTR_CONFIG_MOD_VALUE_EQ_SCALAR)           \
	X(FLOAT, float_value, yyjson_mut_obj_add_real, DTTR_CONFIG_MOD_VALUE_EQ_SCALAR)      \
	X(STRING, string_value, obj_add_strcpy, DTTR_CONFIG_MOD_VALUE_EQ_STRING)

#define DTTR_CONFIG_MOD_VALUE_TYPE_COUNT_ROW(suffix, member, writer, eq) +1
static_assert(
	(0 DTTR_CONFIG_MOD_VALUE_TYPES(DTTR_CONFIG_MOD_VALUE_TYPE_COUNT_ROW))
		== DTTR_CONFIG_MOD_VALUE_STRING + 1,
	"DTTR_CONFIG_MOD_VALUE_TYPES must list exactly one row per DTTR_ConfigModValueType"
);
#undef DTTR_CONFIG_MOD_VALUE_TYPE_COUNT_ROW

static inline bool config_sections_match(const char *lhs, const char *rhs) {
	return lhs == rhs || (lhs && rhs && strcmp(lhs, rhs) == 0);
}

static inline void config_clear_button_map(int *map) {
	for (int i = 0; i < DTTR_GAMEPAD_SOURCE_COUNT; i++) {
		map[i] = DTTR_GAMEPAD_MAPPING_NONE;
	}
}

static inline DTTR_Result config_result(DTTR_Status status, const char *message) {
	return (DTTR_Result){
		.status = status,
		.message = message,
	};
}

static inline DTTR_Result config_ok() {
	return config_result(DTTR_OK, NULL);
}

const DTTR_ConfigFieldSpec *config_schema_find(const char *section, const char *key);

bool config_parse_bool(const char *value, bool *out_value);
bool config_parse_scaling_fit(const char *value, DTTR_ScalingMode *out_value);
bool config_parse_scaling_method(const char *value, DTTR_ScalingMethod *out_value);
bool config_parse_graphics_api(const char *value, DTTR_GraphicsAPI *out_value);
bool config_parse_int(const char *value, int *out_value);
bool config_parse_float(const char *value, float *out_value);
bool config_parse_present_filter(const char *value, SDL_GPUFilter *out_value);
bool config_parse_gamepad_source(const char *value, int *out_value);
bool config_parse_game_action(const char *value, int *out_value);
bool config_parse_gamepad_axis(const char *value, int *out_value);
bool config_parse_log_level(const char *value, int *out_value);
bool config_parse_minidump_type(const char *value, DTTR_MinidumpType *out_value);
bool config_parse_string(const char *value, char *out_value, size_t out_size);
bool config_parse_vertex_precision(const char *value, DTTR_VertexPrecision *out_value);

void config_format_int(int value, char *buf, size_t buf_size);
void config_format_float(float value, char *buf, size_t buf_size);
const char *config_format_scaling_fit(DTTR_ScalingMode mode);
const char *config_format_scaling_method(DTTR_ScalingMethod method);
const char *config_format_graphics_api(DTTR_GraphicsAPI api);
const char *config_format_present_filter(SDL_GPUFilter filter);
const char *config_format_log_level(int level);
const char *config_format_minidump_type(DTTR_MinidumpType type);
const char *config_format_gamepad_source(int source);
const char *config_format_game_action(int action);
const char *config_format_gamepad_axis(int axis);
const char *config_format_vertex_precision(DTTR_VertexPrecision precision);

bool config_apply_entry(
	DTTR_Config *config,
	const char *section,
	const char *key,
	const char *value
);

#endif
