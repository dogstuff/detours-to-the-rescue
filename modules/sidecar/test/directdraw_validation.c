#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <SDL3/SDL.h>
#include <ddraw.h>

#include "graphics/graphics_com_private.h"
#include "graphics/graphics_private.h"

#include <dttr_test_cmocka.h>

DTTR_BackendState dttr_backend;

static int logical_width;
static int logical_height;

void dttr_graphics_set_logical_resolution(int width, int height) {
	logical_width = width;
	logical_height = height;
}

void dttr_graphics_begin_frame() {}
void dttr_graphics_end_frame() {}
DTTR_Graphics_COM_Direct3D7 *dttr_graphics_com_create_direct3d7() { return NULL; }

static void reset_graphics_state() {
	memset(&dttr_backend, 0, sizeof(dttr_backend));
	logical_width = 0;
	logical_height = 0;
	dttr_backend.texture_mutex = SDL_CreateMutex();
	assert_non_null(dttr_backend.texture_mutex);
}

static void cleanup_graphics_state() {
	if (dttr_backend.texture_mutex) {
		SDL_DestroyMutex(dttr_backend.texture_mutex);
		dttr_backend.texture_mutex = NULL;
	}
}

static DTTR_Graphics_COM_DirectDraw7 *make_directdraw() {
	DTTR_Graphics_COM_DirectDraw7 *ddraw = dttr_graphics_com_create_directdraw7();
	assert_non_null(ddraw);
	return ddraw;
}

static DTTR_Graphics_COM_DirectDraw7 *make_ready_directdraw() {
	reset_graphics_state();
	return make_directdraw();
}

static void free_directdraw(DTTR_Graphics_COM_DirectDraw7 *ddraw) {
	free(ddraw);
	cleanup_graphics_state();
}

static DDSURFACEDESC2 surface_desc(DWORD width, DWORD height, DWORD bpp) {
	DDSURFACEDESC2 desc;
	memset(&desc, 0, sizeof(desc));
	desc.dwSize = sizeof(desc);
	desc.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT | DDSD_CAPS;
	desc.dwWidth = width;
	desc.dwHeight = height;
	desc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
	desc.ddpfPixelFormat.dwSize = sizeof(desc.ddpfPixelFormat);
	desc.ddpfPixelFormat.dwFlags = DDPF_RGB;
	desc.ddpfPixelFormat.dwRGBBitCount = bpp;
	desc.ddpfPixelFormat.dwRBitMask = bpp == 32 ? 0x00FF0000u : 0xF800u;
	desc.ddpfPixelFormat.dwGBitMask = bpp == 32 ? 0x0000FF00u : 0x07E0u;
	desc.ddpfPixelFormat.dwBBitMask = bpp == 32 ? 0x000000FFu : 0x001Fu;
	desc.ddpfPixelFormat.dwRGBAlphaBitMask = bpp == 32 ? 0xFF000000u : 0u;
	return desc;
}

static void release_surface(DTTR_Graphics_COM_DirectDrawSurface7 *surface) {
	if (surface) {
		surface->vtbl->Release(surface);
	}
}

static void assert_create_surface_rejected(
	DTTR_Graphics_COM_DirectDraw7 *ddraw,
	DDSURFACEDESC2 desc
) {
	void *surface = (void *)0x1;
	assert_int_equal(
		ddraw->vtbl->CreateSurface(ddraw, &desc, &surface, NULL),
		DDERR_INVALIDPARAMS
	);
	assert_null(surface);
}

static void create_surface_rejects_invalid_descriptions(void **state) {
	DTTR_Graphics_COM_DirectDraw7 *ddraw = make_ready_directdraw();
	assert_create_surface_rejected(ddraw, surface_desc(0, 480, 16));
	assert_create_surface_rejected(ddraw, surface_desc(640, 480, 24));
	assert_create_surface_rejected(ddraw, surface_desc(UINT32_MAX, 2, 32));
	free_directdraw(ddraw);
}

static void create_surface_accepts_valid_formats(void **state) {
	DTTR_Graphics_COM_DirectDraw7 *ddraw = make_ready_directdraw();
	DDSURFACEDESC2 desc16 = surface_desc(64, 32, 16);
	DTTR_Graphics_COM_DirectDrawSurface7 *surface16 = NULL;
	assert_int_equal(
		ddraw->vtbl->CreateSurface(ddraw, &desc16, (void **)&surface16, NULL),
		S_OK
	);
	assert_non_null(surface16);
	assert_non_null(surface16->pixels);
	assert_int_equal(surface16->pitch, 128);
	assert_int_equal(logical_width, 64);
	assert_int_equal(logical_height, 32);
	release_surface(surface16);

	DDSURFACEDESC2 desc32 = surface_desc(8, 4, 32);
	DTTR_Graphics_COM_DirectDrawSurface7 *surface32 = NULL;
	assert_int_equal(
		ddraw->vtbl->CreateSurface(ddraw, &desc32, (void **)&surface32, NULL),
		S_OK
	);
	assert_non_null(surface32);
	assert_non_null(surface32->pixels);
	assert_int_equal(surface32->pitch, 32);
	release_surface(surface32);
	free_directdraw(ddraw);
}

static DTTR_Graphics_COM_DirectDrawSurface7 *make_surface(uint32_t width, uint32_t height) {
	DTTR_Graphics_COM_DirectDrawSurface7 *surface
		= dttr_graphics_com_create_directdrawsurface7(
			width,
			height,
			16,
			0xF800u,
			0x07E0u,
			0x001Fu,
			0u
		);
	assert_non_null(surface);
	assert_non_null(surface->pixels);
	return surface;
}

static void blt_rejects_inverted_rectangles(void **state) {
	reset_graphics_state();
	DTTR_Graphics_COM_DirectDrawSurface7 *dst = make_surface(8, 8);
	DTTR_Graphics_COM_DirectDrawSurface7 *src = make_surface(8, 8);
	RECT inverted = {.left = 4, .top = 0, .right = 2, .bottom = 8};
	assert_int_equal(
		dst->vtbl->Blt(dst, NULL, src, &inverted, 0, NULL),
		DDERR_INVALIDRECT
	);
	release_surface(src);
	release_surface(dst);
	cleanup_graphics_state();
}

static void blt_ignores_non_intersecting_rectangles(void **state) {
	reset_graphics_state();
	DTTR_Graphics_COM_DirectDrawSurface7 *dst = make_surface(8, 8);
	DTTR_Graphics_COM_DirectDrawSurface7 *src = make_surface(8, 8);
	RECT outside_dst = {.left = 20, .top = 20, .right = 24, .bottom = 24};
	assert_int_equal(dst->vtbl->Blt(dst, &outside_dst, src, NULL, 0, NULL), S_OK);
	assert_false(dst->dirty);
	release_surface(src);
	release_surface(dst);
	cleanup_graphics_state();
}

static const DTTR_TestCase TEST_CASES[] = {
	{"create-surface-rejects-invalid-descriptions",
	 create_surface_rejects_invalid_descriptions},
	{"create-surface-accepts-valid-formats", create_surface_accepts_valid_formats},
	{"blt-rejects-inverted-rectangles", blt_rejects_inverted_rectangles},
	{"blt-ignores-non-intersecting-rectangles", blt_ignores_non_intersecting_rectangles},
};

DTTR_TEST_MAIN(TEST_CASES)
