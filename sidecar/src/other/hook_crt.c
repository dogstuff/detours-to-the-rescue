#include "dttr_hooks.h"
#include "dttr_sidecar.h"
#include "log.h"
#include "sds.h"

#include <SDL3/SDL.h>
#include <sys/stat.h>
#include <windows.h>

#define IS_READ_ONLY_MODE(m) ((m) && (m)[0] == 'r' && !strchr((m), '+'))

static bool s_is_relative_path(const char *path) {
	if (!path || !path[0])
		return false;

	// A path is absolute if it contains a drive letter like "C:\".
	return path[1] != ':' || path[2] != '\\';
}

static bool s_redirect_saves_initialized = false;

static void s_build_saves_dir(char *buf, size_t buf_size) {
	if (s_is_relative_path(g_dttr_config.m_saves_path)) {
		snprintf(buf, buf_size, "%s%s", g_dttr_loader_dir, g_dttr_config.m_saves_path);
		return;
	}

	snprintf(buf, buf_size, "%s", g_dttr_config.m_saves_path);
}

static void s_ensure_save_dir(void) {
	if (s_redirect_saves_initialized)
		return;

	s_redirect_saves_initialized = true;

	char dir[MAX_PATH];

	s_build_saves_dir(dir, sizeof(dir));
	CreateDirectoryA(dir, NULL);

	size_t len = strlen(dir);
	snprintf(dir + len, sizeof(dir) - len, "\\%s", g_dttr_exe_hash);
	CreateDirectoryA(dir, NULL);
}

static const char *s_redirect_path(const char *path, char *buf, size_t buf_size, const char *mode) {
	if (!g_dttr_config.m_saves_path[0])
		return path;

	if (!s_is_relative_path(path))
		return path;

	s_ensure_save_dir();

	char saves_dir[MAX_PATH];
	s_build_saves_dir(saves_dir, sizeof(saves_dir));
	snprintf(buf, buf_size, "%s\\%s\\%s", saves_dir, g_dttr_exe_hash, path);

	if (IS_READ_ONLY_MODE(mode) && GetFileAttributesA(buf) == INVALID_FILE_ATTRIBUTES)
		return path;

	log_debug("Redirecting \"%s\" -> \"%s\"", path, buf);
	return buf;
}

void *__cdecl dttr_crt_hook_open_file_callback(const char *path, char *mode) {
	char redirected[MAX_PATH];
	path = s_redirect_path(path, redirected, sizeof(redirected), mode);

	void *result = dttr_crt_open_file_with_mode(path, mode, 0x40);
	if (result)
		return result;

	// The game handles missing files and read-only failures correctly.
	if (IS_READ_ONLY_MODE(mode) || errno == 0 || errno == ENOENT) {
		log_error("File \"%s\" does not exist; passing to game.", path);
		return result;
	}

	// Windows gives EBADF instead of EACCES when writing to a read-only file.
	const bool wants_write = (mode && strchr(mode, 'w'));
	const bool is_perm_error = (errno == EACCES || errno == EPERM || (errno == EBADF && wants_write));

	if (!is_perm_error) {
		log_error("Failed to open \"%s\" (mode \"%s\"): %s", path, mode, strerror(errno));
		goto fallback;
	}

	log_error("Permission error opening \"%s\" (mode \"%s\"): %s", path, mode, strerror(errno));

	const int perms = (strchr(mode, 'r') ? _S_IREAD : 0) | (wants_write ? _S_IWRITE : 0);

	sds prompt = sdscatprintf(
		sdsempty(),
		"Failed to open file \"%s\" (mode \"%s\"): %s\n\n"
		"This is typically the result of a permissions issue, especially if you're using Wine.\n\n"
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
		.title = "DttR: File Permission Error Jumpscare",
		.message = prompt,
		.numbuttons = 2,
		.buttons = buttons,
	};

	int button_id = 0;
	SDL_ShowMessageBox(&msgbox, &button_id);
	sdsfree(prompt);

	if (button_id == 1) {
		log_debug("chmod \"%s\" 0o%03o", path, perms);
		chmod(path, perms);
		result = dttr_crt_open_file_with_mode(path, mode, 0x40);
		if (result)
			return result;
		log_error("chmod didn't resolve permission error for \"%s\": %s", path, strerror(errno));
	}

fallback:;
	sds msg = sdscatprintf(
		sdsempty(),
		"Failed to open \"%s\" (mode \"%s\"). This file will not be written.\n\n%s",
		path,
		mode,
		strerror(errno)
	);
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "DttR: File Error", msg, NULL);
	sdsfree(msg);

	return dttr_crt_open_file_with_mode("NUL", mode, 0x40);
}
