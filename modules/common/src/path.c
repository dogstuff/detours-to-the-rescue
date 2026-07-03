#include <dttr_path.h>

#include <string.h>

#include <cwalk.h>
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

bool DTTR_Path_IsSeparator(char ch) {
	return ch == '\\' || ch == '/';
}

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

static sds cwalk_absolute_sds(const char *base, const char *path) {
	const size_t len = cwk_path_get_absolute(base, path, NULL, 0);
	sds resolved = sdsnewlen(NULL, len);
	if (!resolved) {
		return NULL;
	}

	cwk_path_get_absolute(base, path, resolved, len + 1);
	return resolved;
}

static sds cwalk_join_sds(const char *path, const char *segment) {
	const size_t len = cwk_path_join(path, segment, NULL, 0);
	sds joined = sdsnewlen(NULL, len);
	if (!joined) {
		return NULL;
	}

	cwk_path_join(path, segment, joined, len + 1);
	return joined;
}

static sds cwalk_normalize_native_sds(const char *path) {
	const size_t len = cwk_path_normalize(path, NULL, 0);
	sds normalized = sdsnewlen(NULL, len);
	if (!normalized) {
		return NULL;
	}

	cwk_path_normalize(path, normalized, len + 1);
	return normalized;
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

	sds normalized = cwalk_normalize_native_sds(path);
	if (normalized) {
		sdsmapchars(normalized, "\\", "/", 1);
	}

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

	return cwk_path_is_absolute(path);
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
		return cwalk_normalize_native_sds(path);
	}

	if (!base_dir || !*base_dir) {
		return cwalk_normalize_native_sds(path);
	}

	return cwalk_absolute_sds(base_dir, path);
}

sds DTTR_Path_NativeRoot(const char *path, const char **rest) {
	if (!path || !rest) {
		return NULL;
	}

	size_t root_len = 0;
	cwk_path_get_root(path, &root_len);
	if (root_len > 0 && cwk_path_is_absolute(path)) {
		*rest = DTTR_Path_SkipSeparators(path + root_len);
		return sdsnewlen(path, root_len);
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
	sds joined = cwalk_join_sds(*path, segment);
	if (!joined) {
		return false;
	}

	char separators[] = {separator, separator, '\0'};
	sdsmapchars(joined, "\\/", separators, 2);

	sdsfree(*path);
	*path = joined;
	return true;
}
