#include <dttr_loader_paths.h>

#include <sds.h>
#include <xxhash.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static const wchar_t *const S_GAME_SUBPATHS[] = {
	L"pcdogs.exe",
	L"Setup\\102Dalms\\pcdogs.exe",
};

static const wchar_t S_ISO_SUFFIX[] = L".iso";
static const char *S_ISO_GAME_ROOT = "Setup/102Dalms";
static const char *S_ISO_GAME_EXE_PATH = "Setup/102Dalms/pcdogs.exe";
static const char *S_ISO_GAME_PKG_PATH = "Setup/102Dalms/pcdogs.pkg";
static const char *S_ISO_GAME_DATA_PATH = "Setup/102Dalms/data";
static wchar_t s_ascii_lower_w(wchar_t ch) {
	if (ch >= L'A' && ch <= L'Z') {
		return (wchar_t)(ch - L'A' + L'a');
	}
	return ch;
}

static char s_normalized_path_char(char ch) {
	if (ch >= 'A' && ch <= 'Z') {
		ch = (char)(ch - 'A' + 'a');
	}
	return ch == '/' ? '\\' : ch;
}

bool dttr_loader_path_is_iso_w(const wchar_t *path) {
	if (!path) {
		return false;
	}

	const size_t path_len = wcslen(path);
	const size_t suffix_len = sizeof(S_ISO_SUFFIX) / sizeof(S_ISO_SUFFIX[0]) - 1;
	if (path_len < suffix_len) {
		return false;
	}

	const wchar_t *candidate = path + path_len - suffix_len;
	for (size_t i = 0; i < suffix_len; i++) {
		if (s_ascii_lower_w(candidate[i]) != S_ISO_SUFFIX[i]) {
			return false;
		}
	}

	return true;
}

size_t dttr_loader_game_subpath_count(void) {
	return sizeof(S_GAME_SUBPATHS) / sizeof(S_GAME_SUBPATHS[0]);
}

const wchar_t *dttr_loader_game_subpath_at(size_t index) {
	if (index >= dttr_loader_game_subpath_count()) {
		return NULL;
	}
	return S_GAME_SUBPATHS[index];
}

const char *dttr_loader_iso_game_root(void) { return S_ISO_GAME_ROOT; }

const char *dttr_loader_iso_game_exe_path(void) { return S_ISO_GAME_EXE_PATH; }

const char *dttr_loader_iso_game_pkg_path(void) { return S_ISO_GAME_PKG_PATH; }

const char *dttr_loader_iso_game_data_path(void) { return S_ISO_GAME_DATA_PATH; }

static uint64_t s_hash_path(const char *path) {
	sds normalized = sdsnew(path);
	if (!normalized) {
		return 0;
	}

	for (char *ch = normalized; *ch; ch++) {
		*ch = s_normalized_path_char(*ch);
	}

	const XXH64_hash_t hash = XXH3_64bits(normalized, sdslen(normalized));
	sdsfree(normalized);
	return hash;
}

bool dttr_loader_iso_cache_root_for_path(
	const char *cache_base_dir,
	const char *iso_path,
	char *out_path,
	size_t out_path_size
) {
	if (!cache_base_dir || !cache_base_dir[0] || !iso_path || !iso_path[0] || !out_path
		|| out_path_size == 0) {
		return false;
	}

	const uint64_t hash = s_hash_path(iso_path);
	const size_t cache_base_len = strlen(cache_base_dir);
	const bool needs_separator = cache_base_len > 0
								 && cache_base_dir[cache_base_len - 1] != '\\'
								 && cache_base_dir[cache_base_len - 1] != '/';

	const int written = snprintf(
		out_path,
		out_path_size,
		"%s%sDetoursToTheRescue\\cache\\iso\\%016llx",
		cache_base_dir,
		needs_separator ? "\\" : "",
		(unsigned long long)hash
	);
	return written > 0 && (size_t)written < out_path_size;
}
