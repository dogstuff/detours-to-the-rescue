// Translates IDirect3DDevice7 calls to the SDL3 GPU backend
// https://archive.org/details/dx7sdk-7001

#include "dttr_sidecar.h"
#include "graphics_com_internal.h"
#include "graphics_internal.h"
#include <dttr_log.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define MAX_VERTICES 4096
#define DTTR_MAT4_ELEMS 16
#define DTTR_MAT4_BYTES (sizeof(float) * DTTR_MAT4_ELEMS)
static DTTR_Vertex s_d3d_device7_verts[MAX_VERTICES];
static DTTR_Vertex s_d3d_device7_expanded_verts[DTTR_MAX_FRAME_VERTICES * 3];

/// Multiplies two row-major 4x4 float matrices into `out`
static void s_d3d_device7_mat4_multiply_f(
	float *restrict out,
	const float *restrict a,
	const float *restrict b
) {
	float tmp[DTTR_MAT4_ELEMS];
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			tmp[i * 4 + j] = 0;
			for (int k = 0; k < 4; k++)
				tmp[i * 4 + j] += a[i * 4 + k] * b[k * 4 + j];
		}
	}
	memcpy(out, tmp, sizeof(tmp));
}

/// Selects the backend transform matrix for a D3D transform state token
static bool s_d3d_device7_get_transform_state(DWORD type, float **out_matrix_f) {
	DTTR_BackendState *state = &g_dttr_backend;
	switch (type) {
	case D3DTRANSFORMSTATE_WORLD:
		*out_matrix_f = state->m_model;
		return true;
	case D3DTRANSFORMSTATE_VIEW:
		*out_matrix_f = state->m_view;
		return true;
	case D3DTRANSFORMSTATE_PROJECTION:
		*out_matrix_f = state->m_proj;
		return true;
	default:
		*out_matrix_f = NULL;
		return false;
	}
}

static const char *s_d3d_device7_transform_label(DWORD type) {
	switch (type) {
	case D3DTRANSFORMSTATE_WORLD:
		return "WORLD";

	case D3DTRANSFORMSTATE_VIEW:
		return "VIEW";

	case D3DTRANSFORMSTATE_PROJECTION:
		return "PROJECTION";

	default:
		return "UNKNOWN";
	}
}

static void s_d3d_device7_set_transform_state(DWORD type, const float *m) {
	float *matrix_f = NULL;

	if (!m)
		return;

	if (!s_d3d_device7_get_transform_state(type, &matrix_f))
		return;

	for (int i = 0; i < DTTR_MAT4_ELEMS; i++) {
		if (!isfinite(m[i])) {
			DTTR_LOG_WARN(
				"SetTransform(%s) rejected non-finite matrix input",
				s_d3d_device7_transform_label(type)
			);
			return;
		}
	}

	memcpy(matrix_f, m, DTTR_MAT4_BYTES);
}

/// Expands a triangle strip into a triangle list
static uint32_t s_d3d_device7_expand_strip(
	const DTTR_Vertex *restrict in,
	uint32_t count,
	DTTR_Vertex *restrict out
) {
	if (count < 3)
		return 0;
	uint32_t n = 0;
	for (uint32_t i = 0; i < count - 2; i++) {
		if (i & 1) {
			out[n++] = in[i + 1];
			out[n++] = in[i];
			out[n++] = in[i + 2];
		} else {
			out[n++] = in[i];
			out[n++] = in[i + 1];
			out[n++] = in[i + 2];
		}
	}
	return n;
}

/// Expands a triangle fan into a triangle list
static uint32_t s_d3d_device7_expand_fan(
	const DTTR_Vertex *restrict in,
	uint32_t count,
	DTTR_Vertex *restrict out
) {
	if (count < 3)
		return 0;
	uint32_t n = 0;
	for (uint32_t i = 0; i < count - 2; i++) {
		out[n++] = in[0];
		out[n++] = in[i + 1];
		out[n++] = in[i + 2];
	}
	return n;
}

/// Maps a D3D primitive code to the internal primitive enum
static DTTR_PrimitiveType s_d3d_device7_map_primitive_type(DWORD prim_type) {
	switch (prim_type) {
	case DTTR_D3DPT_POINTLIST:
		return DTTR_PRIM_POINTLIST;
	case DTTR_D3DPT_LINELIST:
		return DTTR_PRIM_LINELIST;
	case DTTR_D3DPT_LINESTRIP:
		return DTTR_PRIM_LINESTRIP;
	case DTTR_D3DPT_TRIANGLELIST:
		return DTTR_PRIM_TRIANGLELIST;
	case DTTR_D3DPT_TRIANGLESTRIP:
		return DTTR_PRIM_TRIANGLESTRIP;
	case DTTR_D3DPT_TRIANGLEFAN:
		return DTTR_PRIM_TRIANGLEFAN;
	default:
		return DTTR_PRIM_TRIANGLELIST;
	}
}

