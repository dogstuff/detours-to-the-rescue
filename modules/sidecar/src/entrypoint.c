#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include "dttr_crashdump.h"
#include "graphics/graphics_com_private.h"
#include "inputs/inputs_private.h"
#include <dttr_config.h>
#include <dttr_log.h>
#include <dttr_path.h>
#include <sds.h>

#include <SDL3/SDL.h>

#include "audio/hooks_private.h"
#include "dttr_errors.h"
#include "dttr_hooks.h"
#include "game/hooks_private.h"
#include "graphics/graphics_private.h"
#include "graphics/hooks_private.h"
#include "inputs/hooks_private.h"
#include "movies/hooks_private.h"
#include "movies/movies_private.h"
#include "sidecar_private.h"
#include <dttr_pcdogs.h>
#include <dttr_runtime.h>
#include <xxhash.h>

#ifdef DTTR_MODS_ENABLED
#include "graphics/imgui_overlay_private.h"
#include "mods/mods_private.h"
#endif

HINSTANCE dttr_sidecar_module;
char dttr_loader_dir[MAX_PATH];
char dttr_exe_hash[DTTR_EXE_HASH_LENGTH + 1];

static HMODULE pc_dogs_module;
static DTTR_Mods_Context sidecar_ctx;

static bool dttr_sidecar_store_sds(char *dst, size_t dst_size, sds src) {
	if (!src) {
		return false;
	}

	if (dst_size > 0) {
		size_t copy_len = sdslen(src);
		if (copy_len >= dst_size) {
			copy_len = dst_size - 1u;
		}
		memcpy(dst, src, copy_len);
		dst[copy_len] = '\0';
	}

	sdsfree(src);
	return true;
}

static bool dttr_sidecar_write_exception_report(
	const DTTR_Mods_ExceptionReportRequest *request,
	DTTR_Mods_ExceptionReport *report
) {
	if (!request || !report
		|| request->struct_size != sizeof(DTTR_Mods_ExceptionReportRequest)
		|| report->struct_size != sizeof(DTTR_Mods_ExceptionReport)
		|| request->exception_record.ExceptionCode == 0) {
		return false;
	}

	memset(report, 0, sizeof(*report));
	report->struct_size = sizeof(*report);

	EXCEPTION_RECORD exception_record = request->exception_record;
	CONTEXT context = request->context;
	EXCEPTION_POINTERS exception_pointers = {
		.ExceptionRecord = &exception_record,
		.ContextRecord = &context,
	};
	const DWORD current_thread_id = GetCurrentThreadId();
	const DWORD thread_id = request->thread_id ? request->thread_id : current_thread_id;

	sds dump_path = DTTR_CrashDump_Write(
		GetCurrentProcess(),
		GetCurrentProcessId(),
		thread_id,
		&exception_pointers
	);
	report->dump_written = dttr_sidecar_store_sds(
		report->dump_path,
		sizeof(report->dump_path),
		dump_path
	);
	if (!report->dump_written) {
		report->win32_error = GetLastError();
	}

	HANDLE thread = GetCurrentThread();
	bool close_thread = false;
	if (thread_id != current_thread_id) {
		thread = OpenThread(
			THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION,
			FALSE,
			thread_id
		);
		close_thread = thread != NULL;
		if (!thread && report->win32_error == ERROR_SUCCESS) {
			report->win32_error = GetLastError();
		}
	}

	sds stack_trace = DTTR_CrashDump_FormatStackTrace(
		GetCurrentProcess(),
		thread,
		&context
	);
	if (close_thread) {
		CloseHandle(thread);
	}

	report->stack_trace_written = dttr_sidecar_store_sds(
		report->stack_trace,
		sizeof(report->stack_trace),
		stack_trace
	);

	return report->dump_written || report->stack_trace_written;
}

static const DTTR_Mods_API MOD_API = {
	.log = DTTR_Log,
	.log_is_enabled = DTTR_Log_IsEnabled,
	.log_unchecked = DTTR_Log_Unchecked,
	.struct_size = sizeof(DTTR_Mods_API),
	.abi_version = DTTR_SDK_ABI_VERSION,
	.write_exception_report = dttr_sidecar_write_exception_report,
};

