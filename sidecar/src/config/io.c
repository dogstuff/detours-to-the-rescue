#include "cJSON.h"
#include "config_internal.h"
#include "dttr_sidecar.h"
#include "log.h"
#include "sds.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
	if (s_errors && sdslen(s_errors) > 0) {
		MessageBoxA(NULL, s_errors, "DTTR: Configuration Error", MB_OK | MB_ICONERROR);
	}
}

/// Strip C-style comments from a JSON buffer in-place
/// Handles // line comments and block comments
/// Preserves content inside "strings" (with \" escape handling)
static void s_strip_json_comments(char *buf) {
	char *p = buf;
	while (*p) {
		if (*p == '"') {
			// Skip over string literal
			p++;
			while (*p && *p != '"') {
				if (*p == '\\' && p[1]) {
					p++;
				}
				p++;
			}
			if (*p == '"') {
				p++;
			}
		} else if (p[0] == '/' && p[1] == '/') {
			// Blank out a line comment until end of line
			while (*p && *p != '\n') {
				*p++ = ' ';
			}
		} else if (p[0] == '/' && p[1] == '*') {
			// Blank out a block comment until */
			*p++ = ' ';
			*p++ = ' ';
			while (*p && !(p[0] == '*' && p[1] == '/')) {
				*p++ = ' ';
			}
			if (*p) {
				*p++ = ' ';
				*p++ = ' ';
			}
		} else {
			p++;
		}
	}
}

/// Convert a cJSON value to a string suitable for the existing parsers
/// For strings, returns the string value directly
/// For booleans, returns "true" or "false"
/// For numbers, formats into the provided static buffer
/// Returns NULL if the value cannot be converted
static const char *s_cjson_value_as_string(const cJSON *item, char *buf, size_t buf_size) {
	if (cJSON_IsString(item)) {
		return cJSON_GetStringValue(item);
	}
	if (cJSON_IsBool(item)) {
		return cJSON_IsTrue(item) ? "true" : "false";
	}
	if (cJSON_IsNumber(item)) {
		snprintf(buf, buf_size, "%d", (int)cJSON_GetNumberValue(item));
		return buf;
	}
	return NULL;
}

static int s_gamepad_button_key_to_index(const char *key) {
	if (strcmp(key, "joy_up") == 0) {
		return PCDOGS_GAMEPAD_IDX_UP;
	}
	if (strcmp(key, "joy_down") == 0) {
		return PCDOGS_GAMEPAD_IDX_DOWN;
	}
	if (strcmp(key, "joy_left") == 0) {
		return PCDOGS_GAMEPAD_IDX_LEFT;
	}
	if (strcmp(key, "joy_right") == 0) {
		return PCDOGS_GAMEPAD_IDX_RIGHT;
	}
	if (strcmp(key, "joy_pov_up") == 0) {
		return PCDOGS_GAMEPAD_IDX_POV_UP;
	}
	if (strcmp(key, "joy_pov_down") == 0) {
		return PCDOGS_GAMEPAD_IDX_POV_DOWN;
	}

	if (strncmp(key, "joy_", 4) == 0) {
		char *end = NULL;
		const long n = strtol(key + 4, &end, 10);
		if (end != key + 4 && *end == '\0' && n >= 1 && n <= 13) {
			return PCDOGS_GAMEPAD_IDX_BTN_0 + (int)(n - 1);
		}
	}

	return -1;
}

static void s_config_apply_buttons(DTTR_Config *config, const cJSON *buttons) {
	if (!cJSON_IsObject(buttons)) {
		return;
	}

	for (int i = 0; i < DTTR_GAMEPAD_MAPPING_COUNT; i++) {
		config->m_gamepad_mappings[i] = DTTR_GAMEPAD_MAPPING_NONE;
	}

	const cJSON *item = NULL;
	cJSON_ArrayForEach(item, buttons) {
		const int index = s_gamepad_button_key_to_index(item->string);
		if (index < 0) {
			s_errors_addf("gamepad.buttons.%s: unknown button slot", item->string);
			continue;
		}

		const char *value = cJSON_GetStringValue(item);
		if (!value) {
			s_errors_addf("gamepad.buttons.%s: expected string value", item->string);
			continue;
		}

		int button = DTTR_GAMEPAD_MAPPING_NONE;
		if (!s_config_parse_gamepad_button(value, &button)) {
			s_errors_addf("gamepad.buttons.%s: invalid button name \"%s\"", item->string, value);
			return;
		}

		config->m_gamepad_mappings[index] = button;
	}
}

