#ifndef DTTR_HOOKS_H
#define DTTR_HOOKS_H

#include <dttr_interop.h>
#include <stdbool.h>
#include <stdint.h>
#include <windows.h>

/// Replaces the game's WinMain entry point with sidecar bootstrapping logic
int32_t _stdcall dttr_hook_win_main_callback(
	HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int32_t nCmdShow
);

DTTR_INTEROP_HOOK_FUNC(dttr_hook_win_main, 0x3e590, dttr_hook_win_main_callback)

/// Replaces DirectInput joystick polling with SDL gamepad state
void *__cdecl dttr_inputs_hook_dinput_poll_callback(void *device);
DTTR_INTEROP_HOOK_FUNC(dttr_inputs_hook_dinput_poll, 0x1a390, dttr_inputs_hook_dinput_poll_callback)

/// Replaces GetAsyncKeyState with SDL keyboard state so keyboard input works in the SDL window
SHORT __stdcall dttr_inputs_hook_get_async_key_state_callback(int vkey);

DTTR_INTEROP_PATCH_PTR(
	dttr_inputs_hook_get_async_key_state, 0x168D3E4, dttr_inputs_hook_get_async_key_state_callback
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
HRESULT __stdcall dttr_graphics_hook_directdraw_enumerate_ex_a_callback(
	LPDDENUMCALLBACKEXA lpCallback, LPVOID lpContext, DWORD dwFlags
);

DTTR_INTEROP_PATCH_PTR(
	dttr_hook_directdraw_create_ex, 0x168d2f8, dttr_graphics_hook_directdraw_create_ex_callback
)

DTTR_INTEROP_PATCH_PTR(
	dttr_hook_directdraw_enumerate_ex_a,
	0x168d2fc,
	dttr_graphics_hook_directdraw_enumerate_ex_a_callback
)

/// Wraps the game's internal OpenFileWithMode for direct calls
DTTR_INTEROP_WRAP_CACHED(
	dttr_crt_open_file_with_mode,
	0x463D3,
	void *,
	(const char *path, char *mode, int flags),
	(path, mode, flags)
)

/// Hooks the game's internal OpenFile to gracefully handle failures
void *__cdecl dttr_crt_hook_open_file_callback(const char *path, char *mode);
DTTR_INTEROP_HOOK_FUNC(dttr_crt_hook_open_file, 0x463F3, dttr_crt_hook_open_file_callback)

/// Installs CRT hooks
void dttr_crt_hook_init(HMODULE mod);

/// Removes CRT hooks and frees trampoline memory
void dttr_crt_hook_cleanup(void);

#endif
