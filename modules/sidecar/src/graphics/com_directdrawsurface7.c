// Implements the IDirectDrawSurface7 COM translator
// https://learn.microsoft.com/en-us/windows/win32/api/ddraw/nn-ddraw-idirectdrawsurface7

#include "graphics_com_private.h"
#include "graphics_private.h"
#include <dttr_log.h>
#include <khash.h>
#include <limits.h>
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
} surface_texture_cache_seed;

static khash_t(dttr_surface_texture_cache) * surface_texture_cache;

typedef struct {
	uint32_t x;
	uint32_t y;
	uint32_t w;
	uint32_t h;
} blit_rect;

static bool surface_bgra_upload_size(uint32_t width, uint32_t height, size_t *out_size) {
	if (out_size) {
		*out_size = 0;
	}

	if (!width || !height || width > (uint32_t)INT_MAX || height > (uint32_t)INT_MAX) {
		return false;
	}

	if ((size_t)width > SIZE_MAX / (size_t)height / sizeof(uint32_t)) {
		return false;
	}

	const size_t size = (size_t)width * (size_t)height * sizeof(uint32_t);
	if (size == 0 || size > UINT32_MAX) {
		return false;
	}

	if (out_size) {
		*out_size = size;
	}

	return true;
}

HRESULT dttr_graphics_com_validate_directdrawsurface7(
	uint32_t width,
	uint32_t height,
	uint32_t bpp,
	uint32_t *out_pitch,
	size_t *out_pixel_size
) {
	if (out_pitch) {
		*out_pitch = 0;
	}

	if (out_pixel_size) {
		*out_pixel_size = 0;
	}

	if (!width || !height || (bpp != 16u && bpp != 32u)) {
		return DDERR_INVALIDPARAMS;
	}

	const uint64_t pitch_bits = (uint64_t)width * (uint64_t)bpp;
	const uint64_t pitch = (pitch_bits + 7u) / 8u;
	if (pitch == 0 || pitch > UINT32_MAX || pitch > SIZE_MAX / height) {
		return DDERR_INVALIDPARAMS;
	}

	if (!surface_bgra_upload_size(width, height, NULL)) {
		return DDERR_INVALIDPARAMS;
	}

	if (out_pitch) {
		*out_pitch = (uint32_t)pitch;
	}

	if (out_pixel_size) {
		*out_pixel_size = (size_t)pitch * (size_t)height;
	}

	return S_OK;
}

static HRESULT blit_rect_from_optional(
	const RECT *rect,
	uint32_t default_w,
	uint32_t default_h,
	blit_rect *out
) {
	if (!out) {
		return DDERR_INVALIDPARAMS;
	}

	if (!rect) {
		*out = (blit_rect){0, 0, default_w, default_h};
		return S_OK;
	}

	if (rect->left < 0 || rect->top < 0 || rect->right < rect->left
		|| rect->bottom < rect->top) {
		return DDERR_INVALIDRECT;
	}

	*out = (blit_rect){
		.x = (uint32_t)rect->left,
		.y = (uint32_t)rect->top,
		.w = (uint32_t)(rect->right - rect->left),
		.h = (uint32_t)(rect->bottom - rect->top),
	};

	return S_OK;
}

static bool blit_rect_clip_to_surface(
	blit_rect *rect,
	uint32_t surface_w,
	uint32_t surface_h
) {
	if (!rect || rect->w == 0 || rect->h == 0 || rect->x >= surface_w
		|| rect->y >= surface_h) {
		return false;
	}

	const uint32_t max_w = surface_w - rect->x;
	const uint32_t max_h = surface_h - rect->y;
	if (rect->w > max_w) {
		rect->w = max_w;
	}

	if (rect->h > max_h) {
		rect->h = max_h;
	}

	return rect->w != 0 && rect->h != 0;
}

void dttr_graphics_surface_texture_cache_reset() {
	if (!surface_texture_cache) {
		return;
	}

	kh_destroy(dttr_surface_texture_cache, surface_texture_cache);
	surface_texture_cache = NULL;
}

static bool surface_texture_cache_ensure_initialized() {
	if (surface_texture_cache) {
		return true;
	}

	surface_texture_cache = kh_init(dttr_surface_texture_cache);
	return surface_texture_cache != NULL;
}

static void surface_texture_cache_insert_locked(uint64_t key, DTTR_Texture tex) {
	if (!key || !tex || !surface_texture_cache_ensure_initialized()) {
		return;
	}

	int put_ret = 0;
	const khint_t
		it = kh_put(dttr_surface_texture_cache, surface_texture_cache, key, &put_ret);
	if (it == kh_end(surface_texture_cache)) {
		return;
	}

	kh_value(surface_texture_cache, it) = tex;
}

static void surface_texture_cache_remove_locked(uint64_t key, DTTR_Texture tex) {
	if (!surface_texture_cache || !key || !tex) {
		return;
	}

	const khint_t it = kh_get(dttr_surface_texture_cache, surface_texture_cache, key);
	if (it == kh_end(surface_texture_cache)) {
		return;
	}

	if (kh_value(surface_texture_cache, it) == tex) {
		kh_del(dttr_surface_texture_cache, surface_texture_cache, it);
	}
}

