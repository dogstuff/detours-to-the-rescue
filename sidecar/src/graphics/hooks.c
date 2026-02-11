// Graphics hooks intercept DirectDraw and D3D7 creation to return our translators

#include <stdint.h>
#include <stdlib.h>
#include <windows.h>

#include "dttr_hooks.h"
#include "dttr_interop_pcdogs.h"
#include "dttr_sidecar.h"
#include "graphics_com_internal.h"
#include "graphics_internal.h"
#include "log.h"

DTTR_Graphics_COM_DirectDraw7 *g_dttr_graphics_hook_ddraw7;
HWND g_dttr_graphics_hook_hwnd;

static DTTR_Graphics_COM_DirectDraw7 *s_get_or_create_ddraw7(void) {
	if (!g_dttr_graphics_hook_ddraw7) {
		g_dttr_graphics_hook_ddraw7 = dttr_graphics_com_create_directdraw7();
	}

	return g_dttr_graphics_hook_ddraw7;
}

// Returns our DDraw7 translator for DirectDrawCreateEx
// https://learn.microsoft.com/en-us/windows/win32/api/ddraw/nf-ddraw-directdrawcreateex
HRESULT __stdcall dttr_graphics_hook_directdraw_create_ex_callback(
	const void *guid, void **ddraw_out, const void *iid, const void *outer
) {
	DTTR_Graphics_COM_DirectDraw7 *const ddraw7 = s_get_or_create_ddraw7();

	if (!ddraw7) {
		return E_OUTOFMEMORY;
	}

	g_pcdogs_ddraw_object_set(ddraw7);

	if (ddraw_out) {
		DWORD old;
		if (!VirtualProtect(ddraw_out, sizeof(void *), PAGE_READWRITE, &old)) {
			log_error(
				"VirtualProtect failed for ddraw_out=%p error=%lu", ddraw_out, GetLastError()
			);
		} else {
			*ddraw_out = ddraw7;
			VirtualProtect(ddraw_out, sizeof(void *), old, &old);
		}
	}

	log_info("DirectDrawCreateEx returning S_OK, vtbl=%p", ddraw7->m_vtbl);
	return S_OK;
}

// Calls the enumerate callback with our virtual display device
// https://learn.microsoft.com/en-us/windows/win32/api/ddraw/nf-ddraw-directdrawenumerateexa
HRESULT __stdcall dttr_graphics_hook_directdraw_enumerate_ex_a_callback(
	LPDDENUMCALLBACKEXA lpCallback, LPVOID lpContext, DWORD dwFlags
) {
	log_info(
		"DirectDrawEnumerateExA intercepted - callback=%p context=%p flags=0x%x",
		lpCallback,
		lpContext,
		dwFlags
	);

	if (!lpCallback) {
		log_info("DirectDrawEnumerateExA complete");
		return S_OK;
	}

	lpCallback(NULL, "DTTR Virtual Display", "display", lpContext, NULL);

	log_info("DirectDrawEnumerateExA complete");
	return S_OK;
}

// Initializes graphics and patches DirectDraw imports
void dttr_graphics_hook_init(HMODULE module) {
	g_dttr_graphics_hook_hwnd = dttr_graphics_init();

	if (!g_dttr_graphics_hook_hwnd) {
		log_error("Failed to initialize graphics backend");
		return;
	}

	if (!s_get_or_create_ddraw7()) {
		log_error("Failed to create DirectDraw translator");
		return;
	}

	DTTR_INTEROP_PATCH_PTR_LOG(dttr_hook_directdraw_create_ex, module);

	DTTR_INTEROP_PATCH_PTR_LOG(dttr_hook_directdraw_enumerate_ex_a, module);
}

// Removes hooks and releases translator state
void dttr_graphics_hook_cleanup(void) {
	DTTR_INTEROP_UNHOOK_LOG(dttr_hook_directdraw_create_ex);

	DTTR_INTEROP_UNHOOK_LOG(dttr_hook_directdraw_enumerate_ex_a);

	if (!g_dttr_graphics_hook_ddraw7) {
		return;
	}

	free(g_dttr_graphics_hook_ddraw7);
	g_dttr_graphics_hook_ddraw7 = NULL;
}

// Returns the window handle created by graphics initialization
HWND dttr_graphics_get_hwnd(void) { return g_dttr_graphics_hook_hwnd; }
