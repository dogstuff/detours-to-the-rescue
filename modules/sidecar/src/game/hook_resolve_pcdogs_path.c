#include "context_private.h"
#include "hooks_private.h"
#include <dttr_pcdogs.h>

#include <dttr_log.h>
#include <windows.h>

// Writes the package base path expected by PCDogs from the loaded module path.
uint32_t __cdecl dttr_hook_resolve_pcdogs_path_callback() {
	char (*path_buffer)[DTTR_PCDOGS_D_AUDIO_OPEN_STREAM_PKG_BASE_PATH_COUNT]
		= DTTR_PCDOGS_D_Audio_OpenStream_PKGBasePath->Ptr();
	if (!path_buffer) {
		DTTR_LOG_ERROR("PCDOGS package base path was unavailable");
		return 0;
	}

	char *out_path = *path_buffer;

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
