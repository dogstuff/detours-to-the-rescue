#ifndef DTTR_D3D_COM_H
#define DTTR_D3D_COM_H

#include <d3d.h>
#include <ddraw.h>
#include <stdbool.h>
#include <stdint.h>
#include <windows.h>

// ---------------------------------------------------------------------------
// D3D and DDraw struct sizes used by stub memset and size checks
// https://learn.microsoft.com/en-us/windows/win32/api/ddraw/ns-ddraw-ddsurfacedesc2
// https://learn.microsoft.com/en-us/windows/win32/api/ddraw/ns-ddraw-ddpixelformat
// ---------------------------------------------------------------------------
#define DTTR_SIZEOF_D3DDEVICEDESC7 224
#define DTTR_SIZEOF_D3DMATERIAL7 76
#define DTTR_SIZEOF_D3DLIGHT7 104
#define DTTR_SIZEOF_D3DCLIPSTATUS 8
#define DTTR_SIZEOF_DDPIXELFORMAT 32
#define DTTR_SIZEOF_DDSCAPS2 16
#define DTTR_SIZEOF_DDSURFACEDESC 108
#define DTTR_SIZEOF_DDSURFACEDESC2 124
#define DTTR_SIZEOF_DDDEVICEIDENTIFIER2 1084

// ---------------------------------------------------------------------------
// DirectDraw bit-depth flags (DDBD_*)
// https://learn.microsoft.com/en-us/windows/win32/api/ddraw/ns-ddraw-ddcaps_dx7
// ---------------------------------------------------------------------------
#define DTTR_DDBD_16 0x00000400
#define DTTR_DDBD_32 0x00000200

// ---------------------------------------------------------------------------
// HRESULT and error codes
// ---------------------------------------------------------------------------
#define DTTR_DDERR_GENERIC 0x887601C2

// ---------------------------------------------------------------------------
// D3D Flexible Vertex Format (FVF) masks and flags
// https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dfvf
// https://learn.microsoft.com/en-us/windows/win32/direct3d9/fixed-function-fvf-codes
// ---------------------------------------------------------------------------
#define DTTR_D3DFVF_POSITION_MASK 0x400E

#define DTTR_D3DFVF_XYZ 0x0002
#define DTTR_D3DFVF_XYZRHW 0x0004
#define DTTR_D3DFVF_XYZB1 0x0006
#define DTTR_D3DFVF_XYZB2 0x0008
#define DTTR_D3DFVF_XYZB3 0x000A
#define DTTR_D3DFVF_XYZB4 0x000C
#define DTTR_D3DFVF_XYZB5 0x000E
#define DTTR_D3DFVF_XYZW 0x4002

#define DTTR_D3DFVF_NORMAL 0x010
#define DTTR_D3DFVF_PSIZE 0x020
#define DTTR_D3DFVF_DIFFUSE 0x040
#define DTTR_D3DFVF_SPECULAR 0x080
#define DTTR_D3DFVF_TEXCOUNT_MASK 0xF00
#define DTTR_D3DFVF_TEXCOUNT_SHIFT 8
#define DTTR_D3DFVF_LASTBETA_UBYTE4 0x1000
#define DTTR_D3DFVF_LASTBETA_D3DCOLOR 0x8000

// ---------------------------------------------------------------------------
// D3D primitive types (D3DPRIMITIVETYPE enum values)
// https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dprimitivetype
// ---------------------------------------------------------------------------
#define DTTR_D3DPT_POINTLIST 1
#define DTTR_D3DPT_LINELIST 2
#define DTTR_D3DPT_LINESTRIP 3
#define DTTR_D3DPT_TRIANGLELIST 4
#define DTTR_D3DPT_TRIANGLESTRIP 5
#define DTTR_D3DPT_TRIANGLEFAN 6

#define DTTR_COM_NOOP_HRESULT(fn, ...)                                                   \
	static HRESULT __stdcall fn(__VA_ARGS__) { return S_OK; }

#define DTTR_COM_ADDREF(fn, type)                                                        \
	static ULONG __stdcall fn(type *self) { return 1; }

#define DTTR_COM_RELEASE(fn, type)                                                       \
	static ULONG __stdcall fn(type *self) { return 0; }

#define DTTR_COM_QI_SELF(fn, type)                                                       \
	static HRESULT __stdcall fn(type *self, void *riid, void **ppv) {                    \
		if (ppv)                                                                         \
			*ppv = self;                                                                 \
		return S_OK;                                                                     \
	}

