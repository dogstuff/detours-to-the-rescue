#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include "dttr_crashdump.h"
#include "dttr_sidecar.h"
#include "graphics/graphics_com_private.h"
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
		|| request->struct_size < sizeof(DTTR_Mods_ExceptionReportRequest)
		|| report->struct_size < sizeof(DTTR_Mods_ExceptionReport)) {
		return false;
	}

	memset(report, 0, sizeof(*report));
	report->struct_size = sizeof(*report);

	EXCEPTION_RECORD exception_record = request->exception_record;
	if (exception_record.ExceptionCode == 0) {
		exception_record.ExceptionCode = request->exception_code;
	}

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
	.api_version = DTTR_MODS_API_VERSION,
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
	.api_version = DTTR_RUNTIME_API_VERSION,
};

// Exposes the single sidecar context shared by hooks, mods, and runtime calls.
const DTTR_Mods_Context *dttr_sidecar_context() { return &sidecar_ctx; }
const DTTR_Core_Context *dttr_sidecar_runtime_context() { return &sidecar_ctx.runtime; }

typedef bool (*dttr_required_symbol_check_fn)();

typedef struct dttr_required_symbol {
	const char *name;
	dttr_required_symbol_check_fn is_resolved;
} dttr_required_symbol;

// Captures module handles and APIs before callbacks expose sidecar state.
static void init_sidecar_context(HMODULE game_module, HMODULE sidecar_module) {
	sidecar_ctx = (DTTR_Mods_Context){
		.api_version = DTTR_MODS_API_VERSION,
		.runtime =
			(DTTR_Core_Context){
				.game_module = game_module,
				.api = &RUNTIME_API,
				.struct_size = sizeof(DTTR_Core_Context),
				.api_version = DTTR_RUNTIME_API_VERSION,
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

static bool check_required_symbol(const dttr_required_symbol *symbol) {
	if (symbol->is_resolved()) {
		return true;
	}

	DTTR_LOG_ERROR("Required SDK game symbol was not resolved: %s", symbol->name);
	return false;
}

static const DTTR_PCDOGS_T_Symbol_Data *pcdogs_symbol_data(
	DTTR_PCDOGS_T_Symbol_Data_Id id
) {
	return DTTR_PCDOGS_SymbolDataAt((uint32_t)id);
}

static bool movie_file_names_resolved() {
	const DTTR_PCDOGS_T_Symbol_Data *symbol = pcdogs_symbol_data(
		DTTR_PCDOGS_SYMBOL_DATA_ID_MOVIE_FILE_NAMES
	);
	return symbol && symbol->address != 0;
}

static char **movie_file_names_ptr() {
	const DTTR_PCDOGS_T_Symbol_Data *symbol = pcdogs_symbol_data(
		DTTR_PCDOGS_SYMBOL_DATA_ID_MOVIE_FILE_NAMES
	);
	return symbol && symbol->address ? (char **)symbol->address : NULL;
}

// Populates SDK symbol storage before enforcing the sidecar startup contract.
static bool resolve_required_sidecar_symbols(const DTTR_Core_Context *runtime) {
	DTTR_PCDOGS_ResolveAll(runtime);

	const dttr_required_symbol required_symbols[] = {
		{"PKGFindAndOpenFile", DTTR_PCDOGS_F_PKGFindAndOpenFile->IsResolved},
		{"ResourceInitializeGameEngine",
		 DTTR_PCDOGS_F_ResourceInitializeGameEngine->IsResolved},
		{"D3DInitializeGraphicsSubsystem",
		 DTTR_PCDOGS_F_D3DInitializeGraphicsSubsystem->IsResolved},
		{"PkgInitializeSystem", DTTR_PCDOGS_F_PkgInitializeSystem->IsResolved},
		{"DisplaySetMode", DTTR_PCDOGS_F_DisplaySetMode->IsResolved},
		{"InputResetState", DTTR_PCDOGS_F_InputResetState->IsResolved},
		{"ConfigLoadFromINIAlternate",
		 DTTR_PCDOGS_F_ConfigLoadFromINIAlternate->IsResolved},
		{"RenderFrame", DTTR_PCDOGS_F_RenderFrame->IsResolved},
		{"FileOpenWithMode", DTTR_PCDOGS_F_FileOpenWithMode->IsResolved},
		{"CRTMalloc", DTTR_PCDOGS_F_CRTMalloc->IsResolved},
		{"AudioInitializeSystem", DTTR_PCDOGS_F_AudioInitializeSystem->IsResolved},
		{"AudioShutdownSystem", DTTR_PCDOGS_F_AudioShutdownSystem->IsResolved},
		{"AudioStopAllSamples", DTTR_PCDOGS_F_AudioStopAllSamples->IsResolved},
		{"AudioStopAllSounds", DTTR_PCDOGS_F_AudioStopAllSounds->IsResolved},
		{"AudioInitializeLevelAudio",
		 DTTR_PCDOGS_F_AudioInitializeLevelAudio->IsResolved},
		{"MoviePlayFile", DTTR_PCDOGS_F_MoviePlayFile->IsResolved},
		{"DdrawObject", DTTR_PCDOGS_D_DdrawObject->IsResolved},
		{"GameInitialized", DTTR_PCDOGS_D_GameInitialized->IsResolved},
		{"JoystickAvailable", DTTR_PCDOGS_D_JoystickAvailable->IsResolved},
		{"MainWindowHandle", DTTR_PCDOGS_D_MainWindowHandle->IsResolved},
		{"MainWindowHandle2", DTTR_PCDOGS_D_MainWindowHandle2->IsResolved},
		{"RenderingEnabled", DTTR_PCDOGS_D_RenderingEnabled->IsResolved},
		{"ShouldQuit", DTTR_PCDOGS_D_ShouldQuit->IsResolved},
		{"PkgBasePath", DTTR_PCDOGS_D_PkgBasePath->IsResolved},
		{"AudioDigitalDriver", DTTR_PCDOGS_D_AudioDigitalDriver->IsResolved},
		{"MovieFileNames", movie_file_names_resolved},
		{"MoviePathPrefix", DTTR_PCDOGS_D_MoviePathPrefix->IsResolved},
		{"TitleBonusReplayResource", DTTR_PCDOGS_D_TitleBonusReplayResource->IsResolved},
		{"TitleResourceHandle1", DTTR_PCDOGS_D_TitleResourceHandle1->IsResolved},
		{"TitleResourceHandle0", DTTR_PCDOGS_D_TitleResourceHandle0->IsResolved},
		{"TitleMaterialBase", DTTR_PCDOGS_D_TitleMaterialBase->IsResolved},
		{"TitleResourcePackage", DTTR_PCDOGS_D_TitleResourcePackage->IsResolved},
	};

	bool ok = true;
	for (size_t i = 0; i < DTTR_ARRAY_COUNT(required_symbols); ++i) {
		ok = check_required_symbol(&required_symbols[i]) && ok;
	}

	return ok;
}

// Initializes subsystems that own required hooks. The order mirrors cleanup_runtime().
static bool install_required_sidecar_hooks(const DTTR_Mods_Context *ctx) {
	bool ok = true;
	ok = dttr_game_hooks_init(ctx) && ok;

	DTTR_Inputs_Init();
	ok = dttr_inputs_hooks_init(ctx) && ok;
	ok = dttr_graphics_hooks_init(ctx) && ok;
	ok = dttr_audio_init(ctx) && ok;

	DTTR_Movies_Init();
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

	if (DTTR_Movies_HandleEvent(event)) {
		after_sdl_event(event, true);
		return;
	}

	switch (event->type) {
	case SDL_EVENT_QUIT:
		DTTR_PCDOGS_D_ShouldQuit->Write(1);
		after_sdl_event(event, true);
		return;

	case SDL_EVENT_GAMEPAD_ADDED:
	case SDL_EVENT_GAMEPAD_REMOVED:
		DTTR_Inputs_HandleDeviceEvent(event);
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
		DTTR_Graphics_HandleWindowResize(event->window.data1, event->window.data2);
		after_sdl_event(event, true);
		return;

	default:
		break;
	}

	after_sdl_event(event, false);
}

// Drains SDL events through the sidecar event bridge during modding builds.
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
	DTTR_Movies_Cleanup();
	dttr_audio_cleanup(ctx);
	dttr_game_hooks_cleanup(ctx);
	dttr_graphics_hooks_cleanup(ctx);
	dttr_inputs_hooks_cleanup(ctx);
	DTTR_Inputs_Cleanup();
	DTTR_Graphics_Cleanup();
#ifdef DTTR_MODS_ENABLED
	dttr_mods_cleanup();
#endif
	DTTR_Core_HookCleanupAll();
}

static bool require_pcdogs_call(const char *name, bool called) {
	if (!called) {
		DTTR_LOG_ERROR("Required PCDOGS startup call failed: %s", name);
	}

	return called;
}

// Publishes the runtime API once the game window exists.
static bool initialize_pcdogs_runtime(const DTTR_Core_Context *ctx, HWND hwnd) {
	int32_t ret = 0;
	return require_pcdogs_call(
			   "PKFindAndOpenFile",
			   DTTR_PCDOGS_F_PKGFindAndOpenFile->Try(ctx, &ret)
		   )
		   && require_pcdogs_call(
			   "ResourceInitializeGameEngine",
			   DTTR_PCDOGS_F_ResourceInitializeGameEngine->Try(ctx, &ret)
		   )
		   && require_pcdogs_call(
			   "D3DInitializeGraphicsSubsystem",
			   DTTR_PCDOGS_F_D3DInitializeGraphicsSubsystem->Try(ctx, hwnd, NULL, &ret)
		   )
		   && require_pcdogs_call(
			   "PkInitializeSystem",
			   DTTR_PCDOGS_F_PkgInitializeSystem->Try(ctx, &ret)
		   );
}

// Moves the modding runtime into its started state after initialization succeeds.
static bool start_pcdogs_runtime(const DTTR_Core_Context *ctx, HWND hwnd) {
	int32_t ret = 0;
	int32_t config_ret = 0;
	return require_pcdogs_call(
			   "DisplaySetMode",
			   DTTR_PCDOGS_F_DisplaySetMode->Try(ctx, hwnd, &ret)
		   )
		   && require_pcdogs_call(
			   "InputResetState",
			   DTTR_PCDOGS_F_InputResetState->Try(ctx, &ret)
		   )
		   && require_pcdogs_call(
			   "ConfigLoadFromINIAlternate",
			   DTTR_PCDOGS_F_ConfigLoadFromINIAlternate->Try(ctx, &config_ret)
		   );
}

// Runs per-frame sidecar systems before yielding back to the original game loop.
static void tick_main_loop() {
	if (DTTR_Movies_MovieIsPlaying()) {
		DTTR_Movies_Tick();
		return;
	}

	SDL_DelayNS(1);

	int32_t rendering_enabled = 0;
	DTTR_PCDOGS_D_RenderingEnabled->Read(&rendering_enabled);

	if (rendering_enabled) {
#ifdef DTTR_MODS_ENABLED
		if (dttr_mods_should_advance_game_frame()) {
			DTTR_PCDOGS_F_RenderFrame->Call(dttr_sidecar_runtime_context(), 0);
			dttr_mods_game_frame_advanced();
		} else {
			dttr_graphics_begin_frame();
			dttr_graphics_end_frame();
			dttr_mods_game_frame_blocked();
		}

#else
		DTTR_PCDOGS_F_RenderFrame->Call(dttr_sidecar_runtime_context(), 0);
#endif
	}

#ifdef DTTR_MODS_ENABLED
	dttr_mods_tick();
#endif
}

// Plays startup movies through the normal sidecar tick loop.
static void attempt_play_startup_movies() {
	if (dttr_config.skip_intro_movies) {
		return;
	}

	const char *const prefix = DTTR_PCDOGS_D_MoviePathPrefix->Ptr();
	char **const names = movie_file_names_ptr();
	if (!prefix || !names) {
		return;
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

		DTTR_Movies_Start(path);
		sdsfree(path);

		while (DTTR_Movies_MovieIsPlaying()) {
			dttr_sidecar_poll_sdl_events();
			tick_main_loop();
		}

		const DTTR_MovieResult ret = DTTR_Movies_Stop();

		if (ret == DTTR_MOVIE_QUIT) {
			DTTR_PCDOGS_D_ShouldQuit->Write(1);
		}

		if (ret != DTTR_MOVIE_ENDED) {
			break;
		}
	}
}

// Hooks WinMain so sidecar initialization can wrap game startup and shutdown.
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

// Wraps the game WinMain so the sidecar can initialize hooks before the original loop runs.
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

	HWND hwnd = DTTR_Graphics_Init();

	if (!hwnd) {
		DTTR_LOG_ERROR("Failed to initialize - aborting");
		exit_code = 1;
		goto cleanup;
	}

	DTTR_LOG_INFO("Resolving required sidecar SDK game symbols...");
	if (!resolve_required_sidecar_symbols(&ctx->runtime)) {
		DTTR_LOG_ERROR("Failed to resolve required SDK game symbols - aborting");
		DTTR_Graphics_Cleanup();
		DTTR_Core_HookCleanupAll();
		exit_code = 1;
		goto cleanup;
	}

	dttr_pcdogs_crash_symbols_register(&ctx->runtime);

	if (!install_required_sidecar_hooks(ctx)) {
		DTTR_LOG_ERROR("Failed to install required sidecar hooks - aborting");
		exit_code = 1;
		goto cleanup_sidecar_runtime;
	}

#ifdef DTTR_MODS_ENABLED
	dttr_imgui_init(
		DTTR_Graphics_GetWindow(),
		DTTR_Graphics_GetDevice(),
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

	DTTR_PCDOGS_D_MainWindowHandle2->Write(hwnd);
	DTTR_PCDOGS_D_MainWindowHandle->Write(hwnd);

	if (!initialize_pcdogs_runtime(&ctx->runtime, hwnd)) {
		exit_code = 1;
		goto cleanup_sidecar_runtime;
	}

	attempt_play_startup_movies();

	if (!start_pcdogs_runtime(&ctx->runtime, hwnd)) {
		exit_code = 1;
		goto cleanup_sidecar_runtime;
	}

	void *audio_driver = NULL;
	DTTR_PCDOGS_D_AudioDigitalDriver->Read(&audio_driver);
	if (audio_driver == NULL) {
		DTTR_LOG_WARN("No audio device available - audio disabled");
	}

	DTTR_Inputs_LateInit();
#ifdef DTTR_MODS_ENABLED
	dttr_mods_late_init();
#endif
	DTTR_PCDOGS_D_ShouldQuit->Write(0);

	DTTR_PCDOGS_D_GameInitialized->Write(1);
	DTTR_PCDOGS_D_RenderingEnabled->Write(1);

	DTTR_LOG_INFO("Ready!");

	int32_t should_quit = 0;
	DTTR_PCDOGS_D_ShouldQuit->Read(&should_quit);
	while (should_quit == 0) {
		dttr_sidecar_poll_sdl_events();
		tick_main_loop();
		DTTR_PCDOGS_D_ShouldQuit->Read(&should_quit);
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
		ExitProcess(0);
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