static DTTR_Texture surface_texture_cache_lookup_locked(uint64_t key) {
	if (!surface_texture_cache || !key) {
		return DTTR_INVALID_TEXTURE;
	}

	const khint_t it = kh_get(dttr_surface_texture_cache, surface_texture_cache, key);
	if (it == kh_end(surface_texture_cache)) {
		return DTTR_INVALID_TEXTURE;
	}

	const DTTR_Texture tex = kh_value(surface_texture_cache, it);
	const int idx = (int)tex - 1;
	if (idx < 0 || idx >= dttr_backend.staged_texture_count) {
		kh_del(dttr_surface_texture_cache, surface_texture_cache, it);
		return DTTR_INVALID_TEXTURE;
	}

	const DTTR_StagedTexture *st = &dttr_backend.staged_textures[idx];
	if (st->refcount == 0) {
		kh_del(dttr_surface_texture_cache, surface_texture_cache, it);
		return DTTR_INVALID_TEXTURE;
	}

	return tex;
}

static uint64_t surface_texture_cache_key(
	const DTTR_Graphics_COM_DirectDrawSurface7 *self,
	uint32_t width,
	uint32_t height,
	uint64_t source_hash
) {
	const surface_texture_cache_seed seed = {
		.source_hash = source_hash,
		.width = width,
		.height = height,
		.bpp = self->bpp,
		.r_mask = self->r_mask,
		.g_mask = self->g_mask,
		.b_mask = self->b_mask,
		.a_mask = self->a_mask,
		.colorkey = self->colorkey,
		.has_colorkey = self->has_colorkey ? 1 : 0,
	};

	return XXH3_64bits(&seed, sizeof(seed));
}

static void surface_fill_pixelformat(
	const DTTR_Graphics_COM_DirectDrawSurface7 *self,
	DDPIXELFORMAT *pf
) {
	memset(pf, 0, sizeof(*pf));
	pf->dwSize = sizeof(*pf);
	pf->dwFlags = DDPF_RGB | (self->a_mask ? DDPF_ALPHAPIXELS : 0);
	pf->dwFourCC = 0;
	pf->dwRGBBitCount = self->bpp;
	pf->dwRBitMask = self->r_mask;
	pf->dwGBitMask = self->g_mask;
	pf->dwBBitMask = self->b_mask;
	pf->dwRGBAlphaBitMask = self->a_mask;
}

/// Converts ARGB4444 surface pixels to BGRA8888 pixels.
static void surface_convert_argb4444_to_bgra8888(
	const uint16_t *restrict src,
	uint32_t *restrict dst,
	uint32_t w,
	uint32_t h,
	uint32_t src_pitch
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
			dst[y * w + x] = ((uint32_t)a << 24) | ((uint32_t)r << 16)
							 | ((uint32_t)g << 8) | b;
		}
	}
}