#define DTTR_COM_STUB_SET(fn, out_type, val, ...)                                        \
	static HRESULT __stdcall fn(__VA_ARGS__, out_type *out) {                            \
		if (out)                                                                         \
			*out = val;                                                                  \
		return S_OK;                                                                     \
	}

#define DTTR_COM_STUB_MEMSET(fn, size, buf_type, ...)                                    \
	static HRESULT __stdcall fn(__VA_ARGS__, buf_type *buf) {                            \
		if (buf)                                                                         \
			memset(buf, 0, size);                                                        \
		return S_OK;                                                                     \
	}

typedef struct DTTR_Graphics_COM_Direct3D7 DTTR_Graphics_COM_Direct3D7;
typedef struct DTTR_Graphics_COM_Direct3DDevice7 DTTR_Graphics_COM_Direct3DDevice7;
typedef struct DTTR_Graphics_COM_DirectDraw7 DTTR_Graphics_COM_DirectDraw7;

/// Creates an IDirectDraw7 translator instance
DTTR_Graphics_COM_DirectDraw7 *dttr_graphics_com_create_directdraw7(void);
/// Creates an IDirect3D7 translator instance
DTTR_Graphics_COM_Direct3D7 *dttr_graphics_com_create_direct3d7(void);
/// Creates an IDirect3DDevice7 translator instance
DTTR_Graphics_COM_Direct3DDevice7 *dttr_graphics_com_create_direct3ddevice7(void);

