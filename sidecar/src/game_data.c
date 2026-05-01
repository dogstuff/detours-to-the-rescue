#include "game_data_private.h"

#include <dttr_iso.h>
#include <dttr_path.h>

#include <sds.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <windows.h>

typedef struct {
	bool m_is_iso;
	char m_cache_root[DTTR_ISO_MAX_PATH];
	char m_game_root[DTTR_ISO_MAX_PATH];
} S_GameDataSource;

static S_GameDataSource s_source;

static bool s_copy_env(char *out, size_t out_size, const char *name) {
	return dttr_path_copy_string(out, out_size, getenv(name));
}

void dttr_game_data_cleanup(void) { memset(&s_source, 0, sizeof(s_source)); }

void dttr_game_data_init(void) {
	dttr_game_data_cleanup();
	if (!s_copy_env(
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

	s_source.m_is_iso = true;
}

static bool s_name_matches_segment(
	const char *name,
	const char *segment,
	size_t segment_len
) {
	return strlen(name) == segment_len
		   && dttr_path_ascii_ieq_n(name, segment, segment_len);
}

static bool s_find_case_match(
	const char *parent,
	const char *segment,
	size_t segment_len,
	char *out_name,
	size_t out_name_size
) {
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
			dttr_path_copy_string(out_name, out_name_size, data.cFileName);
			found = true;
			break;
		}
	} while (FindNextFileA(find, &data));
	FindClose(find);
	return found;
}

bool dttr_game_data_resolve_existing_read_path(
	const char *path,
	char *out_path,
	size_t out_path_size
) {
	if (!path || !path[0] || !out_path || out_path_size == 0) {
		return false;
	}

	const char *rest = NULL;
	sds resolved = dttr_path_native_root(path, &rest);
	if (!resolved) {
		return false;
	}

	rest = dttr_path_skip_separators(rest);

	bool wrote_segment = false;
	bool ok = true;
	while (*rest) {
		const char *segment = rest;
		size_t segment_len = dttr_path_segment_len(segment);
		if (dttr_path_is_relative_segment(segment, segment_len)) {
			ok = false;
			break;
		}

		char match[DTTR_ISO_MAX_PATH];
		if (!s_find_case_match(resolved, segment, segment_len, match, sizeof(match))) {
			ok = false;
			break;
		}

		if (!dttr_path_append_segment(&resolved, match, DTTR_PATH_NATIVE_SEPARATOR)) {
			ok = false;
			break;
		}

		wrote_segment = true;

		rest = dttr_path_skip_separators(rest + segment_len);
	}

	ok = ok && wrote_segment && dttr_path_exact_exists(resolved)
		 && dttr_path_copy_sds(out_path, out_path_size, resolved);
	sdsfree(resolved);

	return ok;
}

static const char *s_find_cached_segment(const char *path) {
	if (!path) {
		return NULL;
	}
	for (const char *p = path; *p;) {
		const size_t segment_len = dttr_path_segment_len(p);
		if (segment_len == 4 && dttr_path_ascii_ieq_n(p, "data", 4)) {
			return p;
		}

		if (segment_len == 10 && dttr_path_ascii_ieq_n(p, "pcdogs.pkg", 10)) {
			return p;
		}

		p = dttr_path_skip_separators(p + segment_len);
	}
	return NULL;
}

static bool s_append_game_path(const char *relative, char *out, size_t out_size) {
	if (!relative || !dttr_path_is_safe_relative(relative)) {
		return false;
	}

	relative = dttr_path_skip_separators(relative);
	sds path = sdsnew(s_source.m_game_root);
	if (!path || !dttr_path_append_segment(&path, relative, '/')) {
		sdsfree(path);
		return false;
	}

	const bool ok = dttr_path_copy_sds(out, out_size, path);
	sdsfree(path);
	return ok;
}

bool dttr_game_data_resolve_read_path(
	const char *path,
	char *out_path,
	size_t out_path_size
) {
	if (!s_source.m_is_iso || !path || !path[0] || !out_path || out_path_size == 0) {
		return false;
	}

	const char *relative = path;
	if (dttr_path_is_any_absolute(path)) {
		relative = s_find_cached_segment(path);
		if (!relative) {
			return false;
		}
	}

	char iso_path[DTTR_ISO_MAX_PATH];
	if (!s_append_game_path(relative, iso_path, sizeof(iso_path))) {
		return false;
	}

	return dttr_iso_cache_path_for_file(
			   s_source.m_cache_root,
			   iso_path,
			   out_path,
			   out_path_size
		   )
		   && dttr_path_exact_exists(out_path);
}
