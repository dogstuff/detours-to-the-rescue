#include "config_internal.h"
#include "log.h"

#include <khash.h>

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

KHASH_MAP_INIT_STR(dttr_config_lookup, int)

// clang-format off
#define S_FIELD(_section, _key, _field, _type) \
	{ .section = (_section), .key = (_key), .offset = offsetof(DTTR_Config, _field), .value_type = (_type) }

#define S_FIELD_TOP(_key, _field, _type) S_FIELD(NULL, _key, _field, _type)

static const S_ConfigFieldSpec s_config_schema[] = {
	S_FIELD("graphics", "scaling_fit",                m_scaling_fit,              S_CONFIG_SCALING_FIT),
	S_FIELD("graphics", "scaling_method",             m_scaling_method,           S_CONFIG_SCALING_METHOD),
	S_FIELD("graphics", "precision_mode",             m_precision_mode,           S_CONFIG_PRECISION_MODE),
	S_FIELD("graphics", "graphics_api",               m_graphics_api,             S_CONFIG_GRAPHICS_API),
	S_FIELD("graphics", "present_scaling_algorithm",  m_present_filter,           S_CONFIG_PRESENT_FILTER),
	S_FIELD("graphics", "window_width",               m_window_width,             S_CONFIG_INT),
	S_FIELD("graphics", "window_height",              m_window_height,            S_CONFIG_INT),
	S_FIELD("graphics", "msaa_samples",               m_msaa_samples,             S_CONFIG_INT),
	S_FIELD("graphics", "texture_upload_sync",        m_texture_upload_sync,      S_CONFIG_BOOL),
	S_FIELD("graphics", "generate_texture_mipmaps",   m_generate_texture_mipmaps, S_CONFIG_BOOL),
	S_FIELD("graphics", "fullscreen",                 m_fullscreen,               S_CONFIG_BOOL),

	S_FIELD_TOP("log_level",      m_log_level,      S_CONFIG_LOG_LEVEL),
	S_FIELD_TOP("minidump_type",  m_minidump_type,  S_CONFIG_MINIDUMP_TYPE),
	S_FIELD_TOP("pcdogs_path",       m_pcdogs_path,       S_CONFIG_STRING),
	S_FIELD_TOP("saves_path",       m_saves_path,         S_CONFIG_STRING),
};
// clang-format on

static khash_t(dttr_config_lookup) *g_dttr_config_lookup = NULL;

int s_config_schema_count(void) { return (int)(sizeof(s_config_schema) / sizeof(s_config_schema[0])); }

const S_ConfigFieldSpec *s_config_schema_get(int index) {
	if (index < 0 || index >= s_config_schema_count()) {
		return NULL;
	}
	return &s_config_schema[index];
}

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

// clang-format off
#define S_CONFIG_ASSIGN_FN(fn_name, type, default_val, parse_fn) \
	static bool fn_name(char *field, const char *value) { \
		type parsed = default_val; \
		if (!parse_fn(value, &parsed)) { return false; } \
		*(type *)field = parsed; \
		return true; \
	}

S_CONFIG_ASSIGN_FN(s_config_assign_bool,           bool,              false,                       s_config_parse_bool)
S_CONFIG_ASSIGN_FN(s_config_assign_scaling_fit,     DTTR_ScalingMode,  DTTR_SCALING_MODE_LETTERBOX, s_config_parse_scaling_fit)
S_CONFIG_ASSIGN_FN(s_config_assign_scaling_method,  DTTR_ScalingMethod,DTTR_SCALING_METHOD_PRESENT, s_config_parse_scaling_method)
S_CONFIG_ASSIGN_FN(s_config_assign_precision_mode,  DTTR_PrecisionMode,DTTR_PRECISION_MODE_STABILIZED, s_config_parse_precision_mode)
S_CONFIG_ASSIGN_FN(s_config_assign_graphics_api,    DTTR_GraphicsApi,  DTTR_GRAPHICS_API_AUTO,      s_config_parse_graphics_api)
S_CONFIG_ASSIGN_FN(s_config_assign_present_filter,  SDL_GPUFilter,     SDL_GPU_FILTER_LINEAR,       s_config_parse_present_filter)
S_CONFIG_ASSIGN_FN(s_config_assign_log_level,       int,               LOG_INFO,                    s_config_parse_log_level)
S_CONFIG_ASSIGN_FN(s_config_assign_minidump_type,   DTTR_MinidumpType, DTTR_MINIDUMP_NORMAL,        s_config_parse_minidump_type)
S_CONFIG_ASSIGN_FN(s_config_assign_int,             int,               0,                           s_config_parse_int)
// clang-format on

static bool s_config_assign_string(char *field, const char *value) {
	return s_config_parse_string(value, field, sizeof(((DTTR_Config *)0)->m_pcdogs_path));
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

	case S_CONFIG_MINIDUMP_TYPE:
		return s_config_assign_minidump_type(field, value);

	case S_CONFIG_STRING:
		return s_config_assign_string(field, value);

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
