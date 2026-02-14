#ifndef DTTR_LOADER_H
#define DTTR_LOADER_H

#include <sds.h>
#include <windows.h>

void dttr_compat_create_process(
	const WCHAR *image_name, const char *shim_data, size_t shim_data_len, PROCESS_INFORMATION *child_info
);
void dttr_loader_inject_sidecar(const PROCESS_INFORMATION *child_info);
sds dttr_compat_build_shim_data(const WCHAR *image_name);
void dttr_loader_watchdog_attach(const PROCESS_INFORMATION *child_info);
void dttr_loader_watchdog_wait(const PROCESS_INFORMATION *child_info);

#endif /* DTTR_LOADER_H */