/// Appends a clear record to the current frame batch
static void s_d3d_device7_record_clear(
	uint32_t flags,
	uint32_t color,
	float depth,
	uint32_t stencil
) {
	DTTR_BackendState *state = &g_dttr_backend;
	if (!state->m_frame_active && dttr_graphics_is_gpu_thread())
		dttr_graphics_begin_frame();

	if (!state->m_frame_active)
		return;

	DTTR_BatchRecord clear_rec = {0};
	clear_rec.type = DTTR_BATCH_CLEAR;
	clear_rec.clear.flags = flags;
	clear_rec.clear.depth = depth;

	if (flags & DTTR_CLEAR_COLOR) {
		clear_rec.clear.color.r = ((color >> 16) & 0xff) / 255.0f;
		clear_rec.clear.color.g = ((color >> 8) & 0xff) / 255.0f;
		clear_rec.clear.color.b = (color & 0xff) / 255.0f;
		clear_rec.clear.color.a = ((color >> 24) & 0xff) / 255.0f;
		state->m_clear_color = clear_rec.clear.color;
	} else {
		clear_rec.clear.color = state->m_clear_color;
	}

	kv_push(DTTR_BatchRecord, state->m_batch_records, clear_rec);
}

static uint32_t s_d3d_device7_expand_primitive(
	DTTR_PrimitiveType *type,
	const DTTR_Vertex **verts,
	uint32_t count
) {
	switch (*type) {
	case DTTR_PRIM_TRIANGLESTRIP:
		count = s_d3d_device7_expand_strip(*verts, count, s_d3d_device7_expanded_verts);
		*verts = s_d3d_device7_expanded_verts;
		*type = DTTR_PRIM_TRIANGLELIST;
		return count;
	case DTTR_PRIM_TRIANGLEFAN:
		count = s_d3d_device7_expand_fan(*verts, count, s_d3d_device7_expanded_verts);
		*verts = s_d3d_device7_expanded_verts;
		*type = DTTR_PRIM_TRIANGLELIST;
		return count;
	default:
		return count;
	}
}

/// Appends a draw record to the current frame batch
static void s_d3d_device7_record_draw(
	DTTR_PrimitiveType type,
	const DTTR_Vertex *verts,
	uint32_t count,
	bool transformed,
	bool textured
) {
	DTTR_BackendState *state = &g_dttr_backend;

	if (!dttr_graphics_is_gpu_thread() || !verts || count == 0) {
		return;
	}

	if (state->m_backend_type == DTTR_BACKEND_SDL_GPU && !state->m_cmd) {
		return;
	}

	if (!state->m_device && state->m_backend_type == DTTR_BACKEND_SDL_GPU) {
		DTTR_LOG_WARN("DrawPrimitive: missing device/buffers");
		return;
	}
	if (count > DTTR_MAX_FRAME_VERTICES)
		count = DTTR_MAX_FRAME_VERTICES;

	count = s_d3d_device7_expand_primitive(&type, &verts, count);
	if (count == 0) {
		return;
	}

	if (!state->m_transfer_mapped)
		return;
	if (state->m_vertex_offset + count > DTTR_MAX_FRAME_VERTICES) {
		DTTR_LOG_WARN(
			"DrawPrimitive: frame vertex limit reached (%u + %u > %u)",
			state->m_vertex_offset,
			count,
			DTTR_MAX_FRAME_VERTICES
		);
		return;
	}
	memcpy(
		(uint8_t *)state->m_transfer_mapped + state->m_vertex_offset * DTTR_VERTEX_SIZE,
		verts,
		count * DTTR_VERTEX_SIZE
	);

	DTTR_BatchRecord draw_rec = {0};
	draw_rec.type = DTTR_BATCH_DRAW;
	draw_rec.draw.first_vertex = state->m_vertex_offset;
	draw_rec.draw.vertex_count = count;
	draw_rec.draw.blend_mode = DTTR_BLEND_OFF;
	if (state->m_blend_enabled) {
		draw_rec.draw.blend_mode = (state->m_blend_dst == DTTR_BLEND_ONE)
									   ? DTTR_BLEND_ADDITIVE
									   : DTTR_BLEND_ALPHA;
	}
	draw_rec.draw.depth_test = state->m_depth_test;
	draw_rec.draw.depth_write = state->m_depth_write;

	{
		float mv[DTTR_MAT4_ELEMS];
		s_d3d_device7_mat4_multiply_f(mv, state->m_view, state->m_model);
		s_d3d_device7_mat4_multiply_f(draw_rec.draw.uniforms.m_mvp, state->m_proj, mv);
	}
	draw_rec.draw.uniforms.m_screen_size[0] = (float)state->m_logical_width;
	draw_rec.draw.uniforms.m_screen_size[1] = (float)state->m_logical_height;
	draw_rec.draw.uniforms.m_is_2d = transformed
										 ? (g_dttr_config.m_sprite_smooth ? 2.0f : 1.0f)
										 : 0.0f;
	draw_rec.draw.uniforms.m_has_texture = textured ? 1.0f : 0.0f;

	draw_rec.draw.texture = (textured && state->m_bound_texture) ? state->m_bound_texture
																 : state->m_dummy_texture;
	const int cu = (state->m_addr_u == DTTR_TEXADDR_CLAMP) ? 1 : 0;
	const int cv = (state->m_addr_v == DTTR_TEXADDR_CLAMP) ? 1 : 0;
	draw_rec.draw.sampler = state->m_samplers[cu * 2 + cv];
	draw_rec.draw.sampler_index = cu * 2 + cv;
	draw_rec.draw.texture_index = (textured && state->m_bound_texture_handle)
									  ? (uint32_t)(state->m_bound_texture_handle - 1)
									  : UINT32_MAX;

	state->m_vertex_offset += count;

	/* Merge into previous record if state matches and vertices are contiguous */
	size_t n = kv_size(state->m_batch_records);
	if (n > 0) {
		DTTR_BatchRecord *prev = &kv_A(state->m_batch_records, n - 1);
		if (prev->type == DTTR_BATCH_DRAW
			&& prev->draw.first_vertex + prev->draw.vertex_count
				   == draw_rec.draw.first_vertex
			&& prev->draw.blend_mode == draw_rec.draw.blend_mode
			&& prev->draw.depth_test == draw_rec.draw.depth_test
			&& prev->draw.depth_write == draw_rec.draw.depth_write
			&& prev->draw.texture == draw_rec.draw.texture
			&& prev->draw.sampler == draw_rec.draw.sampler
			&& prev->draw.texture_index == draw_rec.draw.texture_index
			&& prev->draw.sampler_index == draw_rec.draw.sampler_index
			&& memcmp(&prev->draw.uniforms, &draw_rec.draw.uniforms, sizeof(DTTR_Uniforms))
				   == 0) {
			prev->draw.vertex_count += draw_rec.draw.vertex_count;
			return;
		}
	}

	kv_push(DTTR_BatchRecord, state->m_batch_records, draw_rec);
}

