#include <stdlib.h>
#include <windows.h>

#include "dttr_crashdump.h"
#include "dttr_sidecar.h"
#include "graphics/graphics_com_internal.h"
#include "log.h"
#include <sds.h>

#include <SDL3/SDL.h>

#include "dttr_errors.h"
#include "dttr_hooks.h"
#include "graphics/graphics_internal.h"
#include <xxhash.h>

DTTR_GameModule g_dttr_pc_dogs_module;
HINSTANCE g_dttr_sidecar_module;
char g_dttr_loader_dir[MAX_PATH];
char g_dttr_exe_hash[17];

static void s_compute_exe_hash(void) {
	char exe_path[MAX_PATH];
	GetModuleFileNameA(g_dttr_pc_dogs_module, exe_path, sizeof(exe_path));

	HANDLE file = CreateFileA(exe_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);

	if (file == INVALID_HANDLE_VALUE) {
		log_error("Failed to open exe for hashing: %s", exe_path);
		strcpy(g_dttr_exe_hash, "0000000000000000");
		return;
	}

	DWORD file_size = GetFileSize(file, NULL);
	void *buf = malloc(file_size);
	DWORD bytes_read;

	ReadFile(file, buf, file_size, &bytes_read, NULL);
	CloseHandle(file);

	XXH64_hash_t hash = XXH3_64bits(buf, bytes_read);
	free(buf);

	snprintf(g_dttr_exe_hash, sizeof(g_dttr_exe_hash), "%016llx", (unsigned long long)hash);
}

static sds s_get_loader_dir(void) {
	char buf[MAX_PATH];
	GetModuleFileNameA(g_dttr_sidecar_module, buf, sizeof(buf));

	char *last_sep = strrchr(buf, '\\');
	if (last_sep) {
		last_sep[1] = '\0';
	}

	return sdsnew(buf);
}

static void s_handle_sdl_event(const SDL_Event *event) {
	if (event->type == SDL_EVENT_QUIT) {
		g_pcdogs_should_quit_set(1);
		return;
	}

	if (event->type == SDL_EVENT_KEY_DOWN && event->key.scancode == SDL_SCANCODE_F11) {
		SDL_Window *const window = g_dttr_backend.m_window;
		const bool is_fullscreen = (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN) != 0;
		SDL_SetWindowFullscreen(window, !is_fullscreen);
		return;
	}

	if (event->type != SDL_EVENT_WINDOW_RESIZED && event->type != SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED) {
		return;
	}

	dttr_graphics_handle_window_resize(event->window.data1, event->window.data2);
}

static void s_tick_main_loop(void) {
	if (g_pcdogs_rendering_enabled_get()) {
		if (pcdogs_render_frame()) {
			SDL_Delay(1);
		}
		return;
	}

	SDL_Delay(10);
}

int32_t _stdcall
dttr_hook_win_main_callback(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int32_t nCmdShow) {
	dttr_crashdump_init("dttr_sidecar");
	OutputDebugStringA("DTTR_SIDECAR_ENTRYPOINT");

	sds loader_dir = s_get_loader_dir();
	strncpy(g_dttr_loader_dir, loader_dir, MAX_PATH - 1);
	g_dttr_loader_dir[MAX_PATH - 1] = '\0';

	s_compute_exe_hash();

	sds log_path = sdscat(sdsdup(loader_dir), "dttr.log");
	sdsfree(loader_dir);

	const char *config_env = getenv("DTTR_CONFIG_PATH");
	sds config_path = sdsnew(config_env ? config_env : DTTR_CONFIG_FILENAME);

	FILE *const log_file = fopen(log_path, "a+");
	if (!log_file) {
		DTTR_FATAL("Could not open log file at %s", log_path);
	}

	log_info("Starting DttR sidecar");
	log_info("Loading configuration file at %s...", config_path);

	if (!dttr_config_load(config_path)) {
		DTTR_FATAL("Failed to load configuration file at %s", config_path);
	}

	const int level = g_dttr_config.m_log_level;
	log_set_level(level);
	log_add_fp(log_file, level);
	log_info("Log level set to %s", log_level_string(level));

	HWND hwnd = dttr_graphics_init();

	if (hwnd == NULL) {
		log_error("Failed to initialize - aborting");
		return 1;
	}

	log_info("Initializing game globals...");
	s_interop_pcdogs_globals_init(g_dttr_pc_dogs_module);

	log_info("Initializing game functions...");
	s_interop_pcdogs_functions_init(g_dttr_pc_dogs_module);

	dttr_other_hook_init(g_dttr_pc_dogs_module);
	dttr_inputs_init(g_dttr_pc_dogs_module);
	dttr_graphics_hook_init(g_dttr_pc_dogs_module);

	g_pcdogs_main_window_handle2_set(hwnd);
	g_pcdogs_main_window_handle_set(hwnd);

	pcdogs_find_and_load_game_pak_file();
	pcdogs_initialize_game_engine();
	pcdogs_initialize_graphics_subsystem(hwnd, NULL);
	pcdogs_initialize_capabilities();

	pcdogs_initialize_window_handle(hwnd);
	pcdogs_reset_input_and_state();
	pcdogs_initialize_game_systems();
	dttr_inputs_late_init();
	g_pcdogs_should_quit_set(0);

	g_pcdogs_game_initialized_set(1);
	g_pcdogs_rendering_enabled_set(1);

	log_info("Ready!");

	while (g_pcdogs_should_quit_get() == 0) {
		SDL_Event event;

		while (SDL_PollEvent(&event)) {
			s_handle_sdl_event(&event);
		}

		s_tick_main_loop();
	}

	log_info("Cleaning up hooks");

	dttr_other_hook_cleanup();
	dttr_graphics_hook_cleanup();
	dttr_inputs_cleanup();
	dttr_graphics_cleanup();

	log_info("Exiting DttR sidecar");
	sdsfree(log_path);
	sdsfree(config_path);
	fclose(log_file);

	return 0;
}

BOOL APIENTRY DllMain(HMODULE module, const DWORD reason_for_call, LPVOID reserved) {
	if (reason_for_call == DLL_PROCESS_ATTACH) {
		g_dttr_sidecar_module = module;

		g_dttr_pc_dogs_module = DTTR_UNWRAP_WINAPI_EXISTS(GetModuleHandleA("pcdogs.exe"));

		dttr_hook_win_main_install(g_dttr_pc_dogs_module);
	}

	return TRUE;
}
