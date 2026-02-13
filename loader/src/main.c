#include <dttr_crashdump.h>
#include <dttr_loader.h>
#include <log.h>
#include <sds.h>
#include <stdio.h>
#include <windows.h>

static const char *LOG_FILE_NAME = "dttr_loader.log";

int main(void) {
	dttr_crashdump_init("dttr_loader");

	FILE *const log_file = fopen(LOG_FILE_NAME, "a+");
	if (log_file) {
		log_add_fp(log_file, LOG_TRACE);
	}
	log_set_level(LOG_TRACE);

	log_info("Starting DttR loader");

	sds shim_data = dttr_compat_build_shim_data(L"pcdogs.exe");

	PROCESS_INFORMATION child_info = {0};
	dttr_compat_create_process(L"pcdogs.exe", shim_data, &child_info);

	sdsfree(shim_data);

	dttr_loader_inject_sidecar(&child_info);

	CloseHandle(child_info.hThread);
	CloseHandle(child_info.hProcess);

	log_info("Loader finished");

	if (log_file) {
		fclose(log_file);
	}

	return 0;
}
