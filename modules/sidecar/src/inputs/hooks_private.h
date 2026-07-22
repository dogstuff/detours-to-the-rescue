#ifndef DTTR_INPUTS_HOOKS_PRIVATE_H
#define DTTR_INPUTS_HOOKS_PRIVATE_H

#include <stddef.h>

#include <SDL3/SDL.h>
#include <dttr_config.h>
#include <dttr_mods.h>
#include <dttr_pcdogs.h>
#include <windows.h>

#define DINPUT_BUTTON_PRESSED 0x80

// Sidecar scancodes and SDL gamepad buttons are between Windows vkeys and
// native gamepad buttons.
enum {
	DTTR_INPUTS_KEY_KEYPAD_ENTER = DTTR_CONFIG_CONTROL_CODE_KEYPAD_ENTER,
	DTTR_INPUTS_KEY_SCANCODE_BASE = DTTR_CONFIG_CONTROL_CODE_SCANCODE_BASE,
	DTTR_INPUTS_GAMEPAD_BUTTON_BASE = DTTR_CONFIG_CONTROL_CODE_NATIVE_GAMEPAD_BASE,
	DTTR_INPUTS_GAMEPAD_BUTTON_COUNT = DTTR_CONFIG_CONTROL_CODE_NATIVE_GAMEPAD_COUNT,
	DTTR_INPUTS_SDL_GAMEPAD_BUTTON_BASE = DTTR_CONFIG_CONTROL_CODE_SDL_GAMEPAD_BUTTON_BASE,
	DTTR_INPUTS_SDL_BUTTON_COUNT = DTTR_CONFIG_CONTROL_CODE_SDL_GAMEPAD_BUTTON_COUNT,
};

_Static_assert(
	DTTR_INPUTS_GAMEPAD_BUTTON_BASE + DTTR_INPUTS_GAMEPAD_BUTTON_COUNT
		<= DTTR_INPUTS_SDL_GAMEPAD_BUTTON_BASE,
	"SDL button control codes must not overlap native gamepad codes"
);

typedef enum {
	DTTR_INPUTS_KEY_CODE_NONE,
	DTTR_INPUTS_KEY_CODE_VKEY,
	DTTR_INPUTS_KEY_CODE_SCANCODE,
	DTTR_INPUTS_KEY_CODE_SDL_GAMEPAD,
	DTTR_INPUTS_KEY_CODE_GAMEPAD,
} DTTR_Input_KeyCodeKind;

extern const DTTR_PCDOGS_T_Patch_Spec dttr_sidecar_input_byte_patch_specs[];
extern const size_t dttr_sidecar_input_byte_patch_spec_count;

// This declaration installs the input hook group.
bool dttr_inputs_hooks_init(const DTTR_Mods_Context *ctx);

// This declaration releases the input hook group.
void dttr_inputs_hooks_cleanup(const DTTR_Mods_Context *ctx);

// Bridges DirectInput polling to sidecar input state.
void *__cdecl dttr_inputs_hook_dinput_poll_callback(void *device);
void dttr_inputs_hook_dinput_neutralize_left_stick(bool neutralize);

// These helpers translate control codes used by input hook callbacks.
bool dttr_inputs_vkey_pressed(
	int vkey,
	const bool *keyboard_state,
	int keyboard_state_count
);
bool dttr_inputs_key_code_pressed(
	int key_code,
	const bool *keyboard_state,
	int keyboard_state_count
);
bool dttr_inputs_control_action_pressed(
	int action,
	const bool *keyboard_state,
	int keyboard_state_count
);
bool dttr_inputs_global_vkey_pressed(
	int vkey,
	const bool *keyboard_state,
	int keyboard_state_count
);
bool dttr_inputs_keyboard_scancode_pressed(
	const bool *keyboard_state,
	int keyboard_state_count,
	int scancode
);
int32_t dttr_inputs_key_code_from_scancode(int scancode);
int32_t dttr_inputs_key_code_from_sdl_button(int button);
int32_t dttr_inputs_key_code_from_gamepad_button(SDL_GamepadButton button);
DTTR_Input_KeyCodeKind dttr_inputs_key_code_kind(int32_t key_code);
bool dttr_inputs_key_state_uses_live_state(int32_t key_code);
bool dttr_inputs_controller_button_pressed(int button);
int dttr_inputs_key_code_scancode(int32_t key_code);
int dttr_inputs_key_code_sdl_button(int32_t key_code);
int dttr_inputs_vkey_scancode(int vkey);
const char *dttr_inputs_key_code_name(int32_t key_code);

// Replaces direct async key-state reads.
SHORT __stdcall dttr_inputs_hook_get_async_key_state_callback(int vkey);

// These declarations manage shared key-state hook storage.
bool dttr_inputs_hook_key_state_prepare(const DTTR_Mods_Context *ctx);
void dttr_inputs_hook_key_state_reset();

// Redirects asynchronous key-press checks.
extern DTTR_PCDOGS_F_Input_IsKeyPressedAsync_proto
	dttr_inputs_hook_is_key_pressed_async_original;
