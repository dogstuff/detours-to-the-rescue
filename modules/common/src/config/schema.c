#include "config_internal.h"
#include <dttr_log.h>
#include <dttr_path.h>

#include <khash.h>

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define CONFIG_LOOKUP_KEY_CAPACITY 64

KHASH_MAP_INIT_STR(dttr_config_lookup, int)

// clang-format off
#define FIELD(_section, _key, _field, _type) \
	{ \
		.section = (_section), \
		.key = (_key), \
		.offset = offsetof(DTTR_Config, _field), \
		.size = sizeof(((DTTR_Config *)0)->_field), \
		.value_type = (_type) \
	}

#define FIELD_TOP(_key, _field, _type) FIELD(NULL, _key, _field, _type)
#define FIELD_GAMEPAD_AXIS(_key, _index) \
	FIELD("gamepad", _key, gamepad_axes[_index], CONFIG_GAMEPAD_AXIS),
#define FIELD_GAMEPAD_DEADZONE(_key, _index) \
	FIELD("gamepad", _key, gamepad_axis_deadzone[_index], CONFIG_INT),
#define FIELD_GAMEPAD_SENSITIVITY(_key, _index) \
	FIELD("gamepad", _key, gamepad_axis_sensitivity[_index], CONFIG_INT),

static const DTTR_ConfigFieldSpec config_schema[] = {
	FIELD_TOP("schema_major_version", schema_major_version, CONFIG_INT),
	FIELD("graphics", "scaling_fit", scaling_fit, CONFIG_SCALING_FIT),
	FIELD("graphics", "scaling_method", scaling_method, CONFIG_SCALING_METHOD),
	FIELD("graphics", "graphics_api", graphics_api, CONFIG_GRAPHICS_API),
	FIELD(
		"graphics",
		"present_scaling_algorithm",
		present_filter,
		CONFIG_PRESENT_FILTER
	),
	FIELD("graphics", "window_width", window_width, CONFIG_INT),
	FIELD("graphics", "window_height", window_height, CONFIG_INT),
	FIELD("graphics", "msaa_samples", msaa_samples, CONFIG_INT),
	FIELD("graphics", "texture_upload_sync", texture_upload_sync, CONFIG_BOOL),
	FIELD("graphics", "generate_texture_mipmaps", generate_texture_mipmaps, CONFIG_BOOL),
	FIELD("graphics", "vertex_precision", vertex_precision, CONFIG_VERTEX_PRECISION),
	FIELD("graphics", "sprite_smooth", sprite_smooth, CONFIG_BOOL),
	FIELD("graphics", "fullscreen", fullscreen, CONFIG_BOOL),

	FIELD("audio", "mss_sample_gain", mss_sample_gain, CONFIG_FLOAT),
	FIELD("audio", "mss_sample_preemphasis", mss_sample_preemphasis, CONFIG_FLOAT),

	FIELD("modding", "hot_reload", hot_reload, CONFIG_BOOL),

	FIELD_TOP("log_level", log_level, CONFIG_LOG_LEVEL),
	FIELD_TOP("minidump_type", minidump_type, CONFIG_MINIDUMP_TYPE),
	FIELD_TOP("show_crash_popup", show_crash_popup, CONFIG_BOOL),
	FIELD_TOP("log_file_path", log_file_path, CONFIG_STRING),
	FIELD_TOP("pcdogs_path", pcdogs_path, CONFIG_STRING),
	FIELD_TOP("saves_path", saves_path, CONFIG_STRING),
	FIELD_TOP("skip_intro_movies", skip_intro_movies, CONFIG_BOOL),

	FIELD("gamepad", "enabled", gamepad_enabled, CONFIG_BOOL),
	FIELD("gamepad", "analog_remap", gamepad_analog_remap, CONFIG_BOOL),
	FIELD("gamepad", "index", gamepad_index, CONFIG_INT),
	CONFIG_GAMEPAD_AXIS_FIELDS(FIELD_GAMEPAD_AXIS)
	CONFIG_GAMEPAD_DEADZONE_FIELDS(FIELD_GAMEPAD_DEADZONE)
	CONFIG_GAMEPAD_SENSITIVITY_FIELDS(FIELD_GAMEPAD_SENSITIVITY)
};

