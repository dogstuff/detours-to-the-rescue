#include <dttr_path.h>

#include <string.h>

#include <windows.h>

static bool copy_path_value(
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

static const char *skip_dot_separators(const char *path) {
	while (path[0] == '.' && DTTR_Path_IsSeparator(path[1])) {
		path += 2;
	}

	return path;
}

static void trim_trailing_separators(sds path) {
	const size_t path_len = sdslen(path);
	size_t len = path_len;
	while (len > 0 && DTTR_Path_IsSeparator(path[len - 1])) {
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

char DTTR_Path_AsciiLower(char ch) {
	if (ch >= 'A' && ch <= 'Z') {
		return (char)(ch - 'A' + 'a');
	}

	return ch;
}

bool DTTR_Path_AsciiIeqN(const char *lhs, const char *rhs, size_t n) {
	for (size_t i = 0; i < n; i++) {
		if (DTTR_Path_AsciiLower(lhs[i]) != DTTR_Path_AsciiLower(rhs[i])) {
			return false;
		}
	}

	return true;
}

bool DTTR_Path_CopyString(char *out, size_t out_size, const char *value) {
	return copy_path_value(out, out_size, value, value ? strlen(value) : 0, false);
}

bool DTTR_Path_CopySds(char *out, size_t out_size, sds value) {
	return copy_path_value(out, out_size, value, value ? sdslen(value) : 0, true);
}

bool DTTR_Path_IsSeparator(char ch) { return ch == '\\' || ch == '/'; }

const char *DTTR_Path_SkipSeparators(const char *path) {
	while (*path && DTTR_Path_IsSeparator(*path)) {
		path++;
	}

	return path;
}

size_t DTTR_Path_SegmentLen(const char *path) {
	size_t len = 0;

	while (path[len] && !DTTR_Path_IsSeparator(path[len])) {
		len++;
	}

	return len;
}

bool DTTR_Path_IsRelativeSegment(const char *segment, size_t segment_len) {
	return segment_len == 0 || (segment_len == 1 && segment[0] == '.')
		   || (segment_len == 2 && segment[0] == '.' && segment[1] == '.');
}

static bool path_has_windows_drive_prefix(const char *path) {
	return path && path[0] && path[1] == ':';
}

bool DTTR_Path_IsSafeRelative(const char *path) {
	if (!path || DTTR_Path_IsAnyAbsolute(path) || path_has_windows_drive_prefix(path)) {
		return false;
	}

	const char *p = path;
	if (!*p) {
		return false;
	}

	while (*p) {
		const size_t segment_len = DTTR_Path_SegmentLen(p);
		if (DTTR_Path_IsRelativeSegment(p, segment_len)) {
			return false;
		}

		p = DTTR_Path_SkipSeparators(p + segment_len);
	}

	return true;
}

static sds normalize_path_for_compare(const char *path) {
	if (!path) {
		return sdsempty();
	}

	path = skip_dot_separators(path);

	sds normalized = sdsempty();
	if (!normalized) {
		return NULL;
	}

	bool previous_separator = false;
	for (const char *p = path; *p; p++) {
		const bool is_separator = DTTR_Path_IsSeparator(*p);
		const char ch = is_separator ? '/' : *p;
		if (is_separator) {
			if (previous_separator) {
				continue;
			}

			previous_separator = true;
		} else {
			previous_separator = false;
		}

		if (!DTTR_Path_AppendChar(&normalized, ch)) {
			sdsfree(normalized);
			return NULL;
		}
	}

	trim_trailing_separators(normalized);
	return normalized;
}

bool DTTR_Path_MatchesNormalized(const char *lhs, const char *rhs) {
	sds normalized_lhs = normalize_path_for_compare(lhs);
	sds normalized_rhs = normalize_path_for_compare(rhs);
	const bool matches = normalized_lhs && normalized_rhs
						 && strcmp(normalized_lhs, normalized_rhs) == 0;
	sdsfree(normalized_lhs);
	sdsfree(normalized_rhs);
	return matches;
}

bool DTTR_Path_IsWindowsAbsolute(const char *path) {
	return path && strlen(path) >= 3 && path[1] == ':' && DTTR_Path_IsSeparator(path[2]);
}

bool DTTR_Path_IsAnyAbsolute(const char *path) {
	if (!path) {
		return false;
	}

	return DTTR_Path_IsWindowsAbsolute(path) || DTTR_Path_IsSeparator(path[0]);
}

bool DTTR_Path_ExactExists(const char *path) {
	return GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES;
}

sds DTTR_Path_CurrentDir() {
	char cwd[MAX_PATH];
	DWORD len = GetCurrentDirectoryA((DWORD)sizeof(cwd), cwd);
	if (len == 0 || len >= sizeof(cwd)) {
		return NULL;
	}

	return sdsnew(cwd);
}

sds DTTR_Path_ModuleDir(void *module) {
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

sds DTTR_Path_ModuleSibling(void *module, const char *relative_path) {
	sds path = DTTR_Path_ModuleDir(module);
	if (!path
		|| !DTTR_Path_AppendSegment(&path, relative_path, DTTR_PATH_NATIVE_SEPARATOR)) {
		sdsfree(path);
		return NULL;
	}

	return path;
}

sds DTTR_Path_ResolveRelativeTo(const char *base_dir, const char *path) {
	if (!path) {
		return NULL;
	}

	if (DTTR_Path_IsAnyAbsolute(path)) {
		return sdsnew(path);
	}

	sds resolved = sdsnew(base_dir ? base_dir : "");
	if (!resolved
		|| !DTTR_Path_AppendSegment(&resolved, path, DTTR_PATH_NATIVE_SEPARATOR)) {
		sdsfree(resolved);
		return NULL;
	}

	return resolved;
}

sds DTTR_Path_NativeRoot(const char *path, const char **rest) {
	if (DTTR_Path_IsWindowsAbsolute(path)) {
		*rest = path + 3;
		return sdscatprintf(sdsempty(), "%c:%c", path[0], DTTR_PATH_NATIVE_SEPARATOR);
	}

	if (DTTR_Path_IsSeparator(path[0])) {
		*rest = DTTR_Path_SkipSeparators(path);
		return sdsnewlen(&(char){DTTR_PATH_NATIVE_SEPARATOR}, 1);
	}

	*rest = path;
	return DTTR_Path_CurrentDir();
}

bool DTTR_Path_AppendChar(sds *path, char ch) {
	sds next = sdscatlen(*path, &ch, 1);
	if (!next) {
		return false;
	}

	*path = next;
	return true;
}

bool DTTR_Path_AppendSeparator(sds *path, char separator) {
	return DTTR_Path_AppendChar(path, separator);
}

bool DTTR_Path_AppendSegment(sds *path, const char *segment, char separator) {
	const size_t len = sdslen(*path);
	if (len > 0 && !DTTR_Path_IsSeparator((*path)[len - 1])) {
		if (!DTTR_Path_AppendChar(path, separator)) {
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
