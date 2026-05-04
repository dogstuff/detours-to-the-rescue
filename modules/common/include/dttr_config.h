#ifndef DTTR_CONFIG_H
#define DTTR_CONFIG_H

#include <stdbool.h>
#include <stddef.h>
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

#define DTTR_DRIVER_AUTO "auto"
#define DTTR_DRIVER_VULKAN "vulkan"
#define DTTR_DRIVER_DIRECT3D12 "direct3d12"
#define DTTR_DRIVER_DIRECT3D12_SHORT "d3d12"
#define DTTR_DRIVER_OPENGL "opengl"

typedef enum {
	DTTR_GRAPHICS_API_AUTO = 0,
	DTTR_GRAPHICS_API_VULKAN = 1,
	DTTR_GRAPHICS_API_DIRECT3D12 = 2,
	DTTR_GRAPHICS_API_OPENGL = 3,
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

#define DTTR_CONFIG_SCHEMA_MAJOR_VERSION 1
#define DTTR_CONFIG_DISABLED_COMPONENTS_MAX 32

#define DTTR_GAMEPAD_AXIS_MAPPING_COUNT 3
#define DTTR_GAMEPAD_AXIS_IDX_STICK_X 0
#define DTTR_GAMEPAD_AXIS_IDX_STICK_Y 1
#define DTTR_GAMEPAD_AXIS_IDX_CAMERA_RZ 2

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
	int m_schema_major_version;
	int m_log_level;
	DTTR_MinidumpType m_minidump_type;
	char m_log_file_path[MAX_PATH];
	char m_pcdogs_path[MAX_PATH];
	char m_saves_path[MAX_PATH];
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
	bool m_mss_sdl_enabled;
	bool m_hot_reload;
	int m_disabled_component_count;
	char m_disabled_components[DTTR_CONFIG_DISABLED_COMPONENTS_MAX][MAX_PATH];
	float m_mss_sample_gain;
	float m_mss_sample_preemphasis;
	bool m_gamepad_enabled;
	int m_gamepad_index;
	int m_gamepad_button_map[DTTR_GAMEPAD_SOURCE_COUNT];
	int m_gamepad_axes[DTTR_GAMEPAD_AXIS_MAPPING_COUNT];
	int m_gamepad_axis_deadzone[DTTR_GAMEPAD_AXIS_MAPPING_COUNT];
} DTTR_Config;

extern DTTR_Config g_dttr_config;

typedef enum {
	DTTR_CONFIG_VALUE_BOOL = 0,
	DTTR_CONFIG_VALUE_SCALING_FIT = 1,
	DTTR_CONFIG_VALUE_SCALING_METHOD = 2,
	DTTR_CONFIG_VALUE_GRAPHICS_API = 3,
	DTTR_CONFIG_VALUE_INT = 4,
	DTTR_CONFIG_VALUE_FLOAT = 5,
	DTTR_CONFIG_VALUE_PRESENT_FILTER = 6,
	DTTR_CONFIG_VALUE_LOG_LEVEL = 7,
	DTTR_CONFIG_VALUE_MINIDUMP_TYPE = 8,
	DTTR_CONFIG_VALUE_STRING = 9,
	DTTR_CONFIG_VALUE_VERTEX_PRECISION = 10,
	DTTR_CONFIG_VALUE_GAMEPAD_AXIS = 11,
} DTTR_ConfigValueType;

typedef struct {
	const char *section;
	const char *key;
	ptrdiff_t offset;
	size_t size;
	DTTR_ConfigValueType value_type;
} DTTR_ConfigFieldSpec;

typedef struct {
	const char *label;
	int value;
} DTTR_ConfigChoice;

typedef enum {
	DTTR_CONFIG_CHOICES_LOG_LEVEL,
	DTTR_CONFIG_CHOICES_MINIDUMP_TYPE,
	DTTR_CONFIG_CHOICES_GRAPHICS_API,
	DTTR_CONFIG_CHOICES_SCALING_FIT,
	DTTR_CONFIG_CHOICES_SCALING_METHOD,
	DTTR_CONFIG_CHOICES_PRESENT_FILTER,
	DTTR_CONFIG_CHOICES_VERTEX_PRECISION,
	DTTR_CONFIG_CHOICES_GAMEPAD_AXIS,
	DTTR_CONFIG_CHOICES_GAME_ACTION,
} DTTR_ConfigChoiceList;

/// Returns the config token for a graphics API selection.
const char *dttr_config_graphics_api_name(DTTR_GraphicsApi api);

int dttr_config_schema_count(void);
const DTTR_ConfigFieldSpec *dttr_config_schema_get(int index);
bool dttr_config_field_changed(
	const DTTR_Config *current,
	const DTTR_Config *base,
	const DTTR_ConfigFieldSpec *spec
);
bool dttr_config_schema_changed(const DTTR_Config *current, const DTTR_Config *base);

int dttr_config_choice_count(DTTR_ConfigChoiceList list);
const DTTR_ConfigChoice *dttr_config_choice_get(DTTR_ConfigChoiceList list, int index);
const DTTR_ConfigChoice *dttr_config_choices(DTTR_ConfigChoiceList list, int *count);

void dttr_config_clear_gamepad_button_map(int *map);

bool dttr_config_is_component_disabled(
	const DTTR_Config *config,
	const char *component_filename
);
bool dttr_config_set_component_enabled(
	DTTR_Config *config,
	const char *component_filename,
	bool enabled
);
bool dttr_config_disabled_components_changed(
	const DTTR_Config *current,
	const DTTR_Config *base
);

/// Resets a config object to built-in defaults.
void dttr_config_set_defaults(DTTR_Config *config);

/// Loads config values from a strict JSON file into the global config object.
bool dttr_config_load(const char *filename);

/// Saves config values back to a strict JSON file.
bool dttr_config_save(const char *filename, const DTTR_Config *config);

#define DTTR_CONFIG_FILENAME "dttr.json"

#endif