// clang-format on

#define CONFIG_SCHEMA_COUNT ((int)SDL_arraysize(config_schema))

#undef FIELD_GAMEPAD_SENSITIVITY
#undef FIELD_GAMEPAD_DEADZONE
#undef FIELD_GAMEPAD_AXIS

static khash_t(dttr_config_lookup) *config_lookup = NULL;
static char config_lookup_keys[CONFIG_SCHEMA_COUNT][CONFIG_LOOKUP_KEY_CAPACITY];

static bool config_build_lookup_key(
	char *out,
	size_t out_size,
	const char *section,
	const char *key
) {
	const int written = section ? snprintf(out, out_size, "%s.%s", section, key)
								: snprintf(out, out_size, "%s", key);
	return written > 0 && (size_t)written < out_size;
}

int DTTR_Config_SchemaCount() {
	return CONFIG_SCHEMA_COUNT;
}

const DTTR_ConfigFieldSpec *DTTR_Config_SchemaGet(int index) {
	if (index < 0 || index >= CONFIG_SCHEMA_COUNT) {
		return NULL;
	}

	return &config_schema[index];
}

static const char *config_field_bytes(
	const DTTR_Config *config,
	const DTTR_ConfigFieldSpec *spec
) {
	return ((const char *)config) + spec->offset;
}

bool DTTR_Config_FieldChanged(
	const DTTR_Config *current,
	const DTTR_Config *base,
	const DTTR_ConfigFieldSpec *spec
) {
	if (!current || !base || !spec) {
		return false;
	}

	const char *current_field = config_field_bytes(current, spec);
	const char *base_field = config_field_bytes(base, spec);
	if (spec->value_type == DTTR_CONFIG_VALUE_STRING) {
		return !DTTR_Path_MatchesNormalized(current_field, base_field);
	}

	return spec->size > 0 && memcmp(current_field, base_field, spec->size) != 0;
}

bool DTTR_Config_SchemaChanged(const DTTR_Config *current, const DTTR_Config *base) {
	for (int i = 0; i < CONFIG_SCHEMA_COUNT; i++) {
		const DTTR_ConfigFieldSpec *spec = &config_schema[i];
		if (DTTR_Config_FieldChanged(current, base, spec)) {
			return true;
		}
	}

	return false;
}

static void config_schema_init() {
	if (config_lookup) {
		return;
	}

	config_lookup = kh_init(dttr_config_lookup);
	if (!config_lookup) {
		return;
	}

	for (int i = 0; i < CONFIG_SCHEMA_COUNT; i++) {
		const DTTR_ConfigFieldSpec *spec = &config_schema[i];
		if (!config_build_lookup_key(
				config_lookup_keys[i],
				sizeof(config_lookup_keys[i]),
				spec->section,
				spec->key
			)) {
			continue;
		}

		int put_ret = 0;
		const khint_t it = kh_put(
			dttr_config_lookup,
			config_lookup,
			config_lookup_keys[i],
			&put_ret
		);
		if (it != kh_end(config_lookup)) {
			kh_value(config_lookup, it) = i;
		}
	}
}

const DTTR_ConfigFieldSpec *config_schema_find(const char *section, const char *key) {
	config_schema_init();
	if (!config_lookup) {
		return NULL;
	}

	char lookup_key[CONFIG_LOOKUP_KEY_CAPACITY];
	if (!config_build_lookup_key(lookup_key, sizeof(lookup_key), section, key)) {
		return NULL;
	}

	const khint_t it = kh_get(dttr_config_lookup, config_lookup, lookup_key);
	if (it == kh_end(config_lookup)) {
		return NULL;
	}

	const int index = kh_value(config_lookup, it);
	return DTTR_Config_SchemaGet(index);
}