static const DTTR_Core_API RUNTIME_API = {
	.sigscan = DTTR_Core_HookCachedSigscan,
	.hook_function = DTTR_Core_HookAttachFunction,
	.hook_pointer = DTTR_Core_HookAttachPointer,
	.patch_bytes = DTTR_Core_HookPatchBytes,
	.unhook = DTTR_Core_HookDetach,
	.hook_is_active = DTTR_Core_HookIsActive,
	.unhook_checked = DTTR_Core_HookDetachChecked,
	.struct_size = sizeof(DTTR_Core_API),
	.abi_version = DTTR_SDK_ABI_VERSION,
};

// Exposes the single sidecar context shared by hooks, mods, and runtime calls.
const DTTR_Mods_Context *dttr_sidecar_context() {
	return &sidecar_ctx;
}

const DTTR_Core_Context *dttr_sidecar_runtime_context() {
	return &sidecar_ctx.runtime;
}

// Captures module handles and APIs before callbacks expose sidecar state.
static void init_sidecar_context(HMODULE game_module, HMODULE sidecar_module) {
	sidecar_ctx = (DTTR_Mods_Context){
		.abi_version = DTTR_SDK_ABI_VERSION,
		.runtime =
			(DTTR_Core_Context){
				.game_module = game_module,
				.api = &RUNTIME_API,
				.struct_size = sizeof(DTTR_Core_Context),
				.abi_version = DTTR_SDK_ABI_VERSION,
			},
		.sidecar_module = sidecar_module,
		.window = NULL,
		.loader_dir = dttr_loader_dir,
		.exe_hash = dttr_exe_hash,
		.config = &dttr_config,
		.api = &MOD_API,
		.struct_size = sizeof(DTTR_Mods_Context),
	};
}

// Initializes subsystems that own required hooks. The order mirrors cleanup_runtime().
static bool install_required_sidecar_hooks(const DTTR_Mods_Context *ctx) {
	bool ok = true;
	ok = dttr_game_hooks_init(ctx) && ok;

	dttr_inputs_init();
	ok = dttr_inputs_hooks_init(ctx) && ok;
	ok = dttr_graphics_hooks_init(ctx) && ok;
	ok = dttr_audio_init(ctx) && ok;

	dttr_movies_init();
	ok = dttr_movies_hooks_init(ctx) && ok;

	return ok;
}

// Falls back to a stable unknown hash when the executable cannot be read during startup.
static void set_default_exe_hash() {
	memcpy(dttr_exe_hash, "0000000000000000", sizeof(dttr_exe_hash));
}

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
	set_default_exe_hash();
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

// Applies the runtime fullscreen toggle to the active graphics window.
static void toggle_fullscreen() {
	SDL_Window *const window = dttr_backend.window;
	const bool is_fullscreen = (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN) != 0;
	if (!SDL_SetWindowFullscreen(window, !is_fullscreen)) {
		DTTR_LOG_WARN("SDL_SetWindowFullscreen failed: %s", SDL_GetError());
	}
}

#ifdef DTTR_MODS_ENABLED
// Sends post-dispatch SDL events to mods after the sidecar and game handlers run.
static void after_sdl_event(const SDL_Event *event, bool consumed) {
	dttr_mods_after_event(event, consumed);
}
#else
#define after_sdl_event(event, consumed)                                                 \
	do {                                                                                 \
	} while (0)
#endif

static bool require_pcdogs_call(const char *name, DTTR_Result result) {
	if (!DTTR_ResultOK(result)) {
		DTTR_LOG_ERROR(
			"Required PCDOGS operation failed: %s (%s)",
			name,
			dttr_sidecar_result_detail(result)
		);
		return false;
	}

	return true;
}

#define REQUIRE_PCDOGS_CALL(expr_) require_pcdogs_call(#expr_, (expr_))