// Lazily creates the GPU texture backing a staged texture slot
static bool s_d3d_device7_ensure_staged_texture(DTTR_StagedTexture *st) {
	DTTR_BackendState *state = &g_dttr_backend;
	if (!st || st->m_gpu_tex || !state->m_device)
		return st && st->m_gpu_tex != NULL;

	const SDL_GPUTextureCreateInfo tex_info = {
		.type = SDL_GPU_TEXTURETYPE_2D,
		.format = SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM,
		.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COLOR_TARGET,
		.width = st->m_width,
		.height = st->m_height,
		.layer_count_or_depth = 1,
		.num_levels = dttr_graphics_calc_mip_levels(st->m_width, st->m_height),
	};
	st->m_gpu_tex = SDL_CreateGPUTexture(state->m_device, &tex_info);
	return st->m_gpu_tex != NULL;
}

static void s_d3d_device7_clear_bound_texture(DTTR_BackendState *state) {
	state->m_bound_texture_handle = DTTR_INVALID_TEXTURE;
	state->m_bound_texture = NULL;
}

/// Binds an internal texture handle for subsequent draw records
static void s_d3d_device7_texture_bind(DTTR_Texture tex) {
	DTTR_BackendState *state = &g_dttr_backend;
	if (!tex) {
		if (state->m_bound_texture_handle == DTTR_INVALID_TEXTURE
			&& !state->m_bound_texture) {
			return;
		}
		s_d3d_device7_clear_bound_texture(state);
		return;
	}

	if (state->m_bound_texture_handle == tex && state->m_bound_texture) {
		return;
	}

	const int idx = (int)tex - 1;
	if (idx < 0 || idx >= state->m_staged_texture_count) {
		s_d3d_device7_clear_bound_texture(state);
		return;
	}

	if (!state->m_texture_mutex) {
		s_d3d_device7_clear_bound_texture(state);
		return;
	}

	SDL_LockMutex(state->m_texture_mutex);
	DTTR_StagedTexture *st = &state->m_staged_textures[idx];
	if (dttr_graphics_is_gpu_thread()) {
		s_d3d_device7_ensure_staged_texture(st);
	}
	state->m_bound_texture_handle = tex;
	state->m_bound_texture = st->m_gpu_tex;
	SDL_UnlockMutex(state->m_texture_mutex);
}

/// Sets whether depth testing is enabled
static void s_d3d_device7_set_depth_test(bool enabled) {
	g_dttr_backend.m_depth_test = enabled;
}

/// Sets whether depth writes are enabled
static void s_d3d_device7_set_depth_write(bool enabled) {
	g_dttr_backend.m_depth_write = enabled;
}

/// Sets the depth compare function
static void s_d3d_device7_set_depth_func(DTTR_CompareFunc func) {}

/// Sets whether blending is enabled
static void s_d3d_device7_set_blend_enabled(bool enabled) {
	g_dttr_backend.m_blend_enabled = enabled;
}

/// Sets source and destination blend factors
static void s_d3d_device7_set_blend_func(DTTR_BlendFactor src, DTTR_BlendFactor dst) {
	if (dst)
		g_dttr_backend.m_blend_dst = dst;
}

/// Sets triangle cull mode
static void s_d3d_device7_set_cull_mode(DTTR_CullMode mode) {}

/// Sets texture addressing mode for U coordinates
static void s_d3d_device7_set_texture_address_u(DTTR_TextureAddress addr) {
	g_dttr_backend.m_addr_u = addr;
}

/// Sets texture addressing mode for V coordinates
static void s_d3d_device7_set_texture_address_v(DTTR_TextureAddress addr) {
	g_dttr_backend.m_addr_v = addr;
}

/// Updates the viewport state used by transformed rendering paths
static void s_d3d_device7_set_viewport(
	int x,
	int y,
	int w,
	int h,
	float min_z,
	float max_z
) {
	DTTR_BackendState *state = &g_dttr_backend;
	if (w <= 0 || h <= 0)
		return;

	state->m_viewport_x = x;
	state->m_viewport_y = y;
	state->m_viewport_w = w;
	state->m_viewport_h = h;
	state->m_viewport_min_z = min_z;
	state->m_viewport_max_z = max_z;
}

/// Sets the color combine operation
static void s_d3d_device7_set_color_op(int op) {}
/// Sets the alpha combine operation
static void s_d3d_device7_set_alpha_op(int op) {}

