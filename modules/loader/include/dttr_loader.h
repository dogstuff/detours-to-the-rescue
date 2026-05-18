#ifndef DTTR_LOADER_H
#define DTTR_LOADER_H

#include <stdbool.h>
#include <windows.h>

typedef struct {
	bool is_iso;
	char cache_root[MAX_PATH];
	char game_root[MAX_PATH];
} DTTR_LoaderIsoContext;

extern const char *dttr_config_path;

void DTTR_Compat_CreateProcess(
	const WCHAR *image_name,
	const char *shim_data,
	size_t shim_data_len,
	PROCESS_INFORMATION *child_info
);
bool DTTR_Loader_InjectSidecar(const PROCESS_INFORMATION *child_info);
bool DTTR_Loader_ResolveEXEPath(
	WCHAR *out,
	const char *configured_path,
	DTTR_LoaderIsoContext *iso_context
);
void DTTR_Loader_WatchdogAttach(const PROCESS_INFORMATION *child_info);
void DTTR_Loader_WatchdogDetach(const PROCESS_INFORMATION *child_info);
bool DTTR_Loader_WatchdogWait(const PROCESS_INFORMATION *child_info);

#endif // DTTR_LOADER_H
