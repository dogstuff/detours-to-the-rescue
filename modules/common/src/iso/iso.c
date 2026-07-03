#include <dttr_iso.h>
#include <dttr_path.h>

#include <physfs.h>
#include <sds.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <direct.h>
#include <windows.h>

#define MKDIR(path)                                                                      \
	(CreateDirectoryA((path), NULL) || GetLastError() == ERROR_ALREADY_EXISTS)

enum { ISO_SECTOR_SIZE = 2048 };

static int physfs_refcount;
static char last_error[256];
static bool last_error_was_not_found;

// Records the latest ISO-layer failure for launcher errors.
static void set_error(const char *message) {
	last_error_was_not_found = false;

	if (!message) {
		message = "unknown error";
	}

	strncpy(last_error, message, sizeof(last_error) - 1);
	last_error[sizeof(last_error) - 1] = '\0';
}

static void set_not_found_error(const char *message) {
	set_error(message);
	last_error_was_not_found = true;
}

// Preserves the PhysicsFS failure text with the operation that triggered it.
static void set_physfs_error(const char *context) {
	last_error_was_not_found = false;
	const char *err = PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode());

	if (!err) {
		err = "unknown PhysicsFS error";
	}

	snprintf(last_error, sizeof(last_error), "%s: %s", context, err);
}

// Returns the current ISO error or a stable default before the first failure.
const char *DTTR_ISO_LastError() {
	return last_error[0] ? last_error : "no error";
}

bool DTTR_ISO_LastErrorWasNotFound() {
	return last_error_was_not_found;
}

static bool is_iso_version_suffix(const char *suffix) {
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

// Returns the segment length without a trailing ISO9660 version suffix.
static size_t strip_iso_version_suffix_len(const char *segment, size_t segment_len) {
	for (size_t i = segment_len; i > 1; i--) {
		if (segment[i - 1] == ';' && is_iso_version_suffix(segment + i - 1)) {
			return i - 1;
		}
	}

	return segment_len;
}

// Normalizes the cache root before appending ISO-relative path segments.
static void trim_trailing_separators(sds path) {
	while (sdslen(path) > 0 && DTTR_Path_IsSeparator(path[sdslen(path) - 1])) {
		sdsrange(path, 0, -2);
	}
}

// Appends one lower-case cache path segment without an ISO version suffix.
static bool sdscat_lower_segment(sds *out, const char *segment, size_t segment_len) {
	segment_len = strip_iso_version_suffix_len(segment, segment_len);

	for (size_t i = 0; i < segment_len; i++) {
		const char ch = DTTR_Path_AsciiLower(segment[i]);

		if (!DTTR_Path_AppendChar(out, ch)) {
			return false;
		}
	}

	return true;
}

// Converts an ISO-relative file path into its deterministic cache path.
bool DTTR_ISO_CachePathForFile(
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

	trim_trailing_separators(path);

	const char *p = DTTR_Path_SkipSeparators(iso_relative_path);

	bool wrote_segment = false;
	bool ok = true;

	while (*p) {
		const char *segment = p;
		size_t segment_len = DTTR_Path_SegmentLen(p);

		if (DTTR_Path_IsRelativeSegment(segment, segment_len)) {
			ok = false;
			break;
		}

		if (!DTTR_Path_AppendSeparator(&path, '\\')
			|| !sdscat_lower_segment(&path, segment, segment_len)) {
			ok = false;
			break;
		}

		wrote_segment = true;

		p += segment_len;

		if (DTTR_Path_IsSeparator(*p)) {
			p = DTTR_Path_SkipSeparators(p);

			if (!*p) {
				ok = false;
				break;
			}
		}
	}

	ok = ok && wrote_segment && DTTR_Path_CopySds(out_path, out_path_size, path);
	sdsfree(path);
	return ok;
}

// Starts PhysicsFS once for ISO access and lets nested callers share the session.
static bool physfs_init() {
	if (physfs_refcount > 0) {
		physfs_refcount++;
		return true;
	}

	if (!PHYSFS_init("dttr")) {
		set_physfs_error("PHYSFS_init failed");
		return false;
	}

	physfs_refcount = 1;

	return true;
}

// Drops one ISO PhysicsFS reference and shuts down after the last image closes.
static void physfs_deinit() {
	if (physfs_refcount <= 0) {
		return;
	}

	physfs_refcount--;

	if (physfs_refcount == 0) {
		PHYSFS_deinit();
	}
}

// Mounts an ISO image through PhysicsFS for case-insensitive extraction.
bool DTTR_ISO_Open(DTTR_IsoImage *iso, const char *iso_path) {
	if (!iso || !iso_path || !iso_path[0] || strlen(iso_path) >= sizeof(iso->iso_path)) {
		set_error("invalid ISO path");
		return false;
	}

	memset(iso, 0, sizeof(*iso));

	if (!physfs_init()) {
		return false;
	}

	if (!PHYSFS_mount(iso_path, NULL, 1)) {
		set_physfs_error("PHYSFS_mount failed");
		physfs_deinit();
		return false;
	}

	DTTR_Path_CopyString(iso->iso_path, sizeof(iso->iso_path), iso_path);
	iso->open = true;

	return true;
}

// Matches a requested path segment against an ISO directory entry.
static bool name_matches_segment(
	const char *name,
	const char *segment,
	size_t segment_len
) {
	const size_t name_len = strlen(name);
	return (name_len == segment_len
			|| (name_len > segment_len && is_iso_version_suffix(name + segment_len)))
		   && DTTR_Path_AsciiIeqN(name, segment, segment_len);
}

// Enumerates an ISO directory to recover stored casing for one segment.
static bool find_case_match(
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
		if (!name_matches_segment(*entry, segment, segment_len)) {
			continue;
		}

		if (!DTTR_Path_CopyString(out_name, out_name_size, *entry)) {
			continue;
		}

		found = true;
		break;
	}

	PHYSFS_freeList(entries);
	return found;
}

