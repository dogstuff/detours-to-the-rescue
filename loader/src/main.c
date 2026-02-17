#include <dttr_config.h>
#include <dttr_crashdump.h>
#include <dttr_loader.h>
#include <log.h>
#include <stdio.h>
#include <windows.h>

#include <gen/packed_sdb.h>

static const char *LOG_FILE_NAME = "dttr.log";

static char s_config_path_buf[MAX_PATH];
const char *g_dttr_config_path = DTTR_CONFIG_FILENAME;

static void s_get_exe_dir(char *buf, size_t buf_size) {
	GetModuleFileNameA(NULL, buf, (DWORD)buf_size);
	char *last_sep = strrchr(buf, '\\');
	if (last_sep) {
		last_sep[1] = '\0';
	}
}

int main(int argc, char *argv[]) {
	char exe_dir[MAX_PATH];
	s_get_exe_dir(exe_dir, sizeof(exe_dir));
	dttr_crashdump_init(exe_dir);

	if (argc > 1) {
		g_dttr_config_path = argv[1];
	}

	GetFullPathNameA(g_dttr_config_path, MAX_PATH, s_config_path_buf, NULL);
	g_dttr_config_path = s_config_path_buf;

	dttr_config_load(g_dttr_config_path);

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

	SetEnvironmentVariableA("DTTR_CONFIG_PATH", g_dttr_config_path);

	/// Includes the packed and bundled in-memory compatibility SDB,
	/// and injects it into the pcdogs.exe child process.
	///
	/// This overrides any compatibility shims applied by Windows, which
	/// resolves the EmulateHeap corruption issue that occurs on Intel
	/// integrated graphics.
	PROCESS_INFORMATION child_info = {0};
	dttr_compat_create_process(
		exe_path,
		(const char *)g_packed_sdb,
		g_packed_sdb_len,
		&child_info
	);

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
