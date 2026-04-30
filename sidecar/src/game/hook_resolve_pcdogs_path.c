#include "dttr_hooks_game.h"
#include "dttr_interop_pcdogs.h"
#include "dttr_sidecar.h"

#include <dttr_log.h>
#include <windows.h>

uint32_t __cdecl dttr_hook_resolve_pcdogs_path_callback(void) {
	char *out_path = g_pcdogs_directory_ptr();

	DWORD module_path_length = GetModuleFileNameA(
		dttr_game_api_get_ctx()->m_game_module,
		out_path,
		0x104
	);

	if (module_path_length == 0) {
		DTTR_LOG_ERROR(
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