// Resolves ISO-relative paths to exact PhysicsFS entry names.
static bool resolve_iso_path_case(
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

	const char *p = DTTR_Path_SkipSeparators(requested);

	bool wrote_segment = false;
	bool ok = true;

	while (*p) {
		const char *segment = p;
		size_t segment_len = DTTR_Path_SegmentLen(p);

		if (DTTR_Path_IsRelativeSegment(segment, segment_len)) {
			ok = false;
			break;
		}

		char match[DTTR_ISO_MAX_PATH];

		if (!find_case_match(path, segment, segment_len, match, sizeof(match))) {
			ok = false;
			break;
		}

		if (!DTTR_Path_AppendSegment(&path, match, '/')) {
			ok = false;
			break;
		}

		wrote_segment = true;

		p += segment_len;
		p = DTTR_Path_SkipSeparators(p);
	}

	ok = ok && wrote_segment && DTTR_Path_CopySds(out_path, out_path_size, path);
	sdsfree(path);
	return ok;
}

// Creates the cache directory chain for an extracted ISO file.
static bool create_parent_dirs(const char *path) {
	char tmp[DTTR_ISO_MAX_PATH];

	const size_t len = strlen(path);

	if (len >= sizeof(tmp)) {
		return false;
	}

	memcpy(tmp, path, len + 1);

	for (size_t i = 1; tmp[i]; i++) {
		if (!DTTR_Path_IsSeparator(tmp[i])) {
			continue;
		}

		if (i == 2 && tmp[1] == ':') {
			continue;
		}

		char saved = tmp[i];
		tmp[i] = '\0';

		if (tmp[0]) {
			MKDIR(tmp);
		}

		tmp[i] = saved;
	}

	return true;
}

// Reuse cached files only when size and bytes match the ISO.
static bool file_matches_physfs_file(
	const char *path,
	PHYSFS_File *physfs_file,
	size_t size
) {
	FILE *file = fopen(path, "rb");

	if (!file) {
		return false;
	}

	bool matches = false;
	if (fseek(file, 0, SEEK_END) != 0) {
		goto done;
	}

	const long existing = ftell(file);
	if (existing < 0 || (size_t)existing != size || fseek(file, 0, SEEK_SET) != 0) {
		goto done;
	}

	if (!PHYSFS_seek(physfs_file, 0)) {
		goto done;
	}

	char disk_buffer[ISO_SECTOR_SIZE];
	char iso_buffer[ISO_SECTOR_SIZE];
	size_t remaining = size;
	matches = true;
	while (remaining > 0) {
		const size_t chunk = remaining < sizeof(disk_buffer) ? remaining
															 : sizeof(disk_buffer);
		if (fread(disk_buffer, 1, chunk, file) != chunk
			|| PHYSFS_readBytes(physfs_file, iso_buffer, chunk) != (PHYSFS_sint64)chunk
			|| memcmp(disk_buffer, iso_buffer, chunk) != 0) {
			matches = false;
			break;
		}

		remaining -= chunk;
	}

done:
	fclose(file);
	if (!PHYSFS_seek(physfs_file, 0)) {
		set_physfs_error("PHYSFS_seek failed");
		return false;
	}

	return matches;
}

// Extends a resolved ISO directory path with one child entry.
static sds child_iso_path(const char *parent, const char *entry) {
	sds child = sdsnew(parent);

	if (!child) {
		return NULL;
	}

	if (!DTTR_Path_AppendSegment(&child, entry, '/')) {
		sdsfree(child);
		return NULL;
	}

	return child;
}

