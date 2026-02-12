// Implements the IDirectDrawSurface7 COM translator
// https://learn.microsoft.com/en-us/windows/win32/api/ddraw/nn-ddraw-idirectdrawsurface7

#include "graphics_com_internal.h"
#include "graphics_internal.h"
#include "log.h"
#include <khash.h>
#include <stdlib.h>
#include <string.h>
#include <xxhash.h>

// https://learn.microsoft.com/en-us/windows/win32/api/ddraw/ns-ddraw-ddsurfacedesc2
// https://learn.microsoft.com/en-us/windows/win32/api/ddraw/ns-ddraw-ddpixelformat

KHASH_MAP_INIT_INT64(dttr_surface_texture_cache, DTTR_Texture)

typedef struct {
	uint64_t source_hash;
	uint32_t width;
	uint32_t height;
	uint32_t bpp;
	uint32_t r_mask;
	uint32_t g_mask;
	uint32_t b_mask;
	uint32_t a_mask;
	uint16_t colorkey;
	uint8_t has_colorkey;
} S_SurfaceTextureCacheSeed;

static khash_t(dttr_surface_texture_cache) * g_surface_texture_cache;

void dttr_graphics_surface_texture_cache_reset(void) {
	if (!g_surface_texture_cache) {
		return;
	}

	kh_destroy(dttr_surface_texture_cache, g_surface_texture_cache);
	g_surface_texture_cache = NULL;
}

static bool s_surface_texture_cache_ensure_initialized(void) {
	if (g_surface_texture_cache) {
		return true;
	}

	g_surface_texture_cache = kh_init(dttr_surface_texture_cache);
	return g_surface_texture_cache != NULL;
}

static void s_surface_texture_cache_insert_locked(uint64_t key, DTTR_Texture tex) {
	if (!key || !tex || !s_surface_texture_cache_ensure_initialized()) {
		return;
	}

	int put_ret = 0;
	const khint_t it = kh_put(dttr_surface_texture_cache, g_surface_texture_cache, key, &put_ret);
	if (it == kh_end(g_surface_texture_cache)) {
		return;
	}

	kh_value(g_surface_texture_cache, it) = tex;
}

static void s_surface_texture_cache_remove_locked(uint64_t key, DTTR_Texture tex) {
	if (!g_surface_texture_cache || !key || !tex) {
		return;
	}

	const khint_t it = kh_get(dttr_surface_texture_cache, g_surface_texture_cache, key);
	if (it == kh_end(g_surface_texture_cache)) {
		return;
	}

	if (kh_value(g_surface_texture_cache, it) == tex) {
		kh_del(dttr_surface_texture_cache, g_surface_texture_cache, it);
	}
}

static DTTR_Texture s_surface_texture_cache_lookup_locked(uint64_t key) {
	if (!g_surface_texture_cache || !key) {
		return DTTR_INVALID_TEXTURE;
	}

	const khint_t it = kh_get(dttr_surface_texture_cache, g_surface_texture_cache, key);
	if (it == kh_end(g_surface_texture_cache)) {
		return DTTR_INVALID_TEXTURE;
	}

	const DTTR_Texture tex = kh_value(g_surface_texture_cache, it);
	const int idx = (int)tex - 1;
	if (idx < 0 || idx >= g_dttr_backend.m_staged_texture_count) {
		kh_del(dttr_surface_texture_cache, g_surface_texture_cache, it);
		return DTTR_INVALID_TEXTURE;
	}

	const DTTR_StagedTexture *st = &g_dttr_backend.m_staged_textures[idx];
	if (st->m_refcount == 0) {
		kh_del(dttr_surface_texture_cache, g_surface_texture_cache, it);
		return DTTR_INVALID_TEXTURE;
	}

	return tex;
}

static uint64_t s_surface_texture_cache_key(
	const DTTR_Graphics_COM_DirectDrawSurface7 *self,
	uint32_t width,
	uint32_t height,
	uint64_t source_hash
) {
	const S_SurfaceTextureCacheSeed seed = {
		.source_hash = source_hash,
		.width = width,
		.height = height,
		.bpp = self->m_bpp,
		.r_mask = self->m_r_mask,
		.g_mask = self->m_g_mask,
		.b_mask = self->m_b_mask,
		.a_mask = self->m_a_mask,
		.colorkey = self->m_colorkey,
		.has_colorkey = self->m_has_colorkey ? 1 : 0,
	};

	return XXH3_64bits(&seed, sizeof(seed));
}

/// Converts ARGB4444 surface pixels to BGRA8888 pixels
static void s_surface_convert_argb4444_to_bgra8888(
	const uint16_t *restrict src, uint32_t *restrict dst, uint32_t w, uint32_t h, uint32_t src_pitch
) {
	for (uint32_t y = 0; y < h; y++) {
		const uint16_t *row = (const uint16_t *)((const uint8_t *)src + y * src_pitch);
		for (uint32_t x = 0; x < w; x++) {
			uint16_t px = row[x];
			// Expand 4-bit channels to 8-bit by replicating each nibble
			uint8_t a = (px >> 12) & 0xF;
			a = (a << 4) | a;
			uint8_t r = (px >> 8) & 0xF;
			r = (r << 4) | r;
			uint8_t g = (px >> 4) & 0xF;
			g = (g << 4) | g;
			uint8_t b = px & 0xF;
			b = (b << 4) | b;
			dst[y * w + x] = ((uint32_t)a << 24) | ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
		}
	}
}