DTTR_COM_QI_SELF(s_d3ddevice7_queryinterface, DTTR_Graphics_COM_Direct3DDevice7)

DTTR_COM_ADDREF(s_d3ddevice7_addref, DTTR_Graphics_COM_Direct3DDevice7)

DTTR_COM_RELEASE(s_d3ddevice7_release, DTTR_Graphics_COM_Direct3DDevice7)

DTTR_COM_STUB_MEMSET(
	s_d3ddevice7_getcaps,
	DTTR_SIZEOF_D3DDEVICEDESC7,
	void,
	DTTR_Graphics_COM_Direct3DDevice7 *self
)

static HRESULT __stdcall s_d3ddevice7_enumtextureformats(
	DTTR_Graphics_COM_Direct3DDevice7 *self,
	void *cb,
	void *ctx
) {

	if (!cb)
		return S_OK;

	const LPD3DENUMPIXELFORMATSCALLBACK callback = (LPD3DENUMPIXELFORMATSCALLBACK)cb;

	// Report ARGB4444 (16-bit with 4-bit alpha) which the game requires
	DDPIXELFORMAT fmt_argb4444 = {
		.dwSize = sizeof(DDPIXELFORMAT),
		.dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS,
		.dwRGBBitCount = 16,
		.dwRBitMask = 0x0F00,
		.dwGBitMask = 0x00F0,
		.dwBBitMask = 0x000F,
		.dwRGBAlphaBitMask = 0xF000,
	};

	HRESULT hr = callback(&fmt_argb4444, ctx);

	if (hr != 1)
		return S_OK; // Stop enumeration because the callback did not return D3DENUMRET_OK

	// Report RGB565 (16-bit, no alpha) as a fallback format
	DDPIXELFORMAT fmt_rgb565 = {
		.dwSize = sizeof(DDPIXELFORMAT),
		.dwFlags = DDPF_RGB,
		.dwRGBBitCount = 16,
		.dwRBitMask = 0xF800,
		.dwGBitMask = 0x07E0,
		.dwBBitMask = 0x001F,
		.dwRGBAlphaBitMask = 0,
	};

	callback(&fmt_rgb565, ctx);

	return S_OK;
}

DTTR_COM_NOOP_HRESULT(s_d3ddevice7_beginscene, DTTR_Graphics_COM_Direct3DDevice7 *self)

DTTR_COM_NOOP_HRESULT(s_d3ddevice7_endscene, DTTR_Graphics_COM_Direct3DDevice7 *self)

DTTR_COM_STUB_SET(
	s_d3ddevice7_getdirect3d,
	void *,
	NULL,
	DTTR_Graphics_COM_Direct3DDevice7 *self
)

DTTR_COM_NOOP_HRESULT(
	s_d3ddevice7_setrendertarget,
	DTTR_Graphics_COM_Direct3DDevice7 *self,
	void *surface,
	DWORD flags
)

DTTR_COM_STUB_SET(
	s_d3ddevice7_getrendertarget,
	void *,
	NULL,
	DTTR_Graphics_COM_Direct3DDevice7 *self
)

static HRESULT __stdcall s_d3ddevice7_clear(
	DTTR_Graphics_COM_Direct3DDevice7 *self,
	DWORD count,
	void *rects,
	DWORD flags,
	DWORD color,
	float z,
	DWORD stencil
) {

	uint32_t f = 0;
	if (flags & D3DCLEAR_TARGET)
		f |= DTTR_CLEAR_COLOR;
	if (flags & D3DCLEAR_ZBUFFER)
		f |= DTTR_CLEAR_DEPTH;
	if (flags & D3DCLEAR_STENCIL)
		f |= DTTR_CLEAR_STENCIL;
	s_d3d_device7_record_clear(f, color, z, stencil);
	return S_OK;
}

static HRESULT __stdcall s_d3ddevice7_settransform(
	DTTR_Graphics_COM_Direct3DDevice7 *self,
	DWORD type,
	void *matrix
) {
	s_d3d_device7_set_transform_state(type, (const float *)matrix);
	return S_OK;
}

static HRESULT __stdcall s_d3ddevice7_gettransform(
	DTTR_Graphics_COM_Direct3DDevice7 *self,
	DWORD type,
	void *matrix
) {
	if (!matrix)
		return S_OK;

	float *matrix_f = NULL;
	if (!s_d3d_device7_get_transform_state(type, &matrix_f)) {
		memset(matrix, 0, DTTR_MAT4_BYTES);
		return S_OK;
	}

	memcpy(matrix, matrix_f, DTTR_MAT4_BYTES);
	return S_OK;
}

static HRESULT __stdcall s_d3ddevice7_setviewport(
	DTTR_Graphics_COM_Direct3DDevice7 *self,
	void *vp
) {

	if (!vp)
		return S_OK;

	const D3DVIEWPORT7 *v = (const D3DVIEWPORT7 *)vp;
	s_d3d_device7_set_viewport(
		(int)v->dwX,
		(int)v->dwY,
		(int)v->dwWidth,
		(int)v->dwHeight,
		v->dvMinZ,
		v->dvMaxZ
	);

	return S_OK;
}

static HRESULT __stdcall s_d3ddevice7_multiplytransform(
	DTTR_Graphics_COM_Direct3DDevice7 *self,
	DWORD type,
	void *matrix
) {
	if (!matrix)
		return S_OK;

	float *matrix_f = NULL;
	if (!s_d3d_device7_get_transform_state(type, &matrix_f))
		return S_OK;

	float result[DTTR_MAT4_ELEMS];
	s_d3d_device7_mat4_multiply_f(result, matrix_f, (const float *)matrix);
	memcpy(matrix_f, result, sizeof(result));
	return S_OK;
}