// Copies one resolved ISO file into the launcher cache.
bool DTTR_ISO_ExtractFile(
	DTTR_IsoImage *iso,
	const char *iso_relative_path,
	const char *cache_root,
	char *out_path,
	size_t out_path_size
) {
	if (!iso || !iso->open) {
		set_error("ISO is not open");
		return false;
	}

	if (!DTTR_ISO_CachePathForFile(
			cache_root,
			iso_relative_path,
			out_path,
			out_path_size
		)) {
		set_error("invalid ISO cache path");
		return false;
	}

	char physfs_path[DTTR_ISO_MAX_PATH];

	if (!resolve_iso_path_case(iso_relative_path, physfs_path, sizeof(physfs_path))) {
		set_not_found_error("file not found in ISO");
		return false;
	}

	PHYSFS_File *in = PHYSFS_openRead(physfs_path);

	if (!in) {
		set_physfs_error("PHYSFS_openRead failed");
		return false;
	}

	const PHYSFS_sint64 length = PHYSFS_fileLength(in);

	if (length < 0) {
		set_physfs_error("PHYSFS_fileLength failed");
		PHYSFS_close(in);
		return false;
	}

	if ((uint64_t)length > (uint64_t)SIZE_MAX) {
		set_error("ISO file is too large to cache");
		PHYSFS_close(in);
		return false;
	}

	if (file_matches_physfs_file(out_path, in, (size_t)length)) {
		PHYSFS_close(in);
		return true;
	}

	if (!PHYSFS_seek(in, 0)) {
		set_physfs_error("PHYSFS_seek failed");
		PHYSFS_close(in);
		return false;
	}

	if (!create_parent_dirs(out_path)) {
		set_error("could not create cache directories");
		PHYSFS_close(in);
		return false;
	}

	FILE *out = fopen(out_path, "wb");

	if (!out) {
		set_error("could not open cache output file");
		PHYSFS_close(in);
		return false;
	}

	char buffer[ISO_SECTOR_SIZE];
	PHYSFS_sint64 remaining = length;

	while (remaining > 0) {
		const PHYSFS_uint64 chunk = remaining < (PHYSFS_sint64)sizeof(buffer)
										? (PHYSFS_uint64)remaining
										: (PHYSFS_uint64)sizeof(buffer);
		const PHYSFS_sint64 got = PHYSFS_readBytes(in, buffer, chunk);

		if (got != (PHYSFS_sint64)chunk
			|| fwrite(buffer, 1, (size_t)chunk, out) != chunk) {
			set_error("could not extract ISO file");
			fclose(out);
			PHYSFS_close(in);
			return false;
		}

		remaining -= got;
	}

	const bool output_closed = fclose(out) == 0;
	const bool input_closed = PHYSFS_close(in) != 0;
	if (!output_closed || !input_closed) {
		set_error("could not finish ISO extraction");
		return false;
	}

	return true;
}

// Walks a resolved ISO directory and extracts every regular file into the cache tree.
static bool extract_tree_path(
	DTTR_IsoImage *iso,
	const char *physfs_path,
	const char *cache_root
) {
	char **entries = PHYSFS_enumerateFiles(physfs_path);

	if (!entries) {
		set_physfs_error("PHYSFS_enumerateFiles failed");
		return false;
	}

	bool ok = true;

	for (char **entry = entries; ok && *entry; entry++) {
		sds child = child_iso_path(physfs_path, *entry);

		if (!child) {
			set_error("could not build ISO tree path");
			ok = false;
			break;
		}

		PHYSFS_Stat stat;

		if (!PHYSFS_stat(child, &stat)) {
			set_physfs_error("PHYSFS_stat failed");
			ok = false;
		} else if (stat.filetype == PHYSFS_FILETYPE_DIRECTORY) {
			ok = extract_tree_path(iso, child, cache_root);
		} else if (stat.filetype == PHYSFS_FILETYPE_REGULAR) {
			char out_path[DTTR_ISO_MAX_PATH];
			ok = DTTR_ISO_ExtractFile(iso, child, cache_root, out_path, sizeof(out_path));
		}

		sdsfree(child);
	}

	PHYSFS_freeList(entries);
	return ok;
}

// Resolves and extracts an ISO directory tree used by the launcher cache.
bool DTTR_ISO_ExtractTree(
	DTTR_IsoImage *iso,
	const char *iso_relative_path,
	const char *cache_root
) {
	if (!iso || !iso->open) {
		set_error("ISO is not open");
		return false;
	}

	char physfs_path[DTTR_ISO_MAX_PATH];

	if (!resolve_iso_path_case(iso_relative_path, physfs_path, sizeof(physfs_path))) {
		set_not_found_error("directory not found in ISO");
		return false;
	}

	PHYSFS_Stat stat;

	if (!PHYSFS_stat(physfs_path, &stat)) {
		set_physfs_error("PHYSFS_stat failed");
		return false;
	}

	if (stat.filetype != PHYSFS_FILETYPE_DIRECTORY) {
		set_error("ISO path is not a directory");
		return false;
	}

	return extract_tree_path(iso, physfs_path, cache_root);
}

// Unmounts the ISO image and releases the shared PhysicsFS reference owned by it.
void DTTR_ISO_Close(DTTR_IsoImage *iso) {
	if (!iso || !iso->open) {
		return;
	}

	PHYSFS_unmount(iso->iso_path);
	physfs_deinit();
	memset(iso, 0, sizeof(*iso));
}