/// Converts RGB565 pixels to BGRA8888 and applies black/colorkey transparency
static void s_surface_convert_rgb565_to_bgra8888(
	const uint16_t *restrict src,
	uint32_t *restrict dst,
	uint32_t w,
	uint32_t h,
	uint32_t src_pitch,
	bool has_colorkey,
	uint16_t colorkey
) {
	for (uint32_t y = 0; y < h; y++) {
		const uint16_t *row = (const uint16_t *)((const uint8_t *)src + y * src_pitch);
		for (uint32_t x = 0; x < w; x++) {
			uint16_t px = row[x];
			if (px == 0x0000 || (has_colorkey && px == colorkey)) {
				dst[y * w + x] = 0x00000000; // Fully transparent
				continue;
			}

			uint8_t r = (px >> 11) & 0x1F;
			r = (r << 3) | (r >> 2);
			uint8_t g = (px >> 5) & 0x3F;
			g = (g << 2) | (g >> 4);
			uint8_t b = px & 0x1F;
			b = (b << 3) | (b >> 2);
			dst[y * w + x] = 0xFF000000 | ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
		}
	}
}

// Ensures the per-surface 32-bit conversion scratch buffer can hold `size` bytes
static uint32_t *
s_surface_ensure_convert_buffer(DTTR_Graphics_COM_DirectDrawSurface7 *self, uint32_t size) {
	if (!self || size == 0)
		return NULL;

	if (self->m_convert_rgba && self->m_convert_rgba_capacity >= size)
		return (uint32_t *)self->m_convert_rgba;

	void *resized = realloc(self->m_convert_rgba, size);
	if (!resized)
		return NULL;

	self->m_convert_rgba = resized;
	self->m_convert_rgba_capacity = size;
	return (uint32_t *)self->m_convert_rgba;
}

// Computes a stable hash of the source surface region and conversion-relevant state
static uint64_t s_surface_hash_source_pixels(
	const DTTR_Graphics_COM_DirectDrawSurface7 *self, uint32_t upload_w, uint32_t upload_h
) {
	if (!self || !self->m_pixels || upload_w == 0 || upload_h == 0) {
		return 0;
	}

	const uint32_t bytes_per_pixel = self->m_bpp / 8;
	const size_t row_bytes = (size_t)upload_w * bytes_per_pixel;
	const uint8_t has_colorkey = self->m_has_colorkey ? 1 : 0;
	XXH3_state_t *hash_state = XXH3_createState();
	if (!hash_state || XXH3_64bits_reset(hash_state) != XXH_OK) {
		XXH3_freeState(hash_state);
		return 0;
	}

	XXH3_64bits_update(hash_state, &self->m_bpp, sizeof(self->m_bpp));
	XXH3_64bits_update(hash_state, &self->m_a_mask, sizeof(self->m_a_mask));
	XXH3_64bits_update(hash_state, &has_colorkey, sizeof(has_colorkey));
	XXH3_64bits_update(hash_state, &self->m_colorkey, sizeof(self->m_colorkey));
	for (uint32_t y = 0; y < upload_h; y++) {
		const uint8_t *row = (const uint8_t *)self->m_pixels + (size_t)y * self->m_pitch;
		XXH3_64bits_update(hash_state, row, row_bytes);
	}
	const uint64_t hash = XXH3_64bits_digest(hash_state);
	XXH3_freeState(hash_state);
	return hash;
}

// Queues one staged texture index for deferred GPU upload if not already queued
static void s_surface_queue_pending_upload_locked(DTTR_BackendState *state, int idx) {
	if (!state || idx < 0 || idx >= state->m_staged_texture_count)
		return;

	DTTR_StagedTexture *st = &state->m_staged_textures[idx];
	if (st->m_pending_upload)
		return;

	st->m_pending_upload = true;
	kv_push(int, state->m_pending_upload_indices, idx);
}

// Returns the active reference count for a staged texture handle
static uint32_t s_surface_texture_refcount(DTTR_Texture tex) {
	DTTR_BackendState *state = &g_dttr_backend;
	if (!tex) {
		return 0;
	}

	const int idx = (int)tex - 1;
	if (idx < 0 || idx >= state->m_staged_texture_count) {
		return 0;
	}

	uint32_t refs = 0;
	SDL_LockMutex(state->m_texture_mutex);
	refs = state->m_staged_textures[idx].m_refcount;
	SDL_UnlockMutex(state->m_texture_mutex);
	return refs;
}

// Adds one shared reference to an existing staged texture handle
static bool s_surface_texture_retain(DTTR_Texture tex) {
	DTTR_BackendState *state = &g_dttr_backend;
	if (!tex) {
		return false;
	}

	const int idx = (int)tex - 1;
	if (idx < 0 || idx >= state->m_staged_texture_count) {
		return false;
	}

	bool retained = false;
	SDL_LockMutex(state->m_texture_mutex);
	DTTR_StagedTexture *st = &state->m_staged_textures[idx];
	if (st->m_refcount > 0) {
		st->m_refcount++;
		retained = true;
	}
	SDL_UnlockMutex(state->m_texture_mutex);
	return retained;
}

