#ifndef DTTR_CONFIG_INTERNAL_H
#define DTTR_CONFIG_INTERNAL_H

#include <dttr_config.h>

#include <stdbool.h>
#include <stddef.h>

typedef enum {
	S_CONFIG_BOOL = 0,
	S_CONFIG_SCALING_FIT = 1,
	S_CONFIG_SCALING_METHOD = 2,
	S_CONFIG_PRECISION_MODE = 3,
	S_CONFIG_GRAPHICS_API = 4,
	S_CONFIG_INT = 5,
	S_CONFIG_PRESENT_FILTER = 6,
	S_CONFIG_LOG_LEVEL = 7,
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
bool s_config_parse_precision_mode(const char *value, DTTR_PrecisionMode *out_value);
bool s_config_parse_graphics_api(const char *value, DTTR_GraphicsApi *out_value);
bool s_config_parse_int(const char *value, int *out_value);
bool s_config_parse_present_filter(const char *value, SDL_GPUFilter *out_value);
bool s_config_parse_gamepad_button(const char *value, int *out_value);
bool s_config_parse_gamepad_axis(const char *value, int *out_value);
bool s_config_parse_log_level(const char *value, int *out_value);

const char *s_config_format_bool(bool value);
void s_config_format_int(int value, char *buf, size_t buf_size);
const char *s_config_format_scaling_fit(DTTR_ScalingMode mode);
const char *s_config_format_scaling_method(DTTR_ScalingMethod method);
const char *s_config_format_precision_mode(DTTR_PrecisionMode mode);
const char *s_config_format_graphics_api(DTTR_GraphicsApi api);
const char *s_config_format_present_filter(SDL_GPUFilter filter);
const char *s_config_format_log_level(int level);
const char *s_config_format_gamepad_button(int button);
const char *s_config_format_gamepad_axis(int axis);

int s_config_schema_count(void);
const S_ConfigFieldSpec *s_config_schema_get(int index);

bool s_config_apply_entry(DTTR_Config *config, const char *section, const char *key, const char *value);

bool s_config_apply_gamepad_entry(DTTR_Config *config, const char *section, const char *key, const char *value);

#endif
