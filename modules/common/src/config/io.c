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

#include <dttr_errors.h>

static const char *const CONFIG_SECTION_NAMES[] = {
	"graphics",
	"audio",
	"modding",
	"gamepad",
	"controls",
};

enum {
	CONFIG_SECTION_GRAPHICS,
	CONFIG_SECTION_AUDIO,
	CONFIG_SECTION_MODDING,
	CONFIG_SECTION_GAMEPAD,
	CONFIG_SECTION_CONTROLS,
	CONFIG_SECTION_COUNT,
};

typedef struct {
	sds text;
} config_error_buffer;

static config_error_buffer errors;

static void error_buffer_clear(config_error_buffer *buffer) {
	if (buffer->text) {
		sdsclear(buffer->text);
	} else {
		buffer->text = sdsempty();
	}
}

static void error_buffer_addv(config_error_buffer *buffer, const char *fmt, va_list args) {
	if (!buffer->text) {
		return;
	}

	buffer->text = sdscatvprintf(buffer->text, fmt, args);
	buffer->text = sdscat(buffer->text, "\n");
}

static bool error_buffer_show(const config_error_buffer *buffer) {
	if (!buffer->text || sdslen(buffer->text) < 1) {
		return false;
	}

	DTTR_LOG_ERROR("Configuration Error: %s", buffer->text);
	return true;
}

static void errors_clear() {
	error_buffer_clear(&errors);
}

static void errors_addf(const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	error_buffer_addv(&errors, fmt, args);
	va_end(args);
}

static bool errors_show() {
	return error_buffer_show(&errors);
}

typedef struct {
	yyjson_doc *value;
} config_json_doc;

typedef struct {
	yyjson_mut_doc *value;
} config_mut_json_doc;

static void config_json_doc_free(config_json_doc *doc) {
	if (!doc || !doc->value) {
		return;
	}

	yyjson_doc_free(doc->value);
	doc->value = NULL;
}

static void config_mut_json_doc_free(config_mut_json_doc *doc) {
	if (!doc || !doc->value) {
		return;
	}

	yyjson_mut_doc_free(doc->value);
	doc->value = NULL;
}

static const char *config_result_message(DTTR_Result result) {
	return result.message ? result.message : "no details";
}

