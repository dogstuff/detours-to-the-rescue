#ifndef DTTR_HOOKS_H
#define DTTR_HOOKS_H

#include <dttr_interop.h>
#include <stdbool.h>
#include <stdint.h>
#include <windows.h>

/// Replaces the game's WinMain entry point with sidecar bootstrapping logic
int32_t _stdcall
dttr_hook_win_main_callback(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int32_t nCmdShow);

DTTR_INTEROP_HOOK_FUNC_SIG(
	dttr_hook_win_main, "\x83\xEC\x40\x53\x8B\x5C\x24", "xxxxxxx", match, dttr_hook_win_main_callback
)

/// Replaces DirectInput joystick polling with SDL gamepad state
void *__cdecl dttr_inputs_hook_dinput_poll_callback(void *device);

DTTR_INTEROP_HOOK_FUNC_SIG(
	dttr_inputs_hook_dinput_poll,
	"\x56\x8B\x74\x24?\x56\x8B\x06",
	"xxxx?xxx",
	match,
	dttr_inputs_hook_dinput_poll_callback
)

/// Replaces GetAsyncKeyState with SDL keyboard state so keyboard input works in the SDL window
SHORT __stdcall dttr_inputs_hook_get_async_key_state_callback(int vkey);

// Sig resolves absolute address from mov operand
DTTR_INTEROP_PATCH_PTR_SIG(
	dttr_inputs_hook_get_async_key_state,
	"\x8B\x1D????\x56\x33\xF6",
	"xx????xxx",
	*(uint32_t *)(match + 2),
	dttr_inputs_hook_get_async_key_state_callback
)

/// Installs DirectDraw-related runtime hooks
void dttr_graphics_hook_init(HMODULE module);
/// Removes DirectDraw-related runtime hooks and releases hook state
void dttr_graphics_hook_cleanup(void);

/// Intercepts DirectDrawCreateEx and returns the sidecar DirectDraw translator
// https://learn.microsoft.com/en-us/windows/win32/api/ddraw/nf-ddraw-directdrawcreateex
HRESULT __stdcall dttr_graphics_hook_directdraw_create_ex_callback(
	const void *guid, void **ddraw_out, const void *iid, const void *outer
);

typedef BOOL(__stdcall *LPDDENUMCALLBACKEXA)(
	GUID *lpGUID, LPSTR lpDriverDescription, LPSTR lpDriverName, LPVOID lpContext, HMONITOR hm
);

/// Intercepts DirectDrawEnumerateExA and enumerates the virtual display adapter
// https://learn.microsoft.com/en-us/windows/win32/api/ddraw/nf-ddraw-directdrawenumerateexa
HRESULT __stdcall
dttr_graphics_hook_directdraw_enumerate_ex_a_callback(LPDDENUMCALLBACKEXA lpCallback, LPVOID lpContext, DWORD dwFlags);

// Sig matches call site; resolves through import thunk to IAT entry
DTTR_INTEROP_PATCH_PTR_SIG(
	dttr_hook_directdraw_create_ex,
	"\xE8????\x85\xC0\x7D?\x68????\x6A\x00\x50\xE8",
	"x????xxx?x????xxxx",
	DTTR_FF25_ADDR(DTTR_E8_TARGET(match)),
	dttr_graphics_hook_directdraw_create_ex_callback
)

// Sig matches call site; resolves through import thunk to IAT entry
DTTR_INTEROP_PATCH_PTR_SIG(
	dttr_hook_directdraw_enumerate_ex_a,
	"\xE8????\x8B\xF0\xA1",
	"x????xxx",
	DTTR_FF25_ADDR(DTTR_E8_TARGET(match)),
	dttr_graphics_hook_directdraw_enumerate_ex_a_callback
)

DTTR_INTEROP_WRAP_CACHED_SIG(
	dttr_crt_open_file_with_mode,
	"\xE8????\x85\xC0\x75?\xC3",
	"x????xxx?x",
	match,
	void *,
	(const char *path, char *mode, int flags),
	(path, mode, flags)
)

/// Hooks the game's internal OpenFile to gracefully handle failures
void *__cdecl dttr_crt_hook_open_file_callback(const char *path, char *mode);

DTTR_INTEROP_HOOK_FUNC_SIG(
	dttr_crt_hook_open_file,
	"\x6A\x40\xFF\x74\x24\x0C\xFF\x74\x24\x0C\xE8",
	"xxxxxxxxxxx",
	match,
	dttr_crt_hook_open_file_callback
)

/// Fixes an issue in later releases of the game that causes the
/// game directory to be incorrectly resolved if the 'p' character
/// is present in the path before 'pcdogs.exe'.
///
/// This issue would cause crashes on boot if the user did not have
/// write access to the incorrectly resolved directory.
int32_t __cdecl dttr_hook_resolve_pcdogs_path_callback(void);

DTTR_INTEROP_HOOK_FUNC_OPTIONAL_SIG(
	dttr_hook_resolve_pcdogs_path, "\x51\x8D\x44\x24?\x57", "xxxx?x", match, dttr_hook_resolve_pcdogs_path_callback
)

/// Initializes miscellaneous hooks
void dttr_other_hook_init(HMODULE mod);

/// Removes miscellaneous hooks and frees trampoline memory/
void dttr_other_hook_cleanup(void);

#endif
