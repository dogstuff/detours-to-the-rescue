#ifndef DTTR_LOADER_H
#define DTTR_LOADER_H

#include <sds.h>
#include <windows.h>

void dttr_compat_create_process(const WCHAR *image_name, sds shim_data, PROCESS_INFORMATION *child_info);
void dttr_loader_inject_sidecar(PROCESS_INFORMATION *child_info);
sds dttr_compat_build_shim_data(const WCHAR *image_name);

#endif /* DTTR_LOADER_H */
