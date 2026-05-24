#include "sidecar_private.h"

#include <dttr_iso.h>
#include <dttr_path.h>

#include <sds.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <windows.h>

typedef struct {
	bool is_iso;
	char cache_root[DTTR_ISO_MAX_PATH];
	char game_root[DTTR_ISO_MAX_PATH];
} game_data_source;

static game_data_source source;

static void clear_source() { memset(&source, 0, sizeof(source)); }

static bool copy_env(char *out, size_t out_size, const char *name) {
	return DTTR_Path_CopyString(out, out_size, getenv(name));
}

void dttr_game_data_cleanup() { clear_source(); }

void dttr_game_data_init() {
	dttr_game_data_cleanup();
	if (!copy_env(source.cache_root, sizeof(source.cache_root), "DTTR_ISO_CACHE_ROOT")
		|| !copy_env(source.game_root, sizeof(source.game_root), "DTTR_ISO_GAME_ROOT")) {
		clear_source();
		return;
	}

	source.is_iso = true;
}

static bool name_matches_segment(
	const char *name,
	const char *segment,
	size_t segment_len
) {
	return strlen(name) == segment_len && DTTR_Path_AsciiIeqN(name, segment, segment_len);
}

static bool find_case_match(
	const char *parent,
	const char *segment,
	size_t segment_len,
	char *out_name,
	size_t out_name_size
) {
	const size_t parent_len = strlen(parent);
	const bool needs_separator = parent_len > 0
								 && !DTTR_Path_IsSeparator(parent[parent_len - 1]);
	char pattern[DTTR_ISO_MAX_PATH];
	const int written = snprintf(
		pattern,
		sizeof(pattern),
		"%s%s*",
		parent,
		needs_separator ? "\\" : ""
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
		if (name_matches_segment(data.cFileName, segment, segment_len)) {
			DTTR_Path_CopyString(out_name, out_name_size, data.cFileName);
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
	sds resolved = DTTR_Path_NativeRoot(path, &rest);
	if (!resolved) {
		return false;
	}

	rest = DTTR_Path_SkipSeparators(rest);

	bool wrote_segment = false;
	bool ok = true;
	while (*rest) {
		const char *segment = rest;
		size_t segment_len = DTTR_Path_SegmentLen(segment);
		if (DTTR_Path_IsRelativeSegment(segment, segment_len)) {
			ok = false;
			break;
		}

		char match[DTTR_ISO_MAX_PATH];
		if (!find_case_match(resolved, segment, segment_len, match, sizeof(match))) {
			ok = false;
			break;
		}

		if (!DTTR_Path_AppendSegment(&resolved, match, DTTR_PATH_NATIVE_SEPARATOR)) {
			ok = false;
			break;
		}

		wrote_segment = true;

		rest = DTTR_Path_SkipSeparators(rest + segment_len);
	}

	ok = ok && wrote_segment && DTTR_Path_ExactExists(resolved)
		 && DTTR_Path_CopySds(out_path, out_path_size, resolved);
	sdsfree(resolved);

	return ok;
}

static const char *find_cached_segment(const char *path) {
	if (!path) {
		return NULL;
	}

	for (const char *p = path; *p;) {
		const size_t segment_len = DTTR_Path_SegmentLen(p);
		if (segment_len == 4 && DTTR_Path_AsciiIeqN(p, "data", 4)) {
			return p;
		}

		if (segment_len == 10 && DTTR_Path_AsciiIeqN(p, "pcdogs.pkg", 10)) {
			return p;
		}

		p = DTTR_Path_SkipSeparators(p + segment_len);
	}

	return NULL;
}

static bool append_game_path(const char *relative, char *out, size_t out_size) {
	if (!relative || !DTTR_Path_IsSafeRelative(relative)) {
		return false;
	}

	relative = DTTR_Path_SkipSeparators(relative);
	sds path = sdsnew(source.game_root);
	if (!path || !DTTR_Path_AppendSegment(&path, relative, '/')) {
		sdsfree(path);
		return false;
	}

	const bool ok = DTTR_Path_CopySds(out, out_size, path);
	sdsfree(path);
	return ok;
}

bool dttr_game_data_resolve_read_path(
	const char *path,
	char *out_path,
	size_t out_path_size
) {
	if (!source.is_iso || !path || !path[0] || !out_path || out_path_size == 0) {
		return false;
	}

	const char *relative = path;
	if (DTTR_Path_IsAnyAbsolute(path)) {
		relative = find_cached_segment(path);
		if (!relative) {
			return false;
		}
	}

	char iso_path[DTTR_ISO_MAX_PATH];
	if (!append_game_path(relative, iso_path, sizeof(iso_path))) {
		return false;
	}

	return DTTR_ISO_CachePathForFile(source.cache_root, iso_path, out_path, out_path_size)
		   && DTTR_Path_ExactExists(out_path);
}
