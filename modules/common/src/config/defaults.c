#include "config_internal.h"
#include <dttr_config.h>
#include <dttr_log.h>
#include <dttr_path.h>

#include <string.h>

typedef struct {
	int source;
	int action;
} S_DefaultGamepadBinding;

static const DTTR_Config s_default_config = {
	.m_schema_major_version = DTTR_CONFIG_SCHEMA_MAJOR_VERSION,
	.m_log_level = DTTR_DEFAULT_LOG_LEVEL,
	.m_minidump_type = DTTR_DEFAULT_MINIDUMP_TYPE,
	.m_log_file_path = "dttr.log",
	.m_saves_path = "saves",
	.m_scaling_fit = DTTR_SCALING_MODE_LETTERBOX,
	.m_scaling_method = DTTR_SCALING_METHOD_LOGICAL,
	.m_graphics_api = DTTR_GRAPHICS_API_AUTO,
	.m_vertex_precision = DTTR_VERTEX_PRECISION_NATIVE,
	.m_sprite_smooth = true,
	.m_present_filter = SDL_GPU_FILTER_LINEAR,
	.m_window_width = WINDOW_WIDTH,
	.m_window_height = WINDOW_HEIGHT,
	.m_msaa_samples = 2,
	.m_texture_upload_sync = false,
	.m_generate_texture_mipmaps = true,
	.m_fullscreen = false,
	.m_hot_reload = false,
	.m_mss_sample_gain = 1.0f,
	.m_mss_sample_preemphasis = 0.0f,
	.m_gamepad_enabled = true,
	.m_gamepad_index = 0,
	.m_gamepad_axes =
		{
			[DTTR_GAMEPAD_AXIS_IDX_STICK_X] = SDL_GAMEPAD_AXIS_LEFTX,
			[DTTR_GAMEPAD_AXIS_IDX_STICK_Y] = SDL_GAMEPAD_AXIS_LEFTY,
			[DTTR_GAMEPAD_AXIS_IDX_CAMERA_RZ] = SDL_GAMEPAD_AXIS_RIGHTX,
		},
	.m_gamepad_axis_deadzone =
		{
			[DTTR_GAMEPAD_AXIS_IDX_STICK_X] = 700,
			[DTTR_GAMEPAD_AXIS_IDX_STICK_Y] = 700,
			[DTTR_GAMEPAD_AXIS_IDX_CAMERA_RZ] = 700,
		},
};

DTTR_Config g_dttr_config;

static const S_DefaultGamepadBinding s_default_button_map[] = {
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

void dttr_config_clear_gamepad_button_map(int *map) {
	if (!map) {
		return;
	}

	s_config_clear_button_map(map);
}

static int s_find_disabled_component(
	const DTTR_Config *config,
	const char *component_filename
) {
	if (!config || !component_filename || !component_filename[0]) {
		return -1;
	}

	const size_t component_filename_len = strlen(component_filename);
	for (int i = 0; i < config->m_disabled_component_count; i++) {
		const char *const disabled_component = config->m_disabled_components[i];
		const size_t disabled_component_len = strlen(disabled_component);
		if (disabled_component_len == component_filename_len
			&& dttr_path_ascii_ieq_n(
				disabled_component,
				component_filename,
				disabled_component_len
			)) {
			return i;
		}
	}

	return -1;
}

bool dttr_config_is_component_disabled(
	const DTTR_Config *config,
	const char *component_filename
) {
	return s_find_disabled_component(config, component_filename) >= 0;
}

static void s_remove_disabled_component(DTTR_Config *config, int index) {
	if (!config) {
		return;
	}

	const int last_index = config->m_disabled_component_count - 1;
	if (index < 0 || index > last_index) {
		return;
	}

	if (index < last_index) {
		memmove(
			config->m_disabled_components[index],
			config->m_disabled_components[index + 1],
			(size_t)(last_index - index) * sizeof(config->m_disabled_components[0])
		);
	}

	config->m_disabled_component_count = last_index;
	config->m_disabled_components[last_index][0] = '\0';
}

static bool s_add_disabled_component(DTTR_Config *config, const char *component_filename) {
	if (!config || !component_filename || !component_filename[0]) {
		return false;
	}

	if (config->m_disabled_component_count >= DTTR_CONFIG_DISABLED_COMPONENTS_MAX) {
		return false;
	}

	if (!dttr_path_copy_string(
			config->m_disabled_components[config->m_disabled_component_count],
			sizeof(config->m_disabled_components[config->m_disabled_component_count]),
			component_filename
		)) {
		return false;
	}

	config->m_disabled_component_count++;
	return true;
}

bool dttr_config_set_component_enabled(
	DTTR_Config *config,
	const char *component_filename,
	bool enabled
) {
	if (!config || !component_filename || !component_filename[0]) {
		return false;
	}

	const int index = s_find_disabled_component(config, component_filename);
	if (enabled && index >= 0) {
		s_remove_disabled_component(config, index);
	}

	if (enabled || index >= 0) {
		return true;
	}

	return s_add_disabled_component(config, component_filename);
}

bool dttr_config_disabled_components_changed(
	const DTTR_Config *current,
	const DTTR_Config *base
) {
	if (!current || !base) {
		return false;
	}

	if (current->m_disabled_component_count != base->m_disabled_component_count) {
		return true;
	}

	for (int i = 0; i < current->m_disabled_component_count; i++) {
		if (!dttr_config_is_component_disabled(base, current->m_disabled_components[i])) {
			return true;
		}
	}

	return false;
}

static void s_set_default_button_map(int *map) {
	dttr_config_clear_gamepad_button_map(map);

	for (size_t i = 0; i < SDL_arraysize(s_default_button_map); i++) {
		map[s_default_button_map[i].source] = s_default_button_map[i].action;
	}
}

const char *dttr_config_graphics_api_name(DTTR_GraphicsApi api) {
	return s_config_format_graphics_api(api);
}

void dttr_config_set_defaults(DTTR_Config *config) {
	if (!config) {
		return;
	}

	*config = s_default_config;
	s_set_default_button_map(config->m_gamepad_button_map);
}
