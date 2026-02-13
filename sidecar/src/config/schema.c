#include "config_internal.h"
#include "log.h"

#include <khash.h>

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

KHASH_MAP_INIT_STR(dttr_config_lookup, int)

static const S_ConfigFieldSpec s_config_schema[] = {
	{
		.section = "graphics",
		.key = "scaling_fit",
		.offset = offsetof(DTTR_Config, m_scaling_fit),
		.value_type = S_CONFIG_SCALING_FIT,
	},
	{
		.section = "graphics",
		.key = "scaling_method",
		.offset = offsetof(DTTR_Config, m_scaling_method),
		.value_type = S_CONFIG_SCALING_METHOD,
	},
	{
		.section = "graphics",
		.key = "precision_mode",
		.offset = offsetof(DTTR_Config, m_precision_mode),
		.value_type = S_CONFIG_PRECISION_MODE,
	},
	{
		.section = "graphics",
		.key = "graphics_api",
		.offset = offsetof(DTTR_Config, m_graphics_api),
		.value_type = S_CONFIG_GRAPHICS_API,
	},
	{
		.section = "graphics",
		.key = "present_scaling_algorithm",
		.offset = offsetof(DTTR_Config, m_present_filter),
		.value_type = S_CONFIG_PRESENT_FILTER,
	},
	{
		.section = "graphics",
		.key = "window_width",
		.offset = offsetof(DTTR_Config, m_window_width),
		.value_type = S_CONFIG_INT,
	},
	{
		.section = "graphics",
		.key = "window_height",
		.offset = offsetof(DTTR_Config, m_window_height),
		.value_type = S_CONFIG_INT,
	},
	{
		.section = "graphics",
		.key = "msaa_samples",
		.offset = offsetof(DTTR_Config, m_msaa_samples),
		.value_type = S_CONFIG_INT,
	},
	{
		.section = "graphics",
		.key = "texture_upload_sync",
		.offset = offsetof(DTTR_Config, m_texture_upload_sync),
		.value_type = S_CONFIG_BOOL,
	},
	{
		.section = "graphics",
		.key = "generate_texture_mipmaps",
		.offset = offsetof(DTTR_Config, m_generate_texture_mipmaps),
		.value_type = S_CONFIG_BOOL,
	},
	{
		.section = "graphics",
		.key = "fullscreen",
		.offset = offsetof(DTTR_Config, m_fullscreen),
		.value_type = S_CONFIG_BOOL,
	},
	{
		.section = NULL,
		.key = "log_level",
		.offset = offsetof(DTTR_Config, m_log_level),
		.value_type = S_CONFIG_LOG_LEVEL,
	},
};

static khash_t(dttr_config_lookup) *g_dttr_config_lookup = NULL;

static int s_config_schema_count(void) { return (int)(sizeof(s_config_schema) / sizeof(s_config_schema[0])); }

static void s_config_schema_init(void) {
	if (g_dttr_config_lookup) {
		return;
	}

	g_dttr_config_lookup = kh_init(dttr_config_lookup);
	if (!g_dttr_config_lookup) {
		return;
	}

	for (int i = 0; i < s_config_schema_count(); i++) {
		const S_ConfigFieldSpec *const spec = &s_config_schema[i];
		int put_ret = 0;
		const khint_t it = kh_put(dttr_config_lookup, g_dttr_config_lookup, (char *)spec->key, &put_ret);
		if (it != kh_end(g_dttr_config_lookup)) {
			kh_value(g_dttr_config_lookup, it) = i;
		}
	}
}

static const S_ConfigFieldSpec *s_config_schema_find(const char *section, const char *key) {
	s_config_schema_init();
	if (!g_dttr_config_lookup) {
		return NULL;
	}

	const khint_t it = kh_get(dttr_config_lookup, g_dttr_config_lookup, key);
	if (it == kh_end(g_dttr_config_lookup)) {
		return NULL;
	}

	const S_ConfigFieldSpec *const spec = &s_config_schema[kh_value(g_dttr_config_lookup, it)];
	if (section != spec->section && (!section || !spec->section || strcmp(spec->section, section) != 0)) {
		return NULL;
	}

	return spec;
}

static bool s_config_assign_bool(char *field, const char *value) {
	bool parsed = false;
	if (!s_config_parse_bool(value, &parsed)) {
		return false;
	}

	*(bool *)field = parsed;
	return true;
}

static bool s_config_assign_scaling_fit(char *field, const char *value) {
	DTTR_ScalingMode parsed = DTTR_SCALING_MODE_LETTERBOX;
	if (!s_config_parse_scaling_fit(value, &parsed)) {
		return false;
	}

	*(DTTR_ScalingMode *)field = parsed;
	return true;
}

static bool s_config_assign_scaling_method(char *field, const char *value) {
	DTTR_ScalingMethod parsed = DTTR_SCALING_METHOD_PRESENT;
	if (!s_config_parse_scaling_method(value, &parsed)) {
		return false;
	}

	*(DTTR_ScalingMethod *)field = parsed;
	return true;
}

