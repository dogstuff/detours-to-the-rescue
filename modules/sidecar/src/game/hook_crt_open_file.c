#include "hooks_private.h"
#include "sidecar_private.h"
#include <dttr_config.h>
#include <dttr_log.h>
#include <dttr_path.h>
#include <dttr_pcdogs.h>
#include <dttr_sdl.h>

#include <SDL3/SDL.h>
#include <sds.h>
#include <sys/stat.h>
#include <windows.h>

#define IS_READ_ONLY_MODE(m) ((m) && (m)[0] == 'r' && !strchr((m), '+'))

// Calls the game's CRT wrapper with the sharing flag expected by original file access.
static DTTR_PCDOGS_T_File_Handle *file_open_with_mode(
	const char *path,
	const char *mode,
	uint8_t sharing_flag
) {
	DTTR_PCDOGS_T_File_Handle *handle = NULL;
	DTTR_Result result = DTTR_PCDOGS_F_File_OpenWithMode->Call(
		dttr_sidecar_runtime_context(),
		path,
		mode,
		sharing_flag,
		&handle
	);
	if (!DTTR_ResultOK(result)) {
		DTTR_LOG_ERROR(
			"File_OpenWithMode failed for \"%s\" (mode \"%s\"): %s",
			path,
			mode,
			result.message ? result.message : DTTR_StatusName(result.status)
		);
		return NULL;
	}

	return handle;
}

// Accepts only non-empty relative paths for save redirection and game-data lookup.
static bool is_relative_path(const char *path) {
	return DTTR_Path_IsSafeRelative(path);
}

// Detects write modes so permission repair prompts for the correct file bits.
static bool mode_wants_write(const char *mode) {
	return mode && (strchr(mode, 'w') || strchr(mode, 'a') || strchr(mode, '+'));
}

static bool redirect_saves_initialized = false;

// Creates save directories only after save redirection resolves a path.
static void create_dir_if_set(const char *path) {
	if (path && path[0]) {
		CreateDirectoryA(path, NULL);
	}
}

// Resolves the root save directory used to move game writes out of the install tree.
static void build_saves_dir(char *buf, size_t buf_size) {
	sds dir = NULL;
	if (is_relative_path(dttr_config.saves_path)) {
		dir = sdsnew(dttr_loader_dir);
		if (!dir || !DTTR_Path_AppendSegment(&dir, dttr_config.saves_path, '\\')) {
			sdsfree(dir);
			buf[0] = '\0';
			return;
		}
	} else {
		dir = sdsnew(dttr_config.saves_path);
	}

	if (!DTTR_Path_CopySds(buf, buf_size, dir)) {
		buf[0] = '\0';
	}

	sdsfree(dir);
}

// Maps slot-specific saves into the redirected save root.
static void build_save_slot_dir(char *buf, size_t buf_size) {
	build_saves_dir(buf, buf_size);

	sds dir = sdsnew(buf);
	if (!dir || !DTTR_Path_AppendSegment(&dir, dttr_exe_hash, '\\')
		|| !DTTR_Path_CopySds(buf, buf_size, dir)) {
		buf[0] = '\0';
	}

	sdsfree(dir);
}

// Creates redirected save folders before the CRT hook returns a writable path.
static void ensure_save_dir() {
	if (redirect_saves_initialized) {
		return;
	}

	redirect_saves_initialized = true;

	char dir[MAX_PATH];
	build_saves_dir(dir, sizeof(dir));
	create_dir_if_set(dir);

	build_save_slot_dir(dir, sizeof(dir));
	create_dir_if_set(dir);
}

// Redirects relative save writes into the configured per-executable save directory.
static const char *redirect_path(
	const char *path,
	char *buf,
	size_t buf_size,
	const char *mode
) {
	if (!dttr_config.saves_path[0]) {
		return path;
	}

	if (!is_relative_path(path)) {
		return path;
	}

	ensure_save_dir();

	build_save_slot_dir(buf, buf_size);
	sds redirected = sdsnew(buf);
	if (!redirected || !DTTR_Path_AppendSegment(&redirected, path, '\\')
		|| !DTTR_Path_CopySds(buf, buf_size, redirected)) {
		sdsfree(redirected);
		return path;
	}

	sdsfree(redirected);

	if (IS_READ_ONLY_MODE(mode) && !DTTR_Path_ExactExists(buf)) {
		return path;
	}

	DTTR_LOG_DEBUG("Redirecting \"%s\" -> \"%s\"", path, buf);
	return buf;
}