typedef struct DTTR_Graphics_COM_Direct3DDevice7_VT {
	HRESULT(__stdcall *QueryInterface)(
		DTTR_Graphics_COM_Direct3DDevice7 *self,
		void *riid,
		void **ppv
	);																	// 0x00
	ULONG(__stdcall *AddRef)(DTTR_Graphics_COM_Direct3DDevice7 *self);	// 0x04
	ULONG(__stdcall *Release)(DTTR_Graphics_COM_Direct3DDevice7 *self); // 0x08
	HRESULT(__stdcall *GetCaps)(
		DTTR_Graphics_COM_Direct3DDevice7 *self,
		void *caps
	); // 0x0C
	HRESULT(__stdcall *EnumTextureFormats)(
		DTTR_Graphics_COM_Direct3DDevice7 *self,
		void *cb,
		void *ctx
	);																		 // 0x10
	HRESULT(__stdcall *BeginScene)(DTTR_Graphics_COM_Direct3DDevice7 *self); // 0x14
	HRESULT(__stdcall *EndScene)(DTTR_Graphics_COM_Direct3DDevice7 *self);	 // 0x18
	HRESULT(__stdcall *GetDirect3D)(
		DTTR_Graphics_COM_Direct3DDevice7 *self,
		void **d3d
	); // 0x1C
	HRESULT(__stdcall *SetRenderTarget)(
		DTTR_Graphics_COM_Direct3DDevice7 *self,
		void *surf,
		DWORD f
	); // 0x20
	HRESULT(__stdcall *GetRenderTarget)(
		DTTR_Graphics_COM_Direct3DDevice7 *self,
		void **surf
	); // 0x24
	HRESULT(__stdcall *Clear)(
		DTTR_Graphics_COM_Direct3DDevice7 *self,
		DWORD n,
		void *rects,
		DWORD f,
		DWORD col,
		float z,
		DWORD s
	); // 0x28
	HRESULT(__stdcall *SetTransform)(
		DTTR_Graphics_COM_Direct3DDevice7 *self,
		DWORD m_type,
		void *mat
	); // 0x2C
	HRESULT(__stdcall *GetTransform)(
		DTTR_Graphics_COM_Direct3DDevice7 *self,
		DWORD m_type,
		void *mat
	); // 0x30
	HRESULT(__stdcall *SetViewport)(
		DTTR_Graphics_COM_Direct3DDevice7 *self,
		void *vp
	); // 0x34
	HRESULT(__stdcall *MultiplyTransform)(
		DTTR_Graphics_COM_Direct3DDevice7 *self,
		DWORD m_type,
		void *m
	); // 0x38
	HRESULT(__stdcall *GetViewport)(
		DTTR_Graphics_COM_Direct3DDevice7 *self,
		void *vp
	); // 0x3C
	HRESULT(__stdcall *SetMaterial)(
		DTTR_Graphics_COM_Direct3DDevice7 *self,
		void *mat
	); // 0x40
	HRESULT(__stdcall *GetMaterial)(
		DTTR_Graphics_COM_Direct3DDevice7 *self,
		void *mat
	); // 0x44
	HRESULT(__stdcall *SetLight)(
		DTTR_Graphics_COM_Direct3DDevice7 *self,
		DWORD idx,
		void *light
	); // 0x48
	HRESULT(__stdcall *GetLight)(
		DTTR_Graphics_COM_Direct3DDevice7 *self,
		DWORD idx,
		void *light
	); // 0x4C
	HRESULT(__stdcall *SetRenderState)(
		DTTR_Graphics_COM_Direct3DDevice7 *self,
		DWORD state,
		DWORD val
	); // 0x50
	HRESULT(__stdcall *GetRenderState)(
		DTTR_Graphics_COM_Direct3DDevice7 *self,
		DWORD state,
		DWORD *v
	);																			  // 0x54
	HRESULT(__stdcall *BeginStateBlock)(DTTR_Graphics_COM_Direct3DDevice7 *self); // 0x58
	HRESULT(__stdcall *EndStateBlock)(
		DTTR_Graphics_COM_Direct3DDevice7 *self,
		DWORD *block
	); // 0x5C
	HRESULT(__stdcall *PreLoad)(DTTR_Graphics_COM_Direct3DDevice7 *self, void *tex); // 0x60
	HRESULT(__stdcall *DrawPrimitive)(
		DTTR_Graphics_COM_Direct3DDevice7 *self,
		DWORD prim,
		DWORD fvf,
		void *v,
		DWORD n,
		DWORD f
	); // 0x64
	HRESULT(__stdcall *DrawIndexedPrimitive)(
		DTTR_Graphics_COM_Direct3DDevice7 *self,
		DWORD prim,
		DWORD fvf,
		void *v,
		DWORD vn,
		WORD *i,
		DWORD in,
		DWORD f
	); // 0x68
	HRESULT(__stdcall *SetClipStatus)(
		DTTR_Graphics_COM_Direct3DDevice7 *self,
		void *status
	); // 0x6C
	HRESULT(__stdcall *GetClipStatus)(
		DTTR_Graphics_COM_Direct3DDevice7 *self,
		void *status
	); // 0x70
	HRESULT(__stdcall *DrawPrimitiveStrided)(
		DTTR_Graphics_COM_Direct3DDevice7 *self,
		DWORD prim,
		DWORD fvf,
		void *d,
		DWORD n,
		DWORD f
	); // 0x74
	HRESULT(__stdcall *DrawIndexedPrimitiveStrided)(
		DTTR_Graphics_COM_Direct3DDevice7 *self,
		DWORD prim,
		DWORD fvf,
		void *d,
		DWORD vn,
		WORD *i,
		DWORD in,
		DWORD f
	); // 0x78
	HRESULT(__stdcall *DrawPrimitiveVB)(
		DTTR_Graphics_COM_Direct3DDevice7 *self,
		DWORD prim,
		void *vb,
		DWORD st,
		DWORD n,
		DWORD f
	); // 0x7C
	HRESULT(__stdcall *DrawIndexedPrimitiveVB)(
		DTTR_Graphics_COM_Direct3DDevice7 *self,
		DWORD prim,
		void *vb,
		DWORD st,
		DWORD vn,
		WORD *i,
		DWORD in,
		DWORD f
	); // 0x80
	HRESULT(__stdcall *ComputeSphereVisibility)(
		DTTR_Graphics_COM_Direct3DDevice7 *self,
		void *c,
		float *r,
		DWORD n,
		DWORD f,
		DWORD *res
	); // 0x84
	HRESULT(__stdcall *GetTexture)(
		DTTR_Graphics_COM_Direct3DDevice7 *self,
		DWORD stage,
		void **tex
	); // 0x88
	HRESULT(__stdcall *SetTexture)(
		DTTR_Graphics_COM_Direct3DDevice7 *self,
		DWORD stage,
		void *tex
	); // 0x8C
	HRESULT(__stdcall *GetTextureStageState)(
		DTTR_Graphics_COM_Direct3DDevice7 *self,
		DWORD stg,
		DWORD t,
		DWORD *v
	); // 0x90
	HRESULT(__stdcall *SetTextureStageState)(
		DTTR_Graphics_COM_Direct3DDevice7 *self,
		DWORD stg,
		DWORD t,
		DWORD v
	); // 0x94
	HRESULT(__stdcall *ValidateDevice)(
		DTTR_Graphics_COM_Direct3DDevice7 *self,
		DWORD *passes
	); // 0x98
	HRESULT(__stdcall *ApplyStateBlock)(
		DTTR_Graphics_COM_Direct3DDevice7 *self,
		DWORD block
	); // 0x9C
	HRESULT(__stdcall *CaptureStateBlock)(
		DTTR_Graphics_COM_Direct3DDevice7 *self,
		DWORD block
	); // 0xA0
	HRESULT(__stdcall *DeleteStateBlock)(
		DTTR_Graphics_COM_Direct3DDevice7 *self,
		DWORD block
	); // 0xA4
	HRESULT(__stdcall *CreateStateBlock)(
		DTTR_Graphics_COM_Direct3DDevice7 *self,
		DWORD t,
		DWORD *blk
	); // 0xA8
	HRESULT(__stdcall *Load)(
		DTTR_Graphics_COM_Direct3DDevice7 *self,
		void *dst,
		void *dstPt,
		void *src,
		void *srcR,
		DWORD f
	); // 0xAC
	HRESULT(__stdcall *LightEnable)(
		DTTR_Graphics_COM_Direct3DDevice7 *self,
		DWORD idx,
		BOOL enable
	); // 0xB0
	HRESULT(__stdcall *GetLightEnable)(
		DTTR_Graphics_COM_Direct3DDevice7 *self,
		DWORD idx,
		BOOL *en
	); // 0xB4
	HRESULT(__stdcall *SetClipPlane)(
		DTTR_Graphics_COM_Direct3DDevice7 *self,
		DWORD idx,
		float *plane
	); // 0xB8
	HRESULT(__stdcall *GetClipPlane)(
		DTTR_Graphics_COM_Direct3DDevice7 *self,
		DWORD idx,
		float *plane
	); // 0xBC
	HRESULT(__stdcall *GetInfo)(
		DTTR_Graphics_COM_Direct3DDevice7 *self,
		DWORD id,
		void *info,
		DWORD sz
	); // 0xC0
} DTTR_Graphics_COM_Direct3DDevice7_VT;

