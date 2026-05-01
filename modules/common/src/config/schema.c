#include "config_internal.h"
#include <dttr_log.h>

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
	S_FIELD("graphics", "scaling_fit", m_scaling_fit, S_CONFIG_SCALING_FIT),
	S_FIELD("graphics", "scaling_method", m_scaling_method, S_CONFIG_SCALING_METHOD),
	S_FIELD("graphics", "graphics_api", m_graphics_api, S_CONFIG_GRAPHICS_API),
	S_FIELD(
		"graphics",
		"present_scaling_algorithm",
		m_present_filter,
		S_CONFIG_PRESENT_FILTER
	),
	S_FIELD("graphics", "window_width", m_window_width, S_CONFIG_INT),
	S_FIELD("graphics", "window_height", m_window_height, S_CONFIG_INT),
	S_FIELD("graphics", "msaa_samples", m_msaa_samples, S_CONFIG_INT),
	S_FIELD("graphics", "texture_upload_sync", m_texture_upload_sync, S_CONFIG_BOOL),
	S_FIELD("graphics", "generate_texture_mipmaps",   m_generate_texture_mipmaps, S_CONFIG_BOOL),
	S_FIELD("graphics", "vertex_precision", m_vertex_precision, S_CONFIG_VERTEX_PRECISION),
	S_FIELD("graphics", "sprite_smooth", m_sprite_smooth, S_CONFIG_BOOL),
	S_FIELD("graphics", "fullscreen", m_fullscreen, S_CONFIG_BOOL),

	S_FIELD("audio", "mss_sdl_enabled", m_mss_sdl_enabled, S_CONFIG_BOOL),
	S_FIELD("audio", "mss_sample_gain", m_mss_sample_gain, S_CONFIG_FLOAT),
	S_FIELD("audio", "mss_sample_preemphasis", m_mss_sample_preemphasis, S_CONFIG_FLOAT),

	S_FIELD_TOP("log_level", m_log_level, S_CONFIG_LOG_LEVEL),
	S_FIELD_TOP("minidump_type", m_minidump_type, S_CONFIG_MINIDUMP_TYPE),
	S_FIELD_TOP("pcdogs_path", m_pcdogs_path, S_CONFIG_STRING),
	S_FIELD_TOP("saves_path", m_saves_path, S_CONFIG_STRING),
};
// clang-format on

static khash_t(dttr_config_lookup) *g_dttr_config_lookup = NULL;

typedef struct {
	const char *key;
	int index;
} S_ConfigKeyIndex;

#define S_CONFIG_GAMEPAD_AXIS_KEYS(X)                                                    \
	X("axis_stick_x", DTTR_GAMEPAD_AXIS_IDX_STICK_X)                                     \
	X("axis_stick_y", DTTR_GAMEPAD_AXIS_IDX_STICK_Y)                                     \
	X("axis_camera_rz", DTTR_GAMEPAD_AXIS_IDX_CAMERA_RZ)

#define S_CONFIG_GAMEPAD_DEADZONE_KEYS(X)                                                \
	X("deadzone_stick_x", DTTR_GAMEPAD_AXIS_IDX_STICK_X)                                 \
	X("deadzone_stick_y", DTTR_GAMEPAD_AXIS_IDX_STICK_Y)                                 \
	X("deadzone_camera_rz", DTTR_GAMEPAD_AXIS_IDX_CAMERA_RZ)

#define S_CONFIG_KEY_INDEX(key_name, key_index) {key_name, key_index},

static const S_ConfigKeyIndex s_gamepad_axis_keys[] = {
	S_CONFIG_GAMEPAD_AXIS_KEYS(S_CONFIG_KEY_INDEX)
};

static const S_ConfigKeyIndex s_gamepad_deadzone_keys[] = {
	S_CONFIG_GAMEPAD_DEADZONE_KEYS(S_CONFIG_KEY_INDEX)
};

#undef S_CONFIG_KEY_INDEX

int s_config_schema_count(void) {
	return (int)(sizeof(s_config_schema) / sizeof(s_config_schema[0]));
}

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
		const khint_t it = kh_put(
			dttr_config_lookup,
			g_dttr_config_lookup,
			(char *)spec->key,
			&put_ret
		);
		if (it != kh_end(g_dttr_config_lookup)) {
			kh_value(g_dttr_config_lookup, it) = i;
		}
	}
}

static bool s_config_sections_match(const char *lhs, const char *rhs) {
	return lhs == rhs || (lhs && rhs && strcmp(lhs, rhs) == 0);
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

	const S_ConfigFieldSpec *const spec = &s_config_schema
											  [kh_value(g_dttr_config_lookup, it)];
	if (!s_config_sections_match(spec->section, section)) {
		return NULL;
	}

	return spec;
}