static bool s_config_assign_precision_mode(char *field, const char *value) {
	DTTR_PrecisionMode parsed = DTTR_PRECISION_MODE_STABILIZED;
	if (!s_config_parse_precision_mode(value, &parsed)) {
		return false;
	}

	*(DTTR_PrecisionMode *)field = parsed;
	return true;
}

static bool s_config_assign_graphics_api(char *field, const char *value) {
	DTTR_GraphicsApi parsed = DTTR_GRAPHICS_API_AUTO;
	if (!s_config_parse_graphics_api(value, &parsed)) {
		return false;
	}

	*(DTTR_GraphicsApi *)field = parsed;
	return true;
}

static bool s_config_assign_present_filter(char *field, const char *value) {
	SDL_GPUFilter parsed = SDL_GPU_FILTER_LINEAR;
	if (!s_config_parse_present_filter(value, &parsed)) {
		return false;
	}

	*(SDL_GPUFilter *)field = parsed;
	return true;
}

static bool s_config_assign_log_level(char *field, const char *value) {
	int parsed = LOG_INFO;
	if (!s_config_parse_log_level(value, &parsed)) {
		return false;
	}

	*(int *)field = parsed;
	return true;
}

static bool s_config_assign_int(char *field, const char *value) {
	int parsed = 0;
	if (!s_config_parse_int(value, &parsed)) {
		return false;
	}

	*(int *)field = parsed;
	return true;
}

bool s_config_apply_entry(DTTR_Config *config, const char *section, const char *key, const char *value) {
	if (!config || !key || !value) {
		return false;
	}

	const S_ConfigFieldSpec *const spec = s_config_schema_find(section, key);
	if (!spec) {
		return false;
	}

	char *const field = ((char *)config) + spec->offset;
	switch (spec->value_type) {
	case S_CONFIG_BOOL:
		return s_config_assign_bool(field, value);

	case S_CONFIG_SCALING_FIT:
		return s_config_assign_scaling_fit(field, value);

	case S_CONFIG_SCALING_METHOD:
		return s_config_assign_scaling_method(field, value);

	case S_CONFIG_PRECISION_MODE:
		return s_config_assign_precision_mode(field, value);

	case S_CONFIG_GRAPHICS_API:
		return s_config_assign_graphics_api(field, value);

	case S_CONFIG_INT:
		return s_config_assign_int(field, value);

	case S_CONFIG_PRESENT_FILTER:
		return s_config_assign_present_filter(field, value);

	case S_CONFIG_LOG_LEVEL:
		return s_config_assign_log_level(field, value);

	default:
		return false;
	}
}

static int s_gamepad_axis_key_to_index(const char *key) {
	if (strcmp(key, "axis_stick_x") == 0) {
		return DTTR_GAMEPAD_AXIS_IDX_STICK_X;
	}
	if (strcmp(key, "axis_stick_y") == 0) {
		return DTTR_GAMEPAD_AXIS_IDX_STICK_Y;
	}
	if (strcmp(key, "axis_camera_rz") == 0) {
		return DTTR_GAMEPAD_AXIS_IDX_CAMERA_RZ;
	}
	return -1;
}

static int s_gamepad_deadzone_key_to_index(const char *key) {
	if (strcmp(key, "deadzone_stick_x") == 0) {
		return DTTR_GAMEPAD_AXIS_IDX_STICK_X;
	}
	if (strcmp(key, "deadzone_stick_y") == 0) {
		return DTTR_GAMEPAD_AXIS_IDX_STICK_Y;
	}
	if (strcmp(key, "deadzone_camera_rz") == 0) {
		return DTTR_GAMEPAD_AXIS_IDX_CAMERA_RZ;
	}
	return -1;
}

bool s_config_apply_gamepad_entry(DTTR_Config *config, const char *section, const char *key, const char *value) {
	if (!config || !section || !key || !value) {
		return false;
	}

	if (strcmp(section, "gamepad") != 0) {
		return false;
	}

	if (strcmp(key, "enabled") == 0) {
		bool enabled = true;
		if (!s_config_parse_bool(value, &enabled)) {
			return false;
		}
		config->m_gamepad_enabled = enabled;
		return true;
	}

	const int axis_index = s_gamepad_axis_key_to_index(key);
	if (axis_index >= 0) {
		int axis = DTTR_GAMEPAD_MAPPING_NONE;
		if (!s_config_parse_gamepad_axis(value, &axis)) {
			return false;
		}

		config->m_gamepad_axes[axis_index] = axis;
		return true;
	}

	const int deadzone_index = s_gamepad_deadzone_key_to_index(key);
	if (deadzone_index >= 0) {
		int deadzone = 0;
		if (!s_config_parse_int(value, &deadzone)) {
			return false;
		}

		config->m_gamepad_axis_deadzone[deadzone_index] = deadzone;
		return true;
	}

	return false;
}