typedef struct DTTR_Graphics_COM_Direct3DDevice7 {
	DTTR_Graphics_COM_Direct3DDevice7_VT *m_vtbl;
} DTTR_Graphics_COM_Direct3DDevice7;

typedef struct DTTR_Graphics_COM_DirectDraw7_VT {
	HRESULT(__stdcall *QueryInterface)(
		DTTR_Graphics_COM_DirectDraw7 *self,
		void *riid,
		void **ppv
	);																  // 0x00
	ULONG(__stdcall *AddRef)(DTTR_Graphics_COM_DirectDraw7 *self);	  // 0x04
	ULONG(__stdcall *Release)(DTTR_Graphics_COM_DirectDraw7 *self);	  // 0x08
	HRESULT(__stdcall *Compact)(DTTR_Graphics_COM_DirectDraw7 *self); // 0x0C
	HRESULT(__stdcall *CreateClipper)(
		DTTR_Graphics_COM_DirectDraw7 *self,
		DWORD f,
		void **clip,
		void *outer
	); // 0x10
	HRESULT(__stdcall *CreatePalette)(
		DTTR_Graphics_COM_DirectDraw7 *self,
		DWORD f,
		void *colors,
		void **pal,
		void *outer
	); // 0x14
	HRESULT(__stdcall *CreateSurface)(
		DTTR_Graphics_COM_DirectDraw7 *self,
		void *desc,
		void **surf,
		void *outer
	); // 0x18
	HRESULT(__stdcall *DuplicateSurface)(
		DTTR_Graphics_COM_DirectDraw7 *self,
		void *src,
		void **dst
	); // 0x1C
	HRESULT(__stdcall *EnumDisplayModes)(
		DTTR_Graphics_COM_DirectDraw7 *self,
		DWORD f,
		void *desc,
		void *ctx,
		void *cb
	); // 0x20
	HRESULT(__stdcall *EnumSurfaces)(
		DTTR_Graphics_COM_DirectDraw7 *self,
		DWORD f,
		void *desc,
		void *ctx,
		void *cb
	);																		   // 0x24
	HRESULT(__stdcall *FlipToGDISurface)(DTTR_Graphics_COM_DirectDraw7 *self); // 0x28
	HRESULT(__stdcall *GetCaps)(
		DTTR_Graphics_COM_DirectDraw7 *self,
		void *drvCaps,
		void *helCaps
	); // 0x2C
	HRESULT(__stdcall *GetDisplayMode)(
		DTTR_Graphics_COM_DirectDraw7 *self,
		void *desc
	); // 0x30
	HRESULT(__stdcall *GetFourCCCodes)(
		DTTR_Graphics_COM_DirectDraw7 *self,
		DWORD *num,
		DWORD *codes
	); // 0x34
	HRESULT(__stdcall *GetGDISurface)(
		DTTR_Graphics_COM_DirectDraw7 *self,
		void **surf
	); // 0x38
	HRESULT(__stdcall *GetMonitorFrequency)(
		DTTR_Graphics_COM_DirectDraw7 *self,
		DWORD *freq
	); // 0x3C
	HRESULT(__stdcall *GetScanLine)(
		DTTR_Graphics_COM_DirectDraw7 *self,
		DWORD *line
	); // 0x40
	HRESULT(__stdcall *GetVerticalBlankStatus)(
		DTTR_Graphics_COM_DirectDraw7 *self,
		DWORD *inVB
	); // 0x44
	HRESULT(__stdcall *Initialize)(DTTR_Graphics_COM_DirectDraw7 *self, void *guid); // 0x48
	HRESULT(__stdcall *RestoreDisplayMode)(DTTR_Graphics_COM_DirectDraw7 *self); // 0x4C
	HRESULT(__stdcall *SetCooperativeLevel)(
		DTTR_Graphics_COM_DirectDraw7 *self,
		HWND hwnd,
		DWORD f
	); // 0x50
	HRESULT(__stdcall *SetDisplayMode)(
		DTTR_Graphics_COM_DirectDraw7 *self,
		DWORD w,
		DWORD h,
		DWORD m_bpp,
		DWORD hz,
		DWORD f
	); // 0x54
	HRESULT(__stdcall *WaitForVerticalBlank)(
		DTTR_Graphics_COM_DirectDraw7 *self,
		DWORD f,
		HANDLE evt
	); // 0x58
	HRESULT(__stdcall *GetAvailableVidMem)(
		DTTR_Graphics_COM_DirectDraw7 *self,
		void *caps,
		DWORD *tot,
		DWORD *free
	); // 0x5C
	HRESULT(__stdcall *GetSurfaceFromDC)(
		DTTR_Graphics_COM_DirectDraw7 *self,
		HDC dc,
		void **surf
	);																			   // 0x60
	HRESULT(__stdcall *RestoreAllSurfaces)(DTTR_Graphics_COM_DirectDraw7 *self);   // 0x64
	HRESULT(__stdcall *TestCooperativeLevel)(DTTR_Graphics_COM_DirectDraw7 *self); // 0x68
	HRESULT(__stdcall *GetDeviceIdentifier)(
		DTTR_Graphics_COM_DirectDraw7 *self,
		void *id,
		DWORD f
	); // 0x6C
	HRESULT(__stdcall *StartModeTest)(
		DTTR_Graphics_COM_DirectDraw7 *self,
		void *modes,
		DWORD n,
		DWORD f
	); // 0x70
	HRESULT(__stdcall *EvaluateMode)(
		DTTR_Graphics_COM_DirectDraw7 *self,
		DWORD f,
		DWORD *timeout
	); // 0x74
} DTTR_Graphics_COM_DirectDraw7_VT;

