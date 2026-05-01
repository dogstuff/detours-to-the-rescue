#ifndef DTTR_LOADER_H
#define DTTR_LOADER_H

#include <stdbool.h>
#include <windows.h>

typedef struct {
	bool m_is_iso;
	char m_cache_root[MAX_PATH];
	char m_game_root[MAX_PATH];
} DTTR_LoaderIsoContext;

extern const char *g_dttr_config_path;

void dttr_compat_create_process(
	const WCHAR *image_name,
	const char *shim_data,
	size_t shim_data_len,
	PROCESS_INFORMATION *child_info
);
void dttr_loader_inject_sidecar(const PROCESS_INFORMATION *child_info);
bool dttr_loader_resolve_exe_path(
	WCHAR *out,
	const char *configured_path,
	DTTR_LoaderIsoContext *iso_context
);
void dttr_loader_watchdog_attach(const PROCESS_INFORMATION *child_info);
void dttr_loader_watchdog_wait(const PROCESS_INFORMATION *child_info);

#endif /* DTTR_LOADER_H */
