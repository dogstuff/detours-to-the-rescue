#ifndef DTTR_INPUTS_HOOKS_PRIVATE_H
#define DTTR_INPUTS_HOOKS_PRIVATE_H

#include <dttr_mods.h>
#include <dttr_pcdogs.h>
#include <windows.h>

#define DINPUT_BUTTON_PRESSED 0x80

// Private PCDOGS key code. Windows reports both Enter keys as VK_RETURN.
enum {
	DTTR_INPUTS_KEY_KEYPAD_ENTER = 0xE8,
};

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
bool dttr_inputs_vkey_or_keypad_enter_pressed(
	int vkey,
	const bool *keyboard_state,
	int keyboard_state_count
);
const char *dttr_inputs_key_code_name(int32_t key_code);
SHORT __stdcall dttr_inputs_hook_get_async_key_state_callback(int vkey);
extern DTTR_PCDOGS_F_Input_IsKeyPressedAsync_proto
	dttr_inputs_hook_is_key_pressed_async_original;
extern DTTR_PCDOGS_F_Input_GetButtonString_proto
	dttr_inputs_hook_get_button_string_original;
bool dttr_inputs_hook_key_state_prepare(const DTTR_Mods_Context *ctx);
void dttr_inputs_hook_key_state_reset();
BOOL __cdecl dttr_inputs_hook_is_key_pressed_async_callback(int32_t virtual_key);
char *__cdecl dttr_inputs_hook_get_button_string_callback(int32_t button_code);
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
