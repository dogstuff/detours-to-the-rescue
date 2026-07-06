#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include "dttr_crashdump.h"
#include "dttr_errors.h"
#include "dttr_hooks.h"
#include <dttr_config.h>
#include <dttr_core.h>
#include <dttr_log.h>
#include <dttr_path.h>
#include <dttr_pcdogs.h>
#include <dttr_runtime.h>
#include <sds.h>
#include <xxhash.h>

#include "bootstrap_private.h"
#include "context_private.h"
#include "crash_private.h"
#include "events_private.h"
#include "game_data_private.h"
#include "graphics/graphics_private.h"
#include "inputs/inputs_private.h"
#include "sidecar_hook_sigs.h"
#include "sidecar_private.h"
#include "timing_private.h"

#ifdef DTTR_MODS_ENABLED
#include "game/frame_pacing_private.h"
#include "graphics/imgui_overlay_private.h"
#include "mods/mods_private.h"
#endif

HINSTANCE dttr_sidecar_module;
char dttr_loader_dir[MAX_PATH];
char dttr_exe_hash[DTTR_EXE_HASH_LENGTH + 1];

static HMODULE pc_dogs_module;

DTTR_DEFINE_HOOK_STORAGE(dttr_hook_win_main)

// Hashes the launched game executable for config, logs, and mods.
static void compute_exe_hash() {
	char exe_path[MAX_PATH];
	GetModuleFileNameA(pc_dogs_module, exe_path, sizeof(exe_path));

	HANDLE file = CreateFileA(
		exe_path,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		0,
		NULL
	);
	void *buf = NULL;
	if (file == INVALID_HANDLE_VALUE) {
		DTTR_LOG_ERROR("Failed to open exe for hashing: %s", exe_path);
		goto fail;
	}

	DWORD file_size = GetFileSize(file, NULL);
	if (file_size == INVALID_FILE_SIZE && GetLastError() != NO_ERROR) {
		DTTR_LOG_ERROR("Failed to get exe size for hashing: %s", exe_path);
		goto fail;
	}

	buf = malloc(file_size);
	if (file_size != 0 && !buf) {
		DTTR_LOG_ERROR("Failed to allocate %lu bytes for exe hashing", file_size);
		goto fail;
	}

	DWORD bytes_read = 0;
	if (!ReadFile(file, buf, file_size, &bytes_read, NULL)) {
		DTTR_LOG_ERROR("Failed to read exe for hashing: %s", exe_path);
		goto fail;
	}

	CloseHandle(file);
	file = INVALID_HANDLE_VALUE;

	XXH64_hash_t hash = XXH3_64bits(buf, bytes_read);
	free(buf);
	buf = NULL;

	snprintf(dttr_exe_hash, sizeof(dttr_exe_hash), "%016llx", (unsigned long long)hash);
	return;

fail:
	if (file != INVALID_HANDLE_VALUE) {
		CloseHandle(file);
	}

	free(buf);
	memcpy(dttr_exe_hash, "0000000000000000", sizeof(dttr_exe_hash));
}

// Resolves the sidecar directory used as the base for config files and mod loading.
static sds get_loader_dir() {
	sds loader_dir = DTTR_Path_ModuleDir(dttr_sidecar_module);
	if (!loader_dir) {
		return NULL;
	}

	const size_t len = sdslen(loader_dir);
	if (len >= 8 && _stricmp(loader_dir + len - 8, "modules\\") == 0) {
		sdsrange(loader_dir, 0, (ssize_t)len - 9);
	}

	return loader_dir;
}

// Builds the sidecar config path beside the loader.
static sds get_config_path() {
	const char *config_env = getenv("DTTR_CONFIG_PATH");
	return sdsnew(config_env ? config_env : DTTR_CONFIG_FILENAME);
}

