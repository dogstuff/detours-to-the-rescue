// IDirectDraw7 COM translator that handles the DDraw7 interface and creates D3D7
// https://learn.microsoft.com/en-us/windows/win32/api/ddraw/nn-ddraw-idirectdraw7

#include "graphics_com_private.h"
#include "graphics_private.h"
#include <dttr_log.h>
#include <stdlib.h>
#include <string.h>

static DTTR_Graphics_COM_Direct3D7 *ddraw7_d3d7;

static DTTR_Graphics_COM_Direct3D7 *ddraw7_get_direct3d() {
	if (!ddraw7_d3d7) {
		ddraw7_d3d7 = dttr_graphics_com_create_direct3d7();
	}

	return ddraw7_d3d7;
}

static HRESULT __stdcall ddraw7_queryinterface(
	DTTR_Graphics_COM_DirectDraw7 *self,
	void *riid,
	void **ppv
) {
	DTTR_Graphics_COM_Direct3D7 *direct3d = ddraw7_get_direct3d();

	if (ppv) {
		*ppv = direct3d;
	}

	return S_OK;
}

DTTR_COM_ADDREF(ddraw7_addref, DTTR_Graphics_COM_DirectDraw7)

DTTR_COM_RELEASE(ddraw7_release, DTTR_Graphics_COM_DirectDraw7)

DTTR_COM_NOOP_HRESULT(ddraw7_compact, DTTR_Graphics_COM_DirectDraw7 *self)

static HRESULT __stdcall ddraw7_createclipper(
	DTTR_Graphics_COM_DirectDraw7 *self,
	DWORD f,
	void **clip,
	void *outer
) {
	if (clip) {
		*clip = NULL;
	}

	return S_OK;
}

static HRESULT __stdcall ddraw7_createpalette(
	DTTR_Graphics_COM_DirectDraw7 *self,
	DWORD f,
	void *colors,
	void **pal,
	void *outer
) {
	if (pal) {
		*pal = NULL;
	}

	return S_OK;
}

static HRESULT __stdcall ddraw7_createsurface(
	DTTR_Graphics_COM_DirectDraw7 *self,
	void *desc,
	void **surf,
	void *outer
) {
	if (surf) {
		*surf = NULL;
	} else {
		return DDERR_INVALIDPARAMS;
	}

	// Use 640x480 RGB565 when the game does not provide surface metadata
	uint32_t width = 640, height = 480, bpp = 16;
	uint32_t r_mask = 0xF800, g_mask = 0x07E0, b_mask = 0x001F, a_mask = 0;
	bool updates_logical_resolution = false;

	if (desc) {
		const DDSURFACEDESC2 *desc2 = (const DDSURFACEDESC2 *)desc;
		const DWORD flags = desc2->dwFlags;

		if (flags & DDSD_WIDTH) {
			width = desc2->dwWidth;
		}

		if (flags & DDSD_HEIGHT) {
			height = desc2->dwHeight;
		}

		if (flags & DDSD_PIXELFORMAT) {
			bpp = desc2->ddpfPixelFormat.dwRGBBitCount;
			r_mask = desc2->ddpfPixelFormat.dwRBitMask;
			g_mask = desc2->ddpfPixelFormat.dwGBitMask;
			b_mask = desc2->ddpfPixelFormat.dwBBitMask;
			a_mask = desc2->ddpfPixelFormat.dwRGBAlphaBitMask;
		}

		if (flags & DDSD_CAPS) {
			const DWORD caps = desc2->ddsCaps.dwCaps;
			if (caps & (DDSCAPS_PRIMARYSURFACE | DDSCAPS_3DDEVICE | DDSCAPS_BACKBUFFER)) {
				updates_logical_resolution = true;
			}
		}
	}

	HRESULT validation = dttr_graphics_com_validate_directdrawsurface7(
		width,
		height,
		bpp,
		NULL,
		NULL
	);
	if (validation != S_OK) {
		return validation;
	}

	DTTR_Graphics_COM_DirectDrawSurface7 *created
		= dttr_graphics_com_create_directdrawsurface7(
			width,
			height,
			bpp,
			r_mask,
			g_mask,
			b_mask,
			a_mask
		);
	if (!created) {
		return DDERR_OUTOFMEMORY;
	}

	if (updates_logical_resolution) {
		dttr_graphics_set_logical_resolution((int)width, (int)height);
	}

	if (surf) {
		*surf = created;
	}

	return S_OK;
}