static HRESULT __stdcall s_d3ddevice7_getviewport(
	DTTR_Graphics_COM_Direct3DDevice7 *self,
	void *vp
) {

	if (!vp)
		return S_OK;

	D3DVIEWPORT7 *v = (D3DVIEWPORT7 *)vp;
	const DTTR_BackendState *state = &g_dttr_backend;
	v->dwX = (DWORD)state->m_viewport_x;
	v->dwY = (DWORD)state->m_viewport_y;
	v->dwWidth = (DWORD)state->m_viewport_w;
	v->dwHeight = (DWORD)state->m_viewport_h;
	v->dvMinZ = state->m_viewport_min_z;
	v->dvMaxZ = state->m_viewport_max_z;
	return S_OK;
}

DTTR_COM_NOOP_HRESULT(
	s_d3ddevice7_setmaterial,
	DTTR_Graphics_COM_Direct3DDevice7 *self,
	void *mat
)

DTTR_COM_STUB_MEMSET(
	s_d3ddevice7_getmaterial,
	DTTR_SIZEOF_D3DMATERIAL7,
	void,
	DTTR_Graphics_COM_Direct3DDevice7 *self
)

DTTR_COM_NOOP_HRESULT(
	s_d3ddevice7_setlight,
	DTTR_Graphics_COM_Direct3DDevice7 *self,
	DWORD idx,
	void *light
)

DTTR_COM_STUB_MEMSET(
	s_d3ddevice7_getlight,
	DTTR_SIZEOF_D3DLIGHT7,
	void,
	DTTR_Graphics_COM_Direct3DDevice7 *self,
	DWORD idx
)

static HRESULT __stdcall s_d3ddevice7_setrenderstate(
	DTTR_Graphics_COM_Direct3DDevice7 *self,
	DWORD state,
	DWORD value
) {

	switch (state) {
	case D3DRENDERSTATE_ZENABLE:
		s_d3d_device7_set_depth_test(value != 0);
		break;
	case D3DRENDERSTATE_ZWRITEENABLE:
		s_d3d_device7_set_depth_write(value != 0);
		break;
	case D3DRENDERSTATE_ZFUNC:
		s_d3d_device7_set_depth_func((DTTR_CompareFunc)value);
		break;
	case D3DRENDERSTATE_ALPHABLENDENABLE:
		s_d3d_device7_set_blend_enabled(value != 0);
		break;
	case D3DRENDERSTATE_SRCBLEND:
		s_d3d_device7_set_blend_func((DTTR_BlendFactor)value, (DTTR_BlendFactor)0);
		break;
	case D3DRENDERSTATE_DESTBLEND:
		s_d3d_device7_set_blend_func((DTTR_BlendFactor)0, (DTTR_BlendFactor)value);
		break;
	case D3DRENDERSTATE_CULLMODE:
		s_d3d_device7_set_cull_mode((DTTR_CullMode)value);
		break;
	}
	return S_OK;
}

DTTR_COM_STUB_SET(
	s_d3ddevice7_getrenderstate,
	DWORD,
	0,
	DTTR_Graphics_COM_Direct3DDevice7 *self,
	DWORD state
)

DTTR_COM_NOOP_HRESULT(
	s_d3ddevice7_beginstateblock,
	DTTR_Graphics_COM_Direct3DDevice7 *self
)

DTTR_COM_STUB_SET(
	s_d3ddevice7_endstateblock,
	DWORD,
	1,
	DTTR_Graphics_COM_Direct3DDevice7 *self
)

DTTR_COM_NOOP_HRESULT(
	s_d3ddevice7_preload,
	DTTR_Graphics_COM_Direct3DDevice7 *self,
	void *tex
)

