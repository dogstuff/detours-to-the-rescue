#ifndef DTTR_CONFIG_H
#define DTTR_CONFIG_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <windows.h>

#include <SDL3/SDL.h>

#include <dttr_result.h>

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
} DTTR_GraphicsAPI;

typedef enum {
	DTTR_VERTEX_PRECISION_NATIVE = 0,
	DTTR_VERTEX_PRECISION_SUBPIXEL = 1,
} DTTR_VertexPrecision;

#define DTTR_GAMEPAD_MAPPING_NONE (-1)

#define DTTR_CONFIG_SCHEMA_MAJOR_VERSION 1
#define DTTR_CONFIG_DISABLED_MODS_MAX 32
#define DTTR_CONFIG_MOD_CONFIGS_MAX 32
#define DTTR_CONFIG_MOD_VALUES_MAX 256
#define DTTR_CONFIG_MOD_ID_MAX 64
#define DTTR_CONFIG_MOD_FIELD_ID_MAX 64
#define DTTR_CONFIG_MOD_STRING_MAX 256
#define DTTR_MODS_SHADOW_PREFIX "_dttr_hot_"

#define DTTR_GAMEPAD_AXIS_MAPPING_COUNT 3
#define DTTR_GAMEPAD_AXIS_IDX_STICK_X 0
#define DTTR_GAMEPAD_AXIS_IDX_STICK_Y 1
#define DTTR_GAMEPAD_AXIS_IDX_CAMERA_RZ 2

#define DTTR_CONFIG_CONTROL_BINDING_NONE (-1)
#define DTTR_CONFIG_CONTROL_ACTION_COUNT 12
#define DTTR_CONFIG_CONTROL_CODE_KEYPAD_ENTER 0xE8
#define DTTR_CONFIG_CONTROL_CODE_SCANCODE_BASE 0x100
#define DTTR_CONFIG_CONTROL_CODE_NATIVE_GAMEPAD_BASE 0x3E8
#define DTTR_CONFIG_CONTROL_CODE_NATIVE_GAMEPAD_COUNT 0x13
#define DTTR_CONFIG_CONTROL_CODE_SDL_GAMEPAD_BUTTON_BASE 0x400
#define DTTR_CONFIG_CONTROL_CODE_SDL_GAMEPAD_BUTTON_COUNT 0x100

typedef enum {
	DTTR_CONFIG_MOD_VALUE_BOOL = 0,
	DTTR_CONFIG_MOD_VALUE_INT = 1,
	DTTR_CONFIG_MOD_VALUE_FLOAT = 2,
	DTTR_CONFIG_MOD_VALUE_STRING = 3,
} DTTR_ConfigModValueType;

typedef struct {
	char mod_id[DTTR_CONFIG_MOD_ID_MAX];
	uint32_t schema_version;
} DTTR_ConfigModConfig;

typedef struct {
	int mod_index;
	char field_id[DTTR_CONFIG_MOD_FIELD_ID_MAX];
	DTTR_ConfigModValueType value_type;
	union {
		bool bool_value;
		int int_value;
		float float_value;
		char string_value[DTTR_CONFIG_MOD_STRING_MAX];
	};
} DTTR_ConfigModValue;

typedef struct {
	DTTR_ConfigModValueType value_type;
	bool bool_value;
	int int_value;
	float float_value;
	const char *string_value;
} DTTR_ConfigModDefault;

/// Copies value into out, succeeding only when it fits with its terminator. Returns
/// false on a null buffer/source or on truncation; callers treat truncation as a hard
/// failure rather than silently storing a clipped string.
static inline bool DTTR_Config_StrCopyChecked(
	char *out,
	size_t out_size,
	const char *value
) {
	return out && value && SDL_strlcpy(out, value, out_size) < out_size;
}

typedef struct {
	int schema_major_version;
	int log_level;
	DTTR_MinidumpType minidump_type;
	bool show_crash_popup;
	char log_file_path[MAX_PATH];
	char pcdogs_path[MAX_PATH];
	char saves_path[MAX_PATH];
	bool skip_intro_movies;
	DTTR_ScalingMode scaling_fit;
	DTTR_ScalingMethod scaling_method;
	DTTR_GraphicsAPI graphics_api;
	DTTR_VertexPrecision vertex_precision;
	bool sprite_smooth;
	SDL_GPUFilter present_filter;
	int window_width;
	int window_height;
	int msaa_samples;
	bool generate_texture_mipmaps;
	bool fullscreen;
	bool hot_reload;
	int disabled_mod_count;
	char disabled_mods[DTTR_CONFIG_DISABLED_MODS_MAX][MAX_PATH];
	int mod_config_count;
	DTTR_ConfigModConfig mod_configs[DTTR_CONFIG_MOD_CONFIGS_MAX];
	int mod_value_count;
	DTTR_ConfigModValue mod_values[DTTR_CONFIG_MOD_VALUES_MAX];
	float mss_sample_gain;
	bool mss_simulate_directsound_delay;
	bool gamepad_enabled;
	bool gamepad_analog_remap;
	int gamepad_index;
	int gamepad_axes[DTTR_GAMEPAD_AXIS_MAPPING_COUNT];
	int gamepad_axis_deadzone[DTTR_GAMEPAD_AXIS_MAPPING_COUNT];
	int gamepad_axis_sensitivity[DTTR_GAMEPAD_AXIS_MAPPING_COUNT];
	int control_bindings[DTTR_CONFIG_CONTROL_ACTION_COUNT];
} DTTR_Config;