/// Converts RGB565 pixels to BGRA8888 and applies black/colorkey transparency.
static void surface_convert_rgb565_to_bgra8888(
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

static uint32_t *surface_ensure_convert_buffer(
	DTTR_Graphics_COM_DirectDrawSurface7 *self,
	size_t size
) {
	if (!self || size == 0) {
		return NULL;
	}

	if (self->convert_rgba && self->convert_rgba_capacity >= size) {
		return (uint32_t *)self->convert_rgba;
	}

	void *resized = realloc(self->convert_rgba, size);
	if (!resized) {
		return NULL;
	}

	self->convert_rgba = resized;
	self->convert_rgba_capacity = size;
	return (uint32_t *)self->convert_rgba;
}

static uint64_t surface_hash_source_pixels(
	const DTTR_Graphics_COM_DirectDrawSurface7 *self,
	uint32_t upload_w,
	uint32_t upload_h
) {
	if (!self || !self->pixels || upload_w == 0 || upload_h == 0) {
		return 0;
	}

	const uint32_t bytes_per_pixel = self->bpp / 8;
	const size_t row_bytes = (size_t)upload_w * bytes_per_pixel;
	const uint8_t has_colorkey = self->has_colorkey ? 1 : 0;
	XXH3_state_t hash_state;
	if (XXH3_64bits_reset(&hash_state) != XXH_OK) {
		return 0;
	}

	XXH3_64bits_update(&hash_state, &self->bpp, sizeof(self->bpp));
	XXH3_64bits_update(&hash_state, &self->a_mask, sizeof(self->a_mask));
	XXH3_64bits_update(&hash_state, &has_colorkey, sizeof(has_colorkey));
	XXH3_64bits_update(&hash_state, &self->colorkey, sizeof(self->colorkey));
	for (uint32_t y = 0; y < upload_h; y++) {
		const uint8_t *row = (const uint8_t *)self->pixels + (size_t)y * self->pitch;
		XXH3_64bits_update(&hash_state, row, row_bytes);
	}

	return XXH3_64bits_digest(&hash_state);
}

static void surface_queue_pending_upload_locked(DTTR_BackendState *state, int idx) {
	if (!state || idx < 0 || idx >= state->staged_texture_count) {
		return;
	}

	DTTR_StagedTexture *st = &state->staged_textures[idx];
	if (st->pending_upload) {
		return;
	}

	st->pending_upload = true;
	kv_push(int, state->pending_upload_indices, idx);
}

static uint32_t surface_texture_refcount(DTTR_Texture tex) {
	DTTR_BackendState *state = &dttr_backend;
	if (!tex) {
		return 0;
	}

	const int idx = (int)tex - 1;
	if (idx < 0 || idx >= state->staged_texture_count) {
		return 0;
	}

	uint32_t refs = 0;
	SDL_LockMutex(state->texture_mutex);
	refs = state->staged_textures[idx].refcount;
	SDL_UnlockMutex(state->texture_mutex);
	return refs;
}

// Adds one shared reference to an existing staged texture handle.
static bool surface_texture_retain(DTTR_Texture tex) {
	DTTR_BackendState *state = &dttr_backend;
	if (!tex) {
		return false;
	}

	const int idx = (int)tex - 1;
	if (idx < 0 || idx >= state->staged_texture_count) {
		return false;
	}

	bool retained = false;
	SDL_LockMutex(state->texture_mutex);
	DTTR_StagedTexture *st = &state->staged_textures[idx];
	if (st->refcount > 0) {
		st->refcount++;
		retained = true;
	}

	SDL_UnlockMutex(state->texture_mutex);
	return retained;
}

/// Creates or reuses a staged GPU texture handle for a CPU pixel buffer.
static DTTR_Texture surface_texture_create_or_retain(
	int width,
	int height,
	const void *pixels,
	uint64_t cache_key
) {
	DTTR_BackendState *state = &dttr_backend;
	size_t size = 0;
	if (width <= 0 || height <= 0
		|| !surface_bgra_upload_size((uint32_t)width, (uint32_t)height, &size)) {
		return DTTR_INVALID_TEXTURE;
	}

	SDL_LockMutex(state->texture_mutex);
	const DTTR_Texture cached_tex = surface_texture_cache_lookup_locked(cache_key);
	if (cached_tex != DTTR_INVALID_TEXTURE) {
		const int cached_idx = (int)cached_tex - 1;
		DTTR_StagedTexture *cached_st = &state->staged_textures[cached_idx];
		cached_st->refcount++;
		SDL_UnlockMutex(state->texture_mutex);
		return cached_tex;
	}

	int idx = -1;
	for (int i = 0; i < state->staged_texture_count; i++) {
		if (state->staged_textures[i].refcount != 0) {
			continue;
		}

		if (state->staged_textures[i].gpu_tex != NULL) {
			continue;
		}

		idx = i;
		break;
	}

	if (idx < 0) {
		if (state->staged_texture_count >= DTTR_MAX_STAGED_TEXTURES) {
			SDL_UnlockMutex(state->texture_mutex);
			DTTR_LOG_ERROR("Too many textures");
			return DTTR_INVALID_TEXTURE;
		}

		idx = state->staged_texture_count++;
	}

	DTTR_StagedTexture *st = &state->staged_textures[idx];
	st->gpu_tex = NULL;
	st->width = width;
	st->height = height;
	st->pending_upload = false;
	st->last_update_frame = state->frame_index;
	st->update_streak = 1;
	st->refcount = 1;
	st->cache_key_valid = cache_key != 0;
	st->cache_key = cache_key;

	const bool is_new_slot = (idx == state->staged_texture_count - 1);
	st->pixels = malloc(size);
	if (!st->pixels) {
		st->refcount = 0;
		if (is_new_slot) {
			state->staged_texture_count--;
		}

		SDL_UnlockMutex(state->texture_mutex);
		return DTTR_INVALID_TEXTURE;
	}

	if (pixels) {
		memcpy(st->pixels, pixels, size);
	} else {
		memset(st->pixels, 0, size);
	}

	if (st->cache_key_valid) {
		surface_texture_cache_insert_locked(st->cache_key, (DTTR_Texture)(idx + 1));
	}

	surface_queue_pending_upload_locked(state, idx);

	SDL_UnlockMutex(state->texture_mutex);
	return (DTTR_Texture)(idx + 1);
}

/// Releases one staged texture reference and destroys GPU resources at zero refs.
static void surface_texture_release(DTTR_Texture tex) {
	DTTR_BackendState *state = &dttr_backend;
	if (!tex) {
		return;
	}

	const int idx = (int)tex - 1;
	if (idx < 0 || idx >= state->staged_texture_count) {
		return;
	}

	SDL_LockMutex(state->texture_mutex);
	DTTR_StagedTexture *st = &state->staged_textures[idx];
	if (st->refcount > 1) {
		st->refcount--;
		SDL_UnlockMutex(state->texture_mutex);
		return;
	}

	if (st->refcount == 0) {
		SDL_UnlockMutex(state->texture_mutex);
		return;
	}

	st->refcount = 0;
	if (st->cache_key_valid) {
		surface_texture_cache_remove_locked(st->cache_key, tex);
		st->cache_key_valid = false;
		st->cache_key = 0;
	}

	if (state->bound_texture == st->gpu_tex) {
		state->bound_texture = NULL;
		state->bound_texture_handle = DTTR_INVALID_TEXTURE;
	}

	state->renderer->defer_texture_destroy(state, idx);
	st->gpu_tex = NULL;
	free(st->pixels);
	st->pixels = NULL;
	st->pending_upload = false;
	st->width = 0;
	st->height = 0;
	st->last_update_frame = 0;
	st->update_streak = 0;
	SDL_UnlockMutex(state->texture_mutex);
}

/// Replaces pixel data and dimensions for an existing uniquely-owned staged texture
/// handle.
static bool surface_texture_update_unique(
	DTTR_Texture tex,
	int width,
	int height,
	const void *pixels,
	uint64_t cache_key
) {
	DTTR_BackendState *state = &dttr_backend;
	size_t size = 0;
	if (!tex || !pixels || width <= 0 || height <= 0
		|| !surface_bgra_upload_size((uint32_t)width, (uint32_t)height, &size)) {
		return false;
	}

	const int idx = (int)tex - 1;
	if (idx < 0 || idx >= state->staged_texture_count) {
		return false;
	}

	bool updated = false;
	SDL_LockMutex(state->texture_mutex);
	DTTR_StagedTexture *st = &state->staged_textures[idx];
	if (st->refcount != 1) {
		SDL_UnlockMutex(state->texture_mutex);
		return false;
	}

	const bool had_cache_key = st->cache_key_valid;
	const uint64_t old_cache_key = st->cache_key;
	if (had_cache_key) {
		surface_texture_cache_remove_locked(old_cache_key, tex);
	}

	void *resized = realloc(st->pixels, size);
	if (!resized) {
		if (had_cache_key) {
			surface_texture_cache_insert_locked(old_cache_key, tex);
		}

		SDL_UnlockMutex(state->texture_mutex);
		return false;
	}

	const uint64_t current_frame = state->frame_index;
	if (current_frame <= st->last_update_frame + 1) {
		st->update_streak++;
	} else {
		st->update_streak = 1;
	}

	st->last_update_frame = current_frame;

	st->pixels = resized;
	memcpy(st->pixels, pixels, size);
	if (st->width != width || st->height != height) {
		state->renderer->defer_texture_destroy(state, idx);
		st->gpu_tex = NULL;
	}

	st->width = width;
	st->height = height;
	st->cache_key = cache_key;
	st->cache_key_valid = cache_key != 0;
	if (st->cache_key_valid) {
		surface_texture_cache_insert_locked(cache_key, tex);
	}

	surface_queue_pending_upload_locked(state, idx);
	updated = true;

	SDL_UnlockMutex(state->texture_mutex);
	return updated;
}

/// Ends and presents the current frame when a frame is active.
static void surface_present() {
	if (dttr_backend.frame_active) {
		dttr_graphics_end_frame();
	}
}

/// Uploads dirty surface pixels to the staged texture cache.
static void surface_upload_texture(DTTR_Graphics_COM_DirectDrawSurface7 *self) {
	if (!self->pixels || self->width == 0 || self->height == 0) {
		return;
	}

	const uint32_t upload_w = self->content_width ? self->content_width : self->width;
	const uint32_t upload_h = self->content_height ? self->content_height : self->height;
	const uint64_t source_hash = surface_hash_source_pixels(self, upload_w, upload_h);
	const uint64_t
		cache_key = surface_texture_cache_key(self, upload_w, upload_h, source_hash);

	if (self->dttr_texture != DTTR_INVALID_TEXTURE && self->last_upload_valid
		&& self->last_upload_width == upload_w && self->last_upload_height == upload_h
		&& self->last_upload_hash == source_hash) {
		self->dirty = false;
		return;
	}

	uint32_t *converted = NULL;

	if (self->bpp == 16) {
		size_t converted_size = 0;
		if (!surface_bgra_upload_size(upload_w, upload_h, &converted_size)) {
			return;
		}

		converted = surface_ensure_convert_buffer(self, converted_size);
		if (!converted) {
			return;
		}

		if (self->a_mask == 0xF000) {
			// ARGB4444 content uses surface pitch for row stride.
			surface_convert_argb4444_to_bgra8888(
				(const uint16_t *)self->pixels,
				converted,
				upload_w,
				upload_h,
				self->pitch
			);
		} else {
			// RGB565 uses black or an explicit color key as transparent.
			surface_convert_rgb565_to_bgra8888(
				(const uint16_t *)self->pixels,
				converted,
				upload_w,
				upload_h,
				self->pitch,
				self->has_colorkey,
				self->colorkey
			);
		}
	} else {
		// 32bpp surfaces are already staged in backend upload format.
		converted = (uint32_t *)self->pixels;
	}

	const void *upload_data = converted;

	bool upload_ok = false;
	if (self->dttr_texture != DTTR_INVALID_TEXTURE
		&& surface_texture_refcount(self->dttr_texture) > 1) {
		// Detach shared content before looking up the new cache key.
		surface_texture_release(self->dttr_texture);
		self->dttr_texture = DTTR_INVALID_TEXTURE;
	}

	if (self->dttr_texture == DTTR_INVALID_TEXTURE) {
		self->dttr_texture = surface_texture_create_or_retain(
			upload_w,
			upload_h,
			upload_data,
			cache_key
		);
		upload_ok = (self->dttr_texture != DTTR_INVALID_TEXTURE);
	} else {
		upload_ok = surface_texture_update_unique(
			self->dttr_texture,
			upload_w,
			upload_h,
			upload_data,
			cache_key
		);
	}

	if (upload_ok) {
		self->last_upload_valid = true;
		self->last_upload_width = upload_w;
		self->last_upload_height = upload_h;
		self->last_upload_hash = source_hash;
	}

	self->dirty = !upload_ok;
}

static HRESULT __stdcall ddrawsurface7_queryinterface(
	DTTR_Graphics_COM_DirectDrawSurface7 *self,
	void *riid,
	void **ppv
) {

	const GUID *iid = (const GUID *)riid;

	if (!iid) {
		if (ppv) {
			*ppv = NULL;
		}

		return E_NOINTERFACE;
	}

	// IDirect3DTexture2 queries return the surface texture interface.
	if (memcmp(iid, &IID_IDirect3DTexture2, sizeof(GUID)) == 0) {

		if (!self->texture) {
			self->texture = dttr_graphics_com_create_direct3d_texture2(self);
		}

		if (ppv) {
			*ppv = self->texture;
		}

		return S_OK;
	}

	// The surface object answers its own and IUnknown queries.
	if (memcmp(iid, &IID_IDirectDrawSurface7, sizeof(GUID)) == 0
		|| memcmp(iid, &IID_IUnknown, sizeof(GUID)) == 0) {
		self->refcount++;
		if (ppv) {
			*ppv = self;
		}

		return S_OK;
	}

	if (ppv) {
		*ppv = NULL;
	}

	return E_NOINTERFACE;
}

static ULONG __stdcall ddrawsurface7_addref(DTTR_Graphics_COM_DirectDrawSurface7 *self) {
	return ++self->refcount;
}

static ULONG __stdcall ddrawsurface7_release(DTTR_Graphics_COM_DirectDrawSurface7 *self) {
	ULONG rc = --self->refcount;

	if (rc != 0) {
		return rc;
	}

	if (self->dttr_texture) {
		surface_texture_release(self->dttr_texture);
		self->dttr_texture = 0;
	}

	if (self->back_buffer) {
		self->back_buffer->vtbl->Release(self->back_buffer);
		self->back_buffer = NULL;
	}

	free(self->pixels);
	self->pixels = NULL;
	free(self->convert_rgba);
	self->convert_rgba = NULL;
	self->convert_rgba_capacity = 0;
	free(self->texture);
	self->texture = NULL;
	free(self);

	return rc;
}

DTTR_COM_NOOP_HRESULT(
	ddrawsurface7_addattachedsurface,
	DTTR_Graphics_COM_DirectDrawSurface7 *self,
	void *surf
)

DTTR_COM_NOOP_HRESULT(
	ddrawsurface7_addoverlaydirtyrect,
	DTTR_Graphics_COM_DirectDrawSurface7 *self,
	void *rect
)

static HRESULT __stdcall ddrawsurface7_blt(
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
		const DTTR_Graphics_COM_DirectDrawSurface7
			*src = (const DTTR_Graphics_COM_DirectDrawSurface7 *)srcSurf;

		if (src->pixels && self->pixels && src->bpp == self->bpp) {
			const uint32_t bpp = src->bpp / 8;
			if (bpp != 2u && bpp != 4u) {
				return DDERR_INVALIDPARAMS;
			}

			blit_rect src_region;
			HRESULT rect_result = blit_rect_from_optional(
				(const RECT *)srcRect,
				src->width,
				src->height,
				&src_region
			);
			if (rect_result != S_OK) {
				return rect_result;
			}

			blit_rect dst_region;
			rect_result = blit_rect_from_optional(
				(const RECT *)dstRect,
				self->width,
				self->height,
				&dst_region
			);
			if (rect_result != S_OK) {
				return rect_result;
			}

			const bool src_visible = blit_rect_clip_to_surface(
				&src_region,
				src->width,
				src->height
			);
			const bool dst_visible = blit_rect_clip_to_surface(
				&dst_region,
				self->width,
				self->height
			);
			if (!src_visible || !dst_visible) {
				return S_OK;
			}

			// Source regions larger than the destination use nearest-neighbor downscale.
			const bool
				needs_scale = (src_region.w > dst_region.w || src_region.h > dst_region.h);

			if (needs_scale) {
				const uint32_t out_w = dst_region.w;
				const uint32_t out_h = dst_region.h;

				for (uint32_t y = 0; y < out_h; y++) {
					const uint32_t src_y = src_region.y + (y * src_region.h) / out_h;
					const uint8_t *src_row = (const uint8_t *)src->pixels
											 + src_y * src->pitch;
					uint8_t *dst_row = (uint8_t *)self->pixels
									   + (dst_region.y + y) * self->pitch;
					for (uint32_t x = 0; x < out_w; x++) {
						const uint32_t src_x = src_region.x + (x * src_region.w) / out_w;
						memcpy(
							dst_row + (dst_region.x + x) * bpp,
							src_row + src_x * bpp,
							bpp
						);
					}
				}

				self->content_width = dst_region.x + out_w;
				self->content_height = dst_region.y + out_h;
			} else {
				uint32_t copy_w = src_region.w;
				uint32_t copy_h = src_region.h;
				if (copy_w > dst_region.w) {
					copy_w = dst_region.w;
				}

				if (copy_h > dst_region.h) {
					copy_h = dst_region.h;
				}

				if (copy_w == 0 || copy_h == 0) {
					return S_OK;
				}

				for (uint32_t y = 0; y < copy_h; y++) {
					const uint8_t *src_row = (const uint8_t *)src->pixels
											 + (src_region.y + y) * src->pitch
											 + src_region.x * bpp;
					uint8_t *dst_row = (uint8_t *)self->pixels
									   + (dst_region.y + y) * self->pitch
									   + dst_region.x * bpp;
					memcpy(dst_row, src_row, copy_w * bpp);
				}

				self->content_width = dst_region.x + copy_w;
				self->content_height = dst_region.y + copy_h;
			}

			self->dirty = true;
			surface_upload_texture(self);
		} else if (src->dttr_texture != DTTR_INVALID_TEXTURE) {
			if (self->dttr_texture != src->dttr_texture) {
				if (self->dttr_texture != DTTR_INVALID_TEXTURE) {
					surface_texture_release(self->dttr_texture);
					self->dttr_texture = DTTR_INVALID_TEXTURE;
				}

				if (surface_texture_retain(src->dttr_texture)) {
					self->dttr_texture = src->dttr_texture;
				}
			}
		}
	}

	return S_OK;
}

