#ifndef DTTR_GAME_FUNCTIONS_H
#define DTTR_GAME_FUNCTIONS_H

#include <dttr_components.h>

#include "log.h"

// Game globals

DTTR_VAR(g_pcdogs_ddraw_object, void *)
DTTR_VAR(g_pcdogs_game_initialized, int)
DTTR_VAR(g_pcdogs_gamepad_button_flags, uint32_t)
DTTR_VAR(g_pcdogs_joystick_available, uint8_t)
DTTR_VAR(g_pcdogs_keyboard_mapping_buttons, void *)
DTTR_VAR(g_pcdogs_keyboard_mapping_keys, void *)
DTTR_VAR(g_pcdogs_main_window_handle, void *)
DTTR_VAR(g_pcdogs_main_window_handle2, void *)
DTTR_VAR(g_pcdogs_mapping_count, uint32_t)
DTTR_VAR(g_pcdogs_rendering_enabled, int)
DTTR_VAR(g_pcdogs_should_quit, int)
DTTR_VAR(g_pcdogs_directory, char)
DTTR_VAR(g_pcdogs_audio_digital_driver, void *)
DTTR_VAR(g_pcdogs_movie_file_names, char *)
DTTR_VAR(g_pcdogs_movie_path_prefix, char)
DTTR_VAR(g_pcdogs_level_asset_0, void *)
DTTR_VAR(g_pcdogs_level_asset_1, void *)
DTTR_VAR(g_pcdogs_level_asset_2, void *)
DTTR_VAR(g_pcdogs_level_asset_3, void *)
DTTR_VAR(g_pcdogs_level_asset_4, void *)

// Resolves globals and functions via inline sigscan and assignment.

