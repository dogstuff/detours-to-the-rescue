#include <dttr_iso.h>
#include <dttr_path.h>

#include <physfs.h>
#include <sds.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <direct.h>
#include <windows.h>

#define S_MKDIR(path)                                                                    \
	(CreateDirectoryA((path), NULL) || GetLastError() == ERROR_ALREADY_EXISTS)

enum { S_ISO_SECTOR_SIZE = 2048 };

static int s_physfs_refcount;
static char s_last_error[256];

static void s_set_error(const char *message) {
	if (!message) {
		message = "unknown error";
	}
	strncpy(s_last_error, message, sizeof(s_last_error) - 1);
	s_last_error[sizeof(s_last_error) - 1] = '\0';
}

static void s_set_physfs_error(const char *context) {
	const char *err = PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode());
	if (!err) {
		err = "unknown PhysicsFS error";
	}
	snprintf(s_last_error, sizeof(s_last_error), "%s: %s", context, err);
}

const char *dttr_iso_last_error(void) {
	return s_last_error[0] ? s_last_error : "no error";
}

static bool s_is_iso_version_suffix(const char *suffix) {
	if (!suffix || suffix[0] != ';' || !suffix[1]) {
		return false;
	}

	for (const char *ch = suffix + 1; *ch; ch++) {
		if (*ch < '0' || *ch > '9') {
			return false;
		}
	}

	return true;
}

static size_t s_strip_iso_version_suffix_len(const char *segment, size_t segment_len) {
	for (size_t i = segment_len; i > 1; i--) {
		if (segment[i - 1] == ';' && s_is_iso_version_suffix(segment + i - 1)) {
			return i - 1;
		}
	}

	return segment_len;
}

static bool s_sdscat_lower_segment(sds *out, const char *segment, size_t segment_len) {
	segment_len = s_strip_iso_version_suffix_len(segment, segment_len);
	for (size_t i = 0; i < segment_len; i++) {
		const char ch = dttr_path_ascii_lower(segment[i]);
		if (!dttr_path_append_char(out, ch)) {
			return false;
		}
	}

	return true;
}

bool dttr_iso_cache_path_for_file(
	const char *cache_root,
	const char *iso_relative_path,
	char *out_path,
	size_t out_path_size
) {
	if (!cache_root || !cache_root[0] || !iso_relative_path || !iso_relative_path[0]
		|| !out_path || out_path_size == 0) {
		return false;
	}

	sds path = sdsnew(cache_root);
	if (!path) {
		return false;
	}

	while (sdslen(path) > 0 && dttr_path_is_separator(path[sdslen(path) - 1])) {
		sdsrange(path, 0, -2);
	}

	const char *p = dttr_path_skip_separators(iso_relative_path);

	bool wrote_segment = false;
	bool ok = true;
	while (*p) {
		const char *segment = p;
		size_t segment_len = dttr_path_segment_len(p);

		if (dttr_path_is_relative_segment(segment, segment_len)) {
			ok = false;
			break;
		}

		if (!dttr_path_append_separator(&path, '\\')
			|| !s_sdscat_lower_segment(&path, segment, segment_len)) {
			ok = false;
			break;
		}

		wrote_segment = true;

		p += segment_len;
		if (dttr_path_is_separator(*p)) {
			p = dttr_path_skip_separators(p);
			if (!*p) {
				ok = false;
				break;
			}
		}
	}

	ok = ok && wrote_segment && dttr_path_copy_sds(out_path, out_path_size, path);
	sdsfree(path);
	return ok;
}

static bool s_physfs_init(void) {
	if (s_physfs_refcount > 0) {
		s_physfs_refcount++;
		return true;
	}

	if (!PHYSFS_init("dttr")) {
		s_set_physfs_error("PHYSFS_init failed");
		return false;
	}

	s_physfs_refcount = 1;

	return true;
}

static void s_physfs_deinit(void) {
	if (s_physfs_refcount <= 0) {
		return;
	}

	s_physfs_refcount--;

	if (s_physfs_refcount == 0) {
		PHYSFS_deinit();
	}
}