/// Creates a staged GPU texture handle from a CPU pixel buffer or reuses a deduplicated
/// one from cache
static DTTR_Texture
s_surface_texture_create_or_retain(int width, int height, const void *pixels, uint64_t cache_key) {
	DTTR_BackendState *state = &g_dttr_backend;
	if (width <= 0 || height <= 0)
		return DTTR_INVALID_TEXTURE;

	SDL_LockMutex(state->m_texture_mutex);
	const DTTR_Texture cached_tex = s_surface_texture_cache_lookup_locked(cache_key);
	if (cached_tex != DTTR_INVALID_TEXTURE) {
		const int cached_idx = (int)cached_tex - 1;
		DTTR_StagedTexture *cached_st = &state->m_staged_textures[cached_idx];
		cached_st->m_refcount++;
		SDL_UnlockMutex(state->m_texture_mutex);
		return cached_tex;
	}

	if (state->m_staged_texture_count >= DTTR_MAX_STAGED_TEXTURES) {
		SDL_UnlockMutex(state->m_texture_mutex);
		log_error(DTTR_PREFIX_GRAPHICS "Too many textures");
		return DTTR_INVALID_TEXTURE;
	}

	const int idx = state->m_staged_texture_count++;
	DTTR_StagedTexture *st = &state->m_staged_textures[idx];
	st->m_gpu_tex = NULL;
	st->m_width = width;
	st->m_height = height;
	st->m_pending_upload = false;
	st->m_last_update_frame = state->m_frame_index;
	st->m_update_streak = 1;
	st->m_refcount = 1;
	st->m_cache_key_valid = cache_key != 0;
	st->m_cache_key = cache_key;

	const uint32_t size = (uint32_t)width * (uint32_t)height * 4;
	st->m_pixels = malloc(size);
	if (!st->m_pixels) {
		state->m_staged_texture_count--;
		SDL_UnlockMutex(state->m_texture_mutex);
		return DTTR_INVALID_TEXTURE;
	}
	if (pixels)
		memcpy(st->m_pixels, pixels, size);
	else
		memset(st->m_pixels, 0, size);
	if (st->m_cache_key_valid) {
		s_surface_texture_cache_insert_locked(st->m_cache_key, (DTTR_Texture)(idx + 1));
	}
	s_surface_queue_pending_upload_locked(state, idx);

	SDL_UnlockMutex(state->m_texture_mutex);
	return (DTTR_Texture)(idx + 1);
}

/// Releases one shared reference to a staged texture handle and destroys underlying
/// resources when the refcount reaches zero
static void s_surface_texture_release(DTTR_Texture tex) {
	DTTR_BackendState *state = &g_dttr_backend;
	if (!tex)
		return;
	const int idx = (int)tex - 1;
	if (idx < 0 || idx >= state->m_staged_texture_count)
		return;

	SDL_LockMutex(state->m_texture_mutex);
	DTTR_StagedTexture *st = &state->m_staged_textures[idx];
	if (st->m_refcount > 1) {
		st->m_refcount--;
		SDL_UnlockMutex(state->m_texture_mutex);
		return;
	}
	if (st->m_refcount == 0) {
		SDL_UnlockMutex(state->m_texture_mutex);
		return;
	}

	st->m_refcount = 0;
	if (st->m_cache_key_valid) {
		s_surface_texture_cache_remove_locked(st->m_cache_key, tex);
		st->m_cache_key_valid = false;
		st->m_cache_key = 0;
	}
	if (state->m_bound_texture == st->m_gpu_tex) {
		state->m_bound_texture = NULL;
		state->m_bound_texture_handle = DTTR_INVALID_TEXTURE;
	}
	if (st->m_gpu_tex && state->m_device) {
		if (dttr_graphics_is_gpu_thread()) {
			SDL_ReleaseGPUTexture(state->m_device, st->m_gpu_tex);
		} else if (state->m_deferred_destroy_count < DTTR_MAX_DEFERRED_DESTROYS) {
			state->m_deferred_destroys[state->m_deferred_destroy_count++] = st->m_gpu_tex;
		}
	}
	st->m_gpu_tex = NULL;
	free(st->m_pixels);
	st->m_pixels = NULL;
	st->m_pending_upload = false;
	st->m_width = 0;
	st->m_height = 0;
	st->m_last_update_frame = 0;
	st->m_update_streak = 0;
	SDL_UnlockMutex(state->m_texture_mutex);
}

/// Replaces pixel data and dimensions for an existing uniquely-owned staged texture handle
static bool s_surface_texture_update_unique(
	DTTR_Texture tex, int width, int height, const void *pixels, uint64_t cache_key
) {
	DTTR_BackendState *state = &g_dttr_backend;
	if (!tex || !pixels)
		return false;
	const int idx = (int)tex - 1;
	if (idx < 0 || idx >= state->m_staged_texture_count)
		return false;

	bool updated = false;
	SDL_LockMutex(state->m_texture_mutex);
	DTTR_StagedTexture *st = &state->m_staged_textures[idx];
	if (st->m_refcount != 1) {
		SDL_UnlockMutex(state->m_texture_mutex);
		return false;
	}

	const bool had_cache_key = st->m_cache_key_valid;
	const uint64_t old_cache_key = st->m_cache_key;
	if (had_cache_key) {
		s_surface_texture_cache_remove_locked(old_cache_key, tex);
	}

	const uint32_t size = (uint32_t)width * (uint32_t)height * 4;
	void *resized = realloc(st->m_pixels, size);
	if (!resized) {
		if (had_cache_key) {
			s_surface_texture_cache_insert_locked(old_cache_key, tex);
		}
		SDL_UnlockMutex(state->m_texture_mutex);
		return false;
	}

	const uint64_t current_frame = state->m_frame_index;
	if (current_frame <= st->m_last_update_frame + 1) {
		st->m_update_streak++;
	} else {
		st->m_update_streak = 1;
	}
	st->m_last_update_frame = current_frame;

	st->m_pixels = resized;
	memcpy(st->m_pixels, pixels, size);
	if (st->m_width != width || st->m_height != height) {
		if (st->m_gpu_tex && state->m_device) {
			if (dttr_graphics_is_gpu_thread()) {
				SDL_ReleaseGPUTexture(state->m_device, st->m_gpu_tex);
			} else if (state->m_deferred_destroy_count < DTTR_MAX_DEFERRED_DESTROYS) {
				state->m_deferred_destroys[state->m_deferred_destroy_count++] = st->m_gpu_tex;
			}
		}
		st->m_gpu_tex = NULL;
	}
	st->m_width = width;
	st->m_height = height;
	st->m_cache_key = cache_key;
	st->m_cache_key_valid = cache_key != 0;
	if (st->m_cache_key_valid) {
		s_surface_texture_cache_insert_locked(cache_key, tex);
	}
	s_surface_queue_pending_upload_locked(state, idx);
	updated = true;

	SDL_UnlockMutex(state->m_texture_mutex);
	return updated;
}