DTTR_COM_STUB_SET(
	ddraw7_duplicatesurface,
	void *,
	NULL,
	DTTR_Graphics_COM_DirectDraw7 *self,
	void *src
)

DTTR_COM_NOOP_HRESULT(
	ddraw7_enumdisplaymodes,
	DTTR_Graphics_COM_DirectDraw7 *self,
	DWORD f,
	void *desc,
	void *ctx,
	void *cb
)

DTTR_COM_NOOP_HRESULT(
	ddraw7_enumsurfaces,
	DTTR_Graphics_COM_DirectDraw7 *self,
	DWORD f,
	void *desc,
	void *ctx,
	void *cb
)

DTTR_COM_NOOP_HRESULT(ddraw7_fliptogdisurface, DTTR_Graphics_COM_DirectDraw7 *self)

DTTR_COM_NOOP_HRESULT(
	ddraw7_getcaps,
	DTTR_Graphics_COM_DirectDraw7 *self,
	void *drvCaps,
	void *helCaps
)

DTTR_COM_NOOP_HRESULT(
	ddraw7_getdisplaymode,
	DTTR_Graphics_COM_DirectDraw7 *self,
	void *desc
)

static HRESULT __stdcall ddraw7_getfourcccodes(
	DTTR_Graphics_COM_DirectDraw7 *self,
	DWORD *num,
	DWORD *codes
) {
	if (num) {
		*num = 0;
	}

	return S_OK;
}

DTTR_COM_STUB_SET(ddraw7_getgdisurface, void *, NULL, DTTR_Graphics_COM_DirectDraw7 *self)

DTTR_COM_STUB_SET(
	ddraw7_getmonitorfrequency,
	DWORD,
	60,
	DTTR_Graphics_COM_DirectDraw7 *self
)

DTTR_COM_STUB_SET(ddraw7_getscanline, DWORD, 0, DTTR_Graphics_COM_DirectDraw7 *self)

DTTR_COM_STUB_SET(
	ddraw7_getverticalblankstatus,
	DWORD,
	0,
	DTTR_Graphics_COM_DirectDraw7 *self
)

DTTR_COM_NOOP_HRESULT(ddraw7_initialize, DTTR_Graphics_COM_DirectDraw7 *self, void *guid)

DTTR_COM_NOOP_HRESULT(ddraw7_restoredisplaymode, DTTR_Graphics_COM_DirectDraw7 *self)

DTTR_COM_NOOP_HRESULT(
	ddraw7_setcooperativelevel,
	DTTR_Graphics_COM_DirectDraw7 *self,
	HWND hwnd,
	DWORD flags
)

static HRESULT __stdcall ddraw7_setdisplaymode(
	DTTR_Graphics_COM_DirectDraw7 *self,
	DWORD w,
	DWORD h,
	DWORD bpp,
	DWORD hz,
	DWORD f
) {
	if (!self || w == 0 || h == 0) {
		return DDERR_INVALIDPARAMS;
	}

	if (bpp == 0) {
		bpp = 16;
	}

	if (hz == 0) {
		hz = 60;
	}

	DTTR_LOG_DEBUG(
		"SetDisplayMode request: %lux%lu %lu-bpp @ %luHz flags=0x%lx",
		(unsigned long)w,
		(unsigned long)h,
		(unsigned long)bpp,
		(unsigned long)hz,
		(unsigned long)f
	);
	dttr_graphics_set_logical_resolution((int)w, (int)h);
	return S_OK;
}

DTTR_COM_NOOP_HRESULT(
	ddraw7_waitforverticalblank,
	DTTR_Graphics_COM_DirectDraw7 *self,
	DWORD f,
	HANDLE evt
)