typedef struct DTTR_Graphics_COM_DirectDraw7 {
	DTTR_Graphics_COM_DirectDraw7_VT *m_vtbl;
} DTTR_Graphics_COM_DirectDraw7;

typedef struct DTTR_Graphics_COM_Direct3D7_VT {
	HRESULT(__stdcall *QueryInterface)(
		DTTR_Graphics_COM_Direct3D7 *self,
		void *riid,
		void **ppv
	);
	ULONG(__stdcall *AddRef)(DTTR_Graphics_COM_Direct3D7 *self);
	ULONG(__stdcall *Release)(DTTR_Graphics_COM_Direct3D7 *self);
	HRESULT(__stdcall *EnumDevices)(
		DTTR_Graphics_COM_Direct3D7 *self,
		void *cb,
		void *ctx
	);
	HRESULT(__stdcall *CreateDevice)(
		DTTR_Graphics_COM_Direct3D7 *self,
		void *guid,
		void *surf,
		void **dev
	);
	HRESULT(__stdcall *CreateVertexBuffer)(
		DTTR_Graphics_COM_Direct3D7 *self,
		void *desc,
		void **vb,
		DWORD f
	);
	HRESULT(__stdcall *EnumZBufferFormats)(
		DTTR_Graphics_COM_Direct3D7 *self,
		void *guid,
		void *cb,
		void *ctx
	);
	HRESULT(__stdcall *EvictManagedTextures)(DTTR_Graphics_COM_Direct3D7 *self);
} DTTR_Graphics_COM_Direct3D7_VT;

