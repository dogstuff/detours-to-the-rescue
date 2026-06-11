#include "config_internal.h"
#include <dttr_config.h>
#include <dttr_log.h>
#include <dttr_path.h>

#include <string.h>

typedef struct {
	int source;
	int action;
} default_gamepad_binding;

static const DTTR_Config default_config = {
	.schema_major_version = DTTR_CONFIG_SCHEMA_MAJOR_VERSION,
	.log_level = DTTR_DEFAULT_LOG_LEVEL,
	.minidump_type = DTTR_DEFAULT_MINIDUMP_TYPE,
	.show_crash_popup = true,
	.log_file_path = "dttr.log",
	.pcdogs_path = "",
	.saves_path = "saves",
	.skip_intro_movies = false,
	.scaling_fit = DTTR_SCALING_MODE_LETTERBOX,
	.scaling_method = DTTR_SCALING_METHOD_LOGICAL,
	.graphics_api = DTTR_GRAPHICS_API_AUTO,
	.vertex_precision = DTTR_VERTEX_PRECISION_NATIVE,
	.sprite_smooth = true,
	.present_filter = SDL_GPU_FILTER_LINEAR,
	.window_width = WINDOW_WIDTH,
	.window_height = WINDOW_HEIGHT,
	.msaa_samples = 2,
	.texture_upload_sync = false,
	.generate_texture_mipmaps = true,
	.fullscreen = false,
	.hot_reload = false,
	.mss_sample_gain = 1.0f,
	.mss_sample_preemphasis = 0.0f,
	.gamepad_enabled = true,
	.gamepad_analog_remap = true,
	.gamepad_index = 0,
	.gamepad_axes =
		{
			[DTTR_GAMEPAD_AXIS_IDX_STICK_X] = SDL_GAMEPAD_AXIS_LEFTX,
			[DTTR_GAMEPAD_AXIS_IDX_STICK_Y] = SDL_GAMEPAD_AXIS_LEFTY,
			[DTTR_GAMEPAD_AXIS_IDX_CAMERA_RZ] = SDL_GAMEPAD_AXIS_RIGHTX,
		},
	.gamepad_axis_deadzone =
		{
			[DTTR_GAMEPAD_AXIS_IDX_STICK_X] = 333,
			[DTTR_GAMEPAD_AXIS_IDX_STICK_Y] = 333,
			[DTTR_GAMEPAD_AXIS_IDX_CAMERA_RZ] = 600,
		},
	.gamepad_axis_sensitivity =
		{
			[DTTR_GAMEPAD_AXIS_IDX_STICK_X] = 100,
			[DTTR_GAMEPAD_AXIS_IDX_STICK_Y] = 100,
			[DTTR_GAMEPAD_AXIS_IDX_CAMERA_RZ] = 100,
		},
};

DTTR_Config dttr_config;

static const default_gamepad_binding default_button_map[] = {
	{SDL_GAMEPAD_BUTTON_SOUTH, PCDOGS_GAMEPAD_IDX_BTN_0},
	{SDL_GAMEPAD_BUTTON_EAST, PCDOGS_GAMEPAD_IDX_BTN_1},
	{SDL_GAMEPAD_BUTTON_WEST, PCDOGS_GAMEPAD_IDX_BTN_2},
	{SDL_GAMEPAD_BUTTON_NORTH, PCDOGS_GAMEPAD_IDX_BTN_3},
	{SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, PCDOGS_GAMEPAD_IDX_BTN_4},
	{SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, PCDOGS_GAMEPAD_IDX_BTN_5},
	{SDL_GAMEPAD_BUTTON_BACK, PCDOGS_GAMEPAD_IDX_BTN_6},
	{SDL_GAMEPAD_BUTTON_START, PCDOGS_GAMEPAD_IDX_BTN_8},
	{SDL_GAMEPAD_BUTTON_DPAD_UP, PCDOGS_GAMEPAD_IDX_UP},
	{SDL_GAMEPAD_BUTTON_DPAD_DOWN, PCDOGS_GAMEPAD_IDX_DOWN},
	{SDL_GAMEPAD_BUTTON_DPAD_LEFT, PCDOGS_GAMEPAD_IDX_LEFT},
	{SDL_GAMEPAD_BUTTON_DPAD_RIGHT, PCDOGS_GAMEPAD_IDX_RIGHT},
	{DTTR_GAMEPAD_SOURCE_TRIGGER_LEFT, PCDOGS_GAMEPAD_IDX_BTN_4},
	{DTTR_GAMEPAD_SOURCE_TRIGGER_RIGHT, PCDOGS_GAMEPAD_IDX_BTN_5},
};

