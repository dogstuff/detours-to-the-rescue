/// Helpers for drawing alpha-blended geometry inside the game's D3D7 render
/// pass and reading the viewport used as draw space. Pair this with
/// dttr_util_worldview.h to project world geometry into that space.
///
/// This header is exposed through dttr_sdk.h only when DTTR_SDK_ENABLE_UNSTABLE
/// is set, and only if <d3d.h> was included first so the D3D7 COM types exist.
///
/// Draw calls only show up while the game's native render state is active,
/// such as inside a Graphics_DrawSortedLists hook. The generic mod render
/// callbacks run outside that pass, so game-pass draws submitted there are
/// discarded.

#ifndef DTTR_UTIL_GAMEDRAW_H
#define DTTR_UTIL_GAMEDRAW_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <dttr_core.h>
#ifndef DTTR_SDK_ENABLE_UNSTABLE
#error "Define DTTR_SDK_ENABLE_UNSTABLE before including dttr_util_gamedraw.h"
#endif
#include <dttr_pcdogs.h>
#include <dttr_util_worldview.h>

// The mingw/Wine d3d.h defines __WINE_D3D_H; the Windows SDK header defines
// DIRECT3D_VERSION/_D3D_H_.
#if defined(DIRECT3D_VERSION) || defined(__WINE_D3D_H) || defined(_D3D_H_)
#define DTTR_UTIL_GAMEDRAW_HAS_D3D 1