bool dttr_iso_open(DTTR_IsoImage *iso, const char *iso_path) {
	if (!iso || !iso_path || !iso_path[0]
		|| strlen(iso_path) >= sizeof(iso->m_iso_path)) {
		s_set_error("invalid ISO path");
		return false;
	}

	memset(iso, 0, sizeof(*iso));

	if (!s_physfs_init()) {
		return false;
	}

	if (!PHYSFS_mount(iso_path, NULL, 1)) {
		s_set_physfs_error("PHYSFS_mount failed");
		s_physfs_deinit();
		return false;
	}

	dttr_path_copy_string(iso->m_iso_path, sizeof(iso->m_iso_path), iso_path);
	iso->m_open = true;

	return true;
}

static bool s_name_matches_segment(
	const char *name,
	const char *segment,
	size_t segment_len
) {
	const size_t name_len = strlen(name);
	return (name_len == segment_len
			|| (name_len > segment_len && s_is_iso_version_suffix(name + segment_len)))
		   && dttr_path_ascii_ieq_n(name, segment, segment_len);
}

static bool s_find_case_match(
	const char *parent,
	const char *segment,
	size_t segment_len,
	char *out_name,
	size_t out_name_size
) {
	char **entries = PHYSFS_enumerateFiles(parent && parent[0] ? parent : "");
	if (!entries) {
		return false;
	}

	bool found = false;
	for (char **entry = entries; *entry; entry++) {
		if (!s_name_matches_segment(*entry, segment, segment_len)) {
			continue;
		}

		dttr_path_copy_string(out_name, out_name_size, *entry);
		found = true;
		break;
	}

	PHYSFS_freeList(entries);
	return found;
}

static bool s_resolve_iso_path_case(
	const char *requested,
	char *out_path,
	size_t out_path_size
) {
	if (!requested || !requested[0] || !out_path || out_path_size == 0) {
		return false;
	}

	sds path = sdsempty();
	if (!path) {
		return false;
	}

	const char *p = dttr_path_skip_separators(requested);

	bool wrote_segment = false;
	bool ok = true;
	while (*p) {
		const char *segment = p;
		size_t segment_len = dttr_path_segment_len(p);
		if (dttr_path_is_relative_segment(segment, segment_len)) {
			ok = false;
			break;
		}

		char match[DTTR_ISO_MAX_PATH];
		if (!s_find_case_match(path, segment, segment_len, match, sizeof(match))) {
			ok = false;
			break;
		}

		if (!dttr_path_append_segment(&path, match, '/')) {
			ok = false;
			break;
		}

		wrote_segment = true;

		p += segment_len;
		p = dttr_path_skip_separators(p);
	}

	ok = ok && wrote_segment && dttr_path_copy_sds(out_path, out_path_size, path);
	sdsfree(path);
	return ok;
}

static bool s_create_parent_dirs(const char *path) {
	char tmp[DTTR_ISO_MAX_PATH];

	const size_t len = strlen(path);
	if (len >= sizeof(tmp)) {
		return false;
	}

	memcpy(tmp, path, len + 1);

	for (size_t i = 1; tmp[i]; i++) {
		if (!dttr_path_is_separator(tmp[i])) {
			continue;
		}

		if (i == 2 && tmp[1] == ':') {
			continue;
		}

		char saved = tmp[i];
		tmp[i] = '\0';

		if (tmp[0]) {
			(void)S_MKDIR(tmp);
		}

		tmp[i] = saved;
	}
	return true;
}

static bool s_file_has_size(const char *path, size_t size) {
	FILE *file = fopen(path, "rb");
	if (!file) {
		return false;
	}

	bool matches = false;

	if (fseek(file, 0, SEEK_END) == 0) {
		const long existing = ftell(file);
		matches = existing >= 0 && (size_t)existing == size;
	}

	fclose(file);

	return matches;
}

static sds s_child_iso_path(const char *parent, const char *entry) {
	sds child = sdsnew(parent);
	if (!child) {
		return NULL;
	}

	if (!dttr_path_append_segment(&child, entry, '/')) {
		sdsfree(child);
		return NULL;
	}

	return child;
}

