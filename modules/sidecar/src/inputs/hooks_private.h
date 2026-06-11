#ifndef DTTR_INPUTS_HOOKS_PRIVATE_H
#define DTTR_INPUTS_HOOKS_PRIVATE_H

#include <dttr_mods.h>
#include <dttr_pcdogs.h>
#include <windows.h>

#define DINPUT_BUTTON_PRESSED 0x80

bool dttr_inputs_hooks_init(const DTTR_Mods_Context *ctx);
void dttr_inputs_hooks_cleanup(const DTTR_Mods_Context *ctx);

void *__cdecl dttr_inputs_hook_dinput_poll_callback(void *device);
SHORT __stdcall dttr_inputs_hook_get_async_key_state_callback(int vkey);
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
