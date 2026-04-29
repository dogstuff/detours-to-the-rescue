#include "config_internal.h"
#include "log.h"
#include <dttr_config.h>

#define DTTR_GAMEPAD_AXIS_DEFAULTS                                                       \
	.m_gamepad_axes = { \
		[DTTR_GAMEPAD_AXIS_IDX_STICK_X]  = SDL_GAMEPAD_AXIS_LEFTX, \
		[DTTR_GAMEPAD_AXIS_IDX_STICK_Y]  = SDL_GAMEPAD_AXIS_LEFTY, \
		[DTTR_GAMEPAD_AXIS_IDX_CAMERA_RZ] = SDL_GAMEPAD_AXIS_RIGHTX, \
	}, \
	.m_gamepad_axis_deadzone = { \
		[DTTR_GAMEPAD_AXIS_IDX_STICK_X]  = 700, \
		[DTTR_GAMEPAD_AXIS_IDX_STICK_Y]  = 700, \
		[DTTR_GAMEPAD_AXIS_IDX_CAMERA_RZ] = 700, \
	}

static const DTTR_Config s_default_config = {
	.m_log_level = DTTR_DEFAULT_LOG_LEVEL,
	.m_minidump_type = DTTR_DEFAULT_MINIDUMP_TYPE,
	.m_saves_path = "./saves",
	.m_scaling_fit = DTTR_SCALING_MODE_LETTERBOX,
	.m_scaling_method = DTTR_SCALING_METHOD_LOGICAL,
	.m_graphics_api = DTTR_GRAPHICS_API_AUTO,
	.m_vertex_precision = DTTR_VERTEX_PRECISION_SUBPIXEL,
	.m_sprite_smooth = true,
	.m_present_filter = SDL_GPU_FILTER_LINEAR,
	.m_window_width = WINDOW_WIDTH,
	.m_window_height = WINDOW_HEIGHT,
	.m_msaa_samples = 2,
	.m_texture_upload_sync = false,
	.m_generate_texture_mipmaps = true,
	.m_fullscreen = false,
	.m_mss_sdl_enabled = true,
	.m_mss_sample_gain = 1.0f,
	.m_mss_sample_preemphasis = 0.0f,
	.m_gamepad_enabled = true,
	.m_gamepad_index = 0,
	DTTR_GAMEPAD_AXIS_DEFAULTS,
};

DTTR_Config g_dttr_config;

static void s_set_default_button_map(int *map) {
	for (int i = 0; i < DTTR_GAMEPAD_SOURCE_COUNT; i++) {
		map[i] = DTTR_GAMEPAD_MAPPING_NONE;
	}

	map[SDL_GAMEPAD_BUTTON_SOUTH] = PCDOGS_GAMEPAD_IDX_BTN_0;
	map[SDL_GAMEPAD_BUTTON_EAST] = PCDOGS_GAMEPAD_IDX_BTN_1;
	map[SDL_GAMEPAD_BUTTON_WEST] = PCDOGS_GAMEPAD_IDX_BTN_2;
	map[SDL_GAMEPAD_BUTTON_NORTH] = PCDOGS_GAMEPAD_IDX_BTN_3;
	map[SDL_GAMEPAD_BUTTON_LEFT_SHOULDER] = PCDOGS_GAMEPAD_IDX_BTN_4;
	map[SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER] = PCDOGS_GAMEPAD_IDX_BTN_5;
	map[SDL_GAMEPAD_BUTTON_BACK] = PCDOGS_GAMEPAD_IDX_BTN_6;
	map[SDL_GAMEPAD_BUTTON_LEFT_STICK] = PCDOGS_GAMEPAD_IDX_BTN_7;
	map[SDL_GAMEPAD_BUTTON_START] = PCDOGS_GAMEPAD_IDX_BTN_8;
	map[SDL_GAMEPAD_BUTTON_RIGHT_STICK] = PCDOGS_GAMEPAD_IDX_BTN_9;
	map[SDL_GAMEPAD_BUTTON_GUIDE] = PCDOGS_GAMEPAD_IDX_BTN_10;
	map[SDL_GAMEPAD_BUTTON_DPAD_UP] = PCDOGS_GAMEPAD_IDX_BTN_11;
	map[SDL_GAMEPAD_BUTTON_DPAD_LEFT] = PCDOGS_GAMEPAD_IDX_BTN_12;
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
