#include <dttr_path.h>

#include <string.h>

#include <windows.h>

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
	if (!value || !value[0] || strlen(value) >= out_size) {
		return false;
	}

	strncpy(out, value, out_size - 1);
	out[out_size - 1] = '\0';
	return true;
}

bool dttr_path_copy_sds(char *out, size_t out_size, sds value) {
	if (!value || sdslen(value) >= out_size) {
		return false;
	}

	memcpy(out, value, sdslen(value) + 1);
	return true;
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
	if (sdslen(*path) > 0 && !dttr_path_is_separator((*path)[sdslen(*path) - 1])) {
		if (!dttr_path_append_separator(path, separator)) {
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
