#ifndef DTTR_INPUTS_HOOKS_PRIVATE_H
#define DTTR_INPUTS_HOOKS_PRIVATE_H

#include <dttr_mods.h>
#include <windows.h>

// Installs input patches that translate legacy game polling through SDL state.
bool dttr_inputs_hooks_init(const DTTR_Mods_Context *ctx);
// Releases input patches installed for legacy game polling.
void dttr_inputs_hooks_cleanup(const DTTR_Mods_Context *ctx);

// Replacement DirectInput poll callback backed by the active SDL gamepad.
void *__cdecl dttr_inputs_hook_dinput_poll_callback(void *device);
// Replacement Win32 key-state callback backed by SDL keyboard state.
SHORT __stdcall dttr_inputs_hook_get_async_key_state_callback(int vkey);

#endif // DTTR_INPUTS_HOOKS_PRIVATE_H
