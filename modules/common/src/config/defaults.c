#include "config_internal.h"
#include <dttr_config.h>
#include <dttr_log.h>
#include <dttr_path.h>

#include <string.h>

static const DTTR_Config default_config = {
	.schema_major_version = DTTR_CONFIG_SCHEMA_MAJOR_VERSION,
	.log_level = DTTR_DEFAULT_LOG_LEVEL,
	.minidump_type = DTTR_DEFAULT_MINIDUMP_TYPE,
	.show_crash_popup = true,
	.log_file_path = "dttr.log",
	.pcdogs_path = "",
	.saves_path = "saves",
	.skip_intro_movies = false,
	.prevent_title_exit = true,
	.scaling_fit = DTTR_SCALING_MODE_LETTERBOX,
	.scaling_method = DTTR_SCALING_METHOD_LOGICAL,
	.graphics_api = DTTR_GRAPHICS_API_AUTO,
	.update_rate_limiter = false,
	.update_rate_limiter_cap = DTTR_CONFIG_DEFAULT_UPDATE_RATE_LIMITER_CAP,
	.vertex_precision = DTTR_VERTEX_PRECISION_NATIVE,
	.sprite_smooth = true,
	.present_filter = SDL_GPU_FILTER_LINEAR,
	.window_width = WINDOW_WIDTH,
	.window_height = WINDOW_HEIGHT,
	.msaa_samples = 2,
	.generate_texture_mipmaps = true,
	.fullscreen = false,
	.hot_reload = false,
	.mss_sample_gain = 1.0f,
	.mss_simulate_directsound_delay = false,
	.gamepad_enabled = true,
	.gamepad_analog_remap = true,
	.gamepad_index = 0,
	.gamepad_axes =
		{
			[DTTR_GAMEPAD_AXIS_IDX_STICK_X] = SDL_GAMEPAD_AXIS_LEFTX,
			[DTTR_GAMEPAD_AXIS_IDX_STICK_Y] = SDL_GAMEPAD_AXIS_LEFTY,
			[DTTR_GAMEPAD_AXIS_IDX_CAMERA_RZ] = SDL_GAMEPAD_AXIS_RIGHTX,
			[DTTR_GAMEPAD_AXIS_IDX_CAMERA_RZ_ALT] = DTTR_GAMEPAD_MAPPING_NONE,
		},
	.gamepad_axis_deadzone =
		{
			[DTTR_GAMEPAD_AXIS_IDX_STICK_X] = 333,
			[DTTR_GAMEPAD_AXIS_IDX_STICK_Y] = 333,
			[DTTR_GAMEPAD_AXIS_IDX_CAMERA_RZ] = 600,
			[DTTR_GAMEPAD_AXIS_IDX_CAMERA_RZ_ALT] = 600,
		},
	.gamepad_axis_sensitivity =
		{
			[DTTR_GAMEPAD_AXIS_IDX_STICK_X] = 100,
			[DTTR_GAMEPAD_AXIS_IDX_STICK_Y] = 100,
			[DTTR_GAMEPAD_AXIS_IDX_CAMERA_RZ] = 100,
			[DTTR_GAMEPAD_AXIS_IDX_CAMERA_RZ_ALT] = 100,
		},
};

DTTR_Config dttr_config;

typedef struct {
	const char *key;
	const char *label;
	int native_config_index;
	uint32_t button_mask;
} config_control_action;

#define CONTROL_ACTION_ROW(key, label, native_config_index, button_mask)                 \
	{key, label, native_config_index, button_mask},
static const config_control_action CONTROL_ACTIONS[] = {
	CONFIG_CONTROL_ACTIONS(CONTROL_ACTION_ROW)
};
#undef CONTROL_ACTION_ROW

const char *DTTR_Config_ControlActionKey(int index) {
	if (index < 0 || index >= (int)SDL_arraysize(CONTROL_ACTIONS)) {
		return NULL;
	}

	return CONTROL_ACTIONS[index].key;
}

