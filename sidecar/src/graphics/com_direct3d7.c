// IDirect3D7 COM translator that creates Device7 instances
// https://archive.org/details/dx7sdk-7001

#define INITGUID
#include "graphics_com_internal.h"
#include <stdlib.h>
#include <string.h>

static DTTR_Graphics_COM_Direct3DDevice7 *s_d3d7_device;

static DTTR_Graphics_COM_Direct3DDevice7 *s_d3d7_get_device(void) {
	if (!s_d3d7_device) {
		s_d3d7_device = dttr_graphics_com_create_direct3ddevice7();
	}

	return s_d3d7_device;
}

DTTR_COM_QI_SELF(s_d3d7_query_interface, DTTR_Graphics_COM_Direct3D7)

DTTR_COM_ADDREF(s_d3d7_addref, DTTR_Graphics_COM_Direct3D7)

DTTR_COM_RELEASE(s_d3d7_release, DTTR_Graphics_COM_Direct3D7)

static HRESULT __stdcall s_d3d7_enum_devices(
	DTTR_Graphics_COM_Direct3D7 *self,
	void *cb,
	void *ctx
) {
	const LPD3DENUMDEVICESCALLBACK7 callback = (LPD3DENUMDEVICESCALLBACK7)cb;

	if (!callback) {
		return S_OK;
	}

	// Advertise one HAL-style hardware T&L device
	D3DDEVICEDESC7 desc;
	memset(&desc, 0, sizeof(desc));

	desc.dwDevCaps = D3DDEVCAPS_FLOATTLVERTEX | D3DDEVCAPS_EXECUTEVIDEOMEMORY
					 | D3DDEVCAPS_TLVERTEXVIDEOMEMORY | D3DDEVCAPS_TEXTUREVIDEOMEMORY
					 | D3DDEVCAPS_DRAWPRIMTLVERTEX | D3DDEVCAPS_CANRENDERAFTERFLIP
					 | D3DDEVCAPS_DRAWPRIMITIVES2 | D3DDEVCAPS_DRAWPRIMITIVES2EX
					 | D3DDEVCAPS_HWTRANSFORMANDLIGHT | D3DDEVCAPS_HWRASTERIZATION;

	desc.dwMinTextureWidth = 1;
	desc.dwMinTextureHeight = 1;
	desc.dwMaxTextureWidth = 4096;
	desc.dwMaxTextureHeight = 4096;
	desc.dwMaxTextureRepeat = 32768;
	desc.dwMaxTextureAspectRatio = 4096;
	desc.dwMaxAnisotropy = 16;

	desc.wMaxTextureBlendStages = 8;
	desc.wMaxSimultaneousTextures = 8;

	desc.dwMaxActiveLights = 8;

	desc.wMaxUserClipPlanes = 6;
	desc.wMaxVertexBlendMatrices = 4;

	desc.deviceGUID = IID_IDirect3DHALDevice;

	desc.dwDeviceRenderBitDepth = DTTR_DDBD_16 | DTTR_DDBD_32;
	desc.dwDeviceZBufferBitDepth = DTTR_DDBD_16 | DTTR_DDBD_32;

	callback("DTTR Hardware Accelerated Device", "HAL", &desc, ctx);

	return S_OK;
}

static HRESULT __stdcall s_d3d7_createdevice(
	DTTR_Graphics_COM_Direct3D7 *self,
	void *guid,
	void *surf,
	void **dev
) {
	DTTR_Graphics_COM_Direct3DDevice7 *device = s_d3d7_get_device();

	if (dev) {
		*dev = device;
	}

	return S_OK;
}

static HRESULT __stdcall s_d3d7_createvertexbuffer(
	DTTR_Graphics_COM_Direct3D7 *self,
	void *desc,
	void **vb,
	DWORD flags
) {
	if (vb) {
		*vb = NULL;
	}

	return S_OK;
}

static HRESULT __stdcall s_d3d7_enumzbufferformats(
	DTTR_Graphics_COM_Direct3D7 *self,
	void *guid,
	void *cb,
	void *ctx
) {
	const LPD3DENUMPIXELFORMATSCALLBACK callback = (LPD3DENUMPIXELFORMATSCALLBACK)cb;

	if (!callback) {
		return S_OK;
	}

	DDPIXELFORMAT fmt16;
	memset(&fmt16, 0, sizeof(fmt16));
	fmt16.dwSize = sizeof(DDPIXELFORMAT);
	fmt16.dwFlags = DDPF_ZBUFFER;
	fmt16.dwZBufferBitDepth = 16;
	fmt16.dwZBitMask = 0xFFFF;

	const HRESULT result = callback(&fmt16, ctx);

	if (result != D3DENUMRET_OK) {
		return S_OK;
	}

	DDPIXELFORMAT fmt24s8;
	memset(&fmt24s8, 0, sizeof(fmt24s8));
	fmt24s8.dwSize = sizeof(DDPIXELFORMAT);
	fmt24s8.dwFlags = DDPF_ZBUFFER | DDPF_STENCILBUFFER;
	fmt24s8.dwZBufferBitDepth = 32;
	fmt24s8.dwStencilBitDepth = 8;
	fmt24s8.dwZBitMask = 0x00FFFFFF;
	fmt24s8.dwStencilBitMask = 0xFF000000;

	callback(&fmt24s8, ctx);

	return S_OK;
}

DTTR_COM_NOOP_HRESULT(s_d3d7_evict_managed_textures, DTTR_Graphics_COM_Direct3D7 *self)

static DTTR_Graphics_COM_Direct3D7_VT s_vtbl = {
	.QueryInterface = s_d3d7_query_interface,
	.AddRef = s_d3d7_addref,
	.Release = s_d3d7_release,
	.EnumDevices = s_d3d7_enum_devices,
	.CreateDevice = s_d3d7_createdevice,
	.CreateVertexBuffer = s_d3d7_createvertexbuffer,
	.EnumZBufferFormats = s_d3d7_enumzbufferformats,
	.EvictManagedTextures = s_d3d7_evict_managed_textures,
};

DTTR_Graphics_COM_Direct3D7 *dttr_graphics_com_create_direct3d7(void) {
	DTTR_Graphics_COM_Direct3D7 *d3d = malloc(sizeof(DTTR_Graphics_COM_Direct3D7));
	if (d3d) {
		d3d->m_vtbl = &s_vtbl;
	}
	return d3d;
}
