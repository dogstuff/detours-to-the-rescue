#ifndef DTTR_INPUTS_HOOKS_PRIVATE_H
#define DTTR_INPUTS_HOOKS_PRIVATE_H

#include <dttr_mods.h>
#include <dttr_pcdogs.h>
#include <windows.h>

#define DINPUT_BUTTON_PRESSED 0x80

// Sidecar scancodes are between Windows vkeys and native gamepad buttons.
enum {
	DTTR_INPUTS_KEY_KEYPAD_ENTER = 0xE8,
	DTTR_INPUTS_KEY_SCANCODE_BASE = 0x100,
	DTTR_INPUTS_GAMEPAD_BUTTON_BASE = 0x3E8,
};

typedef enum {
	DTTR_INPUTS_KEY_CODE_NONE,
	DTTR_INPUTS_KEY_CODE_VKEY,
	DTTR_INPUTS_KEY_CODE_SCANCODE,
	DTTR_INPUTS_KEY_CODE_GAMEPAD,
} DTTR_Input_KeyCodeKind;

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
bool dttr_inputs_keyboard_scancode_pressed(
	const bool *keyboard_state,
	int keyboard_state_count,
	int scancode
);
int32_t dttr_inputs_key_code_from_scancode(int scancode);
DTTR_Input_KeyCodeKind dttr_inputs_key_code_kind(int32_t key_code);
int dttr_inputs_key_code_scancode(int32_t key_code);
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
void dttr_inputs_controls_menu_reset();
int32_t dttr_inputs_controls_menu_pressed_button(
	int32_t pressed_button,
	int32_t remapping_active
);
int32_t dttr_inputs_controls_menu_pressed_keyboard_button(
	int32_t pressed_button,
	int32_t remapping_active,
	const bool *keyboard_state,
	int keyboard_state_count
);
extern DTTR_PCDOGS_F_Input_GetPressedButton_proto
	dttr_inputs_hook_get_pressed_button_original;
bool dttr_inputs_hook_get_pressed_button_prepare(const DTTR_Mods_Context *ctx);
int32_t __cdecl dttr_inputs_hook_get_pressed_button_callback();
void dttr_inputs_hook_get_pressed_button_reset();
bool dttr_inputs_hook_read_gamepad_prepare(const DTTR_Mods_Context *ctx);
void dttr_inputs_hook_read_gamepad_reset();
void __cdecl dttr_inputs_hook_read_gamepad_callback(DTTR_PCDOGS_T_Input_State *state);

extern DTTR_PCDOGS_F_Input_TriggerRumbleIfAllowed_proto dttr_inputs_hook_rumble_original;
bool dttr_inputs_hook_rumble_prepare(const DTTR_Mods_Context *ctx);
void dttr_inputs_hook_rumble_reset();
int32_t __cdecl dttr_inputs_hook_rumble_callback(
	int32_t effect_source_id,
	int32_t strong_feedback,
	int32_t force_magnitude_fixed,
	int32_t duration_units
);

#endif // DTTR_INPUTS_HOOKS_PRIVATE_H