void DTTR_Config_ClearGamepadButtonMap(int *map) {
	if (!map) {
		return;
	}

	config_clear_button_map(map);
}

static int find_disabled_mod(const DTTR_Config *config, const char *mod_filename) {
	if (!config || !mod_filename || !mod_filename[0]) {
		return -1;
	}

	const size_t mod_filename_len = strlen(mod_filename);
	for (int i = 0; i < config->disabled_mod_count; i++) {
		const char *const disabled_mod = config->disabled_mods[i];
		const size_t disabled_mod_len = strlen(disabled_mod);
		if (disabled_mod_len == mod_filename_len
			&& DTTR_Path_AsciiIeqN(disabled_mod, mod_filename, disabled_mod_len)) {
			return i;
		}
	}

	return -1;
}

bool DTTR_Config_IsModDisabled(const DTTR_Config *config, const char *mod_filename) {
	return find_disabled_mod(config, mod_filename) >= 0;
}

static void remove_disabled_mod(DTTR_Config *config, int index) {
	if (!config) {
		return;
	}

	const int last_index = config->disabled_mod_count - 1;
	if (index < 0 || index > last_index) {
		return;
	}

	if (index < last_index) {
		memmove(
			config->disabled_mods[index],
			config->disabled_mods[index + 1],
			(size_t)(last_index - index) * sizeof(config->disabled_mods[0])
		);
	}

	config->disabled_mod_count = last_index;
	config->disabled_mods[last_index][0] = '\0';
}

static bool add_disabled_mod(DTTR_Config *config, const char *mod_filename) {
	if (!config || !mod_filename || !mod_filename[0]) {
		return false;
	}

	if (config->disabled_mod_count >= DTTR_CONFIG_DISABLED_MODS_MAX) {
		return false;
	}

	if (!DTTR_Path_CopyString(
			config->disabled_mods[config->disabled_mod_count],
			sizeof(config->disabled_mods[config->disabled_mod_count]),
			mod_filename
		)) {
		return false;
	}

	config->disabled_mod_count++;
	return true;
}

bool DTTR_Config_SetModEnabled(
	DTTR_Config *config,
	const char *mod_filename,
	bool enabled
) {
	if (!config || !mod_filename || !mod_filename[0]) {
		return false;
	}

	const int index = find_disabled_mod(config, mod_filename);
	if (enabled && index >= 0) {
		remove_disabled_mod(config, index);
	}

	if (enabled || index >= 0) {
		return true;
	}

	return add_disabled_mod(config, mod_filename);
}

bool DTTR_Config_DisabledModsChanged(const DTTR_Config *current, const DTTR_Config *base) {
	if (!current || !base) {
		return false;
	}

	if (current->disabled_mod_count != base->disabled_mod_count) {
		return true;
	}

	for (int i = 0; i < current->disabled_mod_count; i++) {
		if (!DTTR_Config_IsModDisabled(base, current->disabled_mods[i])) {
			return true;
		}
	}

	return false;
}

static void set_default_button_map(int *map) {
	DTTR_Config_ClearGamepadButtonMap(map);

	for (size_t i = 0; i < SDL_arraysize(default_button_map); i++) {
		map[default_button_map[i].source] = default_button_map[i].action;
	}
}

const char *DTTR_Config_GraphicsAPIName(DTTR_GraphicsApi api) {
	return config_format_graphics_api(api);
}

void DTTR_Config_SetDefaults(DTTR_Config *config) {
	if (!config) {
		return;
	}

	*config = default_config;
	set_default_button_map(config->gamepad_button_map);
}