const char *DTTR_Config_LastError() {
	return errors.text && sdslen(errors.text) > 0 ? errors.text : NULL;
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

static bool config_apply_control_binding_value(
	DTTR_Config *config,
	int action,
	yyjson_val *val
) {
	if (!config || action < 0 || action >= DTTR_CONFIG_CONTROL_ACTION_COUNT) {
		return false;
	}

	if (!yyjson_is_int(val)) {
		return false;
	}

	const int64_t parsed = yyjson_get_sint(val);
	if (parsed < INT_MIN || parsed > INT_MAX) {
		return false;
	}

	config->control_bindings[action] = (int)parsed;
	return true;
}

static void config_apply_control_bindings(DTTR_Config *config, yyjson_val *controls) {
	if (!controls) {
		return;
	}

	if (!yyjson_is_obj(controls)) {
		errors_addf("controls: expected object value");
		return;
	}

	const char *binding_key = "special_bindings";
	yyjson_val *bindings = yyjson_obj_get(controls, binding_key);
	if (!bindings) {
		return;
	}

	if (!yyjson_is_obj(bindings)) {
		errors_addf("controls.%s: expected object value", binding_key);
		return;
	}

	yyjson_val *key;
	yyjson_val *val;
	size_t idx;
	size_t max;

	yyjson_obj_foreach(bindings, idx, max, key, val) {
		const char *action_key = yyjson_get_str(key);
		const int action = DTTR_Config_ControlActionIndex(action_key);
		if (action < 0) {
			errors_addf("controls.%s.%s: unknown control action", binding_key, action_key);
			continue;
		}

		if (DTTR_Config_ControlActionInGameBindable(action)) {
			continue;
		}

		if (!config_apply_control_binding_value(config, action, val)) {
			errors_addf(
				"controls.%s.%s: invalid value \"%s\"",
				binding_key,
				action_key,
				yyjson_get_type_desc(val)
			);
		}
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

		DTTR_Result result = DTTR_Config_SetModEnabled(config, mod_filename, false);
		if (!DTTR_ResultOK(result)) {
			errors_addf(
				"modding.disabled_mods[%zu]: could not store \"%s\" (%s: %s)",
				idx,
				mod_filename,
				DTTR_StatusName(result.status),
				config_result_message(result)
			);
		}
	}
}

static DTTR_Result config_apply_mod_value(
	DTTR_Config *config,
	const char *mod_id,
	const char *field_id,
	yyjson_val *val
) {
	if (yyjson_is_bool(val)) {
		return DTTR_Config_SetModBool(config, mod_id, field_id, yyjson_get_bool(val));
	}

	// yyjson_is_int() also matches the UINT subtype; a literal above INT64_MAX is stored
	// as UINT and yyjson_get_sint() would reinterpret its bits into a wrong negative int
	// that still passes the signed range check. Validate the unsigned magnitude first.
	if (yyjson_is_uint(val)) {
		const uint64_t parsed = yyjson_get_uint(val);
		if (parsed > INT_MAX) {
			return config_result(
				DTTR_ERR_INVALID_ARGUMENT,
				"Mod integer value is out of range."
			);
		}

		return DTTR_Config_SetModInt(config, mod_id, field_id, (int)parsed);
	}

	if (yyjson_is_int(val)) {
		const int64_t parsed = yyjson_get_sint(val);
		if (parsed < INT_MIN || parsed > INT_MAX) {
			return config_result(
				DTTR_ERR_INVALID_ARGUMENT,
				"Mod integer value is out of range."
			);
		}

		return DTTR_Config_SetModInt(config, mod_id, field_id, (int)parsed);
	}

	if (yyjson_is_real(val)) {
		const double parsed = yyjson_get_real(val);
		if (!isfinite(parsed) || parsed < -(double)FLT_MAX || parsed > (double)FLT_MAX) {
			return config_result(
				DTTR_ERR_INVALID_ARGUMENT,
				"Mod float value is out of range."
			);
		}

		return DTTR_Config_SetModFloat(config, mod_id, field_id, (float)parsed);
	}

	if (yyjson_is_str(val)) {
		return DTTR_Config_SetModString(config, mod_id, field_id, yyjson_get_str(val));
	}

	return config_result(
		DTTR_ERR_UNSUPPORTED_CONTRACT,
		"Mod config value uses an unsupported JSON type."
	);
}

static void config_apply_mod_config_values(
	DTTR_Config *config,
	const char *mod_id,
	yyjson_val *values
) {
	if (!yyjson_is_obj(values)) {
		return;
	}

	yyjson_val *key;
	yyjson_val *val;
	size_t idx;
	size_t max;

	yyjson_obj_foreach(values, idx, max, key, val) {
		const char *field_id = yyjson_get_str(key);
		if (!field_id || !field_id[0]) {
			DTTR_LOG_WARN(
				"modding.mod_configs.%s.values[%zu]: expected field ID, skipping",
				mod_id,
				idx
			);
			continue;
		}

		DTTR_Result result = config_apply_mod_value(config, mod_id, field_id, val);
		if (!DTTR_ResultOK(result)) {
			DTTR_LOG_WARN(
				"modding.mod_configs.%s.values.%s: skipped %s value (%s: %s)",
				mod_id,
				field_id,
				yyjson_get_type_desc(val),
				DTTR_StatusName(result.status),
				config_result_message(result)
			);
		}
	}
}

static void config_apply_mod_configs(DTTR_Config *config, yyjson_val *mod_configs) {
	if (!mod_configs) {
		return;
	}

	if (!yyjson_is_obj(mod_configs)) {
		errors_addf("modding.mod_configs: expected object value");
		return;
	}

	yyjson_val *key;
	yyjson_val *val;
	size_t idx;
	size_t max;

	yyjson_obj_foreach(mod_configs, idx, max, key, val) {
		const char *mod_id = yyjson_get_str(key);
		if (!mod_id || !mod_id[0] || !yyjson_is_obj(val)) {
			DTTR_LOG_WARN("modding.mod_configs[%zu]: expected mod object, skipping", idx);
			continue;
		}

		yyjson_val *schema_version = yyjson_obj_get(val, "schema_version");
		if (schema_version && !yyjson_is_int(schema_version)) {
			DTTR_LOG_WARN(
				"modding.mod_configs.%s.schema_version: expected integer value, ignoring",
				mod_id
			);
		} else if (schema_version) {
			const int64_t parsed = yyjson_get_sint(schema_version);
			DTTR_Result result = config_ok();
			if (parsed < 0 || parsed > UINT32_MAX) {
				result = config_result(
					DTTR_ERR_INVALID_ARGUMENT,
					"Mod schema version is out of range."
				);
			} else {
				result = DTTR_Config_SetModSchemaVersion(config, mod_id, (uint32_t)parsed);
			}

			if (!DTTR_ResultOK(result)) {
				DTTR_LOG_WARN(
					"modding.mod_configs.%s.schema_version: invalid value, ignoring "
					"(%s: %s)",
					mod_id,
					DTTR_StatusName(result.status),
					config_result_message(result)
				);
			}
		}

		config_apply_mod_config_values(config, mod_id, yyjson_obj_get(val, "values"));
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

	const int64_t parsed_version = yyjson_get_sint(version);
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

	char *field = ((char *)config) + spec->offset;
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

		const int64_t parsed = yyjson_get_sint(val);
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
	config_json_doc doc = {.value = yyjson_read_file(filename, 0, NULL, &err)};

	if (!doc.value) {
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

	bool ok = false;
	yyjson_val *root = yyjson_doc_get_root(doc.value);
	if (!validate_schema_major_version(root, filename)) {
		goto done;
	}

	apply_section(root, NULL);
	for (size_t i = 0; i < CONFIG_SECTION_COUNT; i++) {
		apply_section(
			yyjson_obj_get(root, CONFIG_SECTION_NAMES[i]),
			CONFIG_SECTION_NAMES[i]
		);
	}

	yyjson_val *modding = yyjson_obj_get(root, "modding");
	config_apply_disabled_mods(&dttr_config, yyjson_obj_get(modding, "disabled_mods"));
	config_apply_mod_configs(&dttr_config, yyjson_obj_get(modding, "mod_configs"));
	config_apply_control_bindings(&dttr_config, yyjson_obj_get(root, "controls"));

	ok = !errors_show();

done:
	config_json_doc_free(&doc);
	return ok;
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
	yyjson_mut_val *sections[CONFIG_SECTION_COUNT];
} config_json_objects;

static yyjson_mut_val *config_object_for_section(
	const config_json_objects *objects,
	const char *section
) {
	if (config_sections_match(section, NULL)) {
		return objects->root;
	}

	for (size_t i = 0; i < CONFIG_SECTION_COUNT; i++) {
		if (config_sections_match(section, CONFIG_SECTION_NAMES[i])) {
			return objects->sections[i];
		}
	}

	return NULL;
}

static const char *config_format_field_string(
	const DTTR_Config *config,
	const DTTR_ConfigFieldSpec *spec
) {
	const char *field = ((const char *)config) + spec->offset;
	switch (spec->value_type) {
	case CONFIG_SCALING_FIT:
		return config_format_scaling_fit(*(const DTTR_ScalingMode *)field);
	case CONFIG_SCALING_METHOD:
		return config_format_scaling_method(*(const DTTR_ScalingMethod *)field);
	case CONFIG_GRAPHICS_API:
		return config_format_graphics_api(*(const DTTR_GraphicsAPI *)field);
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

	const char *field = ((const char *)config) + spec->offset;
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

static bool config_add_mod_value(
	yyjson_mut_doc *doc,
	yyjson_mut_val *values,
	const DTTR_ConfigModValue *value
) {
	switch (value->value_type) {
#define X(Suffix, member, writer, eq)                                                    \
	case DTTR_CONFIG_MOD_VALUE_##Suffix:                                                 \
		return writer(doc, values, value->field_id, value->member);
		DTTR_CONFIG_MOD_VALUE_TYPES(X)
#undef X
	default:
		return false;
	}
}

static bool config_add_mod_configs(
	yyjson_mut_doc *doc,
	yyjson_mut_val *modding,
	const DTTR_Config *config
) {
	yyjson_mut_val *mod_configs = yyjson_mut_obj_add_obj(doc, modding, "mod_configs");
	if (!mod_configs) {
		return false;
	}

	for (int mod_index = 0; mod_index < config->mod_config_count; mod_index++) {
		const DTTR_ConfigModConfig *mod = &config->mod_configs[mod_index];
		if (!mod->mod_id[0]) {
			continue;
		}

		yyjson_mut_val *mod_obj = yyjson_mut_obj_add_obj(doc, mod_configs, mod->mod_id);
		if (!mod_obj) {
			return false;
		}

		if (!yyjson_mut_obj_add_uint(doc, mod_obj, "schema_version", mod->schema_version)) {
			return false;
		}

		yyjson_mut_val *values = yyjson_mut_obj_add_obj(doc, mod_obj, "values");
		if (!values) {
			return false;
		}

		for (int value_index = 0; value_index < config->mod_value_count; value_index++) {
			const DTTR_ConfigModValue *value = &config->mod_values[value_index];
			if (value->mod_index == mod_index
				&& !config_add_mod_value(doc, values, value)) {
				return false;
			}
		}
	}

	return true;
}

static bool config_add_control_bindings(
	yyjson_mut_doc *doc,
	yyjson_mut_val *controls,
	const DTTR_Config *config
) {
	yyjson_mut_val *bindings = NULL;

	for (int action = 0; action < DTTR_CONFIG_CONTROL_ACTION_COUNT; action++) {
		if (DTTR_Config_ControlActionInGameBindable(action)
			|| config->control_bindings[action] == DTTR_CONFIG_CONTROL_BINDING_NONE) {
			continue;
		}

		if (!bindings) {
			bindings = yyjson_mut_obj_add_obj(doc, controls, "special_bindings");
			if (!bindings) {
				return false;
			}
		}

		const char *action_key = DTTR_Config_ControlActionKey(action);
		if (!action_key
			|| !yyjson_mut_obj_add_int(
				doc,
				bindings,
				action_key,
				config->control_bindings[action]
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
	config_mut_json_doc doc = {.value = yyjson_mut_doc_new(NULL)};
	if (!doc.value) {
		return false;
	}

	yyjson_mut_val *root = yyjson_mut_obj(doc.value);
	if (!root) {
		goto done;
	}

	yyjson_mut_doc_set_root(doc.value, root);

	config_json_objects objects = {.root = root};
	for (size_t i = 0; i < CONFIG_SECTION_COUNT; i++) {
		objects.sections[i] = yyjson_mut_obj_add_obj(
			doc.value,
			root,
			CONFIG_SECTION_NAMES[i]
		);
		if (!objects.sections[i]) {
			goto done;
		}
	}

	yyjson_mut_val *modding = objects.sections[CONFIG_SECTION_MODDING];

	ok = config_add_schema_fields(doc.value, &objects, config)
		 && config_add_disabled_mods(doc.value, modding, config)
		 && config_add_mod_configs(doc.value, modding, config)
		 && config_add_control_bindings(
			 doc.value,
			 objects.sections[CONFIG_SECTION_CONTROLS],
			 config
		 );

	if (ok) {
		yyjson_write_err err;
		ok = yyjson_mut_write_file(
			filename,
			doc.value,
			YYJSON_WRITE_PRETTY | YYJSON_WRITE_NEWLINE_AT_END,
			NULL,
			&err
		);
		if (!ok) {
			DTTR_LOG_ERROR("Failed to write config %s: %s", filename, err.msg);
		}
	}

done:
	config_mut_json_doc_free(&doc);
	return ok;
}
