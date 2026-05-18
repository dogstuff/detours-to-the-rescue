#include "dttr_sidecar.h"
#include "hooks_private.h"
#include "sidecar_private.h"
#include <dttr_pcdogs.h>

#include <dttr_log.h>
#include <windows.h>

// Writes the game directory expected by PCDogs from the loaded module path.
uint32_t __cdecl dttr_hook_resolve_pcdogs_path_callback() {
	char *out_path = DTTR_PCDOGS_D_Directory->Ptr();

	DWORD module_path_length = GetModuleFileNameA(
		dttr_sidecar_runtime_context()->game_module,
		out_path,
		MAX_PATH
	);

	if (module_path_length == 0) {
		DTTR_LOG_ERROR(
			"GetModuleFileNameA failed (error %lu), falling back to current directory",
			GetLastError()
		);
		return GetCurrentDirectoryA(MAX_PATH, out_path);
	}

	while (module_path_length > 0 && out_path[module_path_length - 1] != '\\') {
		module_path_length--;
	}

	out_path[module_path_length] = '\0';

	return module_path_length;
}