int DTTR_Config_ControlActionIndex(const char *key) {
	if (!key || !key[0]) {
		return -1;
	}

	for (int i = 0; i < (int)SDL_arraysize(CONTROL_ACTIONS); i++) {
		if (strcmp(CONTROL_ACTIONS[i].key, key) == 0) {
			return i;
		}
	}

	return -1;
}

const char *DTTR_Config_ControlActionLabel(int index) {
	if (index < 0 || index >= (int)SDL_arraysize(CONTROL_ACTIONS)) {
		return NULL;
	}

	return CONTROL_ACTIONS[index].label;
}

int DTTR_Config_ControlActionNativeConfigIndex(int index) {
	if (index < 0 || index >= (int)SDL_arraysize(CONTROL_ACTIONS)) {
		return -1;
	}

	return CONTROL_ACTIONS[index].native_config_index;
}

bool DTTR_Config_ControlActionInGameBindable(int index) {
	const int native_index = DTTR_Config_ControlActionNativeConfigIndex(index);
	return native_index >= 0 && native_index < CONFIG_IN_GAME_CONTROL_ACTION_COUNT;
}

uint32_t DTTR_Config_ControlActionButtonMask(int index) {
	if (index < 0 || index >= (int)SDL_arraysize(CONTROL_ACTIONS)) {
		return 0;
	}

	return CONTROL_ACTIONS[index].button_mask;
}

bool DTTR_Config_ControlBindingsChanged(
	const DTTR_Config *current,
	const DTTR_Config *base
) {
	return current && base
		   && memcmp(
				  current->control_bindings,
				  base->control_bindings,
				  sizeof(current->control_bindings)
			  ) != 0;
}

static int find_disabled_mod(const DTTR_Config *config, const char *mod_filename) {
	if (!config || !mod_filename || !mod_filename[0]) {
		return -1;
	}

	const size_t mod_filename_len = strlen(mod_filename);
	for (int i = 0; i < config->disabled_mod_count; i++) {
		const char *disabled_mod = config->disabled_mods[i];
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

static DTTR_Result add_disabled_mod(DTTR_Config *config, const char *mod_filename) {
	if (!config || !mod_filename || !mod_filename[0]) {
		return config_result(
			DTTR_ERR_INVALID_ARGUMENT,
			"Config and mod filename are required."
		);
	}

	if (config->disabled_mod_count >= DTTR_CONFIG_DISABLED_MODS_MAX) {
		return config_result(DTTR_ERR_OUT_OF_MEMORY, "Disabled mod list is full.");
	}

	if (!DTTR_Path_CopyString(
			config->disabled_mods[config->disabled_mod_count],
			sizeof(config->disabled_mods[config->disabled_mod_count]),
			mod_filename
		)) {
		return config_result(DTTR_ERR_INVALID_ARGUMENT, "Mod filename is too long.");
	}

	config->disabled_mod_count++;
	return config_ok();
}

DTTR_Result DTTR_Config_SetModEnabled(
	DTTR_Config *config,
	const char *mod_filename,
	bool enabled
) {
	if (!config || !mod_filename || !mod_filename[0]) {
		return config_result(
			DTTR_ERR_INVALID_ARGUMENT,
			"Config and mod filename are required."
		);
	}

	const int index = find_disabled_mod(config, mod_filename);
	if (enabled && index >= 0) {
		remove_disabled_mod(config, index);
	}

	if (enabled || index >= 0) {
		return config_ok();
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

const char *DTTR_Config_GraphicsAPIName(DTTR_GraphicsAPI api) {
	return config_format_graphics_api(api);
}

void DTTR_Config_SetDefaults(DTTR_Config *config) {
	if (!config) {
		return;
	}

	*config = default_config;
	for (int action = 0; action < DTTR_CONFIG_CONTROL_ACTION_COUNT; action++) {
		config->control_bindings[action] = DTTR_CONFIG_CONTROL_BINDING_NONE;
	}
}
