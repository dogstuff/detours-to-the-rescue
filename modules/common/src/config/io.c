#include "config_internal.h"
#include "sds.h"
#include "yyjson.h"
#include <dttr_log.h>

#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>

#include "dttr_errors.h"

static sds errors;

static void errors_clear() {
	if (errors) {
		sdsclear(errors);
	} else {
		errors = sdsempty();
	}
}

static void errors_addf(const char *fmt, ...) {
	if (!errors) {
		return;
	}

	va_list args;
	va_start(args, fmt);
	errors = sdscatvprintf(errors, fmt, args);
	va_end(args);
	errors = sdscat(errors, "\n");
}

static bool errors_show() {
	if (!errors || sdslen(errors) < 1) {
		return false;
	}

	DTTR_LOG_ERROR("Configuration Error: %s", errors);
	return true;
}

const char *DTTR_Config_LastError() {
	return errors && sdslen(errors) > 0 ? errors : NULL;
}

static void errors_add_invalid_value(
	const char *section,
	const char *key,
	const char *value
) {
	if (section) {
		errors_addf("%s.%s: invalid value \"%s\"", section, key, value);
		return;
	}

	errors_addf("%s: invalid value \"%s\"", key, value);
}

static void config_apply_buttons(DTTR_Config *config, yyjson_val *buttons) {
	if (!yyjson_is_obj(buttons)) {
		return;
	}

	config_clear_button_map(config->gamepad_button_map);
	yyjson_val *key;
	yyjson_val *val;
	size_t idx;
	size_t max;

	yyjson_obj_foreach(buttons, idx, max, key, val) {
		const char *key_str = yyjson_get_str(key);

		int source = -1;
		if (!config_parse_gamepad_source(key_str, &source)) {
			errors_addf("gamepad.buttons.%s: unknown SDL input", key_str);
			continue;
		}

		const char *value = yyjson_get_str(val);
		if (!value) {
			errors_addf("gamepad.buttons.%s: expected string value", key_str);
			continue;
		}

		int action = DTTR_GAMEPAD_MAPPING_NONE;
		if (!config_parse_game_action(value, &action)) {
			errors_addf("gamepad.buttons.%s: invalid action \"%s\"", key_str, value);
			continue;
		}

		config->gamepad_button_map[source] = action;
	}
}

static void config_apply_disabled_mods(DTTR_Config *config, yyjson_val *disabled_mods) {
	if (!yyjson_is_arr(disabled_mods)) {
		return;
	}

	config->disabled_mod_count = 0;

	yyjson_val *val;
	size_t idx;
	size_t max;
	yyjson_arr_foreach(disabled_mods, idx, max, val) {
		const char *mod_filename = yyjson_get_str(val);
		if (!mod_filename || !mod_filename[0]) {
			errors_addf("modding.disabled_mods[%zu]: expected DLL filename", idx);
			continue;
		}

		if (!DTTR_Config_SetModEnabled(config, mod_filename, false)) {
			errors_addf(
				"modding.disabled_mods[%zu]: could not store \"%s\"",
				idx,
				mod_filename
			);
		}
	}
}

static bool validate_schema_major_version(yyjson_val *root, const char *filename) {
	yyjson_val *version = yyjson_obj_get(root, "schema_major_version");
	if (!version) {
		errors_addf("%s: missing required schema_major_version", filename);
		errors_show();
		return false;
	}

	if (!yyjson_is_int(version)) {
		errors_addf("%s.schema_major_version: expected integer value", filename);
		errors_show();
		return false;
	}

	const int64_t parsed_version = yyjson_get_int(version);
	if (parsed_version < INT_MIN || parsed_version > INT_MAX) {
		errors_addf("%s.schema_major_version: integer value out of range", filename);
		errors_show();
		return false;
	}

	const int schema_major_version = (int)parsed_version;
	if (schema_major_version == DTTR_CONFIG_SCHEMA_MAJOR_VERSION) {
		return true;
	}

	errors_addf(
		"%s.schema_major_version: unsupported major version %d (expected %d)",
		filename,
		schema_major_version,
		DTTR_CONFIG_SCHEMA_MAJOR_VERSION
	);
	errors_show();
	return false;
}

