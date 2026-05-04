#include "config_internal.h"
#include <dttr_log.h>
#include <dttr_path.h>

#include <khash.h>

#include <stddef.h>
#include <string.h>

KHASH_MAP_INIT_STR(dttr_config_lookup, int)

// clang-format off
#define S_FIELD(_section, _key, _field, _type) \
	{ \
		.section = (_section), \
		.key = (_key), \
		.offset = offsetof(DTTR_Config, _field), \
		.size = sizeof(((DTTR_Config *)0)->_field), \
		.value_type = (_type) \
	}

#define S_FIELD_TOP(_key, _field, _type) S_FIELD(NULL, _key, _field, _type)
#define S_FIELD_GAMEPAD_AXIS(_key, _index) \
	S_FIELD("gamepad", _key, m_gamepad_axes[_index], S_CONFIG_GAMEPAD_AXIS),
#define S_FIELD_GAMEPAD_DEADZONE(_key, _index) \
	S_FIELD("gamepad", _key, m_gamepad_axis_deadzone[_index], S_CONFIG_INT),

static const DTTR_ConfigFieldSpec s_config_schema[] = {
	S_FIELD_TOP("schema_major_version", m_schema_major_version, S_CONFIG_INT),
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
	S_FIELD("graphics", "generate_texture_mipmaps", m_generate_texture_mipmaps, S_CONFIG_BOOL),
	S_FIELD("graphics", "vertex_precision", m_vertex_precision, S_CONFIG_VERTEX_PRECISION),
	S_FIELD("graphics", "sprite_smooth", m_sprite_smooth, S_CONFIG_BOOL),
	S_FIELD("graphics", "fullscreen", m_fullscreen, S_CONFIG_BOOL),

	S_FIELD("audio", "mss_sample_gain", m_mss_sample_gain, S_CONFIG_FLOAT),
	S_FIELD("audio", "mss_sample_preemphasis", m_mss_sample_preemphasis, S_CONFIG_FLOAT),

	S_FIELD("modding", "hot_reload", m_hot_reload, S_CONFIG_BOOL),

	S_FIELD_TOP("log_level", m_log_level, S_CONFIG_LOG_LEVEL),
	S_FIELD_TOP("minidump_type", m_minidump_type, S_CONFIG_MINIDUMP_TYPE),
	S_FIELD_TOP("log_file_path", m_log_file_path, S_CONFIG_STRING),
	S_FIELD_TOP("pcdogs_path", m_pcdogs_path, S_CONFIG_STRING),
	S_FIELD_TOP("saves_path", m_saves_path, S_CONFIG_STRING),

	S_FIELD("gamepad", "enabled", m_gamepad_enabled, S_CONFIG_BOOL),
	S_FIELD("gamepad", "index", m_gamepad_index, S_CONFIG_INT),
	S_CONFIG_GAMEPAD_AXIS_FIELDS(S_FIELD_GAMEPAD_AXIS)
	S_CONFIG_GAMEPAD_DEADZONE_FIELDS(S_FIELD_GAMEPAD_DEADZONE)
};

// clang-format on

#define S_CONFIG_SCHEMA_COUNT ((int)SDL_arraysize(s_config_schema))

#undef S_FIELD_GAMEPAD_DEADZONE
#undef S_FIELD_GAMEPAD_AXIS

static khash_t(dttr_config_lookup) *g_dttr_config_lookup = NULL;

int dttr_config_schema_count(void) { return S_CONFIG_SCHEMA_COUNT; }

const DTTR_ConfigFieldSpec *dttr_config_schema_get(int index) {
	if (index < 0 || index >= S_CONFIG_SCHEMA_COUNT) {
		return NULL;
	}

	return &s_config_schema[index];
}

static const char *s_config_field_bytes(
	const DTTR_Config *config,
	const DTTR_ConfigFieldSpec *spec
) {
	return ((const char *)config) + spec->offset;
}

bool dttr_config_field_changed(
	const DTTR_Config *current,
	const DTTR_Config *base,
	const DTTR_ConfigFieldSpec *spec
) {
	if (!current || !base || !spec) {
		return false;
	}

	const char *current_field = s_config_field_bytes(current, spec);
	const char *base_field = s_config_field_bytes(base, spec);
	if (spec->value_type == DTTR_CONFIG_VALUE_STRING) {
		return !dttr_path_matches_normalized(current_field, base_field);
	}

	return spec->size > 0 && memcmp(current_field, base_field, spec->size) != 0;
}

bool dttr_config_schema_changed(const DTTR_Config *current, const DTTR_Config *base) {
	for (int i = 0; i < S_CONFIG_SCHEMA_COUNT; i++) {
		const DTTR_ConfigFieldSpec *const spec = &s_config_schema[i];
		if (dttr_config_field_changed(current, base, spec)) {
			return true;
		}
	}

	return false;
}

static void s_config_schema_init(void) {
	if (g_dttr_config_lookup) {
		return;
	}

	g_dttr_config_lookup = kh_init(dttr_config_lookup);
	if (!g_dttr_config_lookup) {
		return;
	}

	for (int i = 0; i < S_CONFIG_SCHEMA_COUNT; i++) {
		const DTTR_ConfigFieldSpec *const spec = &s_config_schema[i];
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

const DTTR_ConfigFieldSpec *s_config_schema_find(const char *section, const char *key) {
	s_config_schema_init();
	if (!g_dttr_config_lookup) {
		return NULL;
	}

	const khint_t it = kh_get(dttr_config_lookup, g_dttr_config_lookup, key);
	if (it == kh_end(g_dttr_config_lookup)) {
		return NULL;
	}

	const int index = kh_value(g_dttr_config_lookup, it);
	const DTTR_ConfigFieldSpec *const spec = dttr_config_schema_get(index);
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
	  DTTR_VERTEX_PRECISION_NATIVE,                                                      \
	  s_config_parse_vertex_precision,                                                   \
	  s_config_assign_vertex_precision)                                                  \
	X(S_CONFIG_GAMEPAD_AXIS,                                                             \
	  int,                                                                               \
	  DTTR_GAMEPAD_MAPPING_NONE,                                                         \
	  s_config_parse_gamepad_axis,                                                       \
	  s_config_assign_gamepad_axis)

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

static bool s_config_assign_string(char *field, size_t field_size, const char *value) {
	return field_size > 0 && s_config_parse_string(value, field, field_size);
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

	const DTTR_ConfigFieldSpec *const spec = s_config_schema_find(section, key);
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
		return s_config_assign_string(field, spec->size, value);

	default:
		return false;
	}

#undef S_CONFIG_ASSIGN_CASE
}

#undef S_CONFIG_SCHEMA_COUNT