BOOL __cdecl dttr_inputs_hook_is_key_pressed_async_callback(int32_t virtual_key);

// Redirects game button-name lookup.
extern DTTR_PCDOGS_F_Input_GetButtonString_proto
	dttr_inputs_hook_get_button_string_original;
char *__cdecl dttr_inputs_hook_get_button_string_callback(int32_t button_code);

// Formats sidecar control labels.
extern DTTR_PCDOGS_F_Input_FormatButtonName_proto
	dttr_inputs_hook_format_button_name_original;
bool dttr_inputs_hook_format_button_name_prepare(const DTTR_Mods_Context *ctx);
char *__cdecl dttr_inputs_hook_format_button_name_callback(
	int32_t control_code,
	uint32_t button_mask
);

// Rewrites default controller mappings after initialization.
extern DTTR_PCDOGS_F_Input_InitializeButtonMappings_proto
	dttr_inputs_hook_initialize_button_mappings_original;
bool dttr_inputs_hook_initialize_button_mappings_prepare(const DTTR_Mods_Context *ctx);
int32_t __cdecl dttr_inputs_hook_initialize_button_mappings_callback();

// These declarations manage shared control-mapping hook storage.
bool dttr_inputs_hook_mapping_prepare(const DTTR_Mods_Context *ctx);
void dttr_inputs_hook_mapping_reset();

// Captures custom SDL button bindings.
extern DTTR_PCDOGS_F_Input_RegisterButtonMapping_proto
	dttr_inputs_hook_register_button_mapping_original;
int32_t __cdecl dttr_inputs_hook_register_button_mapping_callback(
	int32_t control_code,
	uint32_t button_mask
);

// Reapplies custom control mappings after config changes.
extern DTTR_PCDOGS_F_Config_ApplySettings_proto
	dttr_inputs_hook_config_apply_settings_original;
void __cdecl dttr_inputs_hook_config_apply_settings_callback();

// Applies custom button masks after device reads.
extern DTTR_PCDOGS_F_Input_ReadDevices_proto dttr_inputs_hook_read_devices_original;
bool dttr_inputs_hook_read_devices_prepare(const DTTR_Mods_Context *ctx);
void __cdecl dttr_inputs_hook_read_devices_callback(
	int32_t player_index,
	DTTR_PCDOGS_T_Input_State *state
);

// These helpers store custom SDL button mapping masks.
void dttr_inputs_custom_button_mappings_clear();
uint32_t dttr_inputs_custom_button_mapping_mask(int button);
void dttr_inputs_apply_custom_button_mappings(DTTR_PCDOGS_T_Input_State *state);

// These helpers extend controls-menu remapping input.
void dttr_inputs_controls_menu_reset();
void dttr_inputs_controls_menu_handle_event(const SDL_Event *event);
int32_t dttr_inputs_controls_menu_pressed_button(
	int32_t pressed_button,
	int32_t remapping_active
);
int32_t dttr_inputs_controls_menu_pressed_keyboard_controller_button(
	int32_t pressed_button,
	int32_t remapping_active,
	const bool *keyboard_state,
	int keyboard_state_count,
	const bool *gamepad_button_state,
	int gamepad_button_count,
	const bool *joystick_button_state,
	int joystick_button_count
);

// Reads extended controls-menu buttons.
extern DTTR_PCDOGS_F_Input_GetPressedButton_proto
	dttr_inputs_hook_get_pressed_button_original;
bool dttr_inputs_hook_get_pressed_button_prepare(const DTTR_Mods_Context *ctx);
void dttr_inputs_hook_get_pressed_button_reset();
int32_t __cdecl dttr_inputs_hook_get_pressed_button_callback();

// Merges SDL gamepad state into native input.
extern DTTR_PCDOGS_F_Input_ReadGamepad_proto dttr_inputs_hook_read_gamepad_original;
bool dttr_inputs_hook_read_gamepad_prepare(const DTTR_Mods_Context *ctx);
void dttr_inputs_hook_read_gamepad_reset();
void __cdecl dttr_inputs_hook_read_gamepad_callback(DTTR_PCDOGS_T_Input_State *state);

// Routes game rumble through SDL.
extern DTTR_PCDOGS_F_Input_TriggerRumbleIfAllowed_proto dttr_inputs_hook_rumble_original;
bool dttr_inputs_hook_rumble_prepare(const DTTR_Mods_Context *ctx);
void dttr_inputs_hook_rumble_reset();
int32_t __cdecl dttr_inputs_hook_rumble_callback(
	int32_t effect_source_id,
	int32_t strong_feedback,
	int32_t force_magnitude_fixed,
	int32_t duration_units
);

// Stops active SDL rumble when suppression changes.
extern DTTR_PCDOGS_F_Settings_SetRumbleSuppressFlag_proto
	dttr_inputs_hook_set_rumble_suppress_flag_original;
bool dttr_inputs_hook_set_rumble_suppress_flag_prepare(const DTTR_Mods_Context *ctx);
int32_t __cdecl dttr_inputs_hook_set_rumble_suppress_flag_callback(char suppress_rumble);

#endif // DTTR_INPUTS_HOOKS_PRIVATE_H