typedef struct DTTR_Graphics_COM_Direct3D7 {
	DTTR_Graphics_COM_Direct3D7_VT *m_vtbl;
} DTTR_Graphics_COM_Direct3D7;

typedef struct DTTR_Graphics_COM_DirectDrawSurface7 DTTR_Graphics_COM_DirectDrawSurface7;
typedef struct DTTR_Graphics_COM_Direct3DTexture2 DTTR_Graphics_COM_Direct3DTexture2;

typedef struct DTTR_Graphics_COM_Direct3DTexture2_VT {
	HRESULT(__stdcall *QueryInterface)(
		DTTR_Graphics_COM_Direct3DTexture2 *self,
		void *riid,
		void **ppv
	);
	ULONG(__stdcall *AddRef)(DTTR_Graphics_COM_Direct3DTexture2 *self);
	ULONG(__stdcall *Release)(DTTR_Graphics_COM_Direct3DTexture2 *self);
	HRESULT(__stdcall *GetHandle)(
		DTTR_Graphics_COM_Direct3DTexture2 *self,
		void *m_device,
		DWORD *handle
	);
	HRESULT(__stdcall *PaletteChanged)(
		DTTR_Graphics_COM_Direct3DTexture2 *self,
		DWORD start,
		DWORD count
	);
	HRESULT(__stdcall *Load)(DTTR_Graphics_COM_Direct3DTexture2 *self, void *srcTexture);
} DTTR_Graphics_COM_Direct3DTexture2_VT;

typedef struct DTTR_Graphics_COM_Direct3DTexture2 {
	DTTR_Graphics_COM_Direct3DTexture2_VT *m_vtbl;
	DTTR_Graphics_COM_DirectDrawSurface7 *m_surface;
} DTTR_Graphics_COM_Direct3DTexture2;

