#include "config_internal.h"
#include "sds.h"
#include "yyjson.h"
#include <dttr_log.h>

#include <math.h>
#include <stdarg.h>
#include <string.h>

#include "dttr_errors.h"

static sds s_errors;

static void s_errors_clear(void) {
	if (s_errors) {
		sdsclear(s_errors);
	} else {
		s_errors = sdsempty();
	}
}

static void s_errors_addf(const char *fmt, ...) {
	if (!s_errors) {
		return;
	}

	va_list args;
	va_start(args, fmt);
	s_errors = sdscatvprintf(s_errors, fmt, args);
	va_end(args);
	s_errors = sdscat(s_errors, "\n");
}

static void s_errors_show(void) {
	if (!s_errors || sdslen(s_errors) < 1) {
		return;
	}

	DTTR_FATAL("Configuration Error: %s", s_errors);
}

static void s_errors_add_invalid_value(
	const char *section,
	const char *key,
	const char *value
) {
	if (section) {
		s_errors_addf("%s.%s: invalid value \"%s\"", section, key, value);
		return;
	}

	s_errors_addf("%s: invalid value \"%s\"", key, value);
}

static void s_config_apply_buttons(DTTR_Config *config, yyjson_val *buttons) {
	if (!yyjson_is_obj(buttons)) {
		return;
	}

	s_config_clear_button_map(config->m_gamepad_button_map);
	yyjson_val *key;
	yyjson_val *val;
	size_t idx;
	size_t max;

	yyjson_obj_foreach(buttons, idx, max, key, val) {
		const char *key_str = yyjson_get_str(key);

		int source = -1;
		if (!s_config_parse_gamepad_source(key_str, &source)) {
			s_errors_addf("gamepad.buttons.%s: unknown SDL input", key_str);
			continue;
		}

		const char *value = yyjson_get_str(val);
		if (!value) {
			s_errors_addf("gamepad.buttons.%s: expected string value", key_str);
			continue;
		}

		int action = DTTR_GAMEPAD_MAPPING_NONE;
		if (!s_config_parse_game_action(value, &action)) {
			s_errors_addf("gamepad.buttons.%s: invalid action \"%s\"", key_str, value);
			continue;
		}

		config->m_gamepad_button_map[source] = action;
	}
}

static void s_config_apply_disabled_components(
	DTTR_Config *config,
	yyjson_val *disabled_components
) {
	if (!yyjson_is_arr(disabled_components)) {
		return;
	}

	config->m_disabled_component_count = 0;

	yyjson_val *val;
	size_t idx;
	size_t max;
	yyjson_arr_foreach(disabled_components, idx, max, val) {
		const char *component_filename = yyjson_get_str(val);
		if (!component_filename || !component_filename[0]) {
			s_errors_addf("modding.disabled_components[%zu]: expected DLL filename", idx);
			continue;
		}

		if (!dttr_config_set_component_enabled(config, component_filename, false)) {
			s_errors_addf(
				"modding.disabled_components[%zu]: could not store \"%s\"",
				idx,
				component_filename
			);
		}
	}
}

static bool s_validate_schema_major_version(yyjson_val *root, const char *filename) {
	yyjson_val *version = yyjson_obj_get(root, "schema_major_version");
	if (!version) {
		s_errors_addf("%s: missing required schema_major_version", filename);
		s_errors_show();
		return false;
	}

	if (!yyjson_is_int(version)) {
		s_errors_addf("%s.schema_major_version: expected integer value", filename);
		s_errors_show();
		return false;
	}

	const int schema_major_version = (int)yyjson_get_int(version);
	if (schema_major_version == DTTR_CONFIG_SCHEMA_MAJOR_VERSION) {
		return true;
	}

	s_errors_addf(
		"%s.schema_major_version: unsupported major version %d (expected %d)",
		filename,
		schema_major_version,
		DTTR_CONFIG_SCHEMA_MAJOR_VERSION
	);
	s_errors_show();
	return false;
}

static bool s_config_apply_json_value(
	DTTR_Config *config,
	const DTTR_ConfigFieldSpec *spec,
	yyjson_val *val
) {
	if (!config || !spec || !val) {
		return false;
	}

	char *const field = ((char *)config) + spec->offset;
	switch (spec->value_type) {
	case S_CONFIG_BOOL:
		if (!yyjson_is_bool(val)) {
			return false;
		}

		*(bool *)field = yyjson_get_bool(val);
		return true;

	case S_CONFIG_INT:
		if (!yyjson_is_int(val)) {
			return false;
		}

		*(int *)field = yyjson_get_int(val);
		return true;

	case S_CONFIG_FLOAT: {
		if (!yyjson_is_num(val)) {
			return false;
		}

		const double parsed = yyjson_get_num(val);
		if (!isfinite(parsed)) {
			return false;
		}

		*(float *)field = (float)parsed;
		return true;
	}

	case S_CONFIG_STRING:
		if (yyjson_is_null(val)) {
			return s_config_apply_entry(config, spec->section, spec->key, "");
		}

		if (!yyjson_is_str(val)) {
			return false;
		}

		return s_config_apply_entry(config, spec->section, spec->key, yyjson_get_str(val));

	default:
		if (!yyjson_is_str(val)) {
			return false;
		}

		return s_config_apply_entry(config, spec->section, spec->key, yyjson_get_str(val));
	}
}

