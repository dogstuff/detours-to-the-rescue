#include <dttr_path.h>

#include <string.h>

#include <windows.h>

static bool s_copy_path_value(
	char *out,
	size_t out_size,
	const char *value,
	size_t value_len,
	bool allow_empty
) {
	if (!value || (!allow_empty && value_len == 0) || value_len >= out_size) {
		return false;
	}

	memcpy(out, value, value_len);
	out[value_len] = '\0';
	return true;
}

static const char *s_skip_dot_separators(const char *path) {
	while (path[0] == '.' && dttr_path_is_separator(path[1])) {
		path += 2;
	}

	return path;
}

static void s_trim_trailing_separators(sds path) {
	const size_t path_len = sdslen(path);
	size_t len = path_len;
	while (len > 0 && dttr_path_is_separator(path[len - 1])) {
		len--;
	}

	if (len == path_len) {
		return;
	}

	if (len == 0) {
		sdsclear(path);
		return;
	}

	sdsrange(path, 0, (int)len - 1);
}

char dttr_path_ascii_lower(char ch) {
	if (ch >= 'A' && ch <= 'Z') {
		return (char)(ch - 'A' + 'a');
	}

	return ch;
}

bool dttr_path_ascii_ieq_n(const char *lhs, const char *rhs, size_t n) {
	for (size_t i = 0; i < n; i++) {
		if (dttr_path_ascii_lower(lhs[i]) != dttr_path_ascii_lower(rhs[i])) {
			return false;
		}
	}

	return true;
}

bool dttr_path_copy_string(char *out, size_t out_size, const char *value) {
	return s_copy_path_value(out, out_size, value, value ? strlen(value) : 0, false);
}

bool dttr_path_copy_sds(char *out, size_t out_size, sds value) {
	return s_copy_path_value(out, out_size, value, value ? sdslen(value) : 0, true);
}

bool dttr_path_is_separator(char ch) { return ch == '\\' || ch == '/'; }

const char *dttr_path_skip_separators(const char *path) {
	while (*path && dttr_path_is_separator(*path)) {
		path++;
	}

	return path;
}

size_t dttr_path_segment_len(const char *path) {
	size_t len = 0;

	while (path[len] && !dttr_path_is_separator(path[len])) {
		len++;
	}

	return len;
}

bool dttr_path_is_relative_segment(const char *segment, size_t segment_len) {
	return segment_len == 0 || (segment_len == 1 && segment[0] == '.')
		   || (segment_len == 2 && segment[0] == '.' && segment[1] == '.');
}

bool dttr_path_is_safe_relative(const char *path) {
	if (!path) {
		return false;
	}

	const char *p = dttr_path_skip_separators(path);
	if (!*p) {
		return false;
	}

	while (*p) {
		const size_t segment_len = dttr_path_segment_len(p);
		if (dttr_path_is_relative_segment(p, segment_len)) {
			return false;
		}

		p = dttr_path_skip_separators(p + segment_len);
	}

	return true;
}

static sds s_normalize_path_for_compare(const char *path) {
	if (!path) {
		return sdsempty();
	}

	path = s_skip_dot_separators(path);

	sds normalized = sdsempty();
	if (!normalized) {
		return NULL;
	}

	bool previous_separator = false;
	for (const char *p = path; *p; p++) {
		const bool is_separator = dttr_path_is_separator(*p);
		const char ch = is_separator ? '/' : *p;
		if (is_separator) {
			if (previous_separator) {
				continue;
			}

			previous_separator = true;
		} else {
			previous_separator = false;
		}

		if (!dttr_path_append_char(&normalized, ch)) {
			sdsfree(normalized);
			return NULL;
		}
	}

	s_trim_trailing_separators(normalized);
	return normalized;
}

bool dttr_path_matches_normalized(const char *lhs, const char *rhs) {
	sds normalized_lhs = s_normalize_path_for_compare(lhs);
	sds normalized_rhs = s_normalize_path_for_compare(rhs);
	const bool matches = normalized_lhs && normalized_rhs
						 && strcmp(normalized_lhs, normalized_rhs) == 0;
	sdsfree(normalized_lhs);
	sdsfree(normalized_rhs);
	return matches;
}

bool dttr_path_is_windows_absolute(const char *path) {
	return path && strlen(path) >= 3 && path[1] == ':' && dttr_path_is_separator(path[2]);
}

bool dttr_path_is_any_absolute(const char *path) {
	if (!path) {
		return false;
	}

	return dttr_path_is_windows_absolute(path) || dttr_path_is_separator(path[0]);
}

bool dttr_path_exact_exists(const char *path) {
	return GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES;
}

sds dttr_path_current_dir(void) {
	char cwd[MAX_PATH];
	DWORD len = GetCurrentDirectoryA((DWORD)sizeof(cwd), cwd);
	if (len == 0 || len >= sizeof(cwd)) {
		return NULL;
	}

	return sdsnew(cwd);
}

sds dttr_path_module_dir(void *module) {
	char path[MAX_PATH];
	const DWORD len = GetModuleFileNameA((HMODULE)module, path, (DWORD)sizeof(path));
	if (len == 0 || len >= sizeof(path)) {
		return NULL;
	}

	char *last_sep = strrchr(path, '\\');
	if (!last_sep) {
		return NULL;
	}

	last_sep[1] = '\0';
	return sdsnew(path);
}

sds dttr_path_module_sibling(void *module, const char *relative_path) {
	sds path = dttr_path_module_dir(module);
	if (!path
		|| !dttr_path_append_segment(&path, relative_path, DTTR_PATH_NATIVE_SEPARATOR)) {
		sdsfree(path);
		return NULL;
	}

	return path;
}

sds dttr_path_resolve_relative_to(const char *base_dir, const char *path) {
	if (!path) {
		return NULL;
	}

	if (dttr_path_is_any_absolute(path)) {
		return sdsnew(path);
	}

	sds resolved = sdsnew(base_dir ? base_dir : "");
	if (!resolved
		|| !dttr_path_append_segment(&resolved, path, DTTR_PATH_NATIVE_SEPARATOR)) {
		sdsfree(resolved);
		return NULL;
	}

	return resolved;
}

sds dttr_path_native_root(const char *path, const char **rest) {
	if (dttr_path_is_windows_absolute(path)) {
		*rest = path + 3;
		return sdscatprintf(sdsempty(), "%c:%c", path[0], DTTR_PATH_NATIVE_SEPARATOR);
	}

	if (dttr_path_is_separator(path[0])) {
		*rest = dttr_path_skip_separators(path);
		return sdsnewlen(&(char){DTTR_PATH_NATIVE_SEPARATOR}, 1);
	}

	*rest = path;
	return dttr_path_current_dir();
}

bool dttr_path_append_char(sds *path, char ch) {
	sds next = sdscatlen(*path, &ch, 1);
	if (!next) {
		return false;
	}

	*path = next;
	return true;
}

bool dttr_path_append_separator(sds *path, char separator) {
	return dttr_path_append_char(path, separator);
}

bool dttr_path_append_segment(sds *path, const char *segment, char separator) {
	const size_t len = sdslen(*path);
	if (len > 0 && !dttr_path_is_separator((*path)[len - 1])) {
		if (!dttr_path_append_char(path, separator)) {
			return false;
		}
	}

	sds next = sdscat(*path, segment);
	if (!next) {
		return false;
	}

	*path = next;
	return true;
}