#ifdef __cplusplus
extern "C" {
#endif

#define DTTR_UTIL_GAMEDRAW_FVF (D3DFVF_XYZRHW | D3DFVF_DIFFUSE)

/// A draw-space vertex holds pixel coordinates and an ARGB diffuse with
/// alpha in the high byte.
typedef struct DTTR_Util_GameDrawVertex {
	float x;
	float y;
	float z;
	float rhw;
	DWORD diffuse;
} DTTR_Util_GameDrawVertex;

/// Drawing state for one render pass. The device is valid only while the
/// game's native render state is active.
typedef struct DTTR_Util_GameDraw {
	LPDIRECT3DDEVICE7 device;
	bool blend_ready;
} DTTR_Util_GameDraw;

/// Read the game's live D3D7 device for this render pass into `draw`.
static inline bool DTTR_Util_GameDraw_Begin(DTTR_Util_GameDraw *draw) {
	if (!draw) {
		return false;
	}

	draw->device = NULL;
	draw->blend_ready = false;
	if (!DTTR_PCDOGS_D_Graphics_D3DDevice7) {
		return false;
	}

	DTTR_PCDOGS_T_D3D_IDirect3DDevice7 *raw_device = NULL;
	if (!DTTR_StatusOK(DTTR_PCDOGS_D_Graphics_D3DDevice7->Read(&raw_device).status)
		|| !raw_device) {
		return false;
	}

	draw->device = (LPDIRECT3DDEVICE7)raw_device;
	return true;
}

/// Query the device viewport, which is the draw space that game-pass
/// coordinates live in.
static inline bool DTTR_Util_GameDraw_Viewport(
	const DTTR_Util_GameDraw *draw,
	DTTR_PCDOGS_T_Math_RectI32 *out
) {
	if (!draw || !draw->device || !out) {
		return false;
	}

	D3DVIEWPORT7 vp = {0};
	if (FAILED(IDirect3DDevice7_GetViewport(draw->device, &vp)) || vp.dwWidth < 64u
		|| vp.dwWidth > 16384u || vp.dwHeight < 64u || vp.dwHeight > 16384u) {
		return false;
	}

	*out = (DTTR_PCDOGS_T_Math_RectI32){
		.x = (int32_t)vp.dwX,
		.y = (int32_t)vp.dwY,
		.width = vp.dwWidth,
		.height = vp.dwHeight,
	};
	return true;
}

/// Enable standard translucency (srcAlpha/invSrcAlpha) once per pass.
static inline bool DTTR_Util_GameDraw_EnsureAlphaBlend(DTTR_Util_GameDraw *draw) {
	if (!draw || !draw->device) {
		return false;
	}

	if (draw->blend_ready) {
		return true;
	}

	const HRESULT enable_hr = IDirect3DDevice7_SetRenderState(
		draw->device,
		D3DRENDERSTATE_ALPHABLENDENABLE,
		TRUE
	);
	const HRESULT src_hr = IDirect3DDevice7_SetRenderState(
		draw->device,
		D3DRENDERSTATE_SRCBLEND,
		D3DBLEND_SRCALPHA
	);
	const HRESULT dst_hr = IDirect3DDevice7_SetRenderState(
		draw->device,
		D3DRENDERSTATE_DESTBLEND,
		D3DBLEND_INVSRCALPHA
	);
	draw->blend_ready = SUCCEEDED(enable_hr) && SUCCEEDED(src_hr) && SUCCEEDED(dst_hr);
	return draw->blend_ready;
}

// Prepare blend state, then submit one DrawPrimitive call.
static inline bool dttr_util_gamedraw_submit(
	DTTR_Util_GameDraw *draw,
	D3DPRIMITIVETYPE primitive_type,
	const DTTR_Util_GameDrawVertex *vertices,
	uint32_t vertex_count
) {
	if (!DTTR_Util_GameDraw_EnsureAlphaBlend(draw)) {
		return false;
	}

	return SUCCEEDED(IDirect3DDevice7_DrawPrimitive(
		draw->device,
		primitive_type,
		DTTR_UTIL_GAMEDRAW_FVF,
		(LPVOID)vertices, // The D3D7 API is not const-correct.
		vertex_count,
		0
	));
}

/// Submit an alpha-blended triangle list. vertex_count must be non-zero and
/// divisible by 3.
static inline bool DTTR_Util_GameDraw_Triangles(
	DTTR_Util_GameDraw *draw,
	const DTTR_Util_GameDrawVertex *vertices,
	uint32_t vertex_count
) {
	if (!draw || !vertices || vertex_count < 3u || (vertex_count % 3u) != 0u) {
		return false;
	}

	return dttr_util_gamedraw_submit(draw, D3DPT_TRIANGLELIST, vertices, vertex_count);
}

/// Fill a convex polygon (triangle fan) with a single ARGB color (alpha in
/// the high byte). The points are in draw-space perimeter order, and count
/// must be 3..DTTR_UTIL_WORLDVIEW_MAX_POLYGON_POINTS.
static inline bool DTTR_Util_GameDraw_Polygon(
	DTTR_Util_GameDraw *draw,
	const DTTR_PCDOGS_T_Math_Vec2F *points,
	uint32_t count,
	DWORD color
) {
	if (!draw || !points || count < 3u
		|| count > DTTR_UTIL_WORLDVIEW_MAX_POLYGON_POINTS) {
		return false;
	}

	DTTR_Util_GameDrawVertex vertices[DTTR_UTIL_WORLDVIEW_MAX_POLYGON_POINTS];
	for (uint32_t i = 0; i < count; ++i) {
		vertices[i].x = points[i].x;
		vertices[i].y = points[i].y;
		vertices[i].z = 0.0f;
		vertices[i].rhw = 1.0f;
		vertices[i].diffuse = color;
	}

	return dttr_util_gamedraw_submit(draw, D3DPT_TRIANGLEFAN, vertices, count);
}

/// Rebuild a camera frame sized to the live device viewport, falling back to
/// the supplied dimensions when the viewport is unavailable.
static inline bool DTTR_Util_WorldView_RefreshFromDraw(
	DTTR_Util_WorldView *view,
	const DTTR_Util_GameDraw *draw,
	uint32_t fallback_w,
	uint32_t fallback_h
) {
	DTTR_PCDOGS_T_Math_RectI32 target = {
		.x = 0,
		.y = 0,
		.width = fallback_w,
		.height = fallback_h,
	};
	(void)DTTR_Util_GameDraw_Viewport(draw, &target);
	return DTTR_Util_WorldView_Refresh(view, &target);
}

#ifdef __cplusplus
}
#endif

#endif // DTTR_UTIL_GAMEDRAW_HAS_D3D

#endif // DTTR_UTIL_GAMEDRAW_H