static HRESULT __stdcall s_d3ddevice7_drawprimitive(
	DTTR_Graphics_COM_Direct3DDevice7 *self,
	DWORD prim_type,
	DWORD fvf,
	void *vertices,
	DWORD count,
	DWORD flags
) {

	if (!vertices || count == 0)
		return S_OK;

	if (count > MAX_VERTICES)
		count = MAX_VERTICES;

	// Parse the flexible vertex format layout
	// https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dfvf
	const DWORD pos_type = fvf & DTTR_D3DFVF_POSITION_MASK;
	const bool has_rhw = (pos_type == DTTR_D3DFVF_XYZRHW);
	const bool has_xyzw = (pos_type == DTTR_D3DFVF_XYZW);
	const bool has_normal = (!has_rhw && !has_xyzw) && ((fvf & DTTR_D3DFVF_NORMAL) != 0);
	const bool has_psize = (!has_rhw && !has_xyzw) && ((fvf & DTTR_D3DFVF_PSIZE) != 0);
	const bool has_diffuse = (fvf & DTTR_D3DFVF_DIFFUSE) != 0;
	const bool has_specular = (fvf & DTTR_D3DFVF_SPECULAR) != 0;
	const int tex_count = (int)((fvf >> DTTR_D3DFVF_TEXCOUNT_SHIFT) & 0xF);
	const bool has_tex = tex_count > 0;

	const uint8_t *src = (const uint8_t *)vertices;
	const DTTR_BackendState *state = &g_dttr_backend;
	const float logical_w = (float)state->m_logical_width;
	const float logical_h = (float)state->m_logical_height;
	const float vp_x = (float)state->m_viewport_x;
	const float vp_y = (float)state->m_viewport_y;
	const float vp_w = (float)((state->m_viewport_w > 0) ? state->m_viewport_w : 1);
	const float vp_h = (float)((state->m_viewport_h > 0) ? state->m_viewport_h : 1);
	const float vp_min_z = state->m_viewport_min_z;
	const float vp_max_z = state->m_viewport_max_z;
	const float vp_z_span = vp_max_z - vp_min_z;

	size_t pos_bytes = 3 * sizeof(float);
	switch (pos_type) {
	case DTTR_D3DFVF_XYZRHW:
	case DTTR_D3DFVF_XYZW:
	case DTTR_D3DFVF_XYZB1:
		pos_bytes = 4 * sizeof(float);
		break;
	case DTTR_D3DFVF_XYZB2:
		pos_bytes = 5 * sizeof(float);
		break;
	case DTTR_D3DFVF_XYZB3:
		pos_bytes = 6 * sizeof(float);
		break;
	case DTTR_D3DFVF_XYZB4:
		pos_bytes = 7 * sizeof(float);
		break;
	case DTTR_D3DFVF_XYZB5:
		pos_bytes = 8 * sizeof(float);
		break;
	default: // Covers D3DFVF_XYZ and unknown position types
		pos_bytes = 3 * sizeof(float);
		break;
	}

	// For blend-weight formats, LASTBETA flags replace the final beta float
	if ((pos_type >= DTTR_D3DFVF_XYZB1 && pos_type <= DTTR_D3DFVF_XYZB5)
		&& ((fvf & DTTR_D3DFVF_LASTBETA_UBYTE4) != 0
			|| (fvf & DTTR_D3DFVF_LASTBETA_D3DCOLOR) != 0)
		&& pos_bytes >= sizeof(float)) {
		pos_bytes -= sizeof(float);
		pos_bytes += sizeof(DWORD);
	}

	const size_t normal_bytes = has_normal ? (3 * sizeof(float)) : 0;
	const size_t psize_bytes = has_psize ? sizeof(float) : 0;
	const size_t diffuse_off = pos_bytes + normal_bytes + psize_bytes;
	const size_t diffuse_bytes = has_diffuse ? sizeof(DWORD) : 0;
	const size_t specular_off = diffuse_off + diffuse_bytes;
	const size_t specular_bytes = has_specular ? sizeof(DWORD) : 0;
	const size_t tex_off = specular_off + specular_bytes;

	size_t stride = tex_off;
	for (int t = 0; t < tex_count; t++) {
		// Dimension code maps 00=2D, 01=3D, 10=4D, 11=1D
		DWORD dim_code = (fvf >> (16 + t * 2)) & 0x3;
		int dim = 2;
		if (dim_code == 1)
			dim = 3;
		else if (dim_code == 2)
			dim = 4;
		else if (dim_code == 3)
			dim = 1;
		stride += (size_t)dim * sizeof(float);
	}

	// Fall back to a sane minimum to avoid UB on malformed FVF
	const size_t min_stride = has_rhw ? (4 * sizeof(float)) : (3 * sizeof(float));
	if (stride < min_stride)
		stride = min_stride;

	for (DWORD i = 0; i < count; i++) {
		const float *v = (const float *)(src + i * stride);
		float out_x = v[0];
		float out_y = v[1];
		float out_z = v[2];

		if (has_rhw) {
			out_x = ((out_x - vp_x) * logical_w) / vp_w;
			out_y = ((out_y - vp_y) * logical_h) / vp_h;
			if (fabsf(vp_z_span) > 1.0e-8f) {
				out_z = (out_z - vp_min_z) / vp_z_span;
			}
		}

		s_d3d_device7_verts[i].x = out_x;
		s_d3d_device7_verts[i].y = out_y;
		s_d3d_device7_verts[i].z = out_z;
		float rhw = has_rhw ? v[3] : 1.0f;
		if (!isfinite(rhw) || rhw <= 0.0f)
			rhw = 1.0f;
		s_d3d_device7_verts[i].rhw = rhw;

		if (has_diffuse) {
			const DWORD c = *(const DWORD *)(src + i * stride + diffuse_off);
			s_d3d_device7_verts[i].a = ((c >> 24) & 0xFF) / 255.0f;
			s_d3d_device7_verts[i].r = ((c >> 16) & 0xFF) / 255.0f;
			s_d3d_device7_verts[i].g = ((c >> 8) & 0xFF) / 255.0f;
			s_d3d_device7_verts[i].b = (c & 0xFF) / 255.0f;
		} else {
			s_d3d_device7_verts[i].a = 1.0f;
			s_d3d_device7_verts[i].r = 1.0f;
			s_d3d_device7_verts[i].g = 1.0f;
			s_d3d_device7_verts[i].b = 1.0f;
		}

		if (has_tex) {
			const float *tc = (const float *)(src + i * stride + tex_off);
			s_d3d_device7_verts[i].u = tc[0];
			s_d3d_device7_verts[i].v = tc[1];
		} else {
			s_d3d_device7_verts[i].u = s_d3d_device7_verts[i].v = 0.0f;
		}
	}

	if (has_rhw) {
		float max_rhw = 0.0f;
		for (DWORD i = 0; i < count; i++) {
			if (s_d3d_device7_verts[i].rhw > max_rhw)
				max_rhw = s_d3d_device7_verts[i].rhw;
		}
		if (max_rhw > 0.0f) {
			const float inv_max = 1.0f / max_rhw;
			for (DWORD i = 0; i < count; i++)
				s_d3d_device7_verts[i].rhw *= inv_max;
		}
	}

	const DTTR_PrimitiveType type = s_d3d_device7_map_primitive_type(prim_type);
	s_d3d_device7_record_draw(type, s_d3d_device7_verts, count, has_rhw, has_tex);
	return S_OK;
}