bool dttr_config_load(const char *filename) {
	s_errors_clear();

	if (!filename || !filename[0]) {
		log_error("config: Load failed: empty filename");
		dttr_config_set_defaults(&g_dttr_config);
		return false;
	}

	dttr_config_set_defaults(&g_dttr_config);

	// Read file into memory
	FILE *f = fopen(filename, "rb");
	if (!f) {
		log_warn("config: File '%s' not found. Using defaults.", filename);
		return true;
	}

	fseek(f, 0, SEEK_END);
	const long file_size = ftell(f);
	fseek(f, 0, SEEK_SET);

	if (file_size <= 0) {
		fclose(f);
		log_warn("config: File '%s' is empty. Using defaults.", filename);
		return true;
	}

	sds buf = sdsnewlen(SDS_NOINIT, (size_t)file_size);
	if (!buf) {
		fclose(f);
		log_error("config: Load failed: out of memory");
		return false;
	}

	const size_t read_count = fread(buf, 1, (size_t)file_size, f);
	fclose(f);
	buf[read_count] = '\0';
	sdssetlen(buf, read_count);

	// Strip comments before parsing
	s_strip_json_comments(buf);

	cJSON *root = cJSON_Parse(buf);
	sdsfree(buf);

	if (!root) {
		const char *error_ptr = cJSON_GetErrorPtr();

		log_error("config: JSON parse failed near: %.20s", error_ptr ? error_ptr : "(unknown)");

		s_errors_addf("Failed to parse %s (near: %.20s)", filename, error_ptr ? error_ptr : "?");
		s_errors_show();

		return false;
	}

	// Walk top-level keys (non-object values like log_level)
	{
		const cJSON *item = NULL;
		cJSON_ArrayForEach(item, root) {
			if (cJSON_IsObject(item)) {
				continue;
			}

			char num_buf[32];
			const char *value = s_cjson_value_as_string(item, num_buf, sizeof(num_buf));

			if (value && !s_config_apply_entry(&g_dttr_config, NULL, item->string, value)) {
				s_errors_addf("%s: invalid value \"%s\"", item->string, value);
			}
		}
	}

	// Walk the "graphics" object
	const cJSON *graphics = cJSON_GetObjectItemCaseSensitive(root, "graphics");

	if (cJSON_IsObject(graphics)) {
		const cJSON *item = NULL;

		cJSON_ArrayForEach(item, graphics) {
			char num_buf[32];
			const char *value = s_cjson_value_as_string(item, num_buf, sizeof(num_buf));

			if (value && !s_config_apply_entry(&g_dttr_config, "graphics", item->string, value)) {
				s_errors_addf("graphics.%s: invalid value \"%s\"", item->string, value);
			}
		}
	}

	// Walk the "gamepad" object
	const cJSON *gamepad = cJSON_GetObjectItemCaseSensitive(root, "gamepad");
	if (cJSON_IsObject(gamepad)) {
		const cJSON *buttons = cJSON_GetObjectItemCaseSensitive(gamepad, "buttons");
		if (buttons) {
			s_config_apply_buttons(&g_dttr_config, buttons);
		}

		const cJSON *item = NULL;
		cJSON_ArrayForEach(item, gamepad) {
			if (strcmp(item->string, "buttons") == 0) {
				continue;
			}

			char num_buf[32];
			const char *value = s_cjson_value_as_string(item, num_buf, sizeof(num_buf));
			if (value
				&& !s_config_apply_gamepad_entry(&g_dttr_config, "gamepad", item->string, value)) {
				s_errors_addf("gamepad.%s: invalid value \"%s\"", item->string, value);
			}
		}
	}

	cJSON_Delete(root);
	s_errors_show();
	return true;
}
