#include "dttr_hooks.h"
#include "log.h"
#include "sds.h"

#include <sys/stat.h>
#include <windows.h>

void *__cdecl dttr_crt_hook_open_file_callback(const char *path, char *mode) {
	void *result = dttr_crt_open_file_with_mode(path, mode, 0x40);

	// Writing to a read-only file on Windows gives EBADF instead of EACCES.
	const bool is_likely_perm_error
		= (errno == EACCES || errno == EPERM || (errno == EBADF && mode != NULL && strchr(mode, 'w')));

	if (result || errno == ENOENT || (errno == EBADF && !is_likely_perm_error))
		return result;

	if (is_likely_perm_error) {
		log_error("Permission error opening \"%s\" (mode \"%s\"): %s", path, mode, strerror(errno));

		const int minimum_needed_perms = (strchr(mode, 'r') ? _S_IREAD : 0) | (strchr(mode, 'w') ? _S_IWRITE : 0);

		sds prompt = sdscatprintf(
			sdsempty(),
			"Failed to open file \"%s\" (mode \"%s\"): %s\n\n"
			"This is typically the result of a permissions issue, especially if you're using Wine.\n\n"
			"Try granting permissions 0o%03o?",
			path,
			mode,
			strerror(errno),
			minimum_needed_perms
		);

		const int choice
			= MessageBoxA(NULL, prompt, "DttR: File Permission Error Jumpscare", MB_YESNO | MB_ICONWARNING);
		sdsfree(prompt);

		if (choice == IDYES) {
			log_info("chmod \"%s\" 0o%03o", path, minimum_needed_perms);
			chmod(path, minimum_needed_perms);
			result = dttr_crt_open_file_with_mode(path, mode, 0x40);
			if (result)
				return result;
			log_error("chmod didn't resolve permission error for \"%s\": %s", path, strerror(errno));
		}
	}

	log_error("Gracefully handling failed _openfile for \"%s\" (mode \"%s\"): %s", path, mode, strerror(errno));

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