DTTR_COM_NOOP_HRESULT(
	ddrawsurface7_bltbatch,
	DTTR_Graphics_COM_DirectDrawSurface7 *self,
	void *batch,
	DWORD count,
	DWORD flags
)

DTTR_COM_NOOP_HRESULT(
	ddrawsurface7_bltfast,
	DTTR_Graphics_COM_DirectDrawSurface7 *self,
	DWORD x,
	DWORD y,
	void *srcSurf,
	void *srcRect,
	DWORD flags
)

DTTR_COM_NOOP_HRESULT(
	ddrawsurface7_deleteattachedsurface,
	DTTR_Graphics_COM_DirectDrawSurface7 *self,
	DWORD flags,
	void *surf
)

DTTR_COM_NOOP_HRESULT(
	ddrawsurface7_enumattachedsurfaces,
	DTTR_Graphics_COM_DirectDrawSurface7 *self,
	void *ctx,
	void *cb
)

DTTR_COM_NOOP_HRESULT(
	ddrawsurface7_enumoverlayzorders,
	DTTR_Graphics_COM_DirectDrawSurface7 *self,
	DWORD flags,
	void *ctx,
	void *cb
)

static HRESULT __stdcall ddrawsurface7_flip(
	DTTR_Graphics_COM_DirectDrawSurface7 *self,
	void *target,
	DWORD flags
) {

	// Present the active backend frame.
	surface_present();
	return S_OK;
}

