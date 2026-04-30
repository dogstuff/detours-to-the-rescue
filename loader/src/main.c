#include <dttr_config.h>
#include <dttr_crashdump.h>
#include <dttr_loader.h>
#include <dttr_log.h>
#include <stdio.h>
#include <string.h>
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

int main(const int argc, char *argv[]) {
	char exe_dir[MAX_PATH];
	s_get_exe_dir(exe_dir, sizeof(exe_dir));
	dttr_crashdump_init(exe_dir);

	if (argc > 1) {
		g_dttr_config_path = argv[1];
	}

	GetFullPathNameA(g_dttr_config_path, MAX_PATH, s_config_path_buf, NULL);
	g_dttr_config_path = s_config_path_buf;

	dttr_config_load(g_dttr_config_path);

	const int log_level = g_dttr_config.m_log_level;
	dttr_log_set_level(log_level);

	FILE *const log_file = fopen(LOG_FILE_NAME, "a+");
	if (log_file) {
		dttr_log_add_fp(log_file, log_level);
	}

	DTTR_LOG_INFO("Starting DttR loader (log level: %s)", log_level_string(log_level));

	DTTR_LoaderIsoContext iso_context = {0};

	WCHAR exe_path[MAX_PATH];
	if (!dttr_loader_resolve_exe_path(
			exe_path,
			g_dttr_config.m_pcdogs_path,
			&iso_context
		)) {
		DTTR_LOG_INFO("User exited without selecting a game path");
		if (log_file) {
			fclose(log_file);
		}
		return 0;
	}

	SetEnvironmentVariableA("DTTR_CONFIG_PATH", g_dttr_config_path);
	if (iso_context.m_is_direct) {
		SetEnvironmentVariableA("DTTR_ISO_PATH", iso_context.m_iso_path);
		SetEnvironmentVariableA("DTTR_ISO_CACHE_ROOT", iso_context.m_cache_root);
		SetEnvironmentVariableA("DTTR_ISO_GAME_ROOT", iso_context.m_game_root);
	}

	// Override Windows compatibility shims before the sidecar starts.
	PROCESS_INFORMATION child_info = {0};
	dttr_compat_create_process(
		exe_path,
		(const char *)g_packed_sdb,
		g_packed_sdb_len,
		&child_info
	);

	dttr_loader_watchdog_attach(&child_info);
	dttr_loader_inject_sidecar(&child_info);
	dttr_loader_watchdog_wait(&child_info);
	DTTR_LOG_INFO("Exiting loader");

	CloseHandle(child_info.hThread);
	CloseHandle(child_info.hProcess);

	if (log_file) {
		fclose(log_file);
	}

	return 0;
}