DTTR_COM_NOOP_HRESULT(
	s_d3ddevice7_drawindexedprimitive,
	DTTR_Graphics_COM_Direct3DDevice7 *self,
	DWORD prim,
	DWORD fvf,
	void *v,
	DWORD vn,
	WORD *indices,
	DWORD in,
	DWORD flags
)

DTTR_COM_NOOP_HRESULT(
	s_d3ddevice7_setclipstatus,
	DTTR_Graphics_COM_Direct3DDevice7 *self,
	void *status
)

DTTR_COM_STUB_MEMSET(
	s_d3ddevice7_getclipstatus,
	DTTR_SIZEOF_D3DCLIPSTATUS,
	void,
	DTTR_Graphics_COM_Direct3DDevice7 *self
)

DTTR_COM_NOOP_HRESULT(
	s_d3ddevice7_drawprimitivestrided,
	DTTR_Graphics_COM_Direct3DDevice7 *self,
	DWORD prim,
	DWORD fvf,
	void *d,
	DWORD n,
	DWORD f
)

DTTR_COM_NOOP_HRESULT(
	s_d3ddevice7_drawindexedprimitivestrided,
	DTTR_Graphics_COM_Direct3DDevice7 *self,
	DWORD prim,
	DWORD fvf,
	void *d,
	DWORD vn,
	WORD *i,
	DWORD in,
	DWORD f
)

DTTR_COM_NOOP_HRESULT(
	s_d3ddevice7_drawprimitivevb,
	DTTR_Graphics_COM_Direct3DDevice7 *self,
	DWORD prim,
	void *vb,
	DWORD st,
	DWORD n,
	DWORD f
)

DTTR_COM_NOOP_HRESULT(
	s_d3ddevice7_drawindexedprimitivevb,
	DTTR_Graphics_COM_Direct3DDevice7 *self,
	DWORD prim,
	void *vb,
	DWORD st,
	DWORD vn,
	WORD *i,
	DWORD in,
	DWORD f
)

DTTR_COM_STUB_SET(
	s_d3ddevice7_computespherevisibility,
	DWORD,
	0,
	DTTR_Graphics_COM_Direct3DDevice7 *self,
	void *c,
	float *r,
	DWORD n,
	DWORD f
)

DTTR_COM_STUB_SET(
	s_d3ddevice7_gettexture,
	void *,
	NULL,
	DTTR_Graphics_COM_Direct3DDevice7 *self,
	DWORD stage
)

static HRESULT __stdcall s_d3ddevice7_settexture(
	DTTR_Graphics_COM_Direct3DDevice7 *self,
	DWORD stage,
	void *texture
) {

	if (!texture) {
		// Unbind texture
		s_d3d_device7_texture_bind(DTTR_INVALID_TEXTURE);

		return S_OK;
	}

	const DTTR_Graphics_COM_DirectDrawSurface7 *surf
		= (const DTTR_Graphics_COM_DirectDrawSurface7 *)texture;
	s_d3d_device7_texture_bind(surf->m_dttr_texture);

	return S_OK;
}

DTTR_COM_STUB_SET(
	s_d3ddevice7_gettexturestagestate,
	DWORD,
	0,
	DTTR_Graphics_COM_Direct3DDevice7 *self,
	DWORD stage,
	DWORD type
)

static HRESULT __stdcall s_d3ddevice7_settexturestagestate(
	DTTR_Graphics_COM_Direct3DDevice7 *self,
	DWORD stage,
	DWORD type,
	DWORD value
) {

	switch (type) {
	case D3DTSS_COLOROP:
		s_d3d_device7_set_color_op((int)value);
		break;
	case D3DTSS_ALPHAOP:
		s_d3d_device7_set_alpha_op((int)value);
		break;
	case D3DTSS_ADDRESS: // Sets both U and V address mode for legacy combined state
		s_d3d_device7_set_texture_address_u((DTTR_TextureAddress)value);
		s_d3d_device7_set_texture_address_v((DTTR_TextureAddress)value);
		break;
	case D3DTSS_ADDRESSU:
		s_d3d_device7_set_texture_address_u((DTTR_TextureAddress)value);
		break;
	case D3DTSS_ADDRESSV:
		s_d3d_device7_set_texture_address_v((DTTR_TextureAddress)value);
		break;
	}
	return S_OK;
}

DTTR_COM_STUB_SET(
	s_d3ddevice7_validatedevice,
	DWORD,
	1,
	DTTR_Graphics_COM_Direct3DDevice7 *self
)

DTTR_COM_NOOP_HRESULT(
	s_d3ddevice7_applystateblock,
	DTTR_Graphics_COM_Direct3DDevice7 *self,
	DWORD block
)

DTTR_COM_NOOP_HRESULT(
	s_d3ddevice7_capturestateblock,
	DTTR_Graphics_COM_Direct3DDevice7 *self,
	DWORD block
)

DTTR_COM_NOOP_HRESULT(
	s_d3ddevice7_deletestateblock,
	DTTR_Graphics_COM_Direct3DDevice7 *self,
	DWORD block
)

