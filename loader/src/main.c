#include <dttr_config.h>
#include <dttr_crashdump.h>
#include <dttr_loader.h>
#include <log.h>
#include <sds.h>
#include <stdio.h>
#include <windows.h>

static const char *LOG_FILE_NAME = "dttr.log";

int main(void) {
	dttr_crashdump_init("dttr_loader");
	dttr_config_load(DTTR_CONFIG_FILENAME);

	const int level = g_dttr_config.m_log_level;
	log_set_level(level);

	FILE *const log_file = fopen(LOG_FILE_NAME, "a+");
	if (log_file) {
		log_add_fp(log_file, level);
	}

	log_info("Starting DttR loader (log level: %s)", log_level_string(level));

	WCHAR exe_path[MAX_PATH];
	dttr_loader_resolve_exe_path(exe_path, g_dttr_config.m_pcdogs_path);

	char exe_path_narrow[MAX_PATH];
	WideCharToMultiByte(CP_UTF8, 0, exe_path, -1, exe_path_narrow, MAX_PATH, NULL, NULL);

	sds shim_data = dttr_compat_build_shim_data(exe_path);

	PROCESS_INFORMATION child_info = {0};
	dttr_compat_create_process(exe_path, shim_data, sdslen(shim_data), &child_info);

	sdsfree(shim_data);

	dttr_loader_watchdog_attach(&child_info);
	dttr_loader_inject_sidecar(&child_info, exe_path_narrow);
	dttr_loader_watchdog_wait(&child_info);

	log_info("Exiting loader");

	CloseHandle(child_info.hThread);
	CloseHandle(child_info.hProcess);

	if (log_file) {
		fclose(log_file);
	}

	return 0;
}
