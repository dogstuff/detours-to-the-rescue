#ifndef DTTR_GAME_FUNCTIONS_H
#define DTTR_GAME_FUNCTIONS_H

#include <dttr_interop.h>

typedef HMODULE DTTR_GameModule;

// These are game globals used by the sidecar runtime

DTTR_INTEROP_VAR_SIG(
	g_pcdogs_ddraw_object,
	void *,
	"\xA1????\x8B\x15????\x81\xEC\x9C\x00\x00\x00",
	"x????xx????xxxxxx",
	*(uint32_t *)(match + 1)
)

DTTR_INTEROP_VAR_SIG(
	g_pcdogs_game_initialized,
	int,
	"\x89\x35????\xC6\x44\x24?\x10",
	"xx????xxx?x",
	*(uint32_t *)(match + 2)
)

DTTR_INTEROP_VAR_SIG(
	g_pcdogs_gamepad_button_flags,
	uint32_t,
	"\x8B\x15????\x8B\x06\x0B\xC2\x89\x06\x81\xFB\xBC\x02\x00\x00",
	"xx????xxxxxxxxxxxx",
	*(uint32_t *)(match + 2)
)

DTTR_INTEROP_VAR_SIG(
	g_pcdogs_joystick_available,
	uint8_t,
	"\xA0????\x84\xC0\x0F\x84????\xA1",
	"x????xxxx????x",
	*(uint32_t *)(match + 1)
)

DTTR_INTEROP_VAR_SIG(
	g_pcdogs_keyboard_mapping_buttons,
	void *,
	"\x8B\x15????\x50\x56",
	"xx????xx",
	*(uint32_t *)(match + 2)
)

DTTR_INTEROP_VAR_SIG(
	g_pcdogs_keyboard_mapping_keys,
	void *,
	"\xA3????\xA1????\x8D\x14\x8D",
	"x????x????xxx",
	*(uint32_t *)(match + 1)
)

DTTR_INTEROP_VAR_SIG(
	g_pcdogs_main_window_handle,
	void *,
	"\xA1????\x83\xC4\x08\x6A\x03",
	"x????xxxxx",
	*(uint32_t *)(match + 1)
)

DTTR_INTEROP_VAR_SIG(
	g_pcdogs_main_window_handle2,
	void *,
	"\xA3????\xFF\xD7",
	"x????xx",
	*(uint32_t *)(match + 1)
)

DTTR_INTEROP_VAR_SIG(
	g_pcdogs_mapping_count,
	uint32_t,
	"\x8B\x0D????\x8B\x15????\x50",
	"xx????xx????x",
	*(uint32_t *)(match + 2)
)

DTTR_INTEROP_VAR_SIG(
	g_pcdogs_rendering_enabled,
	int,
	"\x39\x35????\x74?\xE8",
	"xx????x?x",
	*(uint32_t *)(match + 2)
)

DTTR_INTEROP_VAR_SIG(
	g_pcdogs_should_quit,
	int,
	"\x39\x35????\x75?\x39\x35",
	"xx????x?xx",
	*(uint32_t *)(match + 2)
)

DTTR_INTEROP_VAR_SIG(
	g_pcdogs_directory,
	char,
	"\x68????\x68\x04\x01\x00\x00\xFF\x15",
	"x????xxxxxxx",
	*(uint32_t *)(match + 1)
)

// clang-format off
#define DTTR_MOVIE_PLAY_INTRO_SIG  "\x8B\x04\xB5????\x50\x68"
#define DTTR_MOVIE_PLAY_INTRO_MASK "xxx????xx"
// clang-format on

DTTR_INTEROP_VAR_SIG(
	g_pcdogs_movie_file_names,
	char *,
	DTTR_MOVIE_PLAY_INTRO_SIG,
	DTTR_MOVIE_PLAY_INTRO_MASK,
	*(uint32_t *)(match + 3)
)

DTTR_INTEROP_VAR_SIG(
	g_pcdogs_movie_path_prefix,
	char,
	DTTR_MOVIE_PLAY_INTRO_SIG,
	DTTR_MOVIE_PLAY_INTRO_MASK,
	*(uint32_t *)(match + 9)
)

// clang-format off
#define DTTR_CLEANUP_LEVEL_ASSETS_SIG  "\x6A\x01\xE8????\xE8????\xA1????\x50\xE8????\x8B\x0D????\x51\xE8????\x8B\x15????\x52\xE8????\xA1????\x50\xE8????\x8B\x0D????\x51\xE8????\x83??"
#define DTTR_CLEANUP_LEVEL_ASSETS_MASK "xxx????x????x????xx????xx????xx????xx????xx????x????xx????xx????xx??"
// clang-format on

DTTR_INTEROP_VAR_SIG(
	g_pcdogs_level_asset_0,
	void *,
	DTTR_CLEANUP_LEVEL_ASSETS_SIG,
	DTTR_CLEANUP_LEVEL_ASSETS_MASK,
	*(uint32_t *)(match + 13)
)
DTTR_INTEROP_VAR_SIG(
	g_pcdogs_level_asset_1,
	void *,
	DTTR_CLEANUP_LEVEL_ASSETS_SIG,
	DTTR_CLEANUP_LEVEL_ASSETS_MASK,
	*(uint32_t *)(match + 25)
)
DTTR_INTEROP_VAR_SIG(
	g_pcdogs_level_asset_2,
	void *,
	DTTR_CLEANUP_LEVEL_ASSETS_SIG,
	DTTR_CLEANUP_LEVEL_ASSETS_MASK,
	*(uint32_t *)(match + 37)
)
DTTR_INTEROP_VAR_SIG(
	g_pcdogs_level_asset_3,
	void *,
	DTTR_CLEANUP_LEVEL_ASSETS_SIG,
	DTTR_CLEANUP_LEVEL_ASSETS_MASK,
	*(uint32_t *)(match + 48)
)
DTTR_INTEROP_VAR_SIG(
	g_pcdogs_level_asset_4,
	void *,
	DTTR_CLEANUP_LEVEL_ASSETS_SIG,
	DTTR_CLEANUP_LEVEL_ASSETS_MASK,
	*(uint32_t *)(match + 60)
)

