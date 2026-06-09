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

static const char *const MODULES_DIR_NAME = "modules";

enum { PATH_ENV_BUFFER_SIZE = 1u << 15 };

static char config_path_buf[MAX_PATH];
const char *dttr_config_path = DTTR_CONFIG_FILENAME;

static bool set_env(const char *name, const char *value) {
	if (SetEnvironmentVariableA(name, value)) {
		return true;
	}

	DTTR_LOG_ERROR("Could not set %s", name);
	return false;
}

static bool resolve_modules_dir(char *out, size_t out_size) {
	sds modules_dir = DTTR_Path_ModuleSibling(NULL, MODULES_DIR_NAME);
	const bool copied = DTTR_Path_CopySds(out, out_size, modules_dir);
	sdsfree(modules_dir);
	return copied;
}

static bool resolve_loader_dir(char *out, size_t out_size) {
	sds loader_dir = DTTR_Path_ModuleDir(NULL);
	const bool copied = DTTR_Path_CopySds(out, out_size, loader_dir);
	sdsfree(loader_dir);
	return copied;
}

static void resolve_config_path(int argc, char *argv[]) {
	sds default_config_path = NULL;
	const char *config_path = argc > 1 ? argv[1] : NULL;
	if (!config_path || !config_path[0]) {
		default_config_path = DTTR_Path_ModuleSibling(NULL, DTTR_CONFIG_FILENAME);
		config_path = default_config_path;
	}

	if (!config_path || !config_path[0]) {
		sdsfree(default_config_path);
		DTTR_FATAL("Could not resolve default loader config path");
	}

	const DWORD len = GetFullPathNameA(config_path, MAX_PATH, config_path_buf, NULL);
	sdsfree(default_config_path);
	if (len == 0 || len >= MAX_PATH) {
		DTTR_FATAL("Could not resolve loader config path");
	}

	dttr_config_path = config_path_buf;
}

static FILE *open_log_file(int log_level) {
	char loader_dir[MAX_PATH];
	const char *base_dir = NULL;
	if (resolve_loader_dir(loader_dir, sizeof(loader_dir))) {
		base_dir = loader_dir;
	}

	sds log_path = DTTR_Path_ResolveRelativeTo(base_dir, dttr_config.log_file_path);
	if (!log_path) {
		return NULL;
	}

	FILE *log_file = fopen(log_path, "a+");
	if (log_file) {
		DTTR_Log_AddFP(log_file, log_level);
	}

	sdsfree(log_path);
	return log_file;
}

static bool terminate_child(PROCESS_INFORMATION *child_info, DWORD exit_code) {
	if (!child_info->hProcess) {
		return true;
	}

	if (!TerminateProcess(child_info->hProcess, exit_code)) {
		DTTR_LOG_ERROR("Could not terminate game process after launch failure");
		return false;
	}

	const DWORD wait_result = WaitForSingleObject(child_info->hProcess, 10000);
	if (wait_result != WAIT_OBJECT_0) {
		DTTR_LOG_ERROR(
			"Timed out waiting for game process to terminate after launch failure"
		);
		return false;
	}

	return true;
}

static void close_child_handles(PROCESS_INFORMATION *child_info) {
	if (child_info->hThread) {
		CloseHandle(child_info->hThread);
	}

	if (child_info->hProcess) {
		CloseHandle(child_info->hProcess);
	}
}

static void prepend_modules_to_path() {
	char old_path[PATH_ENV_BUFFER_SIZE] = "";

	const DWORD old_len = GetEnvironmentVariableA("PATH", old_path, sizeof(old_path));
	if (old_len >= sizeof(old_path)) {
		DTTR_LOG_ERROR("PATH is too long to prepend DttR modules directory");
		return;
	}

	char modules_dir[MAX_PATH];
	if (!resolve_modules_dir(modules_dir, sizeof(modules_dir))) {
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

	if (!set_env("PATH", new_path)) {
		return;
	}

	DTTR_LOG_DEBUG("Prepended DttR modules directory to PATH: %s", modules_dir);
}

__declspec(dllexport) int dttr_launcher_main(int argc, char *argv[]) {
	char exe_dir[MAX_PATH];
	PROCESS_INFORMATION child_info = {0};
	DWORD child_exit_code = 1;

	if (!resolve_loader_dir(exe_dir, sizeof(exe_dir))) {
		DTTR_FATAL("Could not resolve loader directory");
	}

	DTTR_CrashDump_Init(exe_dir);
	DTTR_Errors_SetMessageHandler(DTTR_LoaderUI_ShowError);

	resolve_config_path(argc, argv);
	if (!DTTR_Config_Load(dttr_config_path)) {
		const char *details = DTTR_Config_LastError();
		DTTR_FATAL(
			"Could not load configuration file %s%s%s",
			dttr_config_path,
			details ? ":\n" : "",
			details ? details : ""
		);
	}

	const int log_level = dttr_config.log_level;
	DTTR_Log_SetLevel(log_level);

	FILE *log_file = open_log_file(log_level);

	DTTR_LOG_INFO("Starting DttR loader (log level: %s)", log_level_string(log_level));

	DTTR_LoaderIsoContext iso_context = {0};

	WCHAR exe_path[MAX_PATH];
	if (!DTTR_Loader_ResolveEXEPath(exe_path, dttr_config.pcdogs_path, &iso_context)) {
		DTTR_LOG_INFO("User exited without selecting a game path");
		goto cleanup;
	}

	if (!set_env("DTTR_CONFIG_PATH", dttr_config_path)) {
		DTTR_FATAL("Could not pass configuration path to game process");
	}

	prepend_modules_to_path();

	if (iso_context.is_iso) {
		if (!set_env("DTTR_ISO_CACHE_ROOT", iso_context.cache_root)
			|| !set_env("DTTR_ISO_GAME_ROOT", iso_context.game_root)) {
			DTTR_FATAL("Could not pass ISO extraction paths to game process");
		}
	}

	// Override compatibility shims before the sidecar starts.
	DTTR_Compat_CreateProcess(
		exe_path,
		(const char *)packed_sdb,
		packed_sdb_len,
		&child_info
	);

	DTTR_Loader_WatchdogAttach(&child_info);
	if (!DTTR_Loader_InjectSidecar(&child_info)) {
		DTTR_ERROR("Could not inject sidecar into the game process." DTTR_REPORT_SUFFIX);
		DTTR_Loader_WatchdogDetach(&child_info);
		terminate_child(&child_info, 1);
	} else if (!DTTR_Loader_WatchdogWait(&child_info)) {
		terminate_child(&child_info, 1);
	}

	WaitForSingleObject(child_info.hProcess, INFINITE);
	if (!GetExitCodeProcess(child_info.hProcess, &child_exit_code)) {
		DTTR_LOG_ERROR("Could not read game process exit code");
		child_exit_code = 1;
	}

	DTTR_LOG_INFO("Exiting loader with child exit code %lu", child_exit_code);

cleanup:
	close_child_handles(&child_info);
	DTTR_ImGuiDialog_Shutdown();

	if (log_file) {
		fclose(log_file);
	}

	return child_info.hProcess ? (int)child_exit_code : 0;
}
