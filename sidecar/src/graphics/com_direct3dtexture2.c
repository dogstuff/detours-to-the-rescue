// Implements the IDirect3DTexture2 COM translator
// https://archive.org/details/dx7sdk-7001

#include "graphics_com_internal.h"
#include "log.h"
#include <stdlib.h>

DWORD g_dttr_texture2_next_texture_handle = 1;

DTTR_COM_QI_SELF(s_d3dtexture2_queryinterface, DTTR_Graphics_COM_Direct3DTexture2)

DTTR_COM_ADDREF(s_d3dtexture2_addref, DTTR_Graphics_COM_Direct3DTexture2)

DTTR_COM_RELEASE(s_d3dtexture2_release, DTTR_Graphics_COM_Direct3DTexture2)

static HRESULT __stdcall s_d3dtexture2_gethandle(
	DTTR_Graphics_COM_Direct3DTexture2 *self,
	void *device,
	DWORD *handle
) {
	if (handle) {
		*handle = g_dttr_texture2_next_texture_handle++;
	}

	return S_OK;
}

DTTR_COM_NOOP_HRESULT(
	s_d3dtexture2_palettechanged,
	DTTR_Graphics_COM_Direct3DTexture2 *self,
	DWORD start,
	DWORD count
)

DTTR_COM_NOOP_HRESULT(
	s_d3dtexture2_load,
	DTTR_Graphics_COM_Direct3DTexture2 *self,
	void *srcTexture
)

static DTTR_Graphics_COM_Direct3DTexture2_VT s_vtbl = {
	.QueryInterface = s_d3dtexture2_queryinterface,
	.AddRef = s_d3dtexture2_addref,
	.Release = s_d3dtexture2_release,
	.GetHandle = s_d3dtexture2_gethandle,
	.PaletteChanged = s_d3dtexture2_palettechanged,
	.Load = s_d3dtexture2_load,
};

DTTR_Graphics_COM_Direct3DTexture2 *dttr_graphics_com_create_direct3d_texture2(
	DTTR_Graphics_COM_DirectDrawSurface7 *surface
) {
	DTTR_Graphics_COM_Direct3DTexture2 *tex = malloc(
		sizeof(DTTR_Graphics_COM_Direct3DTexture2)
	);

	if (tex) {
		tex->m_vtbl = &s_vtbl;
		tex->m_surface = surface;
	}
	return tex;
}
