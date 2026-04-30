#include "game/game_data_source_private.h"

#include <dttr_iso.h>
#include <dttr_log.h>

#include <sds.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#define S_PATH_SEP '\\'
#else
#include <dirent.h>
#include <unistd.h>
#define S_PATH_SEP '/'
#endif

typedef struct {
	bool m_is_iso;
	DTTR_IsoImage m_iso;
	char m_iso_path[DTTR_ISO_MAX_PATH];
	char m_cache_root[DTTR_ISO_MAX_PATH];
	char m_game_root[DTTR_ISO_MAX_PATH];
} S_GameDataSource;

static S_GameDataSource s_source;

static bool s_copy_string(char *out, size_t out_size, const char *value) {
	if (!value || !value[0] || strlen(value) >= out_size) {
		return false;
	}

	strncpy(out, value, out_size - 1);
	out[out_size - 1] = '\0';
	return true;
}

static char s_ascii_lower(char ch) {
	if (ch >= 'A' && ch <= 'Z') {
		return (char)(ch - 'A' + 'a');
	}
	return ch;
}

static bool s_is_path_sep(char ch) { return ch == '\\' || ch == '/'; }

static const char *s_skip_path_separators(const char *path) {
	while (*path && s_is_path_sep(*path)) {
		path++;
	}

	return path;
}

static bool s_ascii_ieq_n(const char *lhs, const char *rhs, size_t n) {
	for (size_t i = 0; i < n; i++) {
		if (s_ascii_lower(lhs[i]) != s_ascii_lower(rhs[i])) {
			return false;
		}
	}
	return true;
}

static bool s_copy_env(char *out, size_t out_size, const char *name) {
	return s_copy_string(out, out_size, getenv(name));
}

void dttr_game_data_source_cleanup(void) {
	if (s_source.m_is_iso) {
		dttr_iso_close(&s_source.m_iso);
	}
	memset(&s_source, 0, sizeof(s_source));
}

void dttr_game_data_source_init(void) {
	dttr_game_data_source_cleanup();
	if (!s_copy_env(s_source.m_iso_path, sizeof(s_source.m_iso_path), "DTTR_ISO_PATH")
		|| !s_copy_env(
			s_source.m_cache_root,
			sizeof(s_source.m_cache_root),
			"DTTR_ISO_CACHE_ROOT"
		)
		|| !s_copy_env(
			s_source.m_game_root,
			sizeof(s_source.m_game_root),
			"DTTR_ISO_GAME_ROOT"
		)) {
		memset(&s_source, 0, sizeof(s_source));
		return;
	}

	if (!dttr_iso_open(&s_source.m_iso, s_source.m_iso_path)) {
		DTTR_LOG_ERROR("Could not open direct ISO data source: %s", s_source.m_iso_path);
		memset(&s_source, 0, sizeof(s_source));
		return;
	}
	s_source.m_is_iso = true;
	DTTR_LOG_INFO("Direct ISO data source enabled: %s", s_source.m_iso_path);
}

static bool s_is_absolute_path(const char *path) {
	return path && strlen(path) >= 3 && path[1] == ':' && s_is_path_sep(path[2]);
}

static bool s_is_any_absolute_path(const char *path) {
	if (!path) {
		return false;
	}

	return s_is_absolute_path(path) || s_is_path_sep(path[0]);
}

static bool s_path_exact_exists(const char *path) {
#ifdef _WIN32
	return GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES;
#else
	return access(path, F_OK) == 0;
#endif
}

static sds s_get_current_dir(void) {
	char cwd[DTTR_ISO_MAX_PATH];
#ifdef _WIN32
	DWORD len = GetCurrentDirectoryA((DWORD)sizeof(cwd), cwd);
	if (len == 0 || len >= sizeof(cwd)) {
		return NULL;
	}
#else
	if (!getcwd(cwd, sizeof(cwd))) {
		return NULL;
	}
#endif
	return sdsnew(cwd);
}

static sds s_path_root(const char *path, const char **rest) {
	if (s_is_absolute_path(path)) {
		*rest = path + 3;
		return sdscatprintf(sdsempty(), "%c:%c", path[0], S_PATH_SEP);
	}

	if (s_is_path_sep(path[0])) {
		*rest = s_skip_path_separators(path);
		return sdsnewlen(&(char){S_PATH_SEP}, 1);
	}

	*rest = path;
	return s_get_current_dir();
}

static bool s_copy_sds(char *out, size_t out_size, sds value) {
	if (!value || sdslen(value) >= out_size) {
		return false;
	}

	memcpy(out, value, sdslen(value) + 1);
	return true;
}

static bool s_append_path_segment(sds *path, const char *segment) {
	if (sdslen(*path) > 0 && !s_is_path_sep((*path)[sdslen(*path) - 1])) {
		sds next = sdscatlen(*path, &(char){S_PATH_SEP}, 1);
		if (!next) {
			return false;
		}

		*path = next;
	}

	sds next = sdscat(*path, segment);
	if (!next) {
		return false;
	}

	*path = next;
	return true;
}

