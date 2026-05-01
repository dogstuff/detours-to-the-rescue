#ifndef DTTR_HOOKS_GRAPHICS_H
#define DTTR_HOOKS_GRAPHICS_H

#include <dttr_components.h>

/// Installs DirectDraw-related runtime hooks.
void dttr_graphics_hooks_init(const DTTR_ComponentContext *ctx);
/// Removes DirectDraw-related runtime hooks and releases hook state.
void dttr_graphics_hooks_cleanup(const DTTR_ComponentContext *ctx);

/// Intercepts DirectDrawCreateEx and returns the sidecar DirectDraw translator.
// https://learn.microsoft.com/en-us/windows/win32/api/ddraw/nf-ddraw-directdrawcreateex
HRESULT __stdcall dttr_hook_directdraw_create_ex_callback(
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

/// Intercepts DirectDrawEnumerateExA and enumerates the virtual display adapter.
// https://learn.microsoft.com/en-us/windows/win32/api/ddraw/nf-ddraw-directdrawenumerateexa
HRESULT __stdcall dttr_hook_directdraw_enumerate_ex_a_callback(
	LPDDENUMCALLBACKEXA lpCallback,
	LPVOID lpContext,
	DWORD dwFlags
);

// Pointer hooks (IAT patches)
DTTR_HOOK(dttr_hook_directdraw_create_ex)
DTTR_HOOK(dttr_hook_directdraw_enumerate_ex_a)

// Byte patches for subpixel vertex precision.
DTTR_HOOK(dttr_hook_precision_fast_path)
DTTR_HOOK(dttr_hook_precision_batch_limit_a)
DTTR_HOOK(dttr_hook_precision_batch_limit_b)
DTTR_HOOK(dttr_hook_precision_ftol_x)
DTTR_HOOK(dttr_hook_precision_mov_x)
DTTR_HOOK(dttr_hook_precision_fstp2_x)
DTTR_HOOK(dttr_hook_precision_fild_x)
DTTR_HOOK(dttr_hook_precision_ftol_y)
DTTR_HOOK(dttr_hook_precision_mov_y)
DTTR_HOOK(dttr_hook_precision_fstp2_y)
DTTR_HOOK(dttr_hook_precision_fild_y)

// Optional byte patch for render quad snapping.
DTTR_HOOK(dttr_hook_render_quad_snap)

#endif /* DTTR_HOOKS_GRAPHICS_H */
