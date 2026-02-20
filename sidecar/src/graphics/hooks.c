// Graphics hooks intercept DirectDraw and D3D7 creation to return our translators

#include <stdlib.h>
#include <windows.h>

#include <dttr_config.h>

#include "dttr_hooks_graphics.h"
#include "dttr_interop_pcdogs.h"
#include "dttr_sidecar.h"
#include "graphics_com_internal.h"
#include "graphics_internal.h"
#include "log.h"

DTTR_Graphics_COM_DirectDraw7 *g_dttr_graphics_hook_ddraw7;
HWND g_dttr_graphics_hook_hwnd;

// Patch bytes for subpixel vertex precision
static const uint8_t s_fast_path_patch[] = {0xE9, 0xBA, 0x00, 0x00, 0x00, 0x90};
static const uint8_t s_nop2[] = {0x90, 0x90};
static const uint8_t s_ftol_x_patch[] = {0xD9, 0x1F, 0x90, 0x90, 0x90};
static const uint8_t s_nop4[] = {0x90, 0x90, 0x90, 0x90};
static const uint8_t s_ftol_y_patch[] = {0xD9, 0x5D, 0x00, 0x90, 0x90};
static const uint8_t s_nop3[] = {0x90, 0x90, 0x90};
static const uint8_t s_ret[] = {0xC3};

static DTTR_Graphics_COM_DirectDraw7 *s_get_or_create_ddraw7(void) {
	if (!g_dttr_graphics_hook_ddraw7) {
		g_dttr_graphics_hook_ddraw7 = dttr_graphics_com_create_directdraw7();
	}

	return g_dttr_graphics_hook_ddraw7;
}

// Returns our DDraw7 translator for DirectDrawCreateEx
// https://learn.microsoft.com/en-us/windows/win32/api/ddraw/nf-ddraw-directdrawcreateex
HRESULT __stdcall dttr_graphics_hook_directdraw_create_ex_callback(
	const void *guid,
	void **ddraw_out,
	const void *iid,
	const void *outer
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
				"VirtualProtect failed for ddraw_out=%p error=%lu",
				ddraw_out,
				GetLastError()
			);
		} else {
			*ddraw_out = ddraw7;
			VirtualProtect(ddraw_out, sizeof(void *), old, &old);
		}
	}

	log_debug("DirectDrawCreateEx returning S_OK, vtbl=%p", ddraw7->m_vtbl);
	return S_OK;
}

// Calls the enumerate callback with our virtual display device
// https://learn.microsoft.com/en-us/windows/win32/api/ddraw/nf-ddraw-directdrawenumerateexa
HRESULT __stdcall dttr_graphics_hook_directdraw_enumerate_ex_a_callback(
	LPDDENUMCALLBACKEXA lpCallback,
	LPVOID lpContext,
	DWORD dwFlags
) {
	log_debug(
		"DirectDrawEnumerateExA intercepted - callback=%p context=%p flags=0x%x",
		lpCallback,
		lpContext,
		dwFlags
	);

	if (!lpCallback) {
		log_debug("DirectDrawEnumerateExA complete");
		return S_OK;
	}

	lpCallback(NULL, "DTTR Virtual Display", "display", lpContext, NULL);

	log_debug("DirectDrawEnumerateExA complete");
	return S_OK;
}

// Initializes graphics and patches DirectDraw imports
void dttr_graphics_hook_init(const DTTR_ComponentContext *ctx) {
	g_dttr_graphics_hook_hwnd = dttr_graphics_init();

	if (!g_dttr_graphics_hook_hwnd) {
		log_error("Failed to initialize backend");
		return;
	}

	if (!s_get_or_create_ddraw7()) {
		log_error("Failed to create DirectDraw translator");
		return;
	}

	DTTR_INTEROP_PATCH_PTR_LOG(dttr_hook_directdraw_create_ex, ctx);

	DTTR_INTEROP_PATCH_PTR_LOG(dttr_hook_directdraw_enumerate_ex_a, ctx);

	if (g_dttr_config.m_vertex_precision == DTTR_VERTEX_PRECISION_SUBPIXEL) {
		DTTR_INTEROP_PATCH_BYTES_LOG(
			dttr_hook_precision_fast_path,
			ctx,
			s_fast_path_patch
		);
		DTTR_INTEROP_PATCH_BYTES_LOG(dttr_hook_precision_batch_limit_a, ctx, s_nop2);
		DTTR_INTEROP_PATCH_BYTES_LOG(dttr_hook_precision_batch_limit_b, ctx, s_nop2);
		DTTR_INTEROP_PATCH_BYTES_LOG(dttr_hook_precision_ftol_x, ctx, s_ftol_x_patch);
		DTTR_INTEROP_PATCH_BYTES_LOG(dttr_hook_precision_mov_x, ctx, s_nop4);
		DTTR_INTEROP_PATCH_BYTES_LOG(dttr_hook_precision_fstp2_x, ctx, s_nop2);
		DTTR_INTEROP_PATCH_BYTES_LOG(dttr_hook_precision_fild_x, ctx, s_nop4);
		DTTR_INTEROP_PATCH_BYTES_LOG(dttr_hook_precision_ftol_y, ctx, s_ftol_y_patch);
		DTTR_INTEROP_PATCH_BYTES_LOG(dttr_hook_precision_mov_y, ctx, s_nop4);
		DTTR_INTEROP_PATCH_BYTES_LOG(dttr_hook_precision_fstp2_y, ctx, s_nop3);
		DTTR_INTEROP_PATCH_BYTES_LOG(dttr_hook_precision_fild_y, ctx, s_nop4);
		DTTR_INTEROP_PATCH_BYTES_OPTIONAL_LOG(dttr_hook_render_quad_snap, ctx, s_ret);
	}
}

// Removes hooks and releases translator state
void dttr_graphics_hook_cleanup(const DTTR_ComponentContext *ctx) {
	DTTR_INTEROP_UNHOOK_OPTIONAL_LOG(dttr_hook_render_quad_snap, ctx);

	DTTR_INTEROP_UNHOOK_LOG(dttr_hook_precision_fast_path, ctx);
	DTTR_INTEROP_UNHOOK_LOG(dttr_hook_precision_batch_limit_a, ctx);
	DTTR_INTEROP_UNHOOK_LOG(dttr_hook_precision_batch_limit_b, ctx);
	DTTR_INTEROP_UNHOOK_LOG(dttr_hook_precision_fild_x, ctx);
	DTTR_INTEROP_UNHOOK_LOG(dttr_hook_precision_fstp2_x, ctx);
	DTTR_INTEROP_UNHOOK_LOG(dttr_hook_precision_mov_x, ctx);
	DTTR_INTEROP_UNHOOK_LOG(dttr_hook_precision_ftol_x, ctx);
	DTTR_INTEROP_UNHOOK_LOG(dttr_hook_precision_fild_y, ctx);
	DTTR_INTEROP_UNHOOK_LOG(dttr_hook_precision_fstp2_y, ctx);
	DTTR_INTEROP_UNHOOK_LOG(dttr_hook_precision_mov_y, ctx);
	DTTR_INTEROP_UNHOOK_LOG(dttr_hook_precision_ftol_y, ctx);

	DTTR_INTEROP_UNHOOK_LOG(dttr_hook_directdraw_create_ex, ctx);

	DTTR_INTEROP_UNHOOK_LOG(dttr_hook_directdraw_enumerate_ex_a, ctx);

	if (!g_dttr_graphics_hook_ddraw7) {
		return;
	}

	free(g_dttr_graphics_hook_ddraw7);
	g_dttr_graphics_hook_ddraw7 = NULL;
}
