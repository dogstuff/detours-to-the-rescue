#ifndef DTTR_CONFIG_H
#define DTTR_CONFIG_H

#include <stdbool.h>
#include <stdint.h>
#include <windows.h>

#include <SDL3/SDL.h>

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480

typedef enum {
	DTTR_SCALING_MODE_LETTERBOX = 0,
	DTTR_SCALING_MODE_STRETCH = 1,
	DTTR_SCALING_MODE_INTEGER = 2,
} DTTR_ScalingMode;

typedef enum {
	DTTR_SCALING_METHOD_PRESENT = 0,
	DTTR_SCALING_METHOD_LOGICAL = 1,
} DTTR_ScalingMethod;

typedef enum {
	DTTR_MINIDUMP_NORMAL = 0,
	DTTR_MINIDUMP_DETAILED = 1,
} DTTR_MinidumpType;

typedef enum {
	DTTR_GRAPHICS_API_AUTO = 0,
	DTTR_GRAPHICS_API_VULKAN = 1,
	DTTR_GRAPHICS_API_DIRECT3D12 = 2,
	DTTR_GRAPHICS_API_METAL = 3,
} DTTR_GraphicsApi;

typedef enum {
	DTTR_VERTEX_PRECISION_NATIVE = 0,
	DTTR_VERTEX_PRECISION_SUBPIXEL = 1,
} DTTR_VertexPrecision;

#define DTTR_GAMEPAD_MAPPING_NONE (-1)
#define DTTR_GAMEPAD_TRIGGER_THRESHOLD 300

#define DTTR_GAMEPAD_SOURCE_TRIGGER_LEFT (SDL_GAMEPAD_BUTTON_COUNT)
#define DTTR_GAMEPAD_SOURCE_TRIGGER_RIGHT (SDL_GAMEPAD_BUTTON_COUNT + 1)
#define DTTR_GAMEPAD_SOURCE_COUNT (SDL_GAMEPAD_BUTTON_COUNT + 2)

#define DTTR_GAMEPAD_AXIS_MAPPING_COUNT 3
#define DTTR_GAMEPAD_AXIS_IDX_STICK_X 0
#define DTTR_GAMEPAD_AXIS_IDX_STICK_Y 1
#define DTTR_GAMEPAD_AXIS_IDX_CAMERA_RZ 2

// These are the gamepad mapping indices used by original game input tables
#define PCDOGS_GAMEPAD_IDX_UP 0
#define PCDOGS_GAMEPAD_IDX_DOWN 1
#define PCDOGS_GAMEPAD_IDX_LEFT 2
#define PCDOGS_GAMEPAD_IDX_RIGHT 3
#define PCDOGS_GAMEPAD_IDX_POV_UP 4
#define PCDOGS_GAMEPAD_IDX_POV_DOWN 5
#define PCDOGS_GAMEPAD_IDX_BTN_0 6
#define PCDOGS_GAMEPAD_IDX_BTN_1 7
#define PCDOGS_GAMEPAD_IDX_BTN_2 8
#define PCDOGS_GAMEPAD_IDX_BTN_3 9
#define PCDOGS_GAMEPAD_IDX_BTN_4 10
#define PCDOGS_GAMEPAD_IDX_BTN_5 11
#define PCDOGS_GAMEPAD_IDX_BTN_6 12
#define PCDOGS_GAMEPAD_IDX_BTN_7 13
#define PCDOGS_GAMEPAD_IDX_BTN_8 14
#define PCDOGS_GAMEPAD_IDX_BTN_9 15
#define PCDOGS_GAMEPAD_IDX_BTN_10 16
#define PCDOGS_GAMEPAD_IDX_BTN_11 17
#define PCDOGS_GAMEPAD_IDX_BTN_12 18

typedef struct {
	// These are general settings loaded from the config file
	int m_log_level;
	DTTR_MinidumpType m_minidump_type;
	char m_pcdogs_path[MAX_PATH];
	char m_saves_path[MAX_PATH];
	// These are graphics presentation settings loaded from the config file
	DTTR_ScalingMode m_scaling_fit;
	DTTR_ScalingMethod m_scaling_method;
	DTTR_GraphicsApi m_graphics_api;
	DTTR_VertexPrecision m_vertex_precision;
	bool m_sprite_smooth;
	SDL_GPUFilter m_present_filter;
	int m_window_width;
	int m_window_height;
	int m_msaa_samples;
	bool m_texture_upload_sync;
	bool m_generate_texture_mipmaps;
	bool m_fullscreen;
	bool m_gamepad_enabled;
	int m_gamepad_index;
	int m_gamepad_button_map[DTTR_GAMEPAD_SOURCE_COUNT];
	int m_gamepad_axes[DTTR_GAMEPAD_AXIS_MAPPING_COUNT];
	int m_gamepad_axis_deadzone[DTTR_GAMEPAD_AXIS_MAPPING_COUNT];
} DTTR_Config;

extern DTTR_Config g_dttr_config;

/// Resets a config object to built-in defaults
void dttr_config_set_defaults(DTTR_Config *config);
/// Loads config values from a JSON file into the global config object
bool dttr_config_load(const char *filename);
/// Saves config values back to a JSONC file, preserving comments and formatting
bool dttr_config_save(const char *filename, const DTTR_Config *config);

#define DTTR_CONFIG_FILENAME "dttr.jsonc"

#endif
