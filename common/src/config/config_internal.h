#ifndef DTTR_CONFIG_INTERNAL_H
#define DTTR_CONFIG_INTERNAL_H

#include <dttr_config.h>

#include <stdbool.h>
#include <stddef.h>

typedef enum {
	S_CONFIG_BOOL = 0,
	S_CONFIG_SCALING_FIT = 1,
	S_CONFIG_SCALING_METHOD = 2,
	S_CONFIG_GRAPHICS_API = 3,
	S_CONFIG_INT = 4,
	S_CONFIG_PRESENT_FILTER = 5,
	S_CONFIG_LOG_LEVEL = 6,
	S_CONFIG_MINIDUMP_TYPE = 7,
	S_CONFIG_STRING = 8,
	S_CONFIG_VERTEX_PRECISION = 9,
} S_ConfigValueType;

typedef struct {
	const char *section;
	const char *key;
	ptrdiff_t offset;
	S_ConfigValueType value_type;
} S_ConfigFieldSpec;

bool s_config_parse_bool(const char *value, bool *out_value);
bool s_config_parse_scaling_fit(const char *value, DTTR_ScalingMode *out_value);
bool s_config_parse_scaling_method(const char *value, DTTR_ScalingMethod *out_value);
bool s_config_parse_graphics_api(const char *value, DTTR_GraphicsApi *out_value);
bool s_config_parse_int(const char *value, int *out_value);
bool s_config_parse_present_filter(const char *value, SDL_GPUFilter *out_value);
bool s_config_parse_gamepad_source(const char *value, int *out_value);
bool s_config_parse_game_action(const char *value, int *out_value);
bool s_config_parse_gamepad_axis(const char *value, int *out_value);
bool s_config_parse_log_level(const char *value, int *out_value);
bool s_config_parse_minidump_type(const char *value, DTTR_MinidumpType *out_value);
bool s_config_parse_string(const char *value, char *out_value, size_t out_size);
bool s_config_parse_vertex_precision(const char *value, DTTR_VertexPrecision *out_value);

const char *s_config_format_bool(bool value);
void s_config_format_int(int value, char *buf, size_t buf_size);
const char *s_config_format_scaling_fit(DTTR_ScalingMode mode);
const char *s_config_format_scaling_method(DTTR_ScalingMethod method);
const char *s_config_format_graphics_api(DTTR_GraphicsApi api);
const char *s_config_format_present_filter(SDL_GPUFilter filter);
const char *s_config_format_log_level(int level);
const char *s_config_format_minidump_type(DTTR_MinidumpType type);
const char *s_config_format_gamepad_source(int source);
const char *s_config_format_game_action(int action);
const char *s_config_format_gamepad_axis(int axis);
const char *s_config_format_string(const char *value);
const char *s_config_format_vertex_precision(DTTR_VertexPrecision precision);

int s_config_schema_count(void);
const S_ConfigFieldSpec *s_config_schema_get(int index);

bool s_config_apply_entry(DTTR_Config *config, const char *section, const char *key, const char *value);

bool s_config_apply_gamepad_entry(DTTR_Config *config, const char *section, const char *key, const char *value);

#endif