static HRESULT __stdcall ddraw7_getavailablevidmem(
	DTTR_Graphics_COM_DirectDraw7 *self,
	void *caps,
	DWORD *tot,
	DWORD *free
) {
	if (tot) {
		*tot = 128 * 1024 * 1024;
	}

	if (free) {
		*free = 64 * 1024 * 1024;
	}

	return S_OK;
}

DTTR_COM_STUB_SET(
	ddraw7_getsurfacefromdc,
	void *,
	NULL,
	DTTR_Graphics_COM_DirectDraw7 *self,
	HDC dc
)

DTTR_COM_NOOP_HRESULT(ddraw7_restoreallsurfaces, DTTR_Graphics_COM_DirectDraw7 *self)

DTTR_COM_NOOP_HRESULT(ddraw7_testcooperativelevel, DTTR_Graphics_COM_DirectDraw7 *self)

static HRESULT __stdcall ddraw7_getdeviceidentifier(
	DTTR_Graphics_COM_DirectDraw7 *self,
	void *id,
	DWORD f
) {
	if (id) {
		memset(id, 0, DTTR_SIZEOF_DDDEVICEIDENTIFIER2);
	}

	return S_OK;
}

DTTR_COM_NOOP_HRESULT(
	ddraw7_startmodetest,
	DTTR_Graphics_COM_DirectDraw7 *self,
	void *modes,
	DWORD n,
	DWORD f
)

DTTR_COM_NOOP_HRESULT(
	ddraw7_evaluatemode,
	DTTR_Graphics_COM_DirectDraw7 *self,
	DWORD f,
	DWORD *timeout
)

static DTTR_Graphics_COM_DirectDraw7_VT vtbl = {
	.QueryInterface = ddraw7_queryinterface,
	.AddRef = ddraw7_addref,
	.Release = ddraw7_release,
	.Compact = ddraw7_compact,
	.CreateClipper = ddraw7_createclipper,
	.CreatePalette = ddraw7_createpalette,
	.CreateSurface = ddraw7_createsurface,
	.DuplicateSurface = ddraw7_duplicatesurface,
	.EnumDisplayModes = ddraw7_enumdisplaymodes,
	.EnumSurfaces = ddraw7_enumsurfaces,
	.FlipToGDISurface = ddraw7_fliptogdisurface,
	.GetCaps = ddraw7_getcaps,
	.GetDisplayMode = ddraw7_getdisplaymode,
	.GetFourCCCodes = ddraw7_getfourcccodes,
	.GetGDISurface = ddraw7_getgdisurface,
	.GetMonitorFrequency = ddraw7_getmonitorfrequency,
	.GetScanLine = ddraw7_getscanline,
	.GetVerticalBlankStatus = ddraw7_getverticalblankstatus,
	.Initialize = ddraw7_initialize,
	.RestoreDisplayMode = ddraw7_restoredisplaymode,
	.SetCooperativeLevel = ddraw7_setcooperativelevel,
	.SetDisplayMode = ddraw7_setdisplaymode,
	.WaitForVerticalBlank = ddraw7_waitforverticalblank,
	.GetAvailableVidMem = ddraw7_getavailablevidmem,
	.GetSurfaceFromDC = ddraw7_getsurfacefromdc,
	.RestoreAllSurfaces = ddraw7_restoreallsurfaces,
	.TestCooperativeLevel = ddraw7_testcooperativelevel,
	.GetDeviceIdentifier = ddraw7_getdeviceidentifier,
	.StartModeTest = ddraw7_startmodetest,
	.EvaluateMode = ddraw7_evaluatemode,
};

DTTR_Graphics_COM_DirectDraw7 *dttr_graphics_com_create_directdraw7() {
	DTTR_Graphics_COM_DirectDraw7 *dd7 = malloc(sizeof(DTTR_Graphics_COM_DirectDraw7));
	if (dd7) {
		dd7->vtbl = &vtbl;
		DTTR_LOG_DEBUG(
			"Created DDraw7 translator at %p, vtbl=%p, QueryInterface=%p",
			dd7,
			dd7->vtbl,
			dd7->vtbl->QueryInterface
		);
	}
	return dd7;
}