static bool config_apply_json_value(
	DTTR_Config *config,
	const DTTR_ConfigFieldSpec *spec,
	yyjson_val *val
) {
	if (!config || !spec || !val) {
		return false;
	}

	char *const field = ((char *)config) + spec->offset;
	switch (spec->value_type) {
	case CONFIG_BOOL:
		if (!yyjson_is_bool(val)) {
			return false;
		}

		*(bool *)field = yyjson_get_bool(val);
		return true;

	case CONFIG_INT:
		if (!yyjson_is_int(val)) {
			return false;
		}

		const int64_t parsed = yyjson_get_int(val);
		if (parsed < INT_MIN || parsed > INT_MAX) {
			return false;
		}

		*(int *)field = (int)parsed;
		return true;

	case CONFIG_FLOAT: {
		if (!yyjson_is_num(val)) {
			return false;
		}

		const double parsed = yyjson_get_num(val);
		if (!isfinite(parsed) || parsed < -(double)FLT_MAX || parsed > (double)FLT_MAX) {
			return false;
		}

		*(float *)field = (float)parsed;
		return true;
	}

	case CONFIG_STRING:
		if (yyjson_is_null(val)) {
			return config_apply_entry(config, spec->section, spec->key, "");
		}

		if (!yyjson_is_str(val)) {
			return false;
		}

		return config_apply_entry(config, spec->section, spec->key, yyjson_get_str(val));

	default:
		if (!yyjson_is_str(val)) {
			return false;
		}

		return config_apply_entry(config, spec->section, spec->key, yyjson_get_str(val));
	}
}

static void apply_section(yyjson_val *obj, const char *section) {
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
		const DTTR_ConfigFieldSpec *spec = config_schema_find(section, key_str);
		if (!spec) {
			if (!yyjson_is_arr(val)) {
				errors_add_invalid_value(section, key_str, yyjson_get_type_desc(val));
			}

			continue;
		}

		if (!config_apply_json_value(&dttr_config, spec, val)) {
			errors_add_invalid_value(section, key_str, yyjson_get_type_desc(val));
		}
	}
}

bool DTTR_Config_Load(const char *filename) {
	errors_clear();

	if (!filename || !filename[0]) {
		errors_addf("Load failed: empty filename");
		errors_show();
		DTTR_Config_SetDefaults(&dttr_config);
		return false;
	}

	DTTR_Config_SetDefaults(&dttr_config);

	yyjson_read_err err;
	yyjson_doc *doc = yyjson_read_file(filename, 0, NULL, &err);

	if (!doc) {
		if (err.code == YYJSON_READ_ERROR_FILE_OPEN) {
			DTTR_LOG_WARN("File '%s' not found. Creating it from defaults.", filename);
			if (!DTTR_Config_Save(filename, &dttr_config)) {
				DTTR_LOG_WARN(
					"Could not create default config %s; continuing with built-in "
					"defaults",
					filename
				);
			}
			return true;
		}

		if (err.code == YYJSON_READ_ERROR_EMPTY_CONTENT) {
			DTTR_LOG_WARN("File '%s' is empty. Using defaults.", filename);
			return true;
		}

		DTTR_LOG_ERROR("JSON parse failed: %s at position %zu", err.msg, err.pos);
		errors_addf("Failed to parse %s (%s at position %zu)", filename, err.msg, err.pos);
		errors_show();
		return false;
	}

	yyjson_val *root = yyjson_doc_get_root(doc);
	if (!validate_schema_major_version(root, filename)) {
		yyjson_doc_free(doc);
		return false;
	}

	apply_section(root, NULL);
	apply_section(yyjson_obj_get(root, "graphics"), "graphics");
	apply_section(yyjson_obj_get(root, "audio"), "audio");
	yyjson_val *modding = yyjson_obj_get(root, "modding");
	apply_section(modding, "modding");
	config_apply_disabled_mods(&dttr_config, yyjson_obj_get(modding, "disabled_mods"));

	yyjson_val *gamepad = yyjson_obj_get(root, "gamepad");
	if (yyjson_is_obj(gamepad)) {
		apply_section(gamepad, "gamepad");
		config_apply_buttons(&dttr_config, yyjson_obj_get(gamepad, "buttons"));
	}

	yyjson_doc_free(doc);
	return !errors_show();
}

static bool obj_add_strcpy(
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
} config_json_objects;

