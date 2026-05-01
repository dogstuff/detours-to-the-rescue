#include <dttr_config.h>
#include <dttr_crashdump.h>
#include <dttr_errors.h>
#include <dttr_imgui.h>
#include <dttr_loader.h>
#include <dttr_loader_ui.h>
#include <dttr_log.h>
#include <dttr_path.h>
#include <sds.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>

#include <gen/packed_sdb.h>

static const char *const S_LOG_FILE_NAME = "dttr.log";
static const char *const S_MODULES_DIR_NAME = "modules";

enum { PATH_ENV_BUFFER_SIZE = 1u << 15 };

static char s_config_path_buf[MAX_PATH];
const char *g_dttr_config_path = DTTR_CONFIG_FILENAME;

static bool s_set_env(const char *name, const char *value) {
	if (SetEnvironmentVariableA(name, value)) {
		return true;
	}

	DTTR_LOG_ERROR("Could not set %s", name);
	return false;
}

static bool s_resolve_modules_dir(char *out, size_t out_size) {
	sds modules_dir = dttr_path_module_sibling(NULL, S_MODULES_DIR_NAME);
	const bool copied = dttr_path_copy_sds(out, out_size, modules_dir);
	sdsfree(modules_dir);
	return copied;
}

static void s_prepend_modules_to_path(void) {
	char old_path[PATH_ENV_BUFFER_SIZE] = "";

	const DWORD old_len = GetEnvironmentVariableA("PATH", old_path, sizeof(old_path));
	if (old_len >= sizeof(old_path)) {
		DTTR_LOG_ERROR("PATH is too long to prepend DttR modules directory");
		return;
	}

	char modules_dir[MAX_PATH];
	if (!s_resolve_modules_dir(modules_dir, sizeof(modules_dir))) {
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

static void s_wait_for_child_process(const PROCESS_INFORMATION *child_info) {
	WaitForSingleObject(child_info->hProcess, INFINITE);
}

__declspec(dllexport) int dttr_launcher_main(int argc, char *argv[]) {
	char exe_dir[MAX_PATH];
	PROCESS_INFORMATION child_info = {0};
	sds exe_dir_sds = dttr_path_module_dir(NULL);
	if (!dttr_path_copy_sds(exe_dir, sizeof(exe_dir), exe_dir_sds)) {
		sdsfree(exe_dir_sds);
		DTTR_FATAL("Could not resolve loader directory");
	}
	sdsfree(exe_dir_sds);

	dttr_crashdump_init(exe_dir);
	dttr_errors_set_message_handler(dttr_loader_ui_show_error);

	if (argc > 1) {
		g_dttr_config_path = argv[1];
	}

	GetFullPathNameA(g_dttr_config_path, MAX_PATH, s_config_path_buf, NULL);
	g_dttr_config_path = s_config_path_buf;

	dttr_config_load(g_dttr_config_path);

	const int log_level = g_dttr_config.m_log_level;
	dttr_log_set_level(log_level);

	FILE *log_file = fopen(S_LOG_FILE_NAME, "a+");
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
		goto cleanup;
	}

	s_set_env("DTTR_CONFIG_PATH", g_dttr_config_path);
	s_prepend_modules_to_path();

	if (iso_context.m_is_iso) {
		s_set_env("DTTR_ISO_CACHE_ROOT", iso_context.m_cache_root);
		s_set_env("DTTR_ISO_GAME_ROOT", iso_context.m_game_root);
	}

	// Override Windows compatibility shims before the sidecar starts.
	dttr_compat_create_process(
		exe_path,
		(const char *)g_packed_sdb,
		g_packed_sdb_len,
		&child_info
	);

	dttr_loader_watchdog_attach(&child_info);
	dttr_loader_inject_sidecar(&child_info);
	dttr_loader_watchdog_wait(&child_info);

	s_wait_for_child_process(&child_info);

	DTTR_LOG_INFO("Exiting loader");

cleanup:
	if (child_info.hThread) {
		CloseHandle(child_info.hThread);
	}

	if (child_info.hProcess) {
		CloseHandle(child_info.hProcess);
	}

	dttr_imgui_dialog_shutdown();

	if (log_file) {
		fclose(log_file);
	}

	return 0;
}