static bool s_name_matches_segment(
	const char *name,
	const char *segment,
	size_t segment_len
) {
	return strlen(name) == segment_len && s_ascii_ieq_n(name, segment, segment_len);
}

static bool s_find_case_match(
	const char *parent,
	const char *segment,
	size_t segment_len,
	char *out_name,
	size_t out_name_size
) {
#ifdef _WIN32
	char pattern[DTTR_ISO_MAX_PATH];
	int written = snprintf(
		pattern,
		sizeof(pattern),
		"%s%s*",
		parent,
		(parent[0] && parent[strlen(parent) - 1] != '\\'
		 && parent[strlen(parent) - 1] != '/')
			? "\\"
			: ""
	);
	if (written <= 0 || (size_t)written >= sizeof(pattern)) {
		return false;
	}

	WIN32_FIND_DATAA data;
	HANDLE find = FindFirstFileA(pattern, &data);
	if (find == INVALID_HANDLE_VALUE) {
		return false;
	}

	bool found = false;
	do {
		if (s_name_matches_segment(data.cFileName, segment, segment_len)) {
			s_copy_string(out_name, out_name_size, data.cFileName);
			found = true;
			break;
		}
	} while (FindNextFileA(find, &data));
	FindClose(find);
	return found;
#else
	DIR *dir = opendir(parent);
	if (!dir) {
		return false;
	}
	bool found = false;
	struct dirent *entry;
	while ((entry = readdir(dir)) != NULL) {
		if (s_name_matches_segment(entry->d_name, segment, segment_len)) {
			s_copy_string(out_name, out_name_size, entry->d_name);
			found = true;
			break;
		}
	}
	closedir(dir);
	return found;
#endif
}

bool dttr_game_data_source_resolve_existing_read_path(
	const char *path,
	char *out_path,
	size_t out_path_size
) {
	if (!path || !path[0] || !out_path || out_path_size == 0) {
		return false;
	}

	const char *rest = NULL;
	sds resolved = s_path_root(path, &rest);
	if (!resolved) {
		return false;
	}

	rest = s_skip_path_separators(rest);

	bool wrote_segment = false;
	bool ok = true;
	while (*rest) {
		const char *segment = rest;
		size_t segment_len = 0;
		while (segment[segment_len] && !s_is_path_sep(segment[segment_len])) {
			segment_len++;
		}
		if (segment_len == 0 || (segment_len == 1 && segment[0] == '.')
			|| (segment_len == 2 && segment[0] == '.' && segment[1] == '.')) {
			ok = false;
			break;
		}

		char match[DTTR_ISO_MAX_PATH];
		if (!s_find_case_match(resolved, segment, segment_len, match, sizeof(match))) {
			ok = false;
			break;
		}

		if (!s_append_path_segment(&resolved, match)) {
			ok = false;
			break;
		}

		wrote_segment = true;

		rest = s_skip_path_separators(rest + segment_len);
	}

	ok = ok && wrote_segment && s_path_exact_exists(resolved)
		 && s_copy_sds(out_path, out_path_size, resolved);
	sdsfree(resolved);

	return ok;
}

static const char *s_find_data_segment(const char *path) {
	if (!path) {
		return NULL;
	}
	for (const char *p = path; *p; p++) {
		const bool at_boundary = p == path || s_is_path_sep(p[-1]);
		if (!at_boundary) {
			continue;
		}

		if (p[0] == '\0' || p[1] == '\0' || p[2] == '\0' || p[3] == '\0') {
			return NULL;
		}

		if (s_ascii_ieq_n(p, "data", 4) && s_is_path_sep(p[4])) {
			return p;
		}
	}
	return NULL;
}

static bool s_append_game_path(const char *relative, char *out, size_t out_size) {
	if (!relative || !relative[0]) {
		return false;
	}
	relative = s_skip_path_separators(relative);
	if (!*relative || strstr(relative, "..")) {
		return false;
	}
	const int written = snprintf(out, out_size, "%s/%s", s_source.m_game_root, relative);
	return written > 0 && (size_t)written < out_size;
}

static bool s_extract(const char *iso_path, char *out_path, size_t out_path_size) {
	return dttr_iso_extract_file(
		&s_source.m_iso,
		iso_path,
		s_source.m_cache_root,
		out_path,
		out_path_size
	);
}

bool dttr_game_data_source_resolve_read_path(
	const char *path,
	char *out_path,
	size_t out_path_size
) {
	if (!s_source.m_is_iso || !path || !path[0] || !out_path || out_path_size == 0) {
		return false;
	}

	const char *relative = path;
	if (s_is_any_absolute_path(path)) {
		relative = s_find_data_segment(path);
		if (!relative) {
			return false;
		}
	}

	char iso_path[DTTR_ISO_MAX_PATH];
	if (!s_append_game_path(relative, iso_path, sizeof(iso_path))) {
		return false;
	}

	if (!s_extract(iso_path, out_path, out_path_size)) {
		DTTR_LOG_ERROR("Could not extract ISO game data path: %s", iso_path);
		return false;
	}
	return true;
}