extern DTTR_Config dttr_config;

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
} DTTR_ConfigChoiceList;

/// Returns the config token for a graphics API selection.
const char *DTTR_Config_GraphicsAPIName(DTTR_GraphicsAPI api);

int DTTR_Config_SchemaCount();
const DTTR_ConfigFieldSpec *DTTR_Config_SchemaGet(int index);
bool DTTR_Config_FieldChanged(
	const DTTR_Config *current,
	const DTTR_Config *base,
	const DTTR_ConfigFieldSpec *spec
);
bool DTTR_Config_SchemaChanged(const DTTR_Config *current, const DTTR_Config *base);

int DTTR_Config_ChoiceCount(DTTR_ConfigChoiceList list);
const DTTR_ConfigChoice *DTTR_Config_ChoiceGet(DTTR_ConfigChoiceList list, int index);
const DTTR_ConfigChoice *DTTR_Config_Choices(DTTR_ConfigChoiceList list, int *count);

const char *DTTR_Config_ControlActionKey(int index);
int DTTR_Config_ControlActionIndex(const char *key);
const char *DTTR_Config_ControlActionLabel(int index);
int DTTR_Config_ControlActionNativeConfigIndex(int index);
bool DTTR_Config_ControlActionInGameBindable(int index);
uint32_t DTTR_Config_ControlActionButtonMask(int index);
bool DTTR_Config_ControlBindingsChanged(
	const DTTR_Config *current,
	const DTTR_Config *base
);

bool DTTR_Config_IsModDisabled(const DTTR_Config *config, const char *mod_filename);

DTTR_Result DTTR_Config_SetModEnabled(
	DTTR_Config *config,
	const char *mod_filename,
	bool enabled
);
bool DTTR_Config_DisabledModsChanged(const DTTR_Config *current, const DTTR_Config *base);

DTTR_Result DTTR_Config_GetModSchemaVersion(
	const DTTR_Config *config,
	const char *mod_id,
	uint32_t *out_version
);

DTTR_Result DTTR_Config_SetModSchemaVersion(
	DTTR_Config *config,
	const char *mod_id,
	uint32_t schema_version
);

DTTR_Result DTTR_Config_GetModBool(
	const DTTR_Config *config,
	const char *mod_id,
	const char *field_id,
	bool *out_value
);

DTTR_Result DTTR_Config_SetModBool(
	DTTR_Config *config,
	const char *mod_id,
	const char *field_id,
	bool value
);

DTTR_Result DTTR_Config_GetModInt(
	const DTTR_Config *config,
	const char *mod_id,
	const char *field_id,
	int *out_value
);

DTTR_Result DTTR_Config_SetModInt(
	DTTR_Config *config,
	const char *mod_id,
	const char *field_id,
	int value
);

DTTR_Result DTTR_Config_GetModFloat(
	const DTTR_Config *config,
	const char *mod_id,
	const char *field_id,
	float *out_value
);

DTTR_Result DTTR_Config_SetModFloat(
	DTTR_Config *config,
	const char *mod_id,
	const char *field_id,
	float value
);

DTTR_Result DTTR_Config_GetModString(
	const DTTR_Config *config,
	const char *mod_id,
	const char *field_id,
	char *out_value,
	size_t out_size
);

DTTR_Result DTTR_Config_SetModString(
	DTTR_Config *config,
	const char *mod_id,
	const char *field_id,
	const char *value
);

DTTR_Result DTTR_Config_ApplyModFieldDefault(
	DTTR_Config *config,
	const char *mod_id,
	const char *field_id,
	const DTTR_ConfigModDefault *def,
	bool overwrite
);

bool DTTR_Config_ModConfigsChanged(const DTTR_Config *current, const DTTR_Config *base);

bool DTTR_Config_ModFieldChanged(
	const DTTR_Config *current,
	const DTTR_Config *base,
	const char *mod_id,
	const char *field_id
);

/// Resets a config object to built-in defaults.
void DTTR_Config_SetDefaults(DTTR_Config *config);

/// Loads config values from a strict JSON file into the global config object.
bool DTTR_Config_Load(const char *filename);

/// Returns details from the most recent config load failure, or NULL when none exist.
const char *DTTR_Config_LastError();

/// Saves config values back to a strict JSON file.
bool DTTR_Config_Save(const char *filename, const DTTR_Config *config);

#define DTTR_CONFIG_FILENAME "dttr.json"

#endif