// Routes SDL events through sidecar handlers before game input observes them.
void dttr_sidecar_handle_sdl_event(const SDL_Event *event) {
#ifdef DTTR_MODS_ENABLED
	if (dttr_mods_before_event(event)) {
		after_sdl_event(event, true);
		return;
	}

	if (dttr_imgui_process_event(event)) {
		after_sdl_event(event, true);
		return;
	}

	if (dttr_mods_handle_event(event)) {
		after_sdl_event(event, true);
		return;
	}

#endif

	if (dttr_movies_handle_event(event)) {
		after_sdl_event(event, true);
		return;
	}

	switch (event->type) {
	case SDL_EVENT_QUIT:
		REQUIRE_PCDOGS_CALL(
			DTTR_PCDOGS_D_Input_ProcessWindowMessages_ShouldQuit->Write(1)
		);
		after_sdl_event(event, true);
		return;

	case SDL_EVENT_GAMEPAD_ADDED:
	case SDL_EVENT_GAMEPAD_REMOVED:
		dttr_inputs_handle_device_event(event);
		after_sdl_event(event, true);
		return;

	case SDL_EVENT_AUDIO_DEVICE_ADDED:
	case SDL_EVENT_AUDIO_DEVICE_REMOVED:
		dttr_audio_handle_device_event(event);
		after_sdl_event(event, true);
		return;

	case SDL_EVENT_KEY_DOWN:
		if (event->key.scancode == SDL_SCANCODE_F11) {
			toggle_fullscreen();
			after_sdl_event(event, true);
			return;
		}

		break;

	case SDL_EVENT_WINDOW_RESIZED:
	case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
		dttr_graphics_handle_window_resize(event->window.data1, event->window.data2);
		after_sdl_event(event, true);
		return;

	default:
		break;
	}

	after_sdl_event(event, false);
}

// Drains SDL events through the sidecar event bridge.
void dttr_sidecar_poll_sdl_events() {
	SDL_Event event;

	while (SDL_PollEvent(&event)) {
		dttr_sidecar_handle_sdl_event(&event);
	}
}

// Releases modding runtime hooks and mod state before graphics and audio shutdown.
static void cleanup_runtime(const DTTR_Mods_Context *ctx) {
	dttr_pcdogs_crash_symbols_clear();
	dttr_game_data_cleanup();

#ifdef DTTR_MODS_ENABLED
	dttr_imgui_cleanup();
#endif

	dttr_movies_hooks_cleanup(ctx);
	dttr_movies_cleanup();
	dttr_audio_cleanup(ctx);
	dttr_game_hooks_cleanup(ctx);
	dttr_graphics_hooks_cleanup(ctx);
	dttr_inputs_hooks_cleanup(ctx);
	dttr_inputs_cleanup();
	dttr_graphics_cleanup();
#ifdef DTTR_MODS_ENABLED
	dttr_mods_cleanup();
#endif
	DTTR_Core_HookCleanupAll();
}

// Runs required PCDOGS startup calls after the game window exists.
static bool initialize_pcdogs_runtime(const DTTR_Core_Context *ctx, HWND hwnd) {
	int32_t ret = 0;

	return REQUIRE_PCDOGS_CALL(DTTR_PCDOGS_F_PKG_FindAndOpenFile->Call(ctx, &ret))
		   && REQUIRE_PCDOGS_CALL(
			   DTTR_PCDOGS_F_PKG_InitializeResourceGameEngine->Call(ctx, &ret)
		   )
		   && REQUIRE_PCDOGS_CALL(
			   DTTR_PCDOGS_F_D3D_InitializeGraphicsSubsystem->Call(ctx, hwnd, NULL, &ret)
		   )
		   && REQUIRE_PCDOGS_CALL(DTTR_PCDOGS_F_PKG_InitializeSystem->Call(ctx, &ret));
}

// Moves the modding runtime into its started state after initialization succeeds.
static bool start_pcdogs_runtime(const DTTR_Core_Context *ctx, HWND hwnd) {
	int32_t ret = 0;
	int32_t config_ret = 0;
	return REQUIRE_PCDOGS_CALL(DTTR_PCDOGS_F_Display_SetMode->Call(ctx, hwnd, &ret))
		   && REQUIRE_PCDOGS_CALL(DTTR_PCDOGS_F_Input_ResetState->Call(ctx, &ret))
		   && REQUIRE_PCDOGS_CALL(
			   DTTR_PCDOGS_F_Config_LoadAlternateFromINI->Call(ctx, &config_ret)
		   );
}

typedef enum {
	DTTR_STARTUP_MOVIES_CONTINUE,
	DTTR_STARTUP_MOVIES_QUIT,
	DTTR_STARTUP_MOVIES_FAILED,
} dttr_startup_movies_result;

