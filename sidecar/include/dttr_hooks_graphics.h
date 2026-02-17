#ifndef DTTR_HOOKS_GRAPHICS_H
#define DTTR_HOOKS_GRAPHICS_H

#include <dttr_interop.h>
#include <stdint.h>
#include <windows.h>

/// Installs DirectDraw-related runtime hooks
void dttr_graphics_hook_init(HMODULE module);
/// Removes DirectDraw-related runtime hooks and releases hook state
void dttr_graphics_hook_cleanup(void);

/// Intercepts DirectDrawCreateEx and returns the sidecar DirectDraw translator
// https://learn.microsoft.com/en-us/windows/win32/api/ddraw/nf-ddraw-directdrawcreateex
HRESULT __stdcall dttr_graphics_hook_directdraw_create_ex_callback(
	const void *guid,
	void **ddraw_out,
	const void *iid,
	const void *outer
);

typedef BOOL(__stdcall *LPDDENUMCALLBACKEXA)(
	GUID *lpGUID,
	LPSTR lpDriverDescription,
	LPSTR lpDriverName,
	LPVOID lpContext,
	HMONITOR hm
);

/// Intercepts DirectDrawEnumerateExA and enumerates the virtual display adapter
// https://learn.microsoft.com/en-us/windows/win32/api/ddraw/nf-ddraw-directdrawenumerateexa
HRESULT __stdcall dttr_graphics_hook_directdraw_enumerate_ex_a_callback(
	LPDDENUMCALLBACKEXA lpCallback,
	LPVOID lpContext,
	DWORD dwFlags
);

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

#endif