typedef struct DTTR_Graphics_COM_DirectDrawSurface7_VT {
	HRESULT(__stdcall *QueryInterface)(
		DTTR_Graphics_COM_DirectDrawSurface7 *self,
		void *riid,
		void **ppv
	);
	ULONG(__stdcall *AddRef)(DTTR_Graphics_COM_DirectDrawSurface7 *self);
	ULONG(__stdcall *Release)(DTTR_Graphics_COM_DirectDrawSurface7 *self);
	HRESULT(__stdcall *AddAttachedSurface)(
		DTTR_Graphics_COM_DirectDrawSurface7 *self,
		void *surf
	);
	HRESULT(__stdcall *AddOverlayDirtyRect)(
		DTTR_Graphics_COM_DirectDrawSurface7 *self,
		void *rect
	);
	HRESULT(__stdcall *Blt)(
		DTTR_Graphics_COM_DirectDrawSurface7 *self,
		void *dstRect,
		void *srcSurf,
		void *srcRect,
		DWORD m_flags,
		void *bltFx
	);
	HRESULT(__stdcall *BltBatch)(
		DTTR_Graphics_COM_DirectDrawSurface7 *self,
		void *batch,
		DWORD count,
		DWORD m_flags
	);
	HRESULT(__stdcall *BltFast)(
		DTTR_Graphics_COM_DirectDrawSurface7 *self,
		DWORD x,
		DWORD y,
		void *srcSurf,
		void *srcRect,
		DWORD m_flags
	);
	HRESULT(__stdcall *DeleteAttachedSurface)(
		DTTR_Graphics_COM_DirectDrawSurface7 *self,
		DWORD m_flags,
		void *surf
	);
	HRESULT(__stdcall *EnumAttachedSurfaces)(
		DTTR_Graphics_COM_DirectDrawSurface7 *self,
		void *ctx,
		void *cb
	);
	HRESULT(__stdcall *EnumOverlayZOrders)(
		DTTR_Graphics_COM_DirectDrawSurface7 *self,
		DWORD m_flags,
		void *ctx,
		void *cb
	);
	HRESULT(__stdcall *Flip)(
		DTTR_Graphics_COM_DirectDrawSurface7 *self,
		void *target,
		DWORD m_flags
	);
	HRESULT(__stdcall *GetAttachedSurface)(
		DTTR_Graphics_COM_DirectDrawSurface7 *self,
		void *caps,
		void **surf
	);
	HRESULT(__stdcall *GetBltStatus)(
		DTTR_Graphics_COM_DirectDrawSurface7 *self,
		DWORD m_flags
	);
	HRESULT(__stdcall *GetCaps)(DTTR_Graphics_COM_DirectDrawSurface7 *self, void *caps);
	HRESULT(__stdcall *GetClipper)(
		DTTR_Graphics_COM_DirectDrawSurface7 *self,
		void **clipper
	);
	HRESULT(__stdcall *GetColorKey)(
		DTTR_Graphics_COM_DirectDrawSurface7 *self,
		DWORD m_flags,
		void *colorKey
	);
	HRESULT(__stdcall *GetDC)(DTTR_Graphics_COM_DirectDrawSurface7 *self, HDC *dc);
	HRESULT(__stdcall *GetFlipStatus)(
		DTTR_Graphics_COM_DirectDrawSurface7 *self,
		DWORD m_flags
	);
	HRESULT(__stdcall *GetOverlayPosition)(
		DTTR_Graphics_COM_DirectDrawSurface7 *self,
		LONG *x,
		LONG *y
	);
	HRESULT(__stdcall *GetPalette)(
		DTTR_Graphics_COM_DirectDrawSurface7 *self,
		void **palette
	);
	HRESULT(__stdcall *GetPixelFormat)(
		DTTR_Graphics_COM_DirectDrawSurface7 *self,
		void *fmt
	);
	HRESULT(__stdcall *GetSurfaceDesc)(
		DTTR_Graphics_COM_DirectDrawSurface7 *self,
		void *desc
	);
	HRESULT(__stdcall *Initialize)(
		DTTR_Graphics_COM_DirectDrawSurface7 *self,
		void *dd,
		void *desc
	);
	HRESULT(__stdcall *IsLost)(DTTR_Graphics_COM_DirectDrawSurface7 *self);
	HRESULT(__stdcall *Lock)(
		DTTR_Graphics_COM_DirectDrawSurface7 *self,
		void *rect,
		void *desc,
		DWORD m_flags,
		HANDLE event
	);
	HRESULT(__stdcall *ReleaseDC)(DTTR_Graphics_COM_DirectDrawSurface7 *self, HDC dc);
	HRESULT(__stdcall *Restore)(DTTR_Graphics_COM_DirectDrawSurface7 *self);
	HRESULT(__stdcall *SetClipper)(
		DTTR_Graphics_COM_DirectDrawSurface7 *self,
		void *clipper
	);
	HRESULT(__stdcall *SetColorKey)(
		DTTR_Graphics_COM_DirectDrawSurface7 *self,
		DWORD m_flags,
		void *colorKey
	);
	HRESULT(__stdcall *SetOverlayPosition)(
		DTTR_Graphics_COM_DirectDrawSurface7 *self,
		LONG x,
		LONG y
	);
	HRESULT(__stdcall *SetPalette)(
		DTTR_Graphics_COM_DirectDrawSurface7 *self,
		void *palette
	);
	HRESULT(__stdcall *Unlock)(DTTR_Graphics_COM_DirectDrawSurface7 *self, void *rect);
	HRESULT(__stdcall *UpdateOverlay)(
		DTTR_Graphics_COM_DirectDrawSurface7 *self,
		void *srcRect,
		void *dstSurf,
		void *dstRect,
		DWORD m_flags,
		void *fx
	);
	HRESULT(__stdcall *UpdateOverlayDisplay)(
		DTTR_Graphics_COM_DirectDrawSurface7 *self,
		DWORD m_flags
	);
	HRESULT(__stdcall *UpdateOverlayZOrder)(
		DTTR_Graphics_COM_DirectDrawSurface7 *self,
		DWORD m_flags,
		void *refSurf
	);
	HRESULT(__stdcall *GetDDInterface)(
		DTTR_Graphics_COM_DirectDrawSurface7 *self,
		void **dd
	);
	HRESULT(__stdcall *PageLock)(
		DTTR_Graphics_COM_DirectDrawSurface7 *self,
		DWORD m_flags
	);
	HRESULT(__stdcall *PageUnlock)(
		DTTR_Graphics_COM_DirectDrawSurface7 *self,
		DWORD m_flags
	);
	HRESULT(__stdcall *SetSurfaceDesc)(
		DTTR_Graphics_COM_DirectDrawSurface7 *self,
		void *desc,
		DWORD m_flags
	);
	HRESULT(__stdcall *SetPrivateData)(
		DTTR_Graphics_COM_DirectDrawSurface7 *self,
		void *tag,
		void *data,
		DWORD size,
		DWORD m_flags
	);
	HRESULT(__stdcall *GetPrivateData)(
		DTTR_Graphics_COM_DirectDrawSurface7 *self,
		void *tag,
		void *data,
		DWORD *size
	);
	HRESULT(__stdcall *FreePrivateData)(
		DTTR_Graphics_COM_DirectDrawSurface7 *self,
		void *tag
	);
	HRESULT(__stdcall *GetUniquenessValue)(
		DTTR_Graphics_COM_DirectDrawSurface7 *self,
		DWORD *value
	);
	HRESULT(__stdcall *ChangeUniquenessValue)(DTTR_Graphics_COM_DirectDrawSurface7 *self);
	HRESULT(__stdcall *SetPriority)(
		DTTR_Graphics_COM_DirectDrawSurface7 *self,
		DWORD priority
	);
	HRESULT(__stdcall *GetPriority)(
		DTTR_Graphics_COM_DirectDrawSurface7 *self,
		DWORD *priority
	);
	HRESULT(__stdcall *SetLOD)(DTTR_Graphics_COM_DirectDrawSurface7 *self, DWORD lod);
	HRESULT(__stdcall *GetLOD)(DTTR_Graphics_COM_DirectDrawSurface7 *self, DWORD *lod);
} DTTR_Graphics_COM_DirectDrawSurface7_VT;