// Hooks Window_RunWinMain so sidecar initialization can wrap game startup and shutdown.
static void install_win_main_hook() {
	// DllMain installs this bootstrap hook before dttr_sidecar_init_context() creates a
	// DTTR_Mods_Context.
	uintptr_t site = 0;
	DTTR_Core_AOBFindInModule(pc_dogs_module, DTTR_SIDECAR_AOB_WIN_MAIN, &site);
	if (!site) {
		return;
	}

	dttr_hook_win_main_site = site;
	const int32_t rel = (int32_t)((uintptr_t)DTTR_Hook_WinMainCallback - (site + 5));

	uint8_t jmp[5] = {0xE9};
	memcpy(jmp + 1, &rel, 4);
	dttr_hook_win_main_handle = DTTR_Core_HookPatchBytes(site, jmp, sizeof(jmp));
}

// Wraps the game Window_RunWinMain so the sidecar can initialize hooks before the
// original loop runs.
int32_t _stdcall DTTR_Hook_WinMainCallback(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	int32_t nCmdShow
) {
	int exit_code = 0;
	bool should_exit_process = false;
	FILE *log_file = NULL;
	sds loader_dir = get_loader_dir();
	sds config_path = NULL;
	sds log_path = NULL;

	if (!loader_dir
		|| !DTTR_Path_CopySds(dttr_loader_dir, sizeof(dttr_loader_dir), loader_dir)) {
		goto cleanup;
	}

	DTTR_CrashDump_Init(dttr_loader_dir);
	OutputDebugStringA("DTTR_SIDECAR_ENTRYPOINT");

	compute_exe_hash();

	config_path = get_config_path();
	if (!config_path || !DTTR_Config_Load(config_path)) {
		DTTR_FATAL(
			"Failed to load configuration file at %s",
			config_path ? config_path : DTTR_CONFIG_FILENAME
		);
	}

	log_path = DTTR_Path_ResolveRelativeTo(dttr_loader_dir, dttr_config.log_file_path);
	if (!log_path) {
		goto cleanup;
	}

	log_file = fopen(log_path, "a+");
	if (!log_file) {
		DTTR_FATAL("Could not open log file at %s", log_path);
	}

	DTTR_LOG_INFO("Starting DttR sidecar");
	DTTR_LOG_INFO("Loaded configuration file at %s", config_path);

	const int level = dttr_config.log_level;
	DTTR_Log_SetLevel(level);
	DTTR_Log_AddFP(log_file, level);
	DTTR_LOG_INFO("Log level set to %s", log_level_string(level));
	dttr_game_data_init();

	dttr_sidecar_init_context(pc_dogs_module, dttr_sidecar_module);
	const DTTR_Mods_Context *ctx = dttr_sidecar_context();

	HWND hwnd = dttr_graphics_init();

	if (!hwnd) {
		DTTR_LOG_ERROR("Failed to initialize - aborting");
		exit_code = 1;
		goto cleanup;
	}

	DTTR_LOG_INFO("Resolving sidecar SDK game symbols...");
	DTTR_PCDOGS_ResolveAll(&ctx->runtime);
	dttr_pcdogs_crash_symbols_register(&ctx->runtime);

	if (!dttr_bootstrap_install_required_hooks(ctx)) {
		DTTR_LOG_ERROR("Failed to install required sidecar hooks - aborting");
		exit_code = 1;
		goto cleanup_sidecar_runtime;
	}

#ifdef DTTR_MODS_ENABLED
	dttr_imgui_init(
		dttr_graphics_get_window(),
		dttr_graphics_get_device(),
		dttr_backend.backend_type
	);
	dttr_timing_reset();
	dttr_mods_init();
	dttr_graphics_mod_window_created(&dttr_backend);
	dttr_graphics_mod_device_created(&dttr_backend);
	dttr_graphics_mod_device_restored(&dttr_backend);
	const DTTR_Mods_InputContext input_ctx = {
		.overlay_visible = false,
		.game_input_enabled = true,
	};

	dttr_mods_input_mode_changed(&input_ctx);
	dttr_mods_overlay_visible_changed(false);
#endif

	if (!REQUIRE_PCDOGS_CALL(
			DTTR_PCDOGS_D_Window_RunWinMain_SecondaryWindowHandle->Write(hwnd)
		)
		|| !REQUIRE_PCDOGS_CALL(DTTR_PCDOGS_D_Window_MainHandle->Write(hwnd))) {
		exit_code = 1;
		goto cleanup_sidecar_runtime;
	}

	if (!dttr_bootstrap_initialize_pcdogs_runtime(&ctx->runtime, hwnd)) {
		exit_code = 1;
		goto cleanup_sidecar_runtime;
	}

	const dttr_startup_movies_result
		startup_movies = dttr_bootstrap_attempt_play_startup_movies();
	if (startup_movies == DTTR_STARTUP_MOVIES_FAILED) {
		exit_code = 1;
		goto cleanup_sidecar_runtime;
	}

	if (!dttr_bootstrap_start_pcdogs_runtime(&ctx->runtime, hwnd)) {
		exit_code = 1;
		goto cleanup_sidecar_runtime;
	}

	void *audio_driver = NULL;
	if (!REQUIRE_PCDOGS_CALL(
			DTTR_PCDOGS_D_Audio_InitializeSystem_DigitalDriver->Read(&audio_driver)
		)) {
		exit_code = 1;
		goto cleanup_sidecar_runtime;
	}

	if (audio_driver == NULL) {
		DTTR_LOG_WARN("No MSS audio driver available - audio disabled");
	}

	if (!dttr_inputs_late_init()) {
		exit_code = 1;
		goto cleanup_sidecar_runtime;
	}

#ifdef DTTR_MODS_ENABLED
	dttr_mods_late_init();
#endif
	if ((startup_movies != DTTR_STARTUP_MOVIES_QUIT
		 && !REQUIRE_PCDOGS_CALL(
			 DTTR_PCDOGS_D_Input_ProcessWindowMessages_ShouldQuit->Write(0)
		 ))
		|| !REQUIRE_PCDOGS_CALL(DTTR_PCDOGS_D_Window_ProcessGameProc_Initialized->Write(1))
		|| !REQUIRE_PCDOGS_CALL(
			DTTR_PCDOGS_D_Window_RunWinMain_RenderingEnabled->Write(1)
		)) {
		exit_code = 1;
		goto cleanup_sidecar_runtime;
	}

	DTTR_LOG_INFO("Ready!");

	for (;;) {
		int32_t should_quit = 0;
		if (!REQUIRE_PCDOGS_CALL(
				DTTR_PCDOGS_D_Input_ProcessWindowMessages_ShouldQuit->Read(&should_quit)
			)) {
			exit_code = 1;
			break;
		}

		if (should_quit != 0) {
			break;
		}

		dttr_sidecar_poll_sdl_events();
		if (!dttr_bootstrap_tick_main_loop()) {
			exit_code = 1;
			break;
		}
	}

cleanup_sidecar_runtime:
	DTTR_LOG_INFO("Cleaning up hooks");
	dttr_bootstrap_cleanup_runtime(ctx);

	DTTR_LOG_INFO("Exiting DttR sidecar");
	should_exit_process = true;

cleanup:
	sdsfree(log_path);
	sdsfree(config_path);
	sdsfree(loader_dir);

	if (log_file) {
		fclose(log_file);
	}

	if (should_exit_process) {
		ExitProcess((UINT)exit_code);
	}

	return exit_code;
}

// Captures process attach so the sidecar can bootstrap runtime state.
BOOL APIENTRY DllMain(HMODULE module, const DWORD reason_for_call, LPVOID reserved) {
	if (reason_for_call == DLL_PROCESS_ATTACH) {
		dttr_sidecar_module = module;

		pc_dogs_module = GetModuleHandleA("pcdogs.exe");
		if (!pc_dogs_module) {
			return TRUE;
		}

		install_win_main_hook();
	}

	return TRUE;
}
