#include <dttr_loader_paths.h>

#include <sds.h>
#include <xxhash.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct {
	const char *root;
	const wchar_t *exe_subpath;
} GameLayout;

static const GameLayout GAME_LAYOUTS[] = {
	{".", L"pcdogs.exe"},
	{"Setup/102Dalms", L"Setup\\102Dalms\\pcdogs.exe"},
};

static const wchar_t ISO_SUFFIX[] = L".iso";
static const char *const ISO_CACHE_PATH = "DetoursToTheRescue\\cache\\iso";

static wchar_t ascii_lower_w(wchar_t ch) {
	if (ch >= L'A' && ch <= L'Z') {
		return (wchar_t)(ch - L'A' + L'a');
	}

	return ch;
}

static bool has_suffix_w(const wchar_t *path, const wchar_t *suffix) {
	const size_t path_len = wcslen(path);
	const size_t suffix_len = wcslen(suffix);
	if (path_len < suffix_len) {
		return false;
	}

	const wchar_t *candidate = path + path_len - suffix_len;
	for (size_t i = 0; i < suffix_len; i++) {
		if (ascii_lower_w(candidate[i]) != ascii_lower_w(suffix[i])) {
			return false;
		}
	}

	return true;
}

bool DTTR_LoaderPath_IsISOW(const wchar_t *path) {
	return path && has_suffix_w(path, ISO_SUFFIX);
}

size_t DTTR_Loader_GameSubpathCount() {
	return sizeof(GAME_LAYOUTS) / sizeof(GAME_LAYOUTS[0]);
}

const wchar_t *DTTR_Loader_GameSubpathAt(size_t index) {
	return index < DTTR_Loader_GameSubpathCount() ? GAME_LAYOUTS[index].exe_subpath
												  : NULL;
}

const char *DTTR_Loader_GameRootAt(size_t index) {
	return index < DTTR_Loader_GameSubpathCount() ? GAME_LAYOUTS[index].root : NULL;
}

static uint64_t hash_path(const char *path) {
	sds normalized = sdsnew(path);
	if (!normalized) {
		return 0;
	}

	sdstolower(normalized);
	sdsmapchars(normalized, "/", "\\", 1);

	const XXH64_hash_t hash = XXH3_64bits(normalized, sdslen(normalized));
	sdsfree(normalized);
	return hash;
}

static bool path_needs_separator(const char *path) {
	const size_t len = strlen(path);
	return len > 0 && path[len - 1] != '\\' && path[len - 1] != '/';
}

bool DTTR_LoaderISO_CacheRootForPath(
	const char *cache_base_dir,
	const char *iso_path,
	char *out_path,
	size_t out_path_size
) {
	if (!cache_base_dir || !cache_base_dir[0] || !iso_path || !iso_path[0] || !out_path
		|| out_path_size == 0) {
		return false;
	}

	const uint64_t hash = hash_path(iso_path);
	const bool needs_separator = path_needs_separator(cache_base_dir);

	const int written = snprintf(
		out_path,
		out_path_size,
		"%s%s%s\\%016llx",
		cache_base_dir,
		needs_separator ? "\\" : "",
		ISO_CACHE_PATH,
		(unsigned long long)hash
	);
	return written > 0 && (size_t)written < out_path_size;
}