static void s_apply_section(yyjson_val *obj, const char *section) {
	if (!yyjson_is_obj(obj)) {
		return;
	}

	yyjson_val *key;
	yyjson_val *val;
	size_t idx;
	size_t max;

	yyjson_obj_foreach(obj, idx, max, key, val) {
		if (yyjson_is_obj(val)) {
			continue;
		}

		const char *key_str = yyjson_get_str(key);
		const DTTR_ConfigFieldSpec *spec = s_config_schema_find(section, key_str);
		if (!spec) {
			if (!yyjson_is_arr(val)) {
				s_errors_add_invalid_value(section, key_str, yyjson_get_type_desc(val));
			}

			continue;
		}

		if (!s_config_apply_json_value(&g_dttr_config, spec, val)) {
			s_errors_add_invalid_value(section, key_str, yyjson_get_type_desc(val));
		}
	}
}

bool dttr_config_load(const char *filename) {
	s_errors_clear();

	if (!filename || !filename[0]) {
		DTTR_LOG_ERROR("Load failed: empty filename");
		dttr_config_set_defaults(&g_dttr_config);
		return false;
	}

	dttr_config_set_defaults(&g_dttr_config);

	yyjson_read_err err;
	yyjson_doc *doc = yyjson_read_file(filename, 0, NULL, &err);

	if (!doc) {
		if (err.code == YYJSON_READ_ERROR_FILE_OPEN) {
			DTTR_LOG_WARN("File '%s' not found. Creating it from defaults.", filename);
			return dttr_config_save(filename, &g_dttr_config);
		}

		if (err.code == YYJSON_READ_ERROR_EMPTY_CONTENT) {
			DTTR_LOG_WARN("File '%s' is empty. Using defaults.", filename);
			return true;
		}

		DTTR_LOG_ERROR("JSON parse failed: %s at position %zu", err.msg, err.pos);
		s_errors_addf(
			"Failed to parse %s (%s at position %zu)",
			filename,
			err.msg,
			err.pos
		);
		s_errors_show();
		return false;
	}

	yyjson_val *root = yyjson_doc_get_root(doc);
	if (!s_validate_schema_major_version(root, filename)) {
		yyjson_doc_free(doc);
		return false;
	}

	s_apply_section(root, NULL);
	s_apply_section(yyjson_obj_get(root, "graphics"), "graphics");
	s_apply_section(yyjson_obj_get(root, "audio"), "audio");
	yyjson_val *modding = yyjson_obj_get(root, "modding");
	s_apply_section(modding, "modding");
	s_config_apply_disabled_components(
		&g_dttr_config,
		yyjson_obj_get(modding, "disabled_components")
	);

	yyjson_val *gamepad = yyjson_obj_get(root, "gamepad");
	if (yyjson_is_obj(gamepad)) {
		s_apply_section(gamepad, "gamepad");
		s_config_apply_buttons(&g_dttr_config, yyjson_obj_get(gamepad, "buttons"));
	}

	yyjson_doc_free(doc);
	s_errors_show();
	return true;
}

static bool s_obj_add_strcpy(
	yyjson_mut_doc *doc,
	yyjson_mut_val *obj,
	const char *key,
	const char *value
) {
	return yyjson_mut_obj_add_strcpy(doc, obj, key, value ? value : "");
}

typedef struct {
	yyjson_mut_val *root;
	yyjson_mut_val *graphics;
	yyjson_mut_val *audio;
	yyjson_mut_val *modding;
	yyjson_mut_val *gamepad;
} S_ConfigJsonObjects;

static yyjson_mut_val *s_config_object_for_section(
	const S_ConfigJsonObjects *objects,
	const char *section
) {
	if (s_config_sections_match(section, NULL)) {
		return objects->root;
	}

	if (s_config_sections_match(section, "graphics")) {
		return objects->graphics;
	}

	if (s_config_sections_match(section, "audio")) {
		return objects->audio;
	}

	if (s_config_sections_match(section, "modding")) {
		return objects->modding;
	}

	if (s_config_sections_match(section, "gamepad")) {
		return objects->gamepad;
	}

	return NULL;
}

static const char *s_config_format_field_string(
	const DTTR_Config *config,
	const DTTR_ConfigFieldSpec *spec
) {
	const char *const field = ((const char *)config) + spec->offset;
	switch (spec->value_type) {
	case S_CONFIG_SCALING_FIT:
		return s_config_format_scaling_fit(*(const DTTR_ScalingMode *)field);
	case S_CONFIG_SCALING_METHOD:
		return s_config_format_scaling_method(*(const DTTR_ScalingMethod *)field);
	case S_CONFIG_GRAPHICS_API:
		return s_config_format_graphics_api(*(const DTTR_GraphicsApi *)field);
	case S_CONFIG_PRESENT_FILTER:
		return s_config_format_present_filter(*(const SDL_GPUFilter *)field);
	case S_CONFIG_LOG_LEVEL:
		return s_config_format_log_level(*(const int *)field);
	case S_CONFIG_MINIDUMP_TYPE:
		return s_config_format_minidump_type(*(const DTTR_MinidumpType *)field);
	case S_CONFIG_VERTEX_PRECISION:
		return s_config_format_vertex_precision(*(const DTTR_VertexPrecision *)field);
	case S_CONFIG_GAMEPAD_AXIS:
		return s_config_format_gamepad_axis(*(const int *)field);
	case S_CONFIG_STRING:
		return field;
	default:
		return NULL;
	}
}