#define CONFIG_ASSIGN_TYPES(X)                                                           \
	X(CONFIG_BOOL, bool, false, config_parse_bool, config_assign_bool)                   \
	X(CONFIG_SCALING_FIT,                                                                \
	  DTTR_ScalingMode,                                                                  \
	  DTTR_SCALING_MODE_LETTERBOX,                                                       \
	  config_parse_scaling_fit,                                                          \
	  config_assign_scaling_fit)                                                         \
	X(CONFIG_SCALING_METHOD,                                                             \
	  DTTR_ScalingMethod,                                                                \
	  DTTR_SCALING_METHOD_PRESENT,                                                       \
	  config_parse_scaling_method,                                                       \
	  config_assign_scaling_method)                                                      \
	X(CONFIG_GRAPHICS_API,                                                               \
	  DTTR_GraphicsApi,                                                                  \
	  DTTR_GRAPHICS_API_AUTO,                                                            \
	  config_parse_graphics_api,                                                         \
	  config_assign_graphics_api)                                                        \
	X(CONFIG_INT, int, 0, config_parse_int, config_assign_int)                           \
	X(CONFIG_FLOAT, float, 0.0f, config_parse_float, config_assign_float)                \
	X(CONFIG_PRESENT_FILTER,                                                             \
	  SDL_GPUFilter,                                                                     \
	  SDL_GPU_FILTER_LINEAR,                                                             \
	  config_parse_present_filter,                                                       \
	  config_assign_present_filter)                                                      \
	X(CONFIG_LOG_LEVEL, int, LOG_INFO, config_parse_log_level, config_assign_log_level)  \
	X(CONFIG_MINIDUMP_TYPE,                                                              \
	  DTTR_MinidumpType,                                                                 \
	  DTTR_MINIDUMP_NORMAL,                                                              \
	  config_parse_minidump_type,                                                        \
	  config_assign_minidump_type)                                                       \
	X(CONFIG_VERTEX_PRECISION,                                                           \
	  DTTR_VertexPrecision,                                                              \
	  DTTR_VERTEX_PRECISION_NATIVE,                                                      \
	  config_parse_vertex_precision,                                                     \
	  config_assign_vertex_precision)                                                    \
	X(CONFIG_GAMEPAD_AXIS,                                                               \
	  int,                                                                               \
	  DTTR_GAMEPAD_MAPPING_NONE,                                                         \
	  config_parse_gamepad_axis,                                                         \
	  config_assign_gamepad_axis)

#define CONFIG_ASSIGN_FN(value_type, type, default_val, parse_fn, fn_name)               \
	static bool fn_name(char *field, const char *value) {                                \
		type parsed = default_val;                                                       \
		if (!parse_fn(value, &parsed)) {                                                 \
			return false;                                                                \
		}                                                                                \
		*(type *)field = parsed;                                                         \
		return true;                                                                     \
	}

CONFIG_ASSIGN_TYPES(CONFIG_ASSIGN_FN)

#undef CONFIG_ASSIGN_FN

static bool config_assign_string(char *field, size_t field_size, const char *value) {
	return field_size > 0 && config_parse_string(value, field, field_size);
}

bool config_apply_entry(
	DTTR_Config *config,
	const char *section,
	const char *key,
	const char *value
) {
	if (!config || !key || !value) {
		return false;
	}

	const DTTR_ConfigFieldSpec *spec = config_schema_find(section, key);
	if (!spec) {
		return false;
	}

	char *field = ((char *)config) + spec->offset;
#define CONFIG_ASSIGN_CASE(value_type, type, default_val, parse_fn, fn_name)             \
	case value_type:                                                                     \
		return fn_name(field, value);

	switch (spec->value_type) {
		CONFIG_ASSIGN_TYPES(CONFIG_ASSIGN_CASE)
	case CONFIG_STRING:
		return config_assign_string(field, spec->size, value);

	default:
		return false;
	}

#undef CONFIG_ASSIGN_CASE
}

#undef CONFIG_SCHEMA_COUNT