// Runs per-frame sidecar systems before yielding back to the original game loop.
static bool tick_main_loop() {
	if (dttr_movies_is_playing()) {
		dttr_movies_tick();
		return true;
	}

	SDL_DelayNS(1);

	int32_t rendering_enabled = 0;
	if (!REQUIRE_PCDOGS_CALL(
			DTTR_PCDOGS_D_Window_RunWinMain_RenderingEnabled->Read(&rendering_enabled)
		)) {
		return false;
	}

	if (rendering_enabled) {
#ifdef DTTR_MODS_ENABLED
		if (dttr_mods_should_advance_game_frame()) {
			uint8_t frame_status = 0;
			if (!REQUIRE_PCDOGS_CALL(DTTR_PCDOGS_F_Graphics_RenderFrame->Call(
					dttr_sidecar_runtime_context(),
					&frame_status
				))) {
				return false;
			}

			dttr_mods_game_frame_advanced();
		} else {
			dttr_graphics_begin_frame();
			dttr_graphics_end_frame();
			dttr_mods_game_frame_blocked();
		}

#else
		uint8_t frame_status = 0;
		if (!REQUIRE_PCDOGS_CALL(DTTR_PCDOGS_F_Graphics_RenderFrame->Call(
				dttr_sidecar_runtime_context(),
				&frame_status
			))) {
			return false;
		}
#endif
	}

#ifdef DTTR_MODS_ENABLED
	dttr_mods_tick();
#endif
	return true;
}

// Plays startup movies through the normal sidecar tick loop.
static dttr_startup_movies_result attempt_play_startup_movies() {
	if (dttr_config.skip_intro_movies) {
		return DTTR_STARTUP_MOVIES_CONTINUE;
	}

	const char *const prefix = DTTR_PCDOGS_D_Video_PlayMovieIntro_PathPrefix->Ptr();
	char **const names = (char **)DTTR_PCDOGS_D_Video_PlayMovieIntro_FileNames->Ptr();
	if (!prefix || !names) {
		DTTR_LOG_WARN("Startup movie metadata unavailable; skipping intro movies");
		return DTTR_STARTUP_MOVIES_CONTINUE;
	}

	for (int i = 0; i < 4; i++) {
		if (!names[i]) {
			break;
		}

		sds path = sdsnew(prefix);
		if (!path || !DTTR_Path_AppendSegment(&path, names[i], '\\')) {
			sdsfree(path);
			break;
		}

		dttr_movies_start(path);
		sdsfree(path);

		while (dttr_movies_is_playing()) {
			dttr_sidecar_poll_sdl_events();
			if (!tick_main_loop()) {
				dttr_movies_stop();
				return DTTR_STARTUP_MOVIES_FAILED;
			}
		}

		const dttr_movie_result ret = dttr_movies_stop();

		if (ret == DTTR_MOVIE_QUIT) {
			if (!REQUIRE_PCDOGS_CALL(
					DTTR_PCDOGS_D_Input_ProcessWindowMessages_ShouldQuit->Write(1)
				)) {
				return DTTR_STARTUP_MOVIES_FAILED;
			}

			return DTTR_STARTUP_MOVIES_QUIT;
		}

		if (ret != DTTR_MOVIE_ENDED) {
			break;
		}
	}

	return DTTR_STARTUP_MOVIES_CONTINUE;
}

// Hooks Window_RunWinMain so sidecar initialization can wrap game startup and shutdown.
static void install_win_main_hook() {
	// DllMain installs this bootstrap hook before init_sidecar_context() creates a
	// DTTR_Mods_Context.
	const uintptr_t site = DTTR_Core_HookSigscan(
		pc_dogs_module,
		"\x83\xEC\x40\x53\x8B\x5C\x24",
		"xxxxxxx"
	);
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

	init_sidecar_context(pc_dogs_module, dttr_sidecar_module);
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

	if (!install_required_sidecar_hooks(ctx)) {
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

	if (!initialize_pcdogs_runtime(&ctx->runtime, hwnd)) {
		exit_code = 1;
		goto cleanup_sidecar_runtime;
	}

	const dttr_startup_movies_result startup_movies = attempt_play_startup_movies();
	if (startup_movies == DTTR_STARTUP_MOVIES_FAILED) {
		exit_code = 1;
		goto cleanup_sidecar_runtime;
	}

	if (!start_pcdogs_runtime(&ctx->runtime, hwnd)) {
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
		DTTR_LOG_WARN("No audio device available - audio disabled");
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
		if (!tick_main_loop()) {
			exit_code = 1;
			break;
		}
	}

cleanup_sidecar_runtime:
	DTTR_LOG_INFO("Cleaning up hooks");
	cleanup_runtime(ctx);

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

		pc_dogs_module = DTTR_UNWRAP_WINAPI_EXISTS(GetModuleHandleA("pcdogs.exe"));

		install_win_main_hook();
	}

	return TRUE;
}