/// Ends and presents the current frame when a frame is active
static void s_surface_present(void) {
	if (g_dttr_backend.m_frame_active)
		dttr_graphics_end_frame();
}

/// Uploads dirty surface pixels to the staged texture cache
static void s_surface_upload_texture(DTTR_Graphics_COM_DirectDrawSurface7 *self) {
	if (!self->m_pixels || self->m_width == 0 || self->m_height == 0)
		return;

	// Use content dimensions if set (from Blt), otherwise full surface
	const uint32_t upload_w = self->m_content_width ? self->m_content_width : self->m_width;
	const uint32_t upload_h = self->m_content_height ? self->m_content_height : self->m_height;
	const uint64_t source_hash = s_surface_hash_source_pixels(self, upload_w, upload_h);
	const uint64_t cache_key = s_surface_texture_cache_key(self, upload_w, upload_h, source_hash);

	if (self->m_dttr_texture != DTTR_INVALID_TEXTURE && self->m_last_upload_valid
		&& self->m_last_upload_width == upload_w && self->m_last_upload_height == upload_h
		&& self->m_last_upload_hash == source_hash) {
		self->m_dirty = false;
		return;
	}

	uint32_t *converted = NULL;

	if (self->m_bpp == 16) {
		const uint32_t converted_size = upload_w * upload_h * 4;
		converted = s_surface_ensure_convert_buffer(self, converted_size);
		if (!converted)
			return;

		if (self->m_a_mask == 0xF000) {
			// Format has 4-bit alpha and 4-bit R, G, B channels
			// Convert only the content region, using surface pitch for row stride
			s_surface_convert_argb4444_to_bgra8888(
				(const uint16_t *)self->m_pixels, converted, upload_w, upload_h, self->m_pitch
			);
		} else {
			// Format has no alpha channel; uses black-as-transparent workaround
			s_surface_convert_rgb565_to_bgra8888(
				(const uint16_t *)self->m_pixels,
				converted,
				upload_w,
				upload_h,
				self->m_pitch,
				self->m_has_colorkey,
				self->m_colorkey
			);
		}
	} else {
		// Assumes 32bpp pixels are already in the correct format
		converted = (uint32_t *)self->m_pixels;
	}

	const void *upload_data = converted;

	bool upload_ok = false;
	if (self->m_dttr_texture != DTTR_INVALID_TEXTURE
		&& s_surface_texture_refcount(self->m_dttr_texture) > 1) {
		// Detach the shared texture because its content changed, then create or retain by the new
		// content key
		s_surface_texture_release(self->m_dttr_texture);
		self->m_dttr_texture = DTTR_INVALID_TEXTURE;
	}

	if (self->m_dttr_texture == DTTR_INVALID_TEXTURE) {
		self->m_dttr_texture
			= s_surface_texture_create_or_retain(upload_w, upload_h, upload_data, cache_key);
		upload_ok = (self->m_dttr_texture != DTTR_INVALID_TEXTURE);
	} else {
		upload_ok = s_surface_texture_update_unique(
			self->m_dttr_texture, upload_w, upload_h, upload_data, cache_key
		);
	}

	if (upload_ok) {
		self->m_last_upload_valid = true;
		self->m_last_upload_width = upload_w;
		self->m_last_upload_height = upload_h;
		self->m_last_upload_hash = source_hash;
	}
	self->m_dirty = !upload_ok;
}

static HRESULT __stdcall
s_ddrawsurface7_queryinterface(DTTR_Graphics_COM_DirectDrawSurface7 *self, void *riid, void **ppv) {

	const GUID *iid = (const GUID *)riid;

	// Check if requesting IDirect3DTexture2
	if (iid && memcmp(iid, &IID_IDirect3DTexture2, sizeof(GUID)) == 0) {

		// Create texture interface lazily
		if (!self->m_texture) {
			self->m_texture = dttr_graphics_com_create_direct3d_texture2(self);
		}
		if (ppv)
			*ppv = self->m_texture;
		return S_OK;
	}

	// return self for surface interfaces
	self->m_refcount++;
	if (ppv)
		*ppv = self;
	return S_OK;
}

static ULONG __stdcall s_ddrawsurface7_addref(DTTR_Graphics_COM_DirectDrawSurface7 *self) {
	ULONG rc = ++self->m_refcount;

	return rc;
}

static ULONG __stdcall s_ddrawsurface7_release(DTTR_Graphics_COM_DirectDrawSurface7 *self) {
	ULONG rc = --self->m_refcount;

	if (rc == 0) {

		if (self->m_dttr_texture) {
			s_surface_texture_release(self->m_dttr_texture);
			self->m_dttr_texture = 0;
		}
		if (self->m_back_buffer) {
			self->m_back_buffer->m_vtbl->Release(self->m_back_buffer);
			self->m_back_buffer = NULL;
		}
		free(self->m_pixels);
		self->m_pixels = NULL;
		free(self->m_convert_rgba);
		self->m_convert_rgba = NULL;
		self->m_convert_rgba_capacity = 0;
		free(self->m_texture);
		self->m_texture = NULL;
		free(self);
	}
	return rc;
}

DTTR_COM_NOOP_HRESULT(
	s_ddrawsurface7_addattachedsurface, DTTR_Graphics_COM_DirectDrawSurface7 *self, void *surf
)

DTTR_COM_NOOP_HRESULT(
	s_ddrawsurface7_addoverlaydirtyrect, DTTR_Graphics_COM_DirectDrawSurface7 *self, void *rect
)