static HRESULT __stdcall ddrawsurface7_getattachedsurface(
	DTTR_Graphics_COM_DirectDrawSurface7 *self,
	void *caps,
	void **surf
) {

	if (!surf) {
		return S_OK;
	}

	// Return an existing back buffer or create one matching this surface.
	if (self->back_buffer) {
		*surf = self->back_buffer;
	} else {
		self->back_buffer = dttr_graphics_com_create_directdrawsurface7(
			self->width,
			self->height,
			self->bpp,
			self->r_mask,
			self->g_mask,
			self->b_mask,
			self->a_mask
		);
		*surf = self->back_buffer;
	}

	return S_OK;
}

DTTR_COM_NOOP_HRESULT(
	ddrawsurface7_getbltstatus,
	DTTR_Graphics_COM_DirectDrawSurface7 *self,
	DWORD flags
)

DTTR_COM_STUB_MEMSET(
	ddrawsurface7_getcaps,
	DTTR_SIZEOF_DDSCAPS2,
	void,
	DTTR_Graphics_COM_DirectDrawSurface7 *self
)

DTTR_COM_STUB_SET(
	ddrawsurface7_getclipper,
	void *,
	NULL,
	DTTR_Graphics_COM_DirectDrawSurface7 *self
)

static HRESULT __stdcall ddrawsurface7_getcolorkey(
	DTTR_Graphics_COM_DirectDrawSurface7 *self,
	DWORD flags,
	void *colorKey
) {
	if (colorKey) {
		DDCOLORKEY *ck = (DDCOLORKEY *)colorKey;
		ck->dwColorSpaceLowValue = self->has_colorkey ? self->colorkey : 0;
		ck->dwColorSpaceHighValue = ck->dwColorSpaceLowValue;
	}

	return S_OK;
}