DTTR_COM_STUB_SET(
	s_d3ddevice7_createstateblock,
	DWORD,
	1,
	DTTR_Graphics_COM_Direct3DDevice7 *self,
	DWORD t
)

DTTR_COM_NOOP_HRESULT(
	s_d3ddevice7_load,
	DTTR_Graphics_COM_Direct3DDevice7 *self,
	void *dst,
	void *dstPt,
	void *src,
	void *srcR,
	DWORD f
)

DTTR_COM_NOOP_HRESULT(
	s_d3ddevice7_lightenable,
	DTTR_Graphics_COM_Direct3DDevice7 *self,
	DWORD idx,
	BOOL enable
)

DTTR_COM_STUB_SET(
	s_d3ddevice7_getlightenable,
	BOOL,
	FALSE,
	DTTR_Graphics_COM_Direct3DDevice7 *self,
	DWORD idx
)

DTTR_COM_NOOP_HRESULT(
	s_d3ddevice7_setclipplane,
	DTTR_Graphics_COM_Direct3DDevice7 *self,
	DWORD idx,
	float *plane
)

DTTR_COM_STUB_MEMSET(
	s_d3ddevice7_getclipplane,
	16,
	float,
	DTTR_Graphics_COM_Direct3DDevice7 *self,
	DWORD idx
)

static HRESULT __stdcall s_d3ddevice7_getinfo(
	DTTR_Graphics_COM_Direct3DDevice7 *self,
	DWORD id,
	void *info,
	DWORD sz
) {

	if (info && sz > 0)
		memset(info, 0, sz);
	return S_OK;
}

static DTTR_Graphics_COM_Direct3DDevice7_VT s_vtbl = {
	.QueryInterface = s_d3ddevice7_queryinterface,
	.AddRef = s_d3ddevice7_addref,
	.Release = s_d3ddevice7_release,
	.GetCaps = s_d3ddevice7_getcaps,
	.EnumTextureFormats = s_d3ddevice7_enumtextureformats,
	.BeginScene = s_d3ddevice7_beginscene,
	.EndScene = s_d3ddevice7_endscene,
	.GetDirect3D = s_d3ddevice7_getdirect3d,
	.SetRenderTarget = s_d3ddevice7_setrendertarget,
	.GetRenderTarget = s_d3ddevice7_getrendertarget,
	.Clear = s_d3ddevice7_clear,
	.SetTransform = s_d3ddevice7_settransform,
	.GetTransform = s_d3ddevice7_gettransform,
	.SetViewport = s_d3ddevice7_setviewport,
	.MultiplyTransform = s_d3ddevice7_multiplytransform,
	.GetViewport = s_d3ddevice7_getviewport,
	.SetMaterial = s_d3ddevice7_setmaterial,
	.GetMaterial = s_d3ddevice7_getmaterial,
	.SetLight = s_d3ddevice7_setlight,
	.GetLight = s_d3ddevice7_getlight,
	.SetRenderState = s_d3ddevice7_setrenderstate,
	.GetRenderState = s_d3ddevice7_getrenderstate,
	.BeginStateBlock = s_d3ddevice7_beginstateblock,
	.EndStateBlock = s_d3ddevice7_endstateblock,
	.PreLoad = s_d3ddevice7_preload,
	.DrawPrimitive = s_d3ddevice7_drawprimitive,
	.DrawIndexedPrimitive = s_d3ddevice7_drawindexedprimitive,
	.SetClipStatus = s_d3ddevice7_setclipstatus,
	.GetClipStatus = s_d3ddevice7_getclipstatus,
	.DrawPrimitiveStrided = s_d3ddevice7_drawprimitivestrided,
	.DrawIndexedPrimitiveStrided = s_d3ddevice7_drawindexedprimitivestrided,
	.DrawPrimitiveVB = s_d3ddevice7_drawprimitivevb,
	.DrawIndexedPrimitiveVB = s_d3ddevice7_drawindexedprimitivevb,
	.ComputeSphereVisibility = s_d3ddevice7_computespherevisibility,
	.GetTexture = s_d3ddevice7_gettexture,
	.SetTexture = s_d3ddevice7_settexture,
	.GetTextureStageState = s_d3ddevice7_gettexturestagestate,
	.SetTextureStageState = s_d3ddevice7_settexturestagestate,
	.ValidateDevice = s_d3ddevice7_validatedevice,
	.ApplyStateBlock = s_d3ddevice7_applystateblock,
	.CaptureStateBlock = s_d3ddevice7_capturestateblock,
	.DeleteStateBlock = s_d3ddevice7_deletestateblock,
	.CreateStateBlock = s_d3ddevice7_createstateblock,
	.Load = s_d3ddevice7_load,
	.LightEnable = s_d3ddevice7_lightenable,
	.GetLightEnable = s_d3ddevice7_getlightenable,
	.SetClipPlane = s_d3ddevice7_setclipplane,
	.GetClipPlane = s_d3ddevice7_getclipplane,
	.GetInfo = s_d3ddevice7_getinfo,
};

DTTR_Graphics_COM_Direct3DDevice7 *dttr_graphics_com_create_direct3ddevice7(void) {
	DTTR_Graphics_COM_Direct3DDevice7 *dev = malloc(
		sizeof(DTTR_Graphics_COM_Direct3DDevice7)
	);
	if (dev) {
		dev->m_vtbl = &s_vtbl;
	}
	return dev;
}
