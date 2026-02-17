#ifndef DTTR_HOOKS_INPUTS_H
#define DTTR_HOOKS_INPUTS_H

#include <dttr_interop.h>
#include <stdint.h>
#include <windows.h>

/// Replaces DirectInput joystick polling with SDL gamepad state
void *__cdecl dttr_inputs_hook_dinput_poll_callback(void *device);

DTTR_INTEROP_HOOK_FUNC_SIG(
	dttr_inputs_hook_dinput_poll,
	"\x56\x8B\x74\x24?\x56\x8B\x06",
	"xxxx?xxx",
	match,
	dttr_inputs_hook_dinput_poll_callback
)

/// Replaces GetAsyncKeyState with SDL keyboard state.
/// As a bonus, this restricts keyboard into to the SDL window.
SHORT __stdcall dttr_inputs_hook_get_async_key_state_callback(int vkey);

// Sig resolves absolute address from mov operand
DTTR_INTEROP_PATCH_PTR_SIG(
	dttr_inputs_hook_get_async_key_state,
	"\x8B\x1D????\x56\x33\xF6",
	"xx????xxx",
	*(uint32_t *)(match + 2),
	dttr_inputs_hook_get_async_key_state_callback
)

#endif