DTTR_COM_STUB_SET(
	ddrawsurface7_getdc,
	HDC,
	NULL,
	DTTR_Graphics_COM_DirectDrawSurface7 *self
)

DTTR_COM_NOOP_HRESULT(
	ddrawsurface7_getflipstatus,
	DTTR_Graphics_COM_DirectDrawSurface7 *self,
	DWORD flags
)

static HRESULT __stdcall ddrawsurface7_getoverlayposition(
	DTTR_Graphics_COM_DirectDrawSurface7 *self,
	LONG *x,
	LONG *y
) {
	if (x) {
		*x = 0;
	}

	if (y) {
		*y = 0;
	}

	return S_OK;
}

DTTR_COM_STUB_SET(
	ddrawsurface7_getpalette,
	void *,
	NULL,
	DTTR_Graphics_COM_DirectDrawSurface7 *self
)

static HRESULT __stdcall ddrawsurface7_getpixelformat(
	DTTR_Graphics_COM_DirectDrawSurface7 *self,
	void *fmt
) {

	if (!fmt) {
		return S_OK;
	}

	DDPIXELFORMAT *pf = (DDPIXELFORMAT *)fmt;
	surface_fill_pixelformat(self, pf);

	return S_OK;
}

/// Fills a caller-provided DDSURFACEDESC2 buffer from surface state.
static void surface_fill_desc(
	const DTTR_Graphics_COM_DirectDrawSurface7 *self,
	DDSURFACEDESC2 *d
) {
	d->dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PITCH | DDSD_PIXELFORMAT;
	d->dwHeight = self->height;
	d->dwWidth = self->width;
	d->lPitch = self->pitch;
	surface_fill_pixelformat(self, &d->ddpfPixelFormat);
}

static HRESULT __stdcall ddrawsurface7_getsurfacedesc(
	DTTR_Graphics_COM_DirectDrawSurface7 *self,
	void *desc
) {

	if (!desc) {
		return S_OK;
	}

	DDSURFACEDESC2 *d = (DDSURFACEDESC2 *)desc;
	const DWORD size = d->dwSize;

	if (size == DTTR_SIZEOF_DDSURFACEDESC || size == DTTR_SIZEOF_DDSURFACEDESC2) {
		memset((char *)desc + 4, 0, size - 4);
		surface_fill_desc(self, d);
	} else if (size == 0) {
		d->dwSize = DTTR_SIZEOF_DDSURFACEDESC;
		memset((char *)desc + 4, 0, DTTR_SIZEOF_DDSURFACEDESC - 4);
		surface_fill_desc(self, d);
	}

	return S_OK;
}

DTTR_COM_NOOP_HRESULT(
	ddrawsurface7_initialize,
	DTTR_Graphics_COM_DirectDrawSurface7 *self,
	void *dd,
	void *desc
)