static void s_interop_pcdogs_globals_init(const DTTR_ComponentContext *ctx) {
	DTTR_RESOLVE(
		g_pcdogs_ddraw_object,
		ctx,
		"\xA1????\x8B\x15????\x81\xEC\x9C\x00\x00\x00",
		"x????xx????xxxxxx",
		*(uint32_t *)(match + 1)
	);

	DTTR_RESOLVE(
		g_pcdogs_game_initialized,
		ctx,
		"\x89\x35????\xC6\x44\x24?\x10",
		"xx????xxx?x",
		*(uint32_t *)(match + 2)
	);

	DTTR_RESOLVE(
		g_pcdogs_gamepad_button_flags,
		ctx,
		"\x8B\x15????\x8B\x06\x0B\xC2\x89\x06\x81\xFB\xBC\x02\x00\x00",
		"xx????xxxxxxxxxxxx",
		*(uint32_t *)(match + 2)
	);

	DTTR_RESOLVE(
		g_pcdogs_joystick_available,
		ctx,
		"\xA0????\x84\xC0\x0F\x84????\xA1",
		"x????xxxx????x",
		*(uint32_t *)(match + 1)
	);

	DTTR_RESOLVE(
		g_pcdogs_keyboard_mapping_buttons,
		ctx,
		"\x8B\x15????\x50\x56",
		"xx????xx",
		*(uint32_t *)(match + 2)
	);

	DTTR_RESOLVE(
		g_pcdogs_keyboard_mapping_keys,
		ctx,
		"\xA3????\xA1????\x8D\x14\x8D",
		"x????x????xxx",
		*(uint32_t *)(match + 1)
	);

	DTTR_RESOLVE(
		g_pcdogs_main_window_handle,
		ctx,
		"\xA1????\x83\xC4\x08\x6A\x03",
		"x????xxxxx",
		*(uint32_t *)(match + 1)
	);

	DTTR_RESOLVE(
		g_pcdogs_main_window_handle2,
		ctx,
		"\xA3????\xFF\xD7",
		"x????xx",
		*(uint32_t *)(match + 1)
	);

	DTTR_RESOLVE(
		g_pcdogs_mapping_count,
		ctx,
		"\x8B\x0D????\x8B\x15????\x50",
		"xx????xx????x",
		*(uint32_t *)(match + 2)
	);

	DTTR_RESOLVE(
		g_pcdogs_rendering_enabled,
		ctx,
		"\x39\x35????\x74?\xE8",
		"xx????x?x",
		*(uint32_t *)(match + 2)
	);

	DTTR_RESOLVE(
		g_pcdogs_should_quit,
		ctx,
		"\x39\x35????\x75?\x39\x35",
		"xx????x?xx",
		*(uint32_t *)(match + 2)
	);

	DTTR_RESOLVE(
		g_pcdogs_directory,
		ctx,
		"\x68????\x68\x04\x01\x00\x00\xFF\x15",
		"x????xxxxxxx",
		*(uint32_t *)(match + 1)
	);

	DTTR_RESOLVE(
		g_pcdogs_audio_digital_driver,
		ctx,
		"\xA1????\x6A\x7F\x50\xFF\x15",
		"x????xxxxx",
		*(uint32_t *)(match + 1)
	);

	{
		uintptr_t match = ctx->m_game_api->m_sigscan(
			ctx->m_game_module,
			"\x8B\x04\xB5????\x50\x68",
			"xxx????xx"
		);
		if (match) {
			g_pcdogs_movie_file_names_addr = *(uint32_t *)(match + 3);
			log_debug(
				"Resolved g_pcdogs_movie_file_names at 0x%08X",
				(unsigned)g_pcdogs_movie_file_names_addr
			);
			g_pcdogs_movie_path_prefix_addr = *(uint32_t *)(match + 9);
			log_debug(
				"Resolved g_pcdogs_movie_path_prefix at 0x%08X",
				(unsigned)g_pcdogs_movie_path_prefix_addr
			);
		} else {
			log_error(
				"g_pcdogs_movie_file_names / g_pcdogs_movie_path_prefix:"
				" resolved to NULL"
			);
		}
	}

	{
		uintptr_t match = ctx->m_game_api->m_sigscan(
			ctx->m_game_module,
			"\x6A\x01\xE8????\xE8????\xA1????\x50\xE8????\x8B\x0D????\x51\xE8????"
			"\x8B\x15????\x52\xE8????\xA1????\x50\xE8????\x8B\x0D????\x51\xE8????\x83??",
			"xxx????x????x????xx????xx????xx????xx????xx????x????xx????xx????xx??"
		);
		if (match) {
			g_pcdogs_level_asset_0_addr = *(uint32_t *)(match + 13);
			log_debug(
				"Resolved g_pcdogs_level_asset_0 at 0x%08X",
				(unsigned)g_pcdogs_level_asset_0_addr
			);
			g_pcdogs_level_asset_1_addr = *(uint32_t *)(match + 25);
			log_debug(
				"Resolved g_pcdogs_level_asset_1 at 0x%08X",
				(unsigned)g_pcdogs_level_asset_1_addr
			);
			g_pcdogs_level_asset_2_addr = *(uint32_t *)(match + 37);
			log_debug(
				"Resolved g_pcdogs_level_asset_2 at 0x%08X",
				(unsigned)g_pcdogs_level_asset_2_addr
			);
			g_pcdogs_level_asset_3_addr = *(uint32_t *)(match + 48);
			log_debug(
				"Resolved g_pcdogs_level_asset_3 at 0x%08X",
				(unsigned)g_pcdogs_level_asset_3_addr
			);
			g_pcdogs_level_asset_4_addr = *(uint32_t *)(match + 60);
			log_debug(
				"Resolved g_pcdogs_level_asset_4 at 0x%08X",
				(unsigned)g_pcdogs_level_asset_4_addr
			);
		} else {
			log_error("g_pcdogs_level_asset_*: resolved to NULL");
		}
	}
}

// Game functions

