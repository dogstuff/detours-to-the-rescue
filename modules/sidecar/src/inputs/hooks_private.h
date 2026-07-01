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

bool dttr_inputs_hooks_init(const DTTR_Mods_Context *ctx);
void dttr_inputs_hooks_cleanup(const DTTR_Mods_Context *ctx);

void *__cdecl dttr_inputs_hook_dinput_poll_callback(void *device);
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
SDL_GamepadButton dttr_inputs_key_code_gamepad_button(int32_t key_code);
int dttr_inputs_vkey_scancode(int vkey);
const char *dttr_inputs_key_code_name(int32_t key_code);
SHORT __stdcall dttr_inputs_hook_get_async_key_state_callback(int vkey);
extern DTTR_PCDOGS_F_Input_IsKeyPressedAsync_proto
	dttr_inputs_hook_is_key_pressed_async_original;
extern DTTR_PCDOGS_F_Input_GetButtonString_proto
	dttr_inputs_hook_get_button_string_original;
extern DTTR_PCDOGS_F_Input_FormatButtonName_proto
	dttr_inputs_hook_format_button_name_original;
bool dttr_inputs_hook_key_state_prepare(const DTTR_Mods_Context *ctx);
bool dttr_inputs_hook_format_button_name_prepare(const DTTR_Mods_Context *ctx);
void dttr_inputs_hook_key_state_reset();
BOOL __cdecl dttr_inputs_hook_is_key_pressed_async_callback(int32_t virtual_key);
char *__cdecl dttr_inputs_hook_get_button_string_callback(int32_t button_code);
char *__cdecl dttr_inputs_hook_format_button_name_callback(
	int32_t control_code,
	uint32_t button_mask
);
extern DTTR_PCDOGS_F_Input_RegisterButtonMapping_proto
	dttr_inputs_hook_register_button_mapping_original;
extern DTTR_PCDOGS_F_Config_ApplySettings_proto
	dttr_inputs_hook_config_apply_settings_original;
bool dttr_inputs_hook_mapping_prepare(const DTTR_Mods_Context *ctx);
void dttr_inputs_hook_mapping_reset();
void dttr_inputs_custom_button_mappings_clear();
uint32_t dttr_inputs_custom_button_mapping_mask(int button);
void dttr_inputs_apply_custom_button_mappings(DTTR_PCDOGS_T_Input_State *state);
void dttr_inputs_register_switch_puppies_controller_binding(int32_t control_code);
int32_t __cdecl dttr_inputs_hook_register_button_mapping_callback(
	int32_t control_code,
	uint32_t button_mask
);
void __cdecl dttr_inputs_hook_config_apply_settings_callback();
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
extern DTTR_PCDOGS_F_Input_GetPressedButton_proto
	dttr_inputs_hook_get_pressed_button_original;
bool dttr_inputs_hook_get_pressed_button_prepare(const DTTR_Mods_Context *ctx);
int32_t __cdecl dttr_inputs_hook_get_pressed_button_callback();
void dttr_inputs_hook_get_pressed_button_reset();
bool dttr_inputs_hook_read_gamepad_prepare(const DTTR_Mods_Context *ctx);
void dttr_inputs_hook_read_gamepad_reset();
extern DTTR_PCDOGS_F_Input_ReadGamepad_proto dttr_inputs_hook_read_gamepad_original;
void __cdecl dttr_inputs_hook_read_gamepad_callback(DTTR_PCDOGS_T_Input_State *state);

extern DTTR_PCDOGS_F_Input_TriggerRumbleIfAllowed_proto dttr_inputs_hook_rumble_original;
extern DTTR_PCDOGS_F_Settings_SetRumbleSuppressFlag_proto
	dttr_inputs_hook_set_rumble_suppress_flag_original;
bool dttr_inputs_hook_rumble_prepare(const DTTR_Mods_Context *ctx);
bool dttr_inputs_hook_set_rumble_suppress_flag_prepare(const DTTR_Mods_Context *ctx);
void dttr_inputs_hook_rumble_reset();
int32_t __cdecl dttr_inputs_hook_set_rumble_suppress_flag_callback(char suppress_rumble);
int32_t __cdecl dttr_inputs_hook_rumble_callback(
	int32_t effect_source_id,
	int32_t strong_feedback,
	int32_t force_magnitude_fixed,
	int32_t duration_units
);

#endif // DTTR_INPUTS_HOOKS_PRIVATE_H