DTTR_COM_NOOP_HRESULT(ddrawsurface7_islost, DTTR_Graphics_COM_DirectDrawSurface7 *self)

static HRESULT __stdcall ddrawsurface7_lock(
	DTTR_Graphics_COM_DirectDrawSurface7 *self,
	void *rect,
	void *desc,
	DWORD flags,
	HANDLE event
) {

	if (!self->pixels) {
		return DTTR_DDERR_GENERIC;
	}

	self->locked = true;

	if (!desc) {
		return S_OK;
	}

	DDSURFACEDESC2 *d = (DDSURFACEDESC2 *)desc;
	const DWORD size = d->dwSize;

	if (size == DTTR_SIZEOF_DDSURFACEDESC || size == DTTR_SIZEOF_DDSURFACEDESC2) {
		// Preserve dwSize while clearing the caller-visible fields.
		memset((char *)desc + 4, 0, size - 4);

		surface_fill_desc(self, d);

		d->lpSurface = self->pixels;
		d->dwFlags |= DDSD_LPSURFACE;
	} else if (size == 0) {
		d->dwSize = DTTR_SIZEOF_DDSURFACEDESC;
		memset((char *)desc + 4, 0, DTTR_SIZEOF_DDSURFACEDESC - 4);

		d->dwFlags = DDSD_LPSURFACE | DDSD_PITCH;
		d->lPitch = self->pitch;
		d->lpSurface = self->pixels;
	}

	return S_OK;
}

DTTR_COM_NOOP_HRESULT(
	ddrawsurface7_releasedc,
	DTTR_Graphics_COM_DirectDrawSurface7 *self,
	HDC dc
)

DTTR_COM_NOOP_HRESULT(ddrawsurface7_restore, DTTR_Graphics_COM_DirectDrawSurface7 *self)

DTTR_COM_NOOP_HRESULT(
	ddrawsurface7_setclipper,
	DTTR_Graphics_COM_DirectDrawSurface7 *self,
	void *clipper
)

static HRESULT __stdcall ddrawsurface7_setcolorkey(
	DTTR_Graphics_COM_DirectDrawSurface7 *self,
	DWORD flags,
	void *colorKey
) {
	if (colorKey) {
		const DDCOLORKEY *ck = (const DDCOLORKEY *)colorKey;
		self->has_colorkey = true;
		self->colorkey = (uint16_t)ck->dwColorSpaceLowValue;
		self->last_upload_valid = false;
		self->dirty = true;
		surface_upload_texture(self);
	}

	return S_OK;
}

DTTR_COM_NOOP_HRESULT(
	ddrawsurface7_setoverlayposition,
	DTTR_Graphics_COM_DirectDrawSurface7 *self,
	LONG x,
	LONG y
)

DTTR_COM_NOOP_HRESULT(
	ddrawsurface7_setpalette,
	DTTR_Graphics_COM_DirectDrawSurface7 *self,
	void *palette
)

static HRESULT __stdcall ddrawsurface7_unlock(
	DTTR_Graphics_COM_DirectDrawSurface7 *self,
	void *rect
) {

	if (!self->locked) {
		return S_OK;
	}

	self->locked = false;
	self->dirty = true;

	// Upload changed pixels to the staged GPU texture.
	surface_upload_texture(self);

	return S_OK;
}

DTTR_COM_NOOP_HRESULT(
	ddrawsurface7_updateoverlay,
	DTTR_Graphics_COM_DirectDrawSurface7 *self,
	void *srcRect,
	void *dstSurf,
	void *dstRect,
	DWORD flags,
	void *fx
)

DTTR_COM_NOOP_HRESULT(
	ddrawsurface7_updateoverlaydisplay,
	DTTR_Graphics_COM_DirectDrawSurface7 *self,
	DWORD flags
)

DTTR_COM_NOOP_HRESULT(
	ddrawsurface7_updateoverlayzorder,
	DTTR_Graphics_COM_DirectDrawSurface7 *self,
	DWORD flags,
	void *refSurf
)

DTTR_COM_STUB_SET(
	ddrawsurface7_getddinterface,
	void *,
	NULL,
	DTTR_Graphics_COM_DirectDrawSurface7 *self
)

DTTR_COM_NOOP_HRESULT(
	ddrawsurface7_pagelock,
	DTTR_Graphics_COM_DirectDrawSurface7 *self,
	DWORD flags
)

DTTR_COM_NOOP_HRESULT(
	ddrawsurface7_pageunlock,
	DTTR_Graphics_COM_DirectDrawSurface7 *self,
	DWORD flags
)

DTTR_COM_NOOP_HRESULT(
	ddrawsurface7_setsurfacedesc,
	DTTR_Graphics_COM_DirectDrawSurface7 *self,
	void *desc,
	DWORD flags
)

DTTR_COM_NOOP_HRESULT(
	ddrawsurface7_setprivatedata,
	DTTR_Graphics_COM_DirectDrawSurface7 *self,
	void *tag,
	void *data,
	DWORD size,
	DWORD flags
)

DTTR_COM_STUB_SET(
	ddrawsurface7_getprivatedata,
	DWORD,
	0,
	DTTR_Graphics_COM_DirectDrawSurface7 *self,
	void *tag,
	void *data
)

DTTR_COM_NOOP_HRESULT(
	ddrawsurface7_freeprivatedata,
	DTTR_Graphics_COM_DirectDrawSurface7 *self,
	void *tag
)