static HRESULT __stdcall s_ddrawsurface7_blt(
	DTTR_Graphics_COM_DirectDrawSurface7 *self,
	void *dstRect,
	void *srcSurf,
	void *srcRect,
	DWORD flags,
	void *bltFx
) {

	// The game uses colorfill to fill padding areas in video textures.
	// https://learn.microsoft.com/en-us/windows/win32/api/ddraw/nf-ddraw-idirectdrawsurface7-blt
	if ((flags & DDBLT_COLORFILL)) {
		return S_OK;
	}

	// Blit from source surface to destination
	if (srcSurf) {
		const DTTR_Graphics_COM_DirectDrawSurface7 *src
			= (const DTTR_Graphics_COM_DirectDrawSurface7 *)srcSurf;

		if (src->m_pixels && self->m_pixels && src->m_bpp == self->m_bpp) {
			const uint32_t bpp = src->m_bpp / 8;

			// Parse source and destination rectangles
			uint32_t sx0 = 0, sy0 = 0, sw = src->m_width, sh = src->m_height;
			if (srcRect) {
				const RECT *r = (const RECT *)srcRect;
				sx0 = (uint32_t)r->left;
				sy0 = (uint32_t)r->top;
				sw = (uint32_t)(r->right - r->left);
				sh = (uint32_t)(r->bottom - r->top);
			}

			uint32_t dx0 = 0, dy0 = 0, dw = self->m_width, dh = self->m_height;
			if (dstRect) {
				const RECT *r = (const RECT *)dstRect;
				dx0 = (uint32_t)r->left;
				dy0 = (uint32_t)r->top;
				dw = (uint32_t)(r->right - r->left);
				dh = (uint32_t)(r->bottom - r->top);
			}

			// Clamp source region to source surface bounds
			if (sx0 + sw > src->m_width)
				sw = src->m_width - sx0;
			if (sy0 + sh > src->m_height)
				sh = src->m_height - sy0;

			// Scale when source is larger than dest; otherwise 1:1 copy
			const bool needs_scale = (sw > dw || sh > dh);

			if (needs_scale) {
				// Nearest-neighbor downscale from srcRect into dstRect
				uint32_t out_w = dw, out_h = dh;
				if (dx0 + out_w > self->m_width)
					out_w = self->m_width - dx0;
				if (dy0 + out_h > self->m_height)
					out_h = self->m_height - dy0;

				for (uint32_t y = 0; y < out_h; y++) {
					const uint32_t src_y = sy0 + (y * sh) / out_h;
					const uint8_t *src_row = (const uint8_t *)src->m_pixels + src_y * src->m_pitch;
					uint8_t *dst_row = (uint8_t *)self->m_pixels + (dy0 + y) * self->m_pitch;
					for (uint32_t x = 0; x < out_w; x++) {
						const uint32_t src_x = sx0 + (x * sw) / out_w;
						memcpy(dst_row + (dx0 + x) * bpp, src_row + src_x * bpp, bpp);
					}
				}

				self->m_content_width = dx0 + out_w;
				self->m_content_height = dy0 + out_h;
			} else {
				// Copy pixels 1:1 and clamp to surface bounds
				uint32_t copy_w = sw, copy_h = sh;
				if (dx0 + copy_w > self->m_width)
					copy_w = self->m_width - dx0;
				if (dy0 + copy_h > self->m_height)
					copy_h = self->m_height - dy0;
				if (sx0 + copy_w > src->m_width)
					copy_w = src->m_width - sx0;
				if (sy0 + copy_h > src->m_height)
					copy_h = src->m_height - sy0;

				for (uint32_t y = 0; y < copy_h; y++) {
					const uint8_t *src_row
						= (const uint8_t *)src->m_pixels + (sy0 + y) * src->m_pitch + sx0 * bpp;
					uint8_t *dst_row
						= (uint8_t *)self->m_pixels + (dy0 + y) * self->m_pitch + dx0 * bpp;
					memcpy(dst_row, src_row, copy_w * bpp);
				}

				self->m_content_width = dx0 + copy_w;
				self->m_content_height = dy0 + copy_h;
			}

			self->m_dirty = true;
			s_surface_upload_texture(self);
		} else if (src->m_dttr_texture != DTTR_INVALID_TEXTURE) {
			if (self->m_dttr_texture != src->m_dttr_texture) {
				if (self->m_dttr_texture != DTTR_INVALID_TEXTURE) {
					s_surface_texture_release(self->m_dttr_texture);
					self->m_dttr_texture = DTTR_INVALID_TEXTURE;
				}
				if (s_surface_texture_retain(src->m_dttr_texture)) {
					self->m_dttr_texture = src->m_dttr_texture;
				}
			}
		}
	}

	return S_OK;
}

DTTR_COM_NOOP_HRESULT(
	s_ddrawsurface7_bltbatch,
	DTTR_Graphics_COM_DirectDrawSurface7 *self,
	void *batch,
	DWORD count,
	DWORD flags
)

DTTR_COM_NOOP_HRESULT(
	s_ddrawsurface7_bltfast,
	DTTR_Graphics_COM_DirectDrawSurface7 *self,
	DWORD x,
	DWORD y,
	void *srcSurf,
	void *srcRect,
	DWORD flags
)

DTTR_COM_NOOP_HRESULT(
	s_ddrawsurface7_deleteattachedsurface,
	DTTR_Graphics_COM_DirectDrawSurface7 *self,
	DWORD flags,
	void *surf
)

DTTR_COM_NOOP_HRESULT(
	s_ddrawsurface7_enumattachedsurfaces,
	DTTR_Graphics_COM_DirectDrawSurface7 *self,
	void *ctx,
	void *cb
)

DTTR_COM_NOOP_HRESULT(
	s_ddrawsurface7_enumoverlayzorders,
	DTTR_Graphics_COM_DirectDrawSurface7 *self,
	DWORD flags,
	void *ctx,
	void *cb
)

static HRESULT __stdcall
s_ddrawsurface7_flip(DTTR_Graphics_COM_DirectDrawSurface7 *self, void *target, DWORD flags) {

	// Present the frame to the screen
	s_surface_present();
	return S_OK;
}