typedef struct DTTR_Graphics_COM_DirectDrawSurface7 {
	DTTR_Graphics_COM_DirectDrawSurface7_VT *m_vtbl;
	ULONG m_refcount;
	DTTR_Graphics_COM_DirectDrawSurface7 *m_back_buffer;
	DTTR_Graphics_COM_Direct3DTexture2 *m_texture;
	uint32_t m_width, m_height;
	uint32_t m_bpp;	  // This is either 16 or 32
	uint32_t m_pitch; // Stores the number of bytes per row
	uint32_t m_r_mask, m_g_mask, m_b_mask, m_a_mask;
	void *m_pixels;			  // Points to the game-writable pixel buffer in native format
	uint32_t m_dttr_texture;  // Holds the GPU texture handle, or 0 if not created
	uint32_t m_content_width; // Stores the actual content region width from Blt, or 0 for
							  // the full surface
	uint32_t m_content_height;
	bool m_locked;
	bool m_dirty;
	bool m_has_colorkey;
	uint16_t m_colorkey; // Stores the RGB565 transparent color value
	void *m_convert_rgba;
	uint32_t m_convert_rgba_capacity;
	bool m_last_upload_valid;
	uint32_t m_last_upload_width;
	uint32_t m_last_upload_height;
	uint64_t m_last_upload_hash;
} DTTR_Graphics_COM_DirectDrawSurface7;

/// Creates an IDirectDrawSurface7 translator with the given pixel format
DTTR_Graphics_COM_DirectDrawSurface7 *dttr_graphics_com_create_directdrawsurface7(
	uint32_t m_width,
	uint32_t m_height,
	uint32_t m_bpp,
	uint32_t m_r_mask,
	uint32_t m_g_mask,
	uint32_t m_b_mask,
	uint32_t m_a_mask
);

/// Creates an IDirect3DTexture2 translator bound to a surface translator
DTTR_Graphics_COM_Direct3DTexture2 *dttr_graphics_com_create_direct3d_texture2(
	DTTR_Graphics_COM_DirectDrawSurface7 *m_surface
);

#endif // DTTR_D3D_COM_H