#define S_CONFIG_ASSIGN_TYPES(X)                                                         \
	X(S_CONFIG_BOOL, bool, false, s_config_parse_bool, s_config_assign_bool)             \
	X(S_CONFIG_SCALING_FIT,                                                              \
	  DTTR_ScalingMode,                                                                  \
	  DTTR_SCALING_MODE_LETTERBOX,                                                       \
	  s_config_parse_scaling_fit,                                                        \
	  s_config_assign_scaling_fit)                                                       \
	X(S_CONFIG_SCALING_METHOD,                                                           \
	  DTTR_ScalingMethod,                                                                \
	  DTTR_SCALING_METHOD_PRESENT,                                                       \
	  s_config_parse_scaling_method,                                                     \
	  s_config_assign_scaling_method)                                                    \
	X(S_CONFIG_GRAPHICS_API,                                                             \
	  DTTR_GraphicsApi,                                                                  \
	  DTTR_GRAPHICS_API_AUTO,                                                            \
	  s_config_parse_graphics_api,                                                       \
	  s_config_assign_graphics_api)                                                      \
	X(S_CONFIG_INT, int, 0, s_config_parse_int, s_config_assign_int)                     \
	X(S_CONFIG_FLOAT, float, 0.0f, s_config_parse_float, s_config_assign_float)          \
	X(S_CONFIG_PRESENT_FILTER,                                                           \
	  SDL_GPUFilter,                                                                     \
	  SDL_GPU_FILTER_LINEAR,                                                             \
	  s_config_parse_present_filter,                                                     \
	  s_config_assign_present_filter)                                                    \
	X(S_CONFIG_LOG_LEVEL,                                                                \
	  int,                                                                               \
	  LOG_INFO,                                                                          \
	  s_config_parse_log_level,                                                          \
	  s_config_assign_log_level)                                                         \
	X(S_CONFIG_MINIDUMP_TYPE,                                                            \
	  DTTR_MinidumpType,                                                                 \
	  DTTR_MINIDUMP_NORMAL,                                                              \
	  s_config_parse_minidump_type,                                                      \
	  s_config_assign_minidump_type)                                                     \
	X(S_CONFIG_VERTEX_PRECISION,                                                         \
	  DTTR_VertexPrecision,                                                              \
	  DTTR_VERTEX_PRECISION_SUBPIXEL,                                                    \
	  s_config_parse_vertex_precision,                                                   \
	  s_config_assign_vertex_precision)

#define S_CONFIG_ASSIGN_FN(value_type, type, default_val, parse_fn, fn_name)             \
	static bool fn_name(char *field, const char *value) {                                \
		type parsed = default_val;                                                       \
		if (!parse_fn(value, &parsed)) {                                                 \
			return false;                                                                \
		}                                                                                \
		*(type *)field = parsed;                                                         \
		return true;                                                                     \
	}

S_CONFIG_ASSIGN_TYPES(S_CONFIG_ASSIGN_FN)

#undef S_CONFIG_ASSIGN_FN

static bool s_config_assign_string(char *field, const char *value) {
	return s_config_parse_string(value, field, sizeof(((DTTR_Config *)0)->m_pcdogs_path));
}

bool s_config_apply_entry(
	DTTR_Config *config,
	const char *section,
	const char *key,
	const char *value
) {
	if (!config || !key || !value) {
		return false;
	}

	const S_ConfigFieldSpec *const spec = s_config_schema_find(section, key);
	if (!spec) {
		return false;
	}

	char *const field = ((char *)config) + spec->offset;
#define S_CONFIG_ASSIGN_CASE(value_type, type, default_val, parse_fn, fn_name)           \
	case value_type:                                                                     \
		return fn_name(field, value);

	switch (spec->value_type) {
		S_CONFIG_ASSIGN_TYPES(S_CONFIG_ASSIGN_CASE)
	case S_CONFIG_STRING:
		return s_config_assign_string(field, value);

	default:
		return false;
	}

#undef S_CONFIG_ASSIGN_CASE
}

static int s_config_lookup_index(
	const S_ConfigKeyIndex *entries,
	size_t entry_count,
	const char *key
) {
	for (size_t i = 0; i < entry_count; i++) {
		if (strcmp(entries[i].key, key) == 0) {
			return entries[i].index;
		}
	}

	return -1;
}

bool s_config_apply_gamepad_entry(
	DTTR_Config *config,
	const char *section,
	const char *key,
	const char *value
) {
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

	if (strcmp(key, "index") == 0) {
		int index = 0;

		if (!s_config_parse_int(value, &index)) {
			return false;
		}

		config->m_gamepad_index = index;
		return true;
	}

	const int axis_index = s_config_lookup_index(
		s_gamepad_axis_keys,
		sizeof(s_gamepad_axis_keys) / sizeof(s_gamepad_axis_keys[0]),
		key
	);
	if (axis_index >= 0) {
		int axis = DTTR_GAMEPAD_MAPPING_NONE;
		if (!s_config_parse_gamepad_axis(value, &axis)) {
			return false;
		}

		config->m_gamepad_axes[axis_index] = axis;
		return true;
	}

	const int deadzone_index = s_config_lookup_index(
		s_gamepad_deadzone_keys,
		sizeof(s_gamepad_deadzone_keys) / sizeof(s_gamepad_deadzone_keys[0]),
		key
	);
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