static HRESULT __stdcall s_ddrawsurface7_getattachedsurface(
	DTTR_Graphics_COM_DirectDrawSurface7 *self, void *caps, void **surf
) {

	if (!surf)
		return S_OK;

	// Return the back buffer if we have one
	if (self->m_back_buffer) {
		*surf = self->m_back_buffer;
	} else {
		// Create a back buffer matching the primary surface format
		self->m_back_buffer = dttr_graphics_com_create_directdrawsurface7(
			self->m_width,
			self->m_height,
			self->m_bpp,
			self->m_r_mask,
			self->m_g_mask,
			self->m_b_mask,
			self->m_a_mask
		);
		*surf = self->m_back_buffer;
	}
	return S_OK;
}

DTTR_COM_NOOP_HRESULT(
	s_ddrawsurface7_getbltstatus, DTTR_Graphics_COM_DirectDrawSurface7 *self, DWORD flags
)

DTTR_COM_STUB_MEMSET(
	s_ddrawsurface7_getcaps, DTTR_SIZEOF_DDSCAPS2, void, DTTR_Graphics_COM_DirectDrawSurface7 *self
)

DTTR_COM_STUB_SET(
	s_ddrawsurface7_getclipper, void *, NULL, DTTR_Graphics_COM_DirectDrawSurface7 *self
)

static HRESULT __stdcall s_ddrawsurface7_getcolorkey(
	DTTR_Graphics_COM_DirectDrawSurface7 *self, DWORD flags, void *colorKey
) {
	if (colorKey) {
		DDCOLORKEY *ck = (DDCOLORKEY *)colorKey;
		ck->dwColorSpaceLowValue = self->m_has_colorkey ? self->m_colorkey : 0;
		ck->dwColorSpaceHighValue = ck->dwColorSpaceLowValue;
	}
	return S_OK;
}

DTTR_COM_STUB_SET(s_ddrawsurface7_getdc, HDC, NULL, DTTR_Graphics_COM_DirectDrawSurface7 *self)

DTTR_COM_NOOP_HRESULT(
	s_ddrawsurface7_getflipstatus, DTTR_Graphics_COM_DirectDrawSurface7 *self, DWORD flags
)

static HRESULT __stdcall
s_ddrawsurface7_getoverlayposition(DTTR_Graphics_COM_DirectDrawSurface7 *self, LONG *x, LONG *y) {
	if (x)
		*x = 0;
	if (y)
		*y = 0;
	return S_OK;
}

DTTR_COM_STUB_SET(
	s_ddrawsurface7_getpalette, void *, NULL, DTTR_Graphics_COM_DirectDrawSurface7 *self
)

static HRESULT __stdcall
s_ddrawsurface7_getpixelformat(DTTR_Graphics_COM_DirectDrawSurface7 *self, void *fmt) {

	if (!fmt)
		return S_OK;

	DDPIXELFORMAT *pf = (DDPIXELFORMAT *)fmt;
	memset(pf, 0, sizeof(DDPIXELFORMAT));
	pf->dwSize = sizeof(DDPIXELFORMAT);
	pf->dwFlags = DDPF_RGB | (self->m_a_mask ? DDPF_ALPHAPIXELS : 0);
	pf->dwFourCC = 0;
	pf->dwRGBBitCount = self->m_bpp;
	pf->dwRBitMask = self->m_r_mask;
	pf->dwGBitMask = self->m_g_mask;
	pf->dwBBitMask = self->m_b_mask;
	pf->dwRGBAlphaBitMask = self->m_a_mask;

	return S_OK;
}

/// Fills a caller-provided DDSURFACEDESC2 buffer from surface state
static void
s_surface_fill_desc(const DTTR_Graphics_COM_DirectDrawSurface7 *self, DDSURFACEDESC2 *d) {
	d->dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PITCH | DDSD_PIXELFORMAT;
	d->dwHeight = self->m_height;
	d->dwWidth = self->m_width;
	d->lPitch = self->m_pitch;

	d->ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
	d->ddpfPixelFormat.dwFlags = DDPF_RGB | (self->m_a_mask ? DDPF_ALPHAPIXELS : 0);
	d->ddpfPixelFormat.dwFourCC = 0;
	d->ddpfPixelFormat.dwRGBBitCount = self->m_bpp;
	d->ddpfPixelFormat.dwRBitMask = self->m_r_mask;
	d->ddpfPixelFormat.dwGBitMask = self->m_g_mask;
	d->ddpfPixelFormat.dwBBitMask = self->m_b_mask;
	d->ddpfPixelFormat.dwRGBAlphaBitMask = self->m_a_mask;
}

static HRESULT __stdcall
s_ddrawsurface7_getsurfacedesc(DTTR_Graphics_COM_DirectDrawSurface7 *self, void *desc) {

	if (!desc)
		return S_OK;

	DDSURFACEDESC2 *d = (DDSURFACEDESC2 *)desc;
	const DWORD size = d->dwSize;

	if (size == DTTR_SIZEOF_DDSURFACEDESC || size == DTTR_SIZEOF_DDSURFACEDESC2) {
		memset((char *)desc + 4, 0, size - 4);
		s_surface_fill_desc(self, d);
	} else if (size == 0) {
		d->dwSize = DTTR_SIZEOF_DDSURFACEDESC;
		memset((char *)desc + 4, 0, DTTR_SIZEOF_DDSURFACEDESC - 4);
		s_surface_fill_desc(self, d);
	}

	return S_OK;
}

DTTR_COM_NOOP_HRESULT(
	s_ddrawsurface7_initialize, DTTR_Graphics_COM_DirectDrawSurface7 *self, void *dd, void *desc
)

DTTR_COM_NOOP_HRESULT(s_ddrawsurface7_islost, DTTR_Graphics_COM_DirectDrawSurface7 *self)

