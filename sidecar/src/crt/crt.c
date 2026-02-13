#include "dttr_hooks.h"
#include "log.h"
#include "sds.h"

#include <windows.h>

void *__cdecl dttr_crt_hook_open_file_callback(const char *path, char *mode) {
	void *result = dttr_crt_open_file_with_mode(path, mode, 0x40);

	if (result)
		return result;

	log_error(
		"Gracefully handling failed _openfile for \"%s\" (mode \"%s\"). "
		"This is typically the result of a permissions issue.",
		path,
		mode
	);

	sds msg = sdscatprintf(
		sdsempty(),
		"Failed to open file \"%s\" (mode \"%s\").\n\n"
		"This file will not be written. "
		"This is typically the result of a permissions issue, especially if you're using Wine.",
		path,
		mode
	);
	MessageBoxA(NULL, msg, "DTTR: File Error", MB_OK | MB_ICONERROR);
	sdsfree(msg);

	return dttr_crt_open_file_with_mode("NUL", mode, 0x40);
}

void dttr_crt_hook_init(HMODULE mod) {
	dttr_crt_open_file_with_mode_init(mod);
	DTTR_INTEROP_HOOK_FUNC_LOG(dttr_crt_hook_open_file, mod);
}

void dttr_crt_hook_cleanup(void) { DTTR_INTEROP_UNHOOK_LOG(dttr_crt_hook_open_file); }