// Reports a failed write/open path without pretending the CRT call succeeded.
static DTTR_PCDOGS_T_File_Handle *report_file_open_failure(
	const char *path,
	const char *mode
) {
	sds msg = sdscatprintf(
		sdsempty(),
		"Failed to open \"%s\" (mode \"%s\"). This file will not be written.\n\n%s",
		path,
		mode,
		strerror(errno)
	);
	DTTR_SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "DttR: File Error", msg, NULL);
	sdsfree(msg);

	return NULL;
}

// Offers to repair file permissions when Wine or the host blocks a requested write.
static DTTR_PCDOGS_T_File_Handle *try_fix_permissions(const char *path, const char *mode) {
	const bool wants_write = mode_wants_write(mode);
	const int perms = ((mode && strchr(mode, 'r')) ? _S_IREAD : 0)
					  | (wants_write ? _S_IWRITE : 0);

	DTTR_LOG_ERROR(
		"Permission error opening \"%s\" (mode \"%s\"): %s",
		path,
		mode,
		strerror(errno)
	);

	sds prompt = sdscatprintf(
		sdsempty(),
		"Failed to open file \"%s\" (mode \"%s\"): %s\n\n"
		"This is typically the result of a permissions issue, especially if you're "
		"using "
		"Wine.\n\n"
		"Try granting permissions 0o%03o?",
		path,
		mode,
		strerror(errno),
		perms
	);

	const SDL_MessageBoxButtonData buttons[] = {
		{SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 0, "No"},
		{SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 1, "Yes"},
	};

	const SDL_MessageBoxData msgbox = {
		.flags = SDL_MESSAGEBOX_WARNING,
		.window = NULL,
		.title = "DttR: File Permission Error",
		.message = prompt,
		.numbuttons = 2,
		.buttons = buttons,
	};

	int button_id = 0;
	DTTR_SDL_ShowMessageBox(&msgbox, &button_id);
	sdsfree(prompt);

	if (button_id != 1) {
		return NULL;
	}

	DTTR_LOG_DEBUG("chmod \"%s\" 0o%03o", path, perms);
	chmod(path, perms);

	DTTR_PCDOGS_T_File_Handle *result = file_open_with_mode(path, mode, 0x40);
	if (result) {
		return result;
	}

	DTTR_LOG_ERROR(
		"chmod didn't resolve permission error for \"%s\": %s",
		path,
		strerror(errno)
	);
	return NULL;
}

// Resolves case-insensitive and ISO-backed read paths before the game sees a miss.
static DTTR_PCDOGS_T_File_Handle *try_open_read_path(const char *path, const char *mode) {
	char resolved[MAX_PATH];

	if (dttr_game_data_resolve_existing_read_path(path, resolved, sizeof(resolved))) {
		DTTR_LOG_DEBUG("Resolved case-insensitive read \"%s\" -> \"%s\"", path, resolved);
		return file_open_with_mode(resolved, mode, 0x40);
	}

	char cached[MAX_PATH];
	if (!dttr_game_data_resolve_read_path(path, cached, sizeof(cached))) {
		return NULL;
	}

	DTTR_LOG_DEBUG("Resolved ISO-backed read \"%s\" -> \"%s\"", path, cached);
	return file_open_with_mode(cached, mode, 0x40);
}

// Replaces the game file-open callback with save redirection plus data-file fallback.
DTTR_PCDOGS_T_File_Handle *__cdecl dttr_crt_hook_open_file_callback(
	const char *path,
	const char *mode
) {
	char redirected[MAX_PATH];
	path = redirect_path(path, redirected, sizeof(redirected), mode);

	DTTR_PCDOGS_T_File_Handle *result = file_open_with_mode(path, mode, 0x40);
	if (result) {
		return result;
	}

	if (IS_READ_ONLY_MODE(mode) || errno == 0 || errno == ENOENT) {
		if (IS_READ_ONLY_MODE(mode)) {
			result = try_open_read_path(path, mode);
			if (result) {
				return result;
			}
		}

		DTTR_LOG_ERROR("File \"%s\" does not exist; passing to game.", path);
		return result;
	}

	const bool wants_write = mode_wants_write(mode);
	const bool is_perm_error
		= (errno == EACCES || errno == EPERM || (errno == EBADF && wants_write));

	if (is_perm_error) {
		result = try_fix_permissions(path, mode);
		if (result) {
			return result;
		}
	} else {
		DTTR_LOG_ERROR(
			"Failed to open \"%s\" (mode \"%s\"): %s",
			path,
			mode,
			strerror(errno)
		);
	}

	return report_file_open_failure(path, mode);
}
