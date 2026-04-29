#include "config_internal.h"
#include "log.h"
#include "sds.h"
#include "yyjson.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
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

static const char *s_yyjson_value_as_string(yyjson_val *val, char *buf, size_t buf_size) {
	if (yyjson_is_str(val)) {
		return yyjson_get_str(val);
	}

	if (yyjson_is_bool(val)) {
		return yyjson_get_bool(val) ? "true" : "false";
	}

	if (yyjson_is_int(val)) {
		s_config_format_int((int)yyjson_get_int(val), buf, buf_size);
		return buf;
	}

	if (yyjson_is_real(val)) {
		s_config_format_float((float)yyjson_get_real(val), buf, buf_size);
		return buf;
	}

	if (yyjson_is_null(val)) {
		return "";
	}

	return NULL;
}

static void s_config_apply_buttons(DTTR_Config *config, yyjson_val *buttons) {
	if (!yyjson_is_obj(buttons)) {
		return;
	}

	for (int i = 0; i < DTTR_GAMEPAD_SOURCE_COUNT; i++) {
		config->m_gamepad_button_map[i] = DTTR_GAMEPAD_MAPPING_NONE;
	}

	yyjson_obj_iter iter;
	yyjson_obj_iter_init(buttons, &iter);
	yyjson_val *key;

	while ((key = yyjson_obj_iter_next(&iter))) {
		yyjson_val *val = yyjson_obj_iter_get_val(key);
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

/// Walk a JSON object and apply each non-object entry via s_config_apply_entry.
static void s_apply_section(yyjson_val *obj, const char *section) {
	if (!yyjson_is_obj(obj)) {
		return;
	}

	yyjson_obj_iter iter;
	yyjson_obj_iter_init(obj, &iter);
	yyjson_val *key;

	while ((key = yyjson_obj_iter_next(&iter))) {
		yyjson_val *val = yyjson_obj_iter_get_val(key);

		if (yyjson_is_obj(val)) {
			continue;
		}

		char num_buf[32];
		const char *value = s_yyjson_value_as_string(val, num_buf, sizeof(num_buf));
		const char *key_str = yyjson_get_str(key);

		if (!value || s_config_apply_entry(&g_dttr_config, section, key_str, value)) {
			continue;
		}

		if (section) {
			s_errors_addf("%s.%s: invalid value \"%s\"", section, key_str, value);
		} else {
			s_errors_addf("%s: invalid value \"%s\"", key_str, value);
		}
	}
}

bool dttr_config_load(const char *filename) {
	s_errors_clear();

	if (!filename || !filename[0]) {
		log_error("Load failed: empty filename");
		dttr_config_set_defaults(&g_dttr_config);
		return false;
	}

	dttr_config_set_defaults(&g_dttr_config);

	FILE *f = fopen(filename, "rb");
	if (!f) {
		log_warn("File '%s' not found. Using defaults.", filename);
		return true;
	}

	fseek(f, 0, SEEK_END);
	const long file_size = ftell(f);
	fseek(f, 0, SEEK_SET);

	if (file_size <= 0) {
		fclose(f);
		log_warn("File '%s' is empty. Using defaults.", filename);
		return true;
	}

	sds buf = sdsnewlen(SDS_NOINIT, (size_t)file_size);
	if (!buf) {
		fclose(f);
		log_error("Load failed: out of memory");
		return false;
	}

	const size_t read_count = fread(buf, 1, (size_t)file_size, f);
	fclose(f);
	buf[read_count] = '\0';
	sdssetlen(buf, read_count);

	yyjson_read_err err;
	yyjson_doc *doc = yyjson_read_opts(
		buf,
		read_count,
		YYJSON_READ_ALLOW_COMMENTS | YYJSON_READ_ALLOW_TRAILING_COMMAS,
		NULL,
		&err
	);
	sdsfree(buf);

	if (!doc) {
		log_error("JSON parse failed: %s at position %zu", err.msg, err.pos);
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

	s_apply_section(root, NULL);
	s_apply_section(yyjson_obj_get(root, "graphics"), "graphics");
	s_apply_section(yyjson_obj_get(root, "audio"), "audio");

	// Gamepad section (mix of schema + custom handling)
	yyjson_val *gamepad = yyjson_obj_get(root, "gamepad");

	if (yyjson_is_obj(gamepad)) {
		yyjson_val *buttons = yyjson_obj_get(gamepad, "buttons");
		if (buttons) {
			s_config_apply_buttons(&g_dttr_config, buttons);
		}

		yyjson_obj_iter iter;
		yyjson_obj_iter_init(gamepad, &iter);
		yyjson_val *key;

		while ((key = yyjson_obj_iter_next(&iter))) {
			yyjson_val *val = yyjson_obj_iter_get_val(key);
			const char *key_str = yyjson_get_str(key);

			if (strcmp(key_str, "buttons") == 0) {
				continue;
			}

			char num_buf[32];
			const char *value = s_yyjson_value_as_string(val, num_buf, sizeof(num_buf));

			if (value
				&& !s_config_apply_gamepad_entry(
					&g_dttr_config,
					"gamepad",
					key_str,
					value
				)) {
				s_errors_addf("gamepad.%s: invalid value \"%s\"", key_str, value);
			}
		}
	}

	yyjson_doc_free(doc);
	s_errors_show();
	return true;
}

static inline bool s_is_jsonc_ws(char c) {
	return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

/// Scan past a quoted string. Returns position after closing quote.
static size_t s_jsonc_scan_string(const char *text, size_t text_len, size_t start) {
	size_t i = start + 1;

	while (i < text_len && !(text[i] == '"' && text[i - 1] != '\\')) {
		i++;
	}

	if (i < text_len) {
		i++;
	}

	return i;
}

/// Skip whitespace. Returns position of first non-ws char.
static size_t s_jsonc_skip_ws(const char *text, size_t text_len, size_t pos) {
	while (pos < text_len && s_is_jsonc_ws(text[pos])) {
		pos++;
	}

	return pos;
}

/// Find the byte span of a leaf value in JSONC text at the given key path.
/// Does not support object/array values.
static bool s_jsonc_find_value_span(
	const char *text,
	size_t text_len,
	const char *const *path,
	int path_depth,
	size_t *out_offset,
	size_t *out_len
) {
	int depth = 0;
	int matched = 0;
	bool expect_value = false;

	size_t i = 0;

	while (i < text_len) {
		const char c = text[i];

		// Quoted string: either a value we're looking for, or a key
		if (c == '"') {
			const size_t str_start = i;
			i = s_jsonc_scan_string(text, text_len, i);

			if (expect_value) {
				*out_offset = str_start;
				*out_len = i - str_start;
				return true;
			}

			// Check if this string is a key (followed by ':')
			size_t j = s_jsonc_skip_ws(text, text_len, i);

			if (j >= text_len || text[j] != ':') {
				continue;
			}

			j++;

			// Not at the right depth for matching
			if (depth - 1 != matched || matched >= path_depth) {
				i = j;
				continue;
			}

			// Key doesn't match expected path component
			const size_t key_len = i - str_start - 2;
			const char *expected = path[matched];

			if (key_len != strlen(expected)
				|| memcmp(text + str_start + 1, expected, key_len) != 0) {
				i = j;
				continue;
			}

			// Matched this path component
			matched++;
			i = s_jsonc_skip_ws(text, text_len, j);

			if (matched == path_depth) {
				expect_value = true;
			}

			continue;
		}

		// Line comment
		if (c == '/' && i + 1 < text_len && text[i + 1] == '/') {
			i += 2;

			while (i < text_len && text[i] != '\n') {
				i++;
			}

			continue;
		}

		// Block comment
		if (c == '/' && i + 1 < text_len && text[i + 1] == '*') {
			i += 2;

			while (i + 1 < text_len && !(text[i] == '*' && text[i + 1] == '/')) {
				i++;
			}

			if (i + 1 < text_len) {
				i += 2;
			}

			continue;
		}

		// Brace tracking
		if (c == '{') {
			if (expect_value) {
				expect_value = false;
			}

			depth++;
			i++;
			continue;
		}

		if (c == '}') {
			if (depth - 1 < matched) {
				matched = depth - 1;
				expect_value = false;
			}

			depth--;
			i++;
			continue;
		}

		// Unquoted value (number, bool, null)
		if (expect_value && !s_is_jsonc_ws(c) && c != ',') {
			const size_t val_start = i;

			while (i < text_len && text[i] != ',' && text[i] != '}' && text[i] != ']'
				   && text[i] != '/' && text[i] != '\n' && text[i] != '\r') {
				i++;
			}

			size_t val_end = i;

			while (val_end > val_start && s_is_jsonc_ws(text[val_end - 1])) {
				val_end--;
			}

			*out_offset = val_start;
			*out_len = val_end - val_start;
			return true;
		}

		i++;
	}

	return false;
}

/// Replace a byte range in an sds string, returning a new sds.
static sds s_jsonc_replace_value(
	const char *text,
	size_t text_len,
	size_t offset,
	size_t old_len,
	const char *new_value
) {
	sds result = sdsnewlen(text, offset);
	result = sdscat(result, new_value);
	result = sdscatlen(result, text + offset + old_len, text_len - offset - old_len);
	return result;
}

/// Wrap a string in JSON quotes with escape sequences, returning an sds.
static sds s_sds_json_quoted(const char *str) {
	sds out = sdscatlen(sdsempty(), "\"", 1);

	for (const char *p = str; *p; p++) {
		switch (*p) {
		case '\\':
			out = sdscat(out, "\\\\");
			break;
		case '"':
			out = sdscat(out, "\\\"");
			break;
		case '\n':
			out = sdscat(out, "\\n");
			break;
		case '\r':
			out = sdscat(out, "\\r");
			break;
		case '\t':
			out = sdscat(out, "\\t");
			break;
		default:
			out = sdscatlen(out, p, 1);
			break;
		}
	}

	return sdscatlen(out, "\"", 1);
}

/// Format a schema field's current value as a JSON token (sds, caller frees).
static sds s_config_format_json_value(
	const DTTR_Config *config,
	const S_ConfigFieldSpec *spec
) {
	const char *field = ((const char *)config) + spec->offset;

	switch (spec->value_type) {
	case S_CONFIG_BOOL:
		return sdsnew(s_config_format_bool(*(const bool *)field));

	case S_CONFIG_INT: {
		char buf[32];
		s_config_format_int(*(const int *)field, buf, sizeof(buf));
		return sdsnew(buf);
	}

	case S_CONFIG_FLOAT: {
		char buf[32];
		s_config_format_float(*(const float *)field, buf, sizeof(buf));
		return sdsnew(buf);
	}

	case S_CONFIG_SCALING_FIT:
		return s_sds_json_quoted(
			s_config_format_scaling_fit(*(const DTTR_ScalingMode *)field)
		);

	case S_CONFIG_SCALING_METHOD:
		return s_sds_json_quoted(
			s_config_format_scaling_method(*(const DTTR_ScalingMethod *)field)
		);

	case S_CONFIG_GRAPHICS_API:
		return s_sds_json_quoted(
			s_config_format_graphics_api(*(const DTTR_GraphicsApi *)field)
		);

	case S_CONFIG_PRESENT_FILTER:
		return s_sds_json_quoted(
			s_config_format_present_filter(*(const SDL_GPUFilter *)field)
		);

	case S_CONFIG_LOG_LEVEL:
		return s_sds_json_quoted(s_config_format_log_level(*(const int *)field));

	case S_CONFIG_MINIDUMP_TYPE:
		return s_sds_json_quoted(
			s_config_format_minidump_type(*(const DTTR_MinidumpType *)field)
		);

	case S_CONFIG_STRING:
		return s_sds_json_quoted(s_config_format_string(field));

	case S_CONFIG_VERTEX_PRECISION:
		return s_sds_json_quoted(
			s_config_format_vertex_precision(*(const DTTR_VertexPrecision *)field)
		);

	default:
		return sdsempty();
	}
}

typedef struct {
	size_t offset;
	size_t old_len;
	sds new_value;
} S_Replacement;

static int s_replacement_cmp_desc(const void *a, const void *b) {
	const S_Replacement *ra = (const S_Replacement *)a;
	const S_Replacement *rb = (const S_Replacement *)b;

	if (ra->offset > rb->offset)
		return -1;
	if (ra->offset < rb->offset)
		return 1;
	return 0;
}

/// Find value at path and record a replacement. Frees new_value if key not found.
static void s_try_replace(
	S_Replacement *r,
	int *n,
	const char *text,
	size_t text_len,
	const char *const *path,
	int path_depth,
	sds new_value
) {
	size_t offset, len;

	if (s_jsonc_find_value_span(text, text_len, path, path_depth, &offset, &len)) {
		r[*n].offset = offset;
		r[*n].old_len = len;
		r[*n].new_value = new_value;
		(*n)++;
	} else {
		sdsfree(new_value);
	}
}

bool dttr_config_save(const char *filename, const DTTR_Config *config) {
	if (!filename || !config) {
		return false;
	}

	// Read existing file (or start with empty object)
	sds text = NULL;

	{
		FILE *f = fopen(filename, "rb");

		if (f) {
			fseek(f, 0, SEEK_END);
			const long file_size = ftell(f);
			fseek(f, 0, SEEK_SET);

			if (file_size > 0) {
				text = sdsnewlen(SDS_NOINIT, (size_t)file_size);
				const size_t bytes = fread(text, 1, (size_t)file_size, f);
				text[bytes] = '\0';
				sdssetlen(text, bytes);
			}

			fclose(f);
		}

		if (!text) {
			text = sdsnew("{\n}\n");
		}
	}

	const int schema_count = s_config_schema_count();
	const int max_r = schema_count + 2 + DTTR_GAMEPAD_SOURCE_COUNT
					  + DTTR_GAMEPAD_AXIS_MAPPING_COUNT * 2;
	S_Replacement *const replacements = calloc((size_t)max_r, sizeof(S_Replacement));
	int n = 0;

	// Schema-driven fields
	for (int i = 0; i < schema_count; i++) {
		const S_ConfigFieldSpec *spec = s_config_schema_get(i);
		if (!spec) {
			continue;
		}

		const char *path[2];
		int depth;

		if (spec->section) {
			path[0] = spec->section;
			path[1] = spec->key;
			depth = 2;
		} else {
			path[0] = spec->key;
			depth = 1;
		}

		s_try_replace(
			replacements,
			&n,
			text,
			sdslen(text),
			path,
			depth,
			s_config_format_json_value(config, spec)
		);
	}

	// Gamepad: enabled flag
	{
		const char *path[] = {"gamepad", "enabled"};
		s_try_replace(
			replacements,
			&n,
			text,
			sdslen(text),
			path,
			2,
			sdsnew(s_config_format_bool(config->m_gamepad_enabled))
		);
	}

	// Gamepad: index
	{
		const char *path[] = {"gamepad", "index"};
		char buf[32];
		s_config_format_int(config->m_gamepad_index, buf, sizeof(buf));
		s_try_replace(replacements, &n, text, sdslen(text), path, 2, sdsnew(buf));
	}

	// Gamepad: axis mappings
	{
		static const char *axis_keys[] = {
			"axis_stick_x",
			"axis_stick_y",
			"axis_camera_rz"
		};

		for (int i = 0; i < DTTR_GAMEPAD_AXIS_MAPPING_COUNT; i++) {
			const char *path[] = {"gamepad", axis_keys[i]};
			s_try_replace(
				replacements,
				&n,
				text,
				sdslen(text),
				path,
				2,
				s_sds_json_quoted(s_config_format_gamepad_axis(config->m_gamepad_axes[i]))
			);
		}
	}

	// Gamepad: deadzones
	{
		static const char *deadzone_keys[] = {
			"deadzone_stick_x",
			"deadzone_stick_y",
			"deadzone_camera_rz"
		};

		for (int i = 0; i < DTTR_GAMEPAD_AXIS_MAPPING_COUNT; i++) {
			const char *path[] = {"gamepad", deadzone_keys[i]};
			char buf[32];
			s_config_format_int(config->m_gamepad_axis_deadzone[i], buf, sizeof(buf));
			s_try_replace(replacements, &n, text, sdslen(text), path, 2, sdsnew(buf));
		}
	}

	// Gamepad: button mappings (source -> action)
	for (int i = 0; i < DTTR_GAMEPAD_SOURCE_COUNT; i++) {
		const char *source_name = s_config_format_gamepad_source(i);
		if (!source_name) {
			continue;
		}

		const char *path[] = {"gamepad", "buttons", source_name};
		const int action = config->m_gamepad_button_map[i];
		s_try_replace(
			replacements,
			&n,
			text,
			sdslen(text),
			path,
			3,
			s_sds_json_quoted(s_config_format_game_action(action))
		);
	}

	// Sort by descending offset and apply back-to-front
	qsort(replacements, (size_t)n, sizeof(S_Replacement), s_replacement_cmp_desc);

	for (int i = 0; i < n; i++) {
		sds new_text = s_jsonc_replace_value(
			text,
			sdslen(text),
			replacements[i].offset,
			replacements[i].old_len,
			replacements[i].new_value
		);
		sdsfree(text);
		text = new_text;
	}

	for (int i = 0; i < n; i++) {
		sdsfree(replacements[i].new_value);
	}

	free(replacements);

	// Write back
	bool ok = false;
	FILE *f = fopen(filename, "wb");

	if (f) {
		ok = fwrite(text, 1, sdslen(text), f) == sdslen(text);
		fclose(f);
	}

	sdsfree(text);
	return ok;
}