bool dttr_iso_extract_file(
	DTTR_IsoImage *iso,
	const char *iso_relative_path,
	const char *cache_root,
	char *out_path,
	size_t out_path_size
) {
	if (!iso || !iso->m_open) {
		s_set_error("ISO is not open");
		return false;
	}

	if (!dttr_iso_cache_path_for_file(
			cache_root,
			iso_relative_path,
			out_path,
			out_path_size
		)) {
		s_set_error("invalid ISO cache path");
		return false;
	}

	char physfs_path[DTTR_ISO_MAX_PATH];
	if (!s_resolve_iso_path_case(iso_relative_path, physfs_path, sizeof(physfs_path))) {
		s_set_error("file not found in ISO");
		return false;
	}

	PHYSFS_File *in = PHYSFS_openRead(physfs_path);
	if (!in) {
		s_set_physfs_error("PHYSFS_openRead failed");
		return false;
	}

	const PHYSFS_sint64 length = PHYSFS_fileLength(in);
	if (length < 0) {
		s_set_physfs_error("PHYSFS_fileLength failed");
		PHYSFS_close(in);
		return false;
	}

	if (s_file_has_size(out_path, (size_t)length)) {
		PHYSFS_close(in);
		return true;
	}

	if (!s_create_parent_dirs(out_path)) {
		s_set_error("could not create cache directories");
		PHYSFS_close(in);
		return false;
	}

	FILE *out = fopen(out_path, "wb");
	if (!out) {
		s_set_error("could not open cache output file");
		PHYSFS_close(in);
		return false;
	}

	char buffer[S_ISO_SECTOR_SIZE];
	PHYSFS_sint64 remaining = length;
	while (remaining > 0) {
		const PHYSFS_uint64 chunk = remaining < (PHYSFS_sint64)sizeof(buffer)
										? (PHYSFS_uint64)remaining
										: (PHYSFS_uint64)sizeof(buffer);
		const PHYSFS_sint64 got = PHYSFS_readBytes(in, buffer, chunk);
		if (got != (PHYSFS_sint64)chunk
			|| fwrite(buffer, 1, (size_t)chunk, out) != chunk) {
			s_set_error("could not extract ISO file");
			fclose(out);
			PHYSFS_close(in);
			return false;
		}
		remaining -= got;
	}

	fclose(out);
	PHYSFS_close(in);
	return true;
}

static bool s_extract_tree_path(
	DTTR_IsoImage *iso,
	const char *physfs_path,
	const char *cache_root
) {
	char **entries = PHYSFS_enumerateFiles(physfs_path);
	if (!entries) {
		s_set_physfs_error("PHYSFS_enumerateFiles failed");
		return false;
	}

	bool ok = true;
	for (char **entry = entries; ok && *entry; entry++) {
		sds child = s_child_iso_path(physfs_path, *entry);
		if (!child) {
			s_set_error("could not build ISO tree path");
			ok = false;
			break;
		}

		PHYSFS_Stat stat;
		if (!PHYSFS_stat(child, &stat)) {
			s_set_physfs_error("PHYSFS_stat failed");
			ok = false;
		} else if (stat.filetype == PHYSFS_FILETYPE_DIRECTORY) {
			ok = s_extract_tree_path(iso, child, cache_root);
		} else if (stat.filetype == PHYSFS_FILETYPE_REGULAR) {
			char out_path[DTTR_ISO_MAX_PATH];
			ok = dttr_iso_extract_file(iso, child, cache_root, out_path, sizeof(out_path));
		}

		sdsfree(child);
	}

	PHYSFS_freeList(entries);
	return ok;
}

bool dttr_iso_extract_tree(
	DTTR_IsoImage *iso,
	const char *iso_relative_path,
	const char *cache_root
) {
	if (!iso || !iso->m_open) {
		s_set_error("ISO is not open");
		return false;
	}

	char physfs_path[DTTR_ISO_MAX_PATH];
	if (!s_resolve_iso_path_case(iso_relative_path, physfs_path, sizeof(physfs_path))) {
		s_set_error("directory not found in ISO");
		return false;
	}

	PHYSFS_Stat stat;
	if (!PHYSFS_stat(physfs_path, &stat)) {
		s_set_physfs_error("PHYSFS_stat failed");
		return false;
	}
	if (stat.filetype != PHYSFS_FILETYPE_DIRECTORY) {
		s_set_error("ISO path is not a directory");
		return false;
	}

	return s_extract_tree_path(iso, physfs_path, cache_root);
}

void dttr_iso_close(DTTR_IsoImage *iso) {
	if (!iso || !iso->m_open) {
		return;
	}

	PHYSFS_unmount(iso->m_iso_path);
	s_physfs_deinit();
	memset(iso, 0, sizeof(*iso));
}