DTTR_FUNC(pcdogs_find_and_load_game_pkg_file, __cdecl, void, (void), ())
DTTR_FUNC(pcdogs_initialize_game_engine, __cdecl, int, (void), ())
DTTR_FUNC(
	pcdogs_initialize_graphics_subsystem,
	__cdecl,
	void,
	(HWND hwnd, HINSTANCE h_instance),
	(hwnd, h_instance)
)
DTTR_FUNC(pcdogs_initialize_capabilities, __cdecl, void, (void), ())
DTTR_FUNC(pcdogs_initialize_window_handle, __cdecl, int, (HWND hwnd), (hwnd))
DTTR_FUNC(pcdogs_initialize_game_systems, __cdecl, void, (void), ())
DTTR_FUNC(pcdogs_render_frame, __cdecl, int, (void), ())
DTTR_FUNC(pcdogs_is_key_pressed, __cdecl, unsigned int, (char scan_code), (scan_code))
DTTR_FUNC(pcdogs_reset_input_and_state, __cdecl, void, (void), ())
DTTR_FUNC(pcdogs_take_screenshot, __cdecl, void, (void), ())
DTTR_FUNC(pcdogs_malloc, __cdecl, void *, (size_t size), (size))
DTTR_FUNC(pcdogs_audio_shutdown_system, __cdecl, void, (void), ())

static void s_interop_pcdogs_functions_init(const DTTR_ComponentContext *ctx) {
	DTTR_RESOLVE(
		pcdogs_find_and_load_game_pkg_file,
		ctx,
		"\x81\xEC\x10\x01\x00\x00\x57",
		"xxxxxxx",
		match
	);

	DTTR_RESOLVE(
		pcdogs_initialize_game_engine,
		ctx,
		"\xE8????\x85\xC0\x75?\x32\xC0",
		"x????xxx?xx",
		match
	);

	DTTR_RESOLVE(
		pcdogs_initialize_graphics_subsystem,
		ctx,
		"\xE8????\x8B\x44\x24?\x8B\x4C\x24?\x50",
		"x????xxx?xxx?x",
		match
	);

	DTTR_RESOLVE(
		pcdogs_initialize_capabilities,
		ctx,
		"\xE8????\xA1????\x50\xA3",
		"x????x????xx",
		DTTR_E8_TARGET(match)
	);

	DTTR_RESOLVE(
		pcdogs_initialize_window_handle,
		ctx,
		"\xA1????\x56\x57\x8B\x7C\x24",
		"x????xxxxx",
		match
	);

	DTTR_RESOLVE(
		pcdogs_initialize_game_systems,
		ctx,
		"\xE8????\x8D\x54\x24?\x56",
		"x????xxx?x",
		DTTR_E8_TARGET(match)
	);

	DTTR_RESOLVE(pcdogs_render_frame, ctx, "\x51\x53\xE8????\xA1", "xxx????x", match);

	DTTR_RESOLVE(
		pcdogs_is_key_pressed,
		ctx,
		"\xA1????\x33\xC9\x85\xC0\x53",
		"x????xxxxx",
		match
	);

	DTTR_RESOLVE(
		pcdogs_reset_input_and_state,
		ctx,
		"\xE8????\xA1????\x25\xDF\xF4\xFF\xFF",
		"x????x????xxxxx",
		match
	);

	DTTR_RESOLVE(
		pcdogs_take_screenshot,
		ctx,
		"\x81\xEC\x04\x01\x00\x00\x56",
		"xxxxxxx",
		match
	);

	DTTR_RESOLVE(pcdogs_malloc, ctx, "\xFF\x35????\xFF\x74\x24", "xx????xxx", match);

	DTTR_RESOLVE(
		pcdogs_audio_shutdown_system,
		ctx,
		"\xA1????\x85\xC0\x74?\x53\x8B\x1D",
		"x????xxx?xxx",
		match
	);
}

#endif // DTTR_GAME_FUNCTIONS_H