static HRESULT __stdcall s_ddrawsurface7_lock(
	DTTR_Graphics_COM_DirectDrawSurface7 *self, void *rect, void *desc, DWORD flags, HANDLE event
) {

	if (!self->m_pixels) {
		return DTTR_DDERR_GENERIC;
	}

	self->m_locked = true;

	if (!desc)
		return S_OK;

	DDSURFACEDESC2 *d = (DDSURFACEDESC2 *)desc;
	const DWORD size = d->dwSize;

	if (size == DTTR_SIZEOF_DDSURFACEDESC || size == DTTR_SIZEOF_DDSURFACEDESC2) {
		// Zero everything except dwSize
		memset((char *)desc + 4, 0, size - 4);

		d->dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_PITCH | DDSD_LPSURFACE | DDSD_PIXELFORMAT;
		d->dwHeight = self->m_height;
		d->dwWidth = self->m_width;
		d->lPitch = self->m_pitch;
		d->lpSurface = self->m_pixels;

		s_surface_fill_desc(self, d);
	} else if (size == 0) {
		d->dwSize = DTTR_SIZEOF_DDSURFACEDESC;
		memset((char *)desc + 4, 0, DTTR_SIZEOF_DDSURFACEDESC - 4);

		d->dwFlags = DDSD_LPSURFACE | DDSD_PITCH;
		d->lPitch = self->m_pitch;
		d->lpSurface = self->m_pixels;
	}

	return S_OK;
}

DTTR_COM_NOOP_HRESULT(s_ddrawsurface7_releasedc, DTTR_Graphics_COM_DirectDrawSurface7 *self, HDC dc)

DTTR_COM_NOOP_HRESULT(s_ddrawsurface7_restore, DTTR_Graphics_COM_DirectDrawSurface7 *self)

DTTR_COM_NOOP_HRESULT(
	s_ddrawsurface7_setclipper, DTTR_Graphics_COM_DirectDrawSurface7 *self, void *clipper
)

static HRESULT __stdcall s_ddrawsurface7_setcolorkey(
	DTTR_Graphics_COM_DirectDrawSurface7 *self, DWORD flags, void *colorKey
) {
	if (colorKey) {
		const DDCOLORKEY *ck = (const DDCOLORKEY *)colorKey;
		self->m_has_colorkey = true;
		self->m_colorkey = (uint16_t)ck->dwColorSpaceLowValue;
		self->m_last_upload_valid = false;
		self->m_dirty = true;
		s_surface_upload_texture(self);
	}
	return S_OK;
}

DTTR_COM_NOOP_HRESULT(
	s_ddrawsurface7_setoverlayposition, DTTR_Graphics_COM_DirectDrawSurface7 *self, LONG x, LONG y
)

DTTR_COM_NOOP_HRESULT(
	s_ddrawsurface7_setpalette, DTTR_Graphics_COM_DirectDrawSurface7 *self, void *palette
)

static HRESULT __stdcall
s_ddrawsurface7_unlock(DTTR_Graphics_COM_DirectDrawSurface7 *self, void *rect) {

	if (!self->m_locked) {
		return S_OK;
	}

	self->m_locked = false;
	self->m_dirty = true;

	// Upload pixels to GPU texture
	s_surface_upload_texture(self);

	return S_OK;
}

DTTR_COM_NOOP_HRESULT(
	s_ddrawsurface7_updateoverlay,
	DTTR_Graphics_COM_DirectDrawSurface7 *self,
	void *srcRect,
	void *dstSurf,
	void *dstRect,
	DWORD flags,
	void *fx
)

DTTR_COM_NOOP_HRESULT(
	s_ddrawsurface7_updateoverlaydisplay, DTTR_Graphics_COM_DirectDrawSurface7 *self, DWORD flags
)

DTTR_COM_NOOP_HRESULT(
	s_ddrawsurface7_updateoverlayzorder,
	DTTR_Graphics_COM_DirectDrawSurface7 *self,
	DWORD flags,
	void *refSurf
)

DTTR_COM_STUB_SET(
	s_ddrawsurface7_getddinterface, void *, NULL, DTTR_Graphics_COM_DirectDrawSurface7 *self
)

DTTR_COM_NOOP_HRESULT(
	s_ddrawsurface7_pagelock, DTTR_Graphics_COM_DirectDrawSurface7 *self, DWORD flags
)

DTTR_COM_NOOP_HRESULT(
	s_ddrawsurface7_pageunlock, DTTR_Graphics_COM_DirectDrawSurface7 *self, DWORD flags
)

DTTR_COM_NOOP_HRESULT(
	s_ddrawsurface7_setsurfacedesc,
	DTTR_Graphics_COM_DirectDrawSurface7 *self,
	void *desc,
	DWORD flags
)

DTTR_COM_NOOP_HRESULT(
	s_ddrawsurface7_setprivatedata,
	DTTR_Graphics_COM_DirectDrawSurface7 *self,
	void *tag,
	void *data,
	DWORD size,
	DWORD flags
)

DTTR_COM_STUB_SET(
	s_ddrawsurface7_getprivatedata,
	DWORD,
	0,
	DTTR_Graphics_COM_DirectDrawSurface7 *self,
	void *tag,
	void *data
)

DTTR_COM_NOOP_HRESULT(
	s_ddrawsurface7_freeprivatedata, DTTR_Graphics_COM_DirectDrawSurface7 *self, void *tag
)

DTTR_COM_STUB_SET(
	s_ddrawsurface7_getuniquenessvalue, DWORD, 1, DTTR_Graphics_COM_DirectDrawSurface7 *self
)

DTTR_COM_NOOP_HRESULT(
	s_ddrawsurface7_changeuniquenessvalue, DTTR_Graphics_COM_DirectDrawSurface7 *self
)

DTTR_COM_NOOP_HRESULT(
	s_ddrawsurface7_setpriority, DTTR_Graphics_COM_DirectDrawSurface7 *self, DWORD priority
)

DTTR_COM_STUB_SET(s_ddrawsurface7_getpriority, DWORD, 0, DTTR_Graphics_COM_DirectDrawSurface7 *self)