static bool s_config_add_schema_field(
	yyjson_mut_doc *doc,
	const S_ConfigJsonObjects *objects,
	const DTTR_Config *config,
	const DTTR_ConfigFieldSpec *spec
) {
	yyjson_mut_val *obj = s_config_object_for_section(objects, spec->section);
	if (!obj) {
		return false;
	}

	const char *const field = ((const char *)config) + spec->offset;
	switch (spec->value_type) {
	case S_CONFIG_BOOL:
		return yyjson_mut_obj_add_bool(doc, obj, spec->key, *(const bool *)field);
	case S_CONFIG_INT:
		return yyjson_mut_obj_add_int(doc, obj, spec->key, *(const int *)field);
	case S_CONFIG_FLOAT:
		return yyjson_mut_obj_add_real(doc, obj, spec->key, *(const float *)field);
	default:
		return s_obj_add_strcpy(
			doc,
			obj,
			spec->key,
			s_config_format_field_string(config, spec)
		);
	}
}

static bool s_config_add_schema_fields(
	yyjson_mut_doc *doc,
	const S_ConfigJsonObjects *objects,
	const DTTR_Config *config
) {
	const int count = dttr_config_schema_count();
	for (int i = 0; i < count; i++) {
		const DTTR_ConfigFieldSpec *spec = dttr_config_schema_get(i);
		if (!spec || !s_config_add_schema_field(doc, objects, config, spec)) {
			return false;
		}
	}

	return true;
}

static bool s_config_add_disabled_components(
	yyjson_mut_doc *doc,
	yyjson_mut_val *modding,
	const DTTR_Config *config
) {
	yyjson_mut_val *disabled_components = yyjson_mut_obj_add_arr(
		doc,
		modding,
		"disabled_components"
	);
	if (!disabled_components) {
		return false;
	}

	for (int i = 0; i < config->m_disabled_component_count; i++) {
		if (!yyjson_mut_arr_add_strcpy(
				doc,
				disabled_components,
				config->m_disabled_components[i]
			)) {
			return false;
		}
	}

	return true;
}

static bool s_config_add_gamepad_buttons(
	yyjson_mut_doc *doc,
	yyjson_mut_val *buttons,
	const DTTR_Config *config
) {
	for (int i = 0; i < DTTR_GAMEPAD_SOURCE_COUNT; i++) {
		const char *source_name = s_config_format_gamepad_source(i);
		if (!source_name) {
			continue;
		}

		if (!s_obj_add_strcpy(
				doc,
				buttons,
				source_name,
				s_config_format_game_action(config->m_gamepad_button_map[i])
			)) {
			return false;
		}
	}

	return true;
}

bool dttr_config_save(const char *filename, const DTTR_Config *config) {
	if (!filename || !config) {
		return false;
	}

	bool ok = false;
	yyjson_mut_doc *doc = yyjson_mut_doc_new(NULL);
	if (!doc) {
		return false;
	}

	yyjson_mut_val *root = yyjson_mut_obj(doc);
	if (!root) {
		goto done;
	}

	yyjson_mut_doc_set_root(doc, root);

	S_ConfigJsonObjects objects = {
		.root = root,
		.graphics = yyjson_mut_obj_add_obj(doc, root, "graphics"),
		.audio = yyjson_mut_obj_add_obj(doc, root, "audio"),
		.modding = yyjson_mut_obj_add_obj(doc, root, "modding"),
		.gamepad = yyjson_mut_obj_add_obj(doc, root, "gamepad"),
	};

	if (!objects.graphics || !objects.audio || !objects.modding || !objects.gamepad) {
		goto done;
	}

	yyjson_mut_val *buttons = yyjson_mut_obj_add_obj(doc, objects.gamepad, "buttons");
	if (!buttons) {
		goto done;
	}

	ok = s_config_add_schema_fields(doc, &objects, config)
		 && s_config_add_disabled_components(doc, objects.modding, config)
		 && s_config_add_gamepad_buttons(doc, buttons, config);

	if (ok) {
		yyjson_write_err err;
		ok = yyjson_mut_write_file(
			filename,
			doc,
			YYJSON_WRITE_PRETTY | YYJSON_WRITE_NEWLINE_AT_END,
			NULL,
			&err
		);
		if (!ok) {
			DTTR_LOG_ERROR("Failed to write config %s: %s", filename, err.msg);
		}
	}

done:
	yyjson_mut_doc_free(doc);
	return ok;
}
