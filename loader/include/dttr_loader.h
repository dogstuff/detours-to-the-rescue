#ifndef DTTR_LOADER_H
#define DTTR_LOADER_H

#include <stdbool.h>
#include <windows.h>

/// Stores the path to the target configuration file.
extern const char *g_dttr_config_path;

void dttr_compat_create_process(
	const WCHAR *image_name,
	const char *shim_data,
	size_t shim_data_len,
	PROCESS_INFORMATION *child_info
);
void dttr_loader_inject_sidecar(
	const PROCESS_INFORMATION *child_info,
	const char *exe_path
);
bool dttr_loader_resolve_exe_path(WCHAR *out, const char *configured_dir);
void dttr_loader_watchdog_attach(const PROCESS_INFORMATION *child_info);
void dttr_loader_watchdog_wait(const PROCESS_INFORMATION *child_info);

#endif /* DTTR_LOADER_H */