DTTR_COM_NOOP_HRESULT(s_ddrawsurface7_setlod, DTTR_Graphics_COM_DirectDrawSurface7 *self, DWORD lod)

DTTR_COM_STUB_SET(s_ddrawsurface7_getlod, DWORD, 0, DTTR_Graphics_COM_DirectDrawSurface7 *self)

static DTTR_Graphics_COM_DirectDrawSurface7_VT s_vtbl = {
	.QueryInterface = s_ddrawsurface7_queryinterface,
	.AddRef = s_ddrawsurface7_addref,
	.Release = s_ddrawsurface7_release,
	.AddAttachedSurface = s_ddrawsurface7_addattachedsurface,
	.AddOverlayDirtyRect = s_ddrawsurface7_addoverlaydirtyrect,
	.Blt = s_ddrawsurface7_blt,
	.BltBatch = s_ddrawsurface7_bltbatch,
	.BltFast = s_ddrawsurface7_bltfast,
	.DeleteAttachedSurface = s_ddrawsurface7_deleteattachedsurface,
	.EnumAttachedSurfaces = s_ddrawsurface7_enumattachedsurfaces,
	.EnumOverlayZOrders = s_ddrawsurface7_enumoverlayzorders,
	.Flip = s_ddrawsurface7_flip,
	.GetAttachedSurface = s_ddrawsurface7_getattachedsurface,
	.GetBltStatus = s_ddrawsurface7_getbltstatus,
	.GetCaps = s_ddrawsurface7_getcaps,
	.GetClipper = s_ddrawsurface7_getclipper,
	.GetColorKey = s_ddrawsurface7_getcolorkey,
	.GetDC = s_ddrawsurface7_getdc,
	.GetFlipStatus = s_ddrawsurface7_getflipstatus,
	.GetOverlayPosition = s_ddrawsurface7_getoverlayposition,
	.GetPalette = s_ddrawsurface7_getpalette,
	.GetPixelFormat = s_ddrawsurface7_getpixelformat,
	.GetSurfaceDesc = s_ddrawsurface7_getsurfacedesc,
	.Initialize = s_ddrawsurface7_initialize,
	.IsLost = s_ddrawsurface7_islost,
	.Lock = s_ddrawsurface7_lock,
	.ReleaseDC = s_ddrawsurface7_releasedc,
	.Restore = s_ddrawsurface7_restore,
	.SetClipper = s_ddrawsurface7_setclipper,
	.SetColorKey = s_ddrawsurface7_setcolorkey,
	.SetOverlayPosition = s_ddrawsurface7_setoverlayposition,
	.SetPalette = s_ddrawsurface7_setpalette,
	.Unlock = s_ddrawsurface7_unlock,
	.UpdateOverlay = s_ddrawsurface7_updateoverlay,
	.UpdateOverlayDisplay = s_ddrawsurface7_updateoverlaydisplay,
	.UpdateOverlayZOrder = s_ddrawsurface7_updateoverlayzorder,
	.GetDDInterface = s_ddrawsurface7_getddinterface,
	.PageLock = s_ddrawsurface7_pagelock,
	.PageUnlock = s_ddrawsurface7_pageunlock,
	.SetSurfaceDesc = s_ddrawsurface7_setsurfacedesc,
	.SetPrivateData = s_ddrawsurface7_setprivatedata,
	.GetPrivateData = s_ddrawsurface7_getprivatedata,
	.FreePrivateData = s_ddrawsurface7_freeprivatedata,
	.GetUniquenessValue = s_ddrawsurface7_getuniquenessvalue,
	.ChangeUniquenessValue = s_ddrawsurface7_changeuniquenessvalue,
	.SetPriority = s_ddrawsurface7_setpriority,
	.GetPriority = s_ddrawsurface7_getpriority,
	.SetLOD = s_ddrawsurface7_setlod,
	.GetLOD = s_ddrawsurface7_getlod,
};

DTTR_Graphics_COM_DirectDrawSurface7 *dttr_graphics_com_create_directdrawsurface7(
	uint32_t width,
	uint32_t height,
	uint32_t bpp,
	uint32_t r_mask,
	uint32_t g_mask,
	uint32_t b_mask,
	uint32_t a_mask
) {
	DTTR_Graphics_COM_DirectDrawSurface7 *surf
		= malloc(sizeof(DTTR_Graphics_COM_DirectDrawSurface7));
	if (surf) {
		surf->m_vtbl = &s_vtbl;
		surf->m_refcount = 1;
		surf->m_back_buffer = NULL;
		// Eagerly create texture interface in case game accesses it directly
		surf->m_texture = dttr_graphics_com_create_direct3d_texture2(surf);

		// Initialize surface dimensions and format
		surf->m_width = width;
		surf->m_height = height;
		surf->m_bpp = bpp;
		surf->m_pitch = (width * bpp + 7) / 8; // Round up to byte boundary
		surf->m_r_mask = r_mask;
		surf->m_g_mask = g_mask;
		surf->m_b_mask = b_mask;
		surf->m_a_mask = a_mask;

		// Allocate pixel buffer
		const uint32_t pixel_size = surf->m_pitch * height;
		surf->m_pixels = malloc(pixel_size);
		if (surf->m_pixels) {
			memset(surf->m_pixels, 0, pixel_size);
		}

		surf->m_dttr_texture = DTTR_INVALID_TEXTURE;
		surf->m_content_width = 0;
		surf->m_content_height = 0;
		surf->m_locked = false;
		surf->m_dirty = false;
		surf->m_convert_rgba = NULL;
		surf->m_convert_rgba_capacity = 0;
		surf->m_last_upload_valid = false;
		surf->m_last_upload_width = 0;
		surf->m_last_upload_height = 0;
		surf->m_last_upload_hash = 0;
	}
	return surf;
}
