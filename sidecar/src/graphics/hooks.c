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
HRESULT __stdcall dttr_hook_directdraw_create_ex_callback(
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
HRESULT __stdcall dttr_hook_directdraw_enumerate_ex_a_callback(
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
		return S_OK;
	}

	lpCallback(NULL, "DTTR Virtual Display", "display", lpContext, NULL);

	return S_OK;
}

// Initializes graphics and patches DirectDraw imports
void dttr_graphics_hooks_init(const DTTR_ComponentContext *ctx) {
	g_dttr_graphics_hook_hwnd = dttr_graphics_init();

	if (!g_dttr_graphics_hook_hwnd) {
		log_error("Failed to initialize backend");
		return;
	}

	if (!s_get_or_create_ddraw7()) {
		log_error("Failed to create DirectDraw translator");
		return;
	}

	DTTR_INSTALL_POINTER(
		dttr_hook_directdraw_create_ex,
		ctx,
		"\xE8????\x85\xC0\x7D?\x68????\x6A\x00\x50\xE8",
		"x????xxx?x????xxxx",
		DTTR_FF25_ADDR(DTTR_E8_TARGET(match_))
	);

	DTTR_INSTALL_POINTER(
		dttr_hook_directdraw_enumerate_ex_a,
		ctx,
		"\xE8????\x8B\xF0\xA1",
		"x????xxx",
		DTTR_FF25_ADDR(DTTR_E8_TARGET(match_))
	);

	if (g_dttr_config.m_vertex_precision == DTTR_VERTEX_PRECISION_SUBPIXEL) {
		DTTR_INSTALL_BYTES(
			dttr_hook_precision_fast_path,
			ctx,
			"\x83\xF8?\x7C?\xD9\x43?\xD8\x1D????\xDF\xE0\xF6\xC4\x41\x0F\x85????",
			"xx?x?xx?xx????xxxxxx????",
			19,
			s_fast_path_patch,
			6
		);

		DTTR_INSTALL_BYTES(
			dttr_hook_precision_batch_limit_a,
			ctx,
			"\x8B\x08\xEB?\xA1????\x8B\x0D????\x3B\xC1",
			"xxx?x????xx????xx",
			17,
			s_nop2,
			2
		);

		DTTR_INSTALL_BYTES(
			dttr_hook_precision_batch_limit_b,
			ctx,
			"\x83\xC1\x14\x4E\x75?\xA1????\x8B\x0D????\x3B\xC1",
			"xxxxx?x????xx????xx",
			19,
			s_nop2,
			2
		);

		DTTR_INSTALL_BYTES(
			dttr_hook_precision_ftol_x,
			ctx,
			"\xDB\x44\x24\x30\xD9\x1F",
			"xxxxxx",
			-15,
			s_ftol_x_patch,
			5
		);

		DTTR_INSTALL_BYTES(
			dttr_hook_precision_mov_x,
			ctx,
			"\xDB\x44\x24\x30\xD9\x1F",
			"xxxxxx",
			-10,
			s_nop4,
			4
		);

		DTTR_INSTALL_BYTES(
			dttr_hook_precision_fstp2_x,
			ctx,
			"\x8D\xAE????\xDB\x44\x24\x30\xD9\x1F",
			"xx????xxxxxx",
			10,
			s_nop2,
			2
		);

		DTTR_INSTALL_BYTES(
			dttr_hook_precision_fild_x,
			ctx,
			"\x8D\xAE????\xDB\x44\x24\x30",
			"xx????xxxx",
			6,
			s_nop4,
			4
		);

		DTTR_INSTALL_BYTES(
			dttr_hook_precision_ftol_y,
			ctx,
			"\x8B\x54\x24\x18\x89\x44\x24\x30",
			"xxxxxxxx",
			-5,
			s_ftol_y_patch,
			5
		);

		DTTR_INSTALL_BYTES(
			dttr_hook_precision_mov_y,
			ctx,
			"\x8B\x54\x24\x18\x89\x44\x24\x30",
			"xxxxxxxx",
			4,
			s_nop4,
			4
		);

		DTTR_INSTALL_BYTES(
			dttr_hook_precision_fstp2_y,
			ctx,
			"\x83\xC0\x14\x50\x55\xD9\x5D\x00",
			"xxxxxxxx",
			5,
			s_nop3,
			3
		);

		DTTR_INSTALL_BYTES(
			dttr_hook_precision_fild_y,
			ctx,
			"\x52\xDB\x44\x24\x34",
			"xxxxx",
			1,
			s_nop4,
			4
		);

		DTTR_INSTALL_BYTES_OPTIONAL(
			dttr_hook_render_quad_snap,
			ctx,
			"\x53\x8b\x5c\x24\x14\x55\x33\xc9\x56\x57\x85\xdb",
			"xxxxxxxxxxxx",
			0,
			s_ret,
			1
		);
	}
}

// Removes hooks and releases translator state
void dttr_graphics_hooks_cleanup(const DTTR_ComponentContext *ctx) {
	DTTR_UNINSTALL(dttr_hook_render_quad_snap, ctx);

	DTTR_UNINSTALL(dttr_hook_precision_fast_path, ctx);
	DTTR_UNINSTALL(dttr_hook_precision_batch_limit_a, ctx);
	DTTR_UNINSTALL(dttr_hook_precision_batch_limit_b, ctx);
	DTTR_UNINSTALL(dttr_hook_precision_fild_x, ctx);
	DTTR_UNINSTALL(dttr_hook_precision_fstp2_x, ctx);
	DTTR_UNINSTALL(dttr_hook_precision_mov_x, ctx);
	DTTR_UNINSTALL(dttr_hook_precision_ftol_x, ctx);
	DTTR_UNINSTALL(dttr_hook_precision_fild_y, ctx);
	DTTR_UNINSTALL(dttr_hook_precision_fstp2_y, ctx);
	DTTR_UNINSTALL(dttr_hook_precision_mov_y, ctx);
	DTTR_UNINSTALL(dttr_hook_precision_ftol_y, ctx);

	DTTR_UNINSTALL(dttr_hook_directdraw_create_ex, ctx);

	DTTR_UNINSTALL(dttr_hook_directdraw_enumerate_ex_a, ctx);

	if (!g_dttr_graphics_hook_ddraw7) {
		return;
	}

	free(g_dttr_graphics_hook_ddraw7);
	g_dttr_graphics_hook_ddraw7 = NULL;
}
