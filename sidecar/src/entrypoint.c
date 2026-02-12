#include <windows.h>

#include "dttr_sidecar.h"
#include "graphics/graphics_com_internal.h"
#include "log.h"

#include <SDL3/SDL.h>

#include "dttr_errors.h"
#include "dttr_hooks.h"
#include "graphics/graphics_internal.h"

DTTR_GameModule g_dttr_pc_dogs_module;
HINSTANCE g_dttr_sidecar_module;

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

	if (event->type != SDL_EVENT_WINDOW_RESIZED
		&& event->type != SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED) {
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

int32_t _stdcall dttr_hook_win_main_callback(
	HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int32_t nCmdShow
) {
	FILE *const log_file = fopen("dttr_sidecar.log", "a+");

	if (!log_file) {
		s_raise_error(DTTR_PREFIX_INIT "Could not open log file dttr_sidecar.log");
	}

	log_set_level(LOG_INFO);

	log_info(DTTR_PREFIX_INIT "Loading configuration file at %s...", DTTR_CONFIG_FILENAME);

	if (!dttr_config_load(DTTR_CONFIG_FILENAME)) {
		log_error(DTTR_PREFIX_INIT "Configuration load failed - aborting");
		return 1;
	}

	log_add_fp(log_file, g_dttr_config.m_log_level);
	log_info(
		DTTR_PREFIX_INIT "File log level set to %s", log_level_string(g_dttr_config.m_log_level)
	);

	const HWND hwnd = dttr_graphics_init();

	if (hwnd == NULL) {
		log_error(DTTR_PREFIX_GRAPHICS "Failed to initialize - aborting");
		return 1;
	}

	log_info(DTTR_PREFIX_INIT "Initializing game globals...");
	s_interop_pcdogs_globals_init(g_dttr_pc_dogs_module);

	log_info(DTTR_PREFIX_INIT "Initializing game functions...");
	s_interop_pcdogs_functions_init(g_dttr_pc_dogs_module);

	dttr_crt_hook_init(g_dttr_pc_dogs_module);
	dttr_inputs_init(g_dttr_pc_dogs_module);
	dttr_graphics_hook_init(g_dttr_pc_dogs_module);

	g_pcdogs_main_window_handle2_set(hwnd);
	g_pcdogs_main_window_handle_set(hwnd);

	pcdogs_find_and_load_game_pak_file();
	pcdogs_initialize_game_engine();
	pcdogs_initialize_graphics_subsystem(hwnd, NULL);
	pcdogs_initialize_graphics_capabilities();

	pcdogs_initialize_window_handle(hwnd);
	pcdogs_reset_input_and_state();
	pcdogs_initialize_game_systems();
	dttr_inputs_late_init();
	g_pcdogs_should_quit_set(0);

	g_pcdogs_game_initialized_set(1);
	g_pcdogs_rendering_enabled_set(1);

	while (g_pcdogs_should_quit_get() == 0) {
		SDL_Event event;

		while (SDL_PollEvent(&event)) {
			s_handle_sdl_event(&event);
		}

		s_tick_main_loop();
	}

	dttr_crt_hook_cleanup();
	dttr_graphics_hook_cleanup();
	dttr_inputs_cleanup();
	dttr_graphics_cleanup();

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
