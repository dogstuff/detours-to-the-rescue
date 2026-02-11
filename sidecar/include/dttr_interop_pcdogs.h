#ifndef DTTR_GAME_FUNCTIONS_H
#define DTTR_GAME_FUNCTIONS_H

#include <dttr_interop.h>

typedef HMODULE DTTR_GameModule;

// These are game globals used by the sidecar runtime
DTTR_INTEROP_VAR(g_pcdogs_ddraw_object, void *, 0x168A6F0)

DTTR_INTEROP_VAR(g_pcdogs_game_initialized, int, 0x23490C)

DTTR_INTEROP_VAR(g_pcdogs_gamepad_button_flags, uint32_t, 0x234980)

DTTR_INTEROP_VAR(g_pcdogs_graphics_initialized, uint64_t, 0x15A9570)

DTTR_INTEROP_VAR(g_pcdogs_keyboard_mapping_buttons, void *, 0x9CA3C)

DTTR_INTEROP_VAR(g_pcdogs_keyboard_mapping_keys, void *, 0x9CA38)

DTTR_INTEROP_VAR(g_pcdogs_main_window_handle, void *, 0x234910)

DTTR_INTEROP_VAR(g_pcdogs_main_window_handle2, void *, 0x9CA40)

DTTR_INTEROP_VAR(g_pcdogs_mapping_count, uint32_t, 0x23497C)

DTTR_INTEROP_VAR(g_pcdogs_rendering_enabled, int, 0x234900)

DTTR_INTEROP_VAR(g_pcdogs_should_quit, int, 0x9CA48)

DTTR_INTEROP_VAR(g_pcdogs_joystick_available, uint8_t, 0x15A9570)

// Resolves cached addresses for global-variable wrappers
static void s_interop_pcdogs_globals_init(HMODULE mod) {
	g_pcdogs_ddraw_object_init(mod);
	g_pcdogs_game_initialized_init(mod);
	g_pcdogs_gamepad_button_flags_init(mod);
	g_pcdogs_graphics_initialized_init(mod);
	g_pcdogs_joystick_available_init(mod);
	g_pcdogs_keyboard_mapping_buttons_init(mod);
	g_pcdogs_keyboard_mapping_keys_init(mod);
	g_pcdogs_main_window_handle_init(mod);
	g_pcdogs_main_window_handle2_init(mod);
	g_pcdogs_mapping_count_init(mod);
	g_pcdogs_rendering_enabled_init(mod);
	g_pcdogs_should_quit_init(mod);
}

// These are game functions used by the sidecar runtime
DTTR_INTEROP_WRAP_CACHED_CC(pcdogs_find_and_load_game_pak_file, __cdecl, 0x3eea0, void, (void), ())

DTTR_INTEROP_WRAP_CACHED_CC(pcdogs_initialize_game_engine, __cdecl, 0x2c80f, int, (void), ())

DTTR_INTEROP_WRAP_CACHED_CC(
	pcdogs_initialize_graphics_subsystem,
	__cdecl,
	0x3d740,
	void,
	(HWND hwnd, HINSTANCE h_instance),
	(hwnd, h_instance)
)

DTTR_INTEROP_WRAP_CACHED_CC(
	pcdogs_initialize_graphics_capabilities, __cdecl, 0x3e860, void, (void), ()
)

DTTR_INTEROP_WRAP_CACHED_CC(
	pcdogs_initialize_window_handle, __cdecl, 0x450a0, int, (HWND hwnd), (hwnd)
)

DTTR_INTEROP_WRAP_CACHED_CC(pcdogs_initialize_game_systems, __cdecl, 0x3d770, void, (void), ())

DTTR_INTEROP_WRAP_CACHED_CC(pcdogs_render_frame, __cdecl, 0x3e0a0, int, (void), ())

DTTR_INTEROP_WRAP_CACHED_CC(
	pcdogs_is_key_pressed, __cdecl, 0x45010, unsigned int, (char scan_code), (scan_code)
)

DTTR_INTEROP_WRAP_CACHED_CC(pcdogs_reset_input_and_state, __cdecl, 0x3e2f0, void, (void), ())

DTTR_INTEROP_WRAP_CACHED_CC(pcdogs_take_screenshot, __cdecl, 0x18b90, void, (void), ())

DTTR_INTEROP_WRAP_CACHED_CC(pcdogs_malloc, __cdecl, 0x465d4, void *, (size_t size), (size))

// Resolves cached addresses for function wrappers
static void s_interop_pcdogs_functions_init(DTTR_GameModule mod) {
	pcdogs_find_and_load_game_pak_file_init(mod);
	pcdogs_initialize_game_engine_init(mod);
	pcdogs_initialize_graphics_subsystem_init(mod);
	pcdogs_initialize_graphics_capabilities_init(mod);
	pcdogs_initialize_window_handle_init(mod);
	pcdogs_initialize_game_systems_init(mod);
	pcdogs_render_frame_init(mod);
	pcdogs_is_key_pressed_init(mod);
	pcdogs_reset_input_and_state_init(mod);
	pcdogs_take_screenshot_init(mod);
	pcdogs_malloc_init(mod);
}

#endif // DTTR_GAME_FUNCTIONS_H
