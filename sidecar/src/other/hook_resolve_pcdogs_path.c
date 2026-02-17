#include "dttr_hooks_other.h"
#include "dttr_interop_pcdogs.h"
#include "log.h"

#include <windows.h>

#include "dttr_sidecar.h"

uint32_t __cdecl dttr_hook_resolve_pcdogs_path_callback(void) {
	char *out_path = g_pcdogs_directory_ptr();

	DWORD module_path_length = GetModuleFileNameA(g_dttr_pc_dogs_module, out_path, 0x104);

	if (module_path_length == 0) {
		log_error(
			"GetModuleFileNameA failed (error %lu), falling back to current directory",
			GetLastError()
		);
		return GetCurrentDirectoryA(0x104, out_path);
	}

	while (module_path_length > 0 && out_path[module_path_length - 1] != '\\') {
		module_path_length--;
	}

	out_path[module_path_length] = '\0';

	return module_path_length;
}
