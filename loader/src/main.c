#include <dttr_config.h>
#include <dttr_crashdump.h>
#include <dttr_loader.h>
#include <dttr_log.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>

#include <gen/packed_sdb.h>

static const char *LOG_FILE_NAME = "dttr.log";

enum { PATH_ENV_BUFFER_SIZE = 1u << 15 };

static char s_config_path_buf[MAX_PATH];
const char *g_dttr_config_path = DTTR_CONFIG_FILENAME;

static void s_get_exe_dir(char *buf, size_t buf_size) {
	GetModuleFileNameA(NULL, buf, (DWORD)buf_size);
	char *last_sep = strrchr(buf, '\\');
	if (last_sep) {
		last_sep[1] = '\0';
	}
}

static bool s_set_env(const char *name, const char *value) {
	if (SetEnvironmentVariableA(name, value)) {
		return true;
	}

	DTTR_LOG_ERROR("Could not set %s", name);
	return false;
}

static void s_prepend_modules_to_path(void) {
	char old_path[PATH_ENV_BUFFER_SIZE] = "";

	const DWORD old_len = GetEnvironmentVariableA("PATH", old_path, sizeof(old_path));
	if (old_len >= sizeof(old_path)) {
		DTTR_LOG_ERROR("PATH is too long to prepend DttR modules directory");
		return;
	}

	char modules_dir[MAX_PATH];
	s_get_exe_dir(modules_dir, sizeof(modules_dir));

	const size_t dir_len = strlen(modules_dir);
	const int modules_len = snprintf(
		modules_dir + dir_len,
		sizeof(modules_dir) - dir_len,
		"modules"
	);
	if (modules_len <= 0 || (size_t)modules_len >= sizeof(modules_dir) - dir_len) {
		DTTR_LOG_ERROR("Could not resolve DttR modules directory for PATH");
		return;
	}

	char new_path[PATH_ENV_BUFFER_SIZE];
	const int written = old_len > 0
							? snprintf(
								  new_path,
								  sizeof(new_path),
								  "%s;%s",
								  modules_dir,
								  old_path
							  )
							: snprintf(new_path, sizeof(new_path), "%s", modules_dir);
	if (written <= 0 || (size_t)written >= sizeof(new_path)) {
		DTTR_LOG_ERROR("PATH is too long after prepending DttR modules directory");
		return;
	}

	if (!s_set_env("PATH", new_path)) {
		return;
	}

	DTTR_LOG_DEBUG("Prepended DttR modules directory to PATH: %s", modules_dir);
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

	s_set_env("DTTR_CONFIG_PATH", g_dttr_config_path);
	s_prepend_modules_to_path();
	if (iso_context.m_is_iso) {
		s_set_env("DTTR_ISO_CACHE_ROOT", iso_context.m_cache_root);
		s_set_env("DTTR_ISO_GAME_ROOT", iso_context.m_game_root);
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
