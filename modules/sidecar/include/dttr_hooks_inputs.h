#ifndef DTTR_HOOKS_INPUTS_H
#define DTTR_HOOKS_INPUTS_H

#include <dttr_components.h>

/// Replaces DirectInput joystick polling with SDL gamepad state.
void *__cdecl dttr_inputs_hook_dinput_poll_callback(void *device);

DTTR_HOOK(dttr_inputs_hook_dinput_poll)

/// Replaces GetAsyncKeyState with SDL keyboard state.
/// This also limits keyboard input to the SDL window.
SHORT __stdcall dttr_inputs_hook_get_async_key_state_callback(int vkey);

DTTR_HOOK(dttr_inputs_hook_get_async_key_state)

#endif