static yyjson_mut_val *config_object_for_section(
	const config_json_objects *objects,
	const char *section
) {
	if (config_sections_match(section, NULL)) {
		return objects->root;
	}

	if (config_sections_match(section, "graphics")) {
		return objects->graphics;
	}

	if (config_sections_match(section, "audio")) {
		return objects->audio;
	}

	if (config_sections_match(section, "modding")) {
		return objects->modding;
	}

	if (config_sections_match(section, "gamepad")) {
		return objects->gamepad;
	}

	return NULL;
}

static const char *config_format_field_string(
	const DTTR_Config *config,
	const DTTR_ConfigFieldSpec *spec
) {
	const char *const field = ((const char *)config) + spec->offset;
	switch (spec->value_type) {
	case CONFIG_SCALING_FIT:
		return config_format_scaling_fit(*(const DTTR_ScalingMode *)field);
	case CONFIG_SCALING_METHOD:
		return config_format_scaling_method(*(const DTTR_ScalingMethod *)field);
	case CONFIG_GRAPHICS_API:
		return config_format_graphics_api(*(const DTTR_GraphicsApi *)field);
	case CONFIG_PRESENT_FILTER:
		return config_format_present_filter(*(const SDL_GPUFilter *)field);
	case CONFIG_LOG_LEVEL:
		return config_format_log_level(*(const int *)field);
	case CONFIG_MINIDUMP_TYPE:
		return config_format_minidump_type(*(const DTTR_MinidumpType *)field);
	case CONFIG_VERTEX_PRECISION:
		return config_format_vertex_precision(*(const DTTR_VertexPrecision *)field);
	case CONFIG_GAMEPAD_AXIS:
		return config_format_gamepad_axis(*(const int *)field);
	case CONFIG_STRING:
		return field;
	default:
		return NULL;
	}
}

static bool config_add_schema_field(
	yyjson_mut_doc *doc,
	const config_json_objects *objects,
	const DTTR_Config *config,
	const DTTR_ConfigFieldSpec *spec
) {
	yyjson_mut_val *obj = config_object_for_section(objects, spec->section);
	if (!obj) {
		return false;
	}

	const char *const field = ((const char *)config) + spec->offset;
	switch (spec->value_type) {
	case CONFIG_BOOL:
		return yyjson_mut_obj_add_bool(doc, obj, spec->key, *(const bool *)field);
	case CONFIG_INT:
		return yyjson_mut_obj_add_int(doc, obj, spec->key, *(const int *)field);
	case CONFIG_FLOAT:
		return yyjson_mut_obj_add_real(doc, obj, spec->key, *(const float *)field);
	default:
		return obj_add_strcpy(
			doc,
			obj,
			spec->key,
			config_format_field_string(config, spec)
		);
	}
}

static bool config_add_schema_fields(
	yyjson_mut_doc *doc,
	const config_json_objects *objects,
	const DTTR_Config *config
) {
	const int count = DTTR_Config_SchemaCount();
	for (int i = 0; i < count; i++) {
		const DTTR_ConfigFieldSpec *spec = DTTR_Config_SchemaGet(i);
		if (!spec || !config_add_schema_field(doc, objects, config, spec)) {
			return false;
		}
	}

	return true;
}

static bool config_add_disabled_mods(
	yyjson_mut_doc *doc,
	yyjson_mut_val *modding,
	const DTTR_Config *config
) {
	yyjson_mut_val *disabled_mods = yyjson_mut_obj_add_arr(doc, modding, "disabled_mods");
	if (!disabled_mods) {
		return false;
	}

	for (int i = 0; i < config->disabled_mod_count; i++) {
		if (!yyjson_mut_arr_add_strcpy(doc, disabled_mods, config->disabled_mods[i])) {
			return false;
		}
	}

	return true;
}

static bool config_add_gamepad_buttons(
	yyjson_mut_doc *doc,
	yyjson_mut_val *buttons,
	const DTTR_Config *config
) {
	for (int i = 0; i < DTTR_GAMEPAD_SOURCE_COUNT; i++) {
		const char *source_name = config_format_gamepad_source(i);
		if (!source_name) {
			continue;
		}

		if (!obj_add_strcpy(
				doc,
				buttons,
				source_name,
				config_format_game_action(config->gamepad_button_map[i])
			)) {
			return false;
		}
	}

	return true;
}

bool DTTR_Config_Save(const char *filename, const DTTR_Config *config) {
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

	config_json_objects objects = {
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

	ok = config_add_schema_fields(doc, &objects, config)
		 && config_add_disabled_mods(doc, objects.modding, config)
		 && config_add_gamepad_buttons(doc, buttons, config);

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
