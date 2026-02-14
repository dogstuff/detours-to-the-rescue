#include "dttr_hooks.h"
#include "log.h"
#include "sds.h"

#include <sys/stat.h>
#include <windows.h>

void *__cdecl dttr_crt_hook_open_file_callback(const char *path, char *mode) {
	void *result = dttr_crt_open_file_with_mode(path, mode, 0x40);
	if (result)
		return result;

	// The game handles missing files and read-only failures correctly.
	const bool is_read_only = mode && mode[0] == 'r' && !strchr(mode, '+');

	if (is_read_only || errno == 0 || errno == ENOENT) {
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

	const int choice = MessageBoxA(NULL, prompt, "DttR: File Permission Error Jumpscare", MB_YESNO | MB_ICONWARNING);
	sdsfree(prompt);

	if (choice == IDYES) {
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
	MessageBoxA(NULL, msg, "DttR: File Error", MB_OK | MB_ICONERROR);
	sdsfree(msg);

	return dttr_crt_open_file_with_mode("NUL", mode, 0x40);
}