DTTR_COM_STUB_SET(
	ddrawsurface7_getuniquenessvalue,
	DWORD,
	1,
	DTTR_Graphics_COM_DirectDrawSurface7 *self
)

DTTR_COM_NOOP_HRESULT(
	ddrawsurface7_changeuniquenessvalue,
	DTTR_Graphics_COM_DirectDrawSurface7 *self
)

DTTR_COM_NOOP_HRESULT(
	ddrawsurface7_setpriority,
	DTTR_Graphics_COM_DirectDrawSurface7 *self,
	DWORD priority
)

DTTR_COM_STUB_SET(
	ddrawsurface7_getpriority,
	DWORD,
	0,
	DTTR_Graphics_COM_DirectDrawSurface7 *self
)

DTTR_COM_NOOP_HRESULT(
	ddrawsurface7_setlod,
	DTTR_Graphics_COM_DirectDrawSurface7 *self,
	DWORD lod
)

DTTR_COM_STUB_SET(
	ddrawsurface7_getlod,
	DWORD,
	0,
	DTTR_Graphics_COM_DirectDrawSurface7 *self
)

static DTTR_Graphics_COM_DirectDrawSurface7_VT vtbl = {
	.QueryInterface = ddrawsurface7_queryinterface,
	.AddRef = ddrawsurface7_addref,
	.Release = ddrawsurface7_release,
	.AddAttachedSurface = ddrawsurface7_addattachedsurface,
	.AddOverlayDirtyRect = ddrawsurface7_addoverlaydirtyrect,
	.Blt = ddrawsurface7_blt,
	.BltBatch = ddrawsurface7_bltbatch,
	.BltFast = ddrawsurface7_bltfast,
	.DeleteAttachedSurface = ddrawsurface7_deleteattachedsurface,
	.EnumAttachedSurfaces = ddrawsurface7_enumattachedsurfaces,
	.EnumOverlayZOrders = ddrawsurface7_enumoverlayzorders,
	.Flip = ddrawsurface7_flip,
	.GetAttachedSurface = ddrawsurface7_getattachedsurface,
	.GetBltStatus = ddrawsurface7_getbltstatus,
	.GetCaps = ddrawsurface7_getcaps,
	.GetClipper = ddrawsurface7_getclipper,
	.GetColorKey = ddrawsurface7_getcolorkey,
	.GetDC = ddrawsurface7_getdc,
	.GetFlipStatus = ddrawsurface7_getflipstatus,
	.GetOverlayPosition = ddrawsurface7_getoverlayposition,
	.GetPalette = ddrawsurface7_getpalette,
	.GetPixelFormat = ddrawsurface7_getpixelformat,
	.GetSurfaceDesc = ddrawsurface7_getsurfacedesc,
	.Initialize = ddrawsurface7_initialize,
	.IsLost = ddrawsurface7_islost,
	.Lock = ddrawsurface7_lock,
	.ReleaseDC = ddrawsurface7_releasedc,
	.Restore = ddrawsurface7_restore,
	.SetClipper = ddrawsurface7_setclipper,
	.SetColorKey = ddrawsurface7_setcolorkey,
	.SetOverlayPosition = ddrawsurface7_setoverlayposition,
	.SetPalette = ddrawsurface7_setpalette,
	.Unlock = ddrawsurface7_unlock,
	.UpdateOverlay = ddrawsurface7_updateoverlay,
	.UpdateOverlayDisplay = ddrawsurface7_updateoverlaydisplay,
	.UpdateOverlayZOrder = ddrawsurface7_updateoverlayzorder,
	.GetDDInterface = ddrawsurface7_getddinterface,
	.PageLock = ddrawsurface7_pagelock,
	.PageUnlock = ddrawsurface7_pageunlock,
	.SetSurfaceDesc = ddrawsurface7_setsurfacedesc,
	.SetPrivateData = ddrawsurface7_setprivatedata,
	.GetPrivateData = ddrawsurface7_getprivatedata,
	.FreePrivateData = ddrawsurface7_freeprivatedata,
	.GetUniquenessValue = ddrawsurface7_getuniquenessvalue,
	.ChangeUniquenessValue = ddrawsurface7_changeuniquenessvalue,
	.SetPriority = ddrawsurface7_setpriority,
	.GetPriority = ddrawsurface7_getpriority,
	.SetLOD = ddrawsurface7_setlod,
	.GetLOD = ddrawsurface7_getlod,
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
	uint32_t pitch = 0;
	size_t pixel_size = 0;
	if (dttr_graphics_com_validate_directdrawsurface7(
			width,
			height,
			bpp,
			&pitch,
			&pixel_size
		)
		!= S_OK) {
		return NULL;
	}

	DTTR_Graphics_COM_DirectDrawSurface7 *surf = calloc(
		1,
		sizeof(DTTR_Graphics_COM_DirectDrawSurface7)
	);
	if (surf) {
		surf->vtbl = &vtbl;
		surf->refcount = 1;
		// Create the texture interface up front because the game can query it directly.
		surf->texture = dttr_graphics_com_create_direct3d_texture2(surf);
		if (!surf->texture) {
			free(surf);
			return NULL;
		}

		surf->width = width;
		surf->height = height;
		surf->bpp = bpp;
		surf->pitch = pitch;
		surf->r_mask = r_mask;
		surf->g_mask = g_mask;
		surf->b_mask = b_mask;
		surf->a_mask = a_mask;

		surf->pixels = calloc(1, pixel_size);
		if (!surf->pixels) {
			free(surf->texture);
			free(surf);
			return NULL;
		}
	}

	return surf;
}