// Resolves cached addresses for global-variable wrappers
static void s_interop_pcdogs_globals_init(HMODULE mod) {
	g_pcdogs_ddraw_object_init(mod);
	g_pcdogs_game_initialized_init(mod);
	g_pcdogs_gamepad_button_flags_init(mod);
	g_pcdogs_joystick_available_init(mod);
	g_pcdogs_keyboard_mapping_buttons_init(mod);
	g_pcdogs_keyboard_mapping_keys_init(mod);
	g_pcdogs_main_window_handle_init(mod);
	g_pcdogs_main_window_handle2_init(mod);
	g_pcdogs_mapping_count_init(mod);
	g_pcdogs_rendering_enabled_init(mod);
	g_pcdogs_should_quit_init(mod);
	g_pcdogs_directory_init(mod);

	g_pcdogs_level_asset_0_init(mod);
	g_pcdogs_level_asset_1_init(mod);
	g_pcdogs_level_asset_2_init(mod);
	g_pcdogs_level_asset_3_init(mod);
	g_pcdogs_level_asset_4_init(mod);

	g_pcdogs_movie_file_names_init(mod);
	g_pcdogs_movie_path_prefix_init(mod);
}

DTTR_INTEROP_WRAP_CACHED_CC_SIG(
	pcdogs_find_and_load_game_pkg_file,
	__cdecl,
	"\x81\xEC\x10\x01\x00\x00\x57",
	"xxxxxxx",
	match,
	void,
	(void),
	()
)

DTTR_INTEROP_WRAP_CACHED_CC_SIG(
	pcdogs_initialize_game_engine,
	__cdecl,
	"\xE8????\x85\xC0\x75?\x32\xC0",
	"x????xxx?xx",
	match,
	int,
	(void),
	()
)

DTTR_INTEROP_WRAP_CACHED_CC_SIG(
	pcdogs_initialize_graphics_subsystem,
	__cdecl,
	"\xE8????\x8B\x44\x24?\x8B\x4C\x24?\x50",
	"x????xxx?xxx?x",
	match,
	void,
	(HWND hwnd, HINSTANCE h_instance),
	(hwnd, h_instance)
)

DTTR_INTEROP_WRAP_CACHED_CC_SIG(
	pcdogs_initialize_capabilities,
	__cdecl,
	"\xE8????\xA1????\x50\xA3",
	"x????x????xx",
	DTTR_E8_TARGET(match),
	void,
	(void),
	()
)

DTTR_INTEROP_WRAP_CACHED_CC_SIG(
	pcdogs_initialize_window_handle,
	__cdecl,
	"\xA1????\x56\x57\x8B\x7C\x24",
	"x????xxxxx",
	match,
	int,
	(HWND hwnd),
	(hwnd)
)

// Sig matches call site; resolves relative call to function
DTTR_INTEROP_WRAP_CACHED_CC_SIG(
	pcdogs_initialize_game_systems,
	__cdecl,
	"\xE8????\x8D\x54\x24?\x56",
	"x????xxx?x",
	DTTR_E8_TARGET(match),
	void,
	(void),
	()
)

DTTR_INTEROP_WRAP_CACHED_CC_SIG(
	pcdogs_render_frame,
	__cdecl,
	"\x51\x53\xE8????\xA1",
	"xxx????x",
	match,
	int,
	(void),
	()
)

DTTR_INTEROP_WRAP_CACHED_CC_SIG(
	pcdogs_is_key_pressed,
	__cdecl,
	"\xA1????\x33\xC9\x85\xC0\x53",
	"x????xxxxx",
	match,
	unsigned int,
	(char scan_code),
	(scan_code)
)

DTTR_INTEROP_WRAP_CACHED_CC_SIG(
	pcdogs_reset_input_and_state,
	__cdecl,
	"\xE8????\xA1????\x25\xDF\xF4\xFF\xFF",
	"x????x????xxxxx",
	match,
	void,
	(void),
	()
)

DTTR_INTEROP_WRAP_CACHED_CC_SIG(
	pcdogs_take_screenshot,
	__cdecl,
	"\x81\xEC\x04\x01\x00\x00\x56",
	"xxxxxxx",
	match,
	void,
	(void),
	()
)

DTTR_INTEROP_WRAP_CACHED_CC_SIG(
	pcdogs_malloc,
	__cdecl,
	"\xFF\x35????\xFF\x74\x24",
	"xx????xxx",
	match,
	void *,
	(size_t size),
	(size)
)

static void s_interop_pcdogs_functions_init(DTTR_GameModule mod) {
	pcdogs_find_and_load_game_pkg_file_init(mod);
	pcdogs_initialize_game_engine_init(mod);
	pcdogs_initialize_graphics_subsystem_init(mod);
	pcdogs_initialize_capabilities_init(mod);
	pcdogs_initialize_window_handle_init(mod);
	pcdogs_initialize_game_systems_init(mod);
	pcdogs_render_frame_init(mod);
	pcdogs_is_key_pressed_init(mod);
	pcdogs_reset_input_and_state_init(mod);
	pcdogs_take_screenshot_init(mod);
	pcdogs_malloc_init(mod);
}

#endif // DTTR_GAME_FUNCTIONS_H
