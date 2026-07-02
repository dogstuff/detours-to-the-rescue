// Translates IDirect3DDevice7 calls to the SDL3 GPU backend
// https://archive.org/details/dx7sdk-7001

#include "graphics_com_private.h"
#include "graphics_private.h"
#include <dttr_config.h>
#include <dttr_log.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define MAX_VERTICES 4096
#define DTTR_MAT4_ELEMS 16
#define DTTR_MAT4_BYTES (sizeof(float) * DTTR_MAT4_ELEMS)
static DTTR_Vertex d3d_device7_verts[MAX_VERTICES];

/// Multiplies two row-major 4x4 float matrices into `out`.
static void d3d_device7_mat4_multiply_f(
	float *restrict out,
	const float *restrict a,
	const float *restrict b
) {
	float tmp[DTTR_MAT4_ELEMS];
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			tmp[i * 4 + j] = 0;
			for (int k = 0; k < 4; k++) {
				tmp[i * 4 + j] += a[i * 4 + k] * b[k * 4 + j];
			}
		}
	}

	memcpy(out, tmp, sizeof(tmp));
}

/// Selects the backend transform matrix for a D3D transform state token.
static bool d3d_device7_get_transform_state(DWORD type, float **out_matrix_f) {
	DTTR_BackendState *state = &dttr_backend;
	switch (type) {
	case D3DTRANSFORMSTATE_WORLD:
		*out_matrix_f = state->model;
		return true;
	case D3DTRANSFORMSTATE_VIEW:
		*out_matrix_f = state->view;
		return true;
	case D3DTRANSFORMSTATE_PROJECTION:
		*out_matrix_f = state->proj;
		return true;
	default:
		*out_matrix_f = NULL;
		return false;
	}
}

static const char *d3d_device7_transform_label(DWORD type) {
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

static void d3d_device7_set_transform_state(DWORD type, const float *m) {
	float *matrix_f = NULL;

	if (!m) {
		return;
	}

	if (!d3d_device7_get_transform_state(type, &matrix_f)) {
		return;
	}

	for (int i = 0; i < DTTR_MAT4_ELEMS; i++) {
		if (!isfinite(m[i])) {
			DTTR_LOG_WARN(
				"SetTransform(%s) rejected non-finite matrix input",
				d3d_device7_transform_label(type)
			);
			return;
		}
	}

	memcpy(matrix_f, m, DTTR_MAT4_BYTES);
}

/// Expands a triangle strip into a triangle list.
static uint32_t d3d_device7_expand_strip(
	const DTTR_Vertex *restrict in,
	uint32_t count,
	DTTR_Vertex *restrict out
) {
	if (count < 3) {
		return 0;
	}

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

/// Expands a triangle fan into a triangle list.
static uint32_t d3d_device7_expand_fan(
	const DTTR_Vertex *restrict in,
	uint32_t count,
	DTTR_Vertex *restrict out
) {
	if (count < 3) {
		return 0;
	}

	uint32_t n = 0;
	for (uint32_t i = 0; i < count - 2; i++) {
		out[n++] = in[0];
		out[n++] = in[i + 1];
		out[n++] = in[i + 2];
	}

	return n;
}

/// Maps a D3D primitive code to the internal primitive enum.
static DTTR_PrimitiveType d3d_device7_map_primitive_type(DWORD prim_type) {
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

/// Appends a clear record to the current frame batch.
static void d3d_device7_record_clear(
	uint32_t flags,
	uint32_t color,
	float depth,
	uint32_t stencil
) {
	DTTR_BackendState *state = &dttr_backend;
	if (!state->frame_active && dttr_graphics_is_gpu_thread()) {
		dttr_graphics_begin_frame();
	}

	if (!state->frame_active) {
		return;
	}

	DTTR_BatchRecord clear_rec = {0};
	clear_rec.type = DTTR_BATCH_CLEAR;
	clear_rec.clear.flags = flags;
	clear_rec.clear.depth = depth;

	if (flags & DTTR_CLEAR_COLOR) {
		clear_rec.clear.color.r = ((color >> 16) & 0xff) / 255.0f;
		clear_rec.clear.color.g = ((color >> 8) & 0xff) / 255.0f;
		clear_rec.clear.color.b = (color & 0xff) / 255.0f;
		clear_rec.clear.color.a = ((color >> 24) & 0xff) / 255.0f;
		state->clear_color = clear_rec.clear.color;
	} else {
		clear_rec.clear.color = state->clear_color;
	}

	kv_push(DTTR_BatchRecord, state->batch_records, clear_rec);
}

static DTTR_PrimitiveType d3d_device7_uploaded_primitive_type(DTTR_PrimitiveType type) {
	switch (type) {
	case DTTR_PRIM_TRIANGLESTRIP:
	case DTTR_PRIM_TRIANGLEFAN:
		return DTTR_PRIM_TRIANGLELIST;
	default:
		return type;
	}
}

static uint32_t d3d_device7_expanded_primitive_count(
	DTTR_PrimitiveType type,
	uint32_t count
) {
	switch (type) {
	case DTTR_PRIM_TRIANGLESTRIP:
	case DTTR_PRIM_TRIANGLEFAN:
		return count < 3 ? 0 : (count - 2) * 3;
	default:
		return count;
	}
}

static void d3d_device7_copy_or_expand_primitive(
	DTTR_PrimitiveType *type,
	const DTTR_Vertex *verts,
	uint32_t count,
	DTTR_Vertex *out
) {
	switch (*type) {
	case DTTR_PRIM_TRIANGLESTRIP:
		d3d_device7_expand_strip(verts, count, out);
		*type = DTTR_PRIM_TRIANGLELIST;
		break;
	case DTTR_PRIM_TRIANGLEFAN:
		d3d_device7_expand_fan(verts, count, out);
		*type = DTTR_PRIM_TRIANGLELIST;
		break;
	default:
		memcpy(out, verts, count * DTTR_VERTEX_SIZE);
		break;
	}
}

/// Appends a draw record to the current frame batch.
static void d3d_device7_record_draw(
	DTTR_PrimitiveType type,
	const DTTR_Vertex *verts,
	uint32_t count,
	bool transformed,
	bool textured
) {
	DTTR_BackendState *state = &dttr_backend;

	if (!dttr_graphics_is_gpu_thread() || !verts || count == 0) {
		return;
	}

	if (state->backend_type == DTTR_BACKEND_SDL_GPU && !state->cmd) {
		return;
	}

	if (!state->device && state->backend_type == DTTR_BACKEND_SDL_GPU) {
		DTTR_LOG_WARN("DrawPrimitive: missing device/buffers");
		return;
	}

	if (count > DTTR_MAX_FRAME_VERTICES)
		count = DTTR_MAX_FRAME_VERTICES;

	const DTTR_PrimitiveType upload_type = d3d_device7_uploaded_primitive_type(type);
	const uint32_t upload_count = d3d_device7_expanded_primitive_count(type, count);
	if (upload_count == 0) {
		return;
	}

	const bool fill_mesh_seams = dttr_graphics_should_fill_mesh_seams(
		upload_type,
		transformed,
		state->depth_test,
		state->blend_enabled
	);

	if (!state->transfer_mapped)
		return;
	if (state->vertex_offset + upload_count > DTTR_MAX_FRAME_VERTICES) {
		DTTR_LOG_WARN(
			"DrawPrimitive: frame vertex limit reached (%u + %u > %u)",
			state->vertex_offset,
			upload_count,
			DTTR_MAX_FRAME_VERTICES
		);
		return;
	}

	DTTR_Vertex *upload_verts = (DTTR_Vertex *)((uint8_t *)state->transfer_mapped
												+ state->vertex_offset
													  * DTTR_VERTEX_SIZE);
	d3d_device7_copy_or_expand_primitive(&type, verts, count, upload_verts);

	if (fill_mesh_seams) {
		dttr_graphics_fill_mesh_seams(
			upload_verts,
			upload_count,
			state->logical_width,
			state->logical_height,
			state->width,
			state->height
		);
	}

	DTTR_BatchRecord draw_rec = {0};
	draw_rec.type = DTTR_BATCH_DRAW;
	draw_rec.draw.first_vertex = state->vertex_offset;
	draw_rec.draw.vertex_count = upload_count;
	draw_rec.draw.blend_mode = DTTR_BLEND_OFF;
	if (state->blend_enabled) {
		draw_rec.draw.blend_mode = (state->blend_dst == DTTR_BLEND_ONE)
									   ? DTTR_BLEND_ADDITIVE
									   : DTTR_BLEND_ALPHA;
	}

	draw_rec.draw.depth_test = state->depth_test;
	draw_rec.draw.depth_write = state->depth_write;

	{
		float mv[DTTR_MAT4_ELEMS];
		d3d_device7_mat4_multiply_f(mv, state->view, state->model);
		d3d_device7_mat4_multiply_f(draw_rec.draw.uniforms.mvp, state->proj, mv);
	}

	draw_rec.draw.uniforms.screen_size[0] = (float)state->logical_width;
	draw_rec.draw.uniforms.screen_size[1] = (float)state->logical_height;
	draw_rec.draw.uniforms.is_2d = transformed ? (dttr_config.sprite_smooth ? 2.0f : 1.0f)
											   : 0.0f;
	draw_rec.draw.uniforms.has_texture = textured ? 1.0f : 0.0f;
	draw_rec.draw.uniforms.color_op = (float)state->stage_color_op;
	draw_rec.draw.uniforms.color_arg1 = (float)state->stage_color_arg1;
	draw_rec.draw.uniforms.color_arg2 = (float)state->stage_color_arg2;
	draw_rec.draw.uniforms.alpha_op = (float)state->stage_alpha_op;
	draw_rec.draw.uniforms.alpha_arg1 = (float)state->stage_alpha_arg1;
	draw_rec.draw.uniforms.alpha_arg2 = (float)state->stage_alpha_arg2;

	draw_rec.draw.texture = (textured && state->bound_texture) ? state->bound_texture
															   : state->dummy_texture;
	const int cu = (state->addr_u == DTTR_TEXADDR_CLAMP) ? 1 : 0;
	const int cv = (state->addr_v == DTTR_TEXADDR_CLAMP) ? 1 : 0;
	draw_rec.draw.sampler = state->samplers[cu * 2 + cv];
	draw_rec.draw.sampler_index = cu * 2 + cv;
	draw_rec.draw.texture_index = (textured && state->bound_texture_handle)
									  ? (uint32_t)(state->bound_texture_handle - 1)
									  : UINT32_MAX;

	state->vertex_offset += upload_count;

	// Merge into previous record if state matches and vertices are contiguous
	size_t n = kv_size(state->batch_records);
	if (n > 0) {
		DTTR_BatchRecord *prev = &kv_A(state->batch_records, n - 1);
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

	kv_push(DTTR_BatchRecord, state->batch_records, draw_rec);
}

static void d3d_device7_clear_bound_texture(DTTR_BackendState *state) {
	state->bound_texture_handle = DTTR_INVALID_TEXTURE;
	state->bound_texture = NULL;
}

/// Binds an internal texture handle for subsequent draw records.
static void d3d_device7_texture_bind(DTTR_Texture tex) {
	DTTR_BackendState *state = &dttr_backend;
	if (!tex) {
		if (state->bound_texture_handle == DTTR_INVALID_TEXTURE
			&& !state->bound_texture) {
			return;
		}

		d3d_device7_clear_bound_texture(state);
		return;
	}

	if (state->bound_texture_handle == tex && state->bound_texture) {
		return;
	}

	const int idx = (int)tex - 1;
	if (idx < 0 || idx >= state->staged_texture_count) {
		d3d_device7_clear_bound_texture(state);
		return;
	}

	if (!state->texture_mutex) {
		d3d_device7_clear_bound_texture(state);
		return;
	}

	SDL_LockMutex(state->texture_mutex);
	DTTR_StagedTexture *st = &state->staged_textures[idx];
	if (dttr_graphics_is_gpu_thread()) {
		dttr_graphics_ensure_staged_texture(state, st);
	}

	state->bound_texture_handle = tex;
	state->bound_texture = st->gpu_tex;
	SDL_UnlockMutex(state->texture_mutex);
}

/// Warns once per combo change when the backend's OFF/ALPHA/ADDITIVE mapping
/// cannot represent the requested blend factors.
static void d3d_device7_check_blend_combo(void) {
	static DTTR_BlendFactor warned_src;
	static DTTR_BlendFactor warned_dst;
	const DTTR_BlendFactor src = dttr_backend.blend_src;
	const DTTR_BlendFactor dst = dttr_backend.blend_dst;

	const bool supported = (src == DTTR_BLEND_ONE && dst == DTTR_BLEND_ZERO)
						   || (src == DTTR_BLEND_SRCALPHA
							   && dst == DTTR_BLEND_INVSRCALPHA)
						   || (dst == DTTR_BLEND_ONE
							   && (src == DTTR_BLEND_SRCALPHA || src == DTTR_BLEND_ONE));

	if (supported || (src == warned_src && dst == warned_dst)) {
		return;
	}

	warned_src = src;
	warned_dst = dst;
	DTTR_LOG_WARN("Unsupported blend combo src=%d dst=%d; approximating", src, dst);
}

/// Sets the source blend factor.
static void d3d_device7_set_blend_src(DTTR_BlendFactor src) {
	dttr_backend.blend_src = src;
	d3d_device7_check_blend_combo();
}

/// Sets the destination blend factor.
static void d3d_device7_set_blend_dst(DTTR_BlendFactor dst) {
	dttr_backend.blend_dst = dst;
	d3d_device7_check_blend_combo();
}

/// Sets texture addressing mode for U coordinates.
static void d3d_device7_set_texture_address_u(DTTR_TextureAddress addr) {
	dttr_backend.addr_u = addr;
}

/// Sets texture addressing mode for V coordinates.
static void d3d_device7_set_texture_address_v(DTTR_TextureAddress addr) {
	dttr_backend.addr_v = addr;
}

/// Updates the viewport state used by transformed rendering paths.
static void d3d_device7_set_viewport(int x, int y, int w, int h, float min_z, float max_z) {
	DTTR_BackendState *state = &dttr_backend;
	if (w <= 0 || h <= 0)
		return;

	state->viewport_x = x;
	state->viewport_y = y;
	state->viewport_w = w;
	state->viewport_h = h;
	state->viewport_min_z = min_z;
	state->viewport_max_z = max_z;
}

DTTR_COM_QI_SELF(d3ddevice7_queryinterface, DTTR_Graphics_COM_Direct3DDevice7)

DTTR_COM_ADDREF(d3ddevice7_addref, DTTR_Graphics_COM_Direct3DDevice7)

DTTR_COM_RELEASE(d3ddevice7_release, DTTR_Graphics_COM_Direct3DDevice7)

DTTR_COM_STUB_MEMSET(
	d3ddevice7_getcaps,
	DTTR_SIZEOF_D3DDEVICEDESC7,
	void,
	DTTR_Graphics_COM_Direct3DDevice7 *self
)

static HRESULT __stdcall d3ddevice7_enumtextureformats(
	DTTR_Graphics_COM_Direct3DDevice7 *self,
	void *cb,
	void *ctx
) {

	if (!cb) {
		return S_OK;
	}

	const LPD3DENUMPIXELFORMATSCALLBACK callback = (LPD3DENUMPIXELFORMATSCALLBACK)cb;

	// Report the ARGB4444 format required by the game.
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

	if (hr != 1) {
		return S_OK;
	}

	// Report RGB565 as the fallback texture format.
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

DTTR_COM_NOOP_HRESULT(d3ddevice7_beginscene, DTTR_Graphics_COM_Direct3DDevice7 *self)

DTTR_COM_NOOP_HRESULT(d3ddevice7_endscene, DTTR_Graphics_COM_Direct3DDevice7 *self)

DTTR_COM_STUB_SET(
	d3ddevice7_getdirect3d,
	void *,
	NULL,
	DTTR_Graphics_COM_Direct3DDevice7 *self
)

DTTR_COM_NOOP_HRESULT(
	d3ddevice7_setrendertarget,
	DTTR_Graphics_COM_Direct3DDevice7 *self,
	void *surface,
	DWORD flags
)

DTTR_COM_STUB_SET(
	d3ddevice7_getrendertarget,
	void *,
	NULL,
	DTTR_Graphics_COM_Direct3DDevice7 *self
)

static HRESULT __stdcall d3ddevice7_clear(
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
	d3d_device7_record_clear(f, color, z, stencil);
	return S_OK;
}

static HRESULT __stdcall d3ddevice7_settransform(
	DTTR_Graphics_COM_Direct3DDevice7 *self,
	DWORD type,
	void *matrix
) {
	d3d_device7_set_transform_state(type, (const float *)matrix);
	return S_OK;
}

static HRESULT __stdcall d3ddevice7_gettransform(
	DTTR_Graphics_COM_Direct3DDevice7 *self,
	DWORD type,
	void *matrix
) {
	if (!matrix)
		return S_OK;

	float *matrix_f = NULL;
	if (!d3d_device7_get_transform_state(type, &matrix_f)) {
		memset(matrix, 0, DTTR_MAT4_BYTES);
		return S_OK;
	}

	memcpy(matrix, matrix_f, DTTR_MAT4_BYTES);
	return S_OK;
}

static HRESULT __stdcall d3ddevice7_setviewport(
	DTTR_Graphics_COM_Direct3DDevice7 *self,
	void *vp
) {

	if (!vp)
		return S_OK;

	const D3DVIEWPORT7 *v = (const D3DVIEWPORT7 *)vp;
	d3d_device7_set_viewport(
		(int)v->dwX,
		(int)v->dwY,
		(int)v->dwWidth,
		(int)v->dwHeight,
		v->dvMinZ,
		v->dvMaxZ
	);

	return S_OK;
}

static HRESULT __stdcall d3ddevice7_multiplytransform(
	DTTR_Graphics_COM_Direct3DDevice7 *self,
	DWORD type,
	void *matrix
) {
	if (!matrix)
		return S_OK;

	float *matrix_f = NULL;
	if (!d3d_device7_get_transform_state(type, &matrix_f))
		return S_OK;

	float result[DTTR_MAT4_ELEMS];
	d3d_device7_mat4_multiply_f(result, matrix_f, (const float *)matrix);
	memcpy(matrix_f, result, sizeof(result));
	return S_OK;
}

static HRESULT __stdcall d3ddevice7_getviewport(
	DTTR_Graphics_COM_Direct3DDevice7 *self,
	void *vp
) {

	if (!vp)
		return S_OK;

	D3DVIEWPORT7 *v = (D3DVIEWPORT7 *)vp;
	const DTTR_BackendState *state = &dttr_backend;
	v->dwX = (DWORD)state->viewport_x;
	v->dwY = (DWORD)state->viewport_y;
	v->dwWidth = (DWORD)state->viewport_w;
	v->dwHeight = (DWORD)state->viewport_h;
	v->dvMinZ = state->viewport_min_z;
	v->dvMaxZ = state->viewport_max_z;
	return S_OK;
}

DTTR_COM_NOOP_HRESULT(
	d3ddevice7_setmaterial,
	DTTR_Graphics_COM_Direct3DDevice7 *self,
	void *mat
)

DTTR_COM_STUB_MEMSET(
	d3ddevice7_getmaterial,
	DTTR_SIZEOF_D3DMATERIAL7,
	void,
	DTTR_Graphics_COM_Direct3DDevice7 *self
)

DTTR_COM_NOOP_HRESULT(
	d3ddevice7_setlight,
	DTTR_Graphics_COM_Direct3DDevice7 *self,
	DWORD idx,
	void *light
)

DTTR_COM_STUB_MEMSET(
	d3ddevice7_getlight,
	DTTR_SIZEOF_D3DLIGHT7,
	void,
	DTTR_Graphics_COM_Direct3DDevice7 *self,
	DWORD idx
)

static HRESULT __stdcall d3ddevice7_setrenderstate(
	DTTR_Graphics_COM_Direct3DDevice7 *self,
	DWORD state,
	DWORD value
) {

	switch (state) {
	case D3DRENDERSTATE_ZENABLE:
		dttr_backend.depth_test = value != 0;
		break;
	case D3DRENDERSTATE_ZWRITEENABLE:
		dttr_backend.depth_write = value != 0;
		break;
	case D3DRENDERSTATE_ZFUNC:
		break;
	case D3DRENDERSTATE_ALPHABLENDENABLE:
		dttr_backend.blend_enabled = value != 0;
		break;
	case D3DRENDERSTATE_SRCBLEND:
		d3d_device7_set_blend_src((DTTR_BlendFactor)value);
		break;
	case D3DRENDERSTATE_DESTBLEND:
		d3d_device7_set_blend_dst((DTTR_BlendFactor)value);
		break;
	case D3DRENDERSTATE_CULLMODE:
		break;
	}

	return S_OK;
}

DTTR_COM_STUB_SET(
	d3ddevice7_getrenderstate,
	DWORD,
	0,
	DTTR_Graphics_COM_Direct3DDevice7 *self,
	DWORD state
)

DTTR_COM_NOOP_HRESULT(d3ddevice7_beginstateblock, DTTR_Graphics_COM_Direct3DDevice7 *self)

DTTR_COM_STUB_SET(
	d3ddevice7_endstateblock,
	DWORD,
	1,
	DTTR_Graphics_COM_Direct3DDevice7 *self
)

DTTR_COM_NOOP_HRESULT(
	d3ddevice7_preload,
	DTTR_Graphics_COM_Direct3DDevice7 *self,
	void *tex
)

static HRESULT __stdcall d3ddevice7_drawprimitive(
	DTTR_Graphics_COM_Direct3DDevice7 *self,
	DWORD prim_type,
	DWORD fvf,
	void *vertices,
	DWORD count,
	DWORD flags
) {

	if (!vertices || count == 0) {
		return S_OK;
	}

	if (count > MAX_VERTICES) {
		count = MAX_VERTICES;
	}

	// Parse the flexible vertex format layout.
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
	const DTTR_BackendState *state = &dttr_backend;
	const float logical_w = (float)state->logical_width;
	const float logical_h = (float)state->logical_height;
	const float vp_x = (float)state->viewport_x;
	const float vp_y = (float)state->viewport_y;
	const float vp_w = (float)((state->viewport_w > 0) ? state->viewport_w : 1);
	const float vp_h = (float)((state->viewport_h > 0) ? state->viewport_h : 1);
	const float vp_min_z = state->viewport_min_z;
	const float vp_max_z = state->viewport_max_z;
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
	default:
		// D3DFVF_XYZ and unknown position types use the base XYZ width.
		pos_bytes = 3 * sizeof(float);
		break;
	}

	// LASTBETA flags replace the final beta float in blend-weight formats.
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
		// Dimension code maps 00=2D, 01=3D, 10=4D, 11=1D.
		DWORD dim_code = (fvf >> (16 + t * 2)) & 0x3;
		int dim = 2;
		if (dim_code == 1) {
			dim = 3;
		} else if (dim_code == 2) {
			dim = 4;
		} else if (dim_code == 3) {
			dim = 1;
		}

		stride += (size_t)dim * sizeof(float);
	}

	// Malformed FVF input still needs enough bytes for a position.
	const size_t min_stride = has_rhw ? (4 * sizeof(float)) : (3 * sizeof(float));
	if (stride < min_stride) {
		stride = min_stride;
	}

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

		d3d_device7_verts[i].x = out_x;
		d3d_device7_verts[i].y = out_y;
		d3d_device7_verts[i].z = out_z;
		float rhw = has_rhw ? v[3] : 1.0f;
		if (!isfinite(rhw) || rhw <= 0.0f)
			rhw = 1.0f;
		d3d_device7_verts[i].rhw = rhw;

		if (has_diffuse) {
			const DWORD c = *(const DWORD *)(src + i * stride + diffuse_off);
			d3d_device7_verts[i].a = ((c >> 24) & 0xFF) / 255.0f;
			d3d_device7_verts[i].r = ((c >> 16) & 0xFF) / 255.0f;
			d3d_device7_verts[i].g = ((c >> 8) & 0xFF) / 255.0f;
			d3d_device7_verts[i].b = (c & 0xFF) / 255.0f;
		} else {
			d3d_device7_verts[i].a = 1.0f;
			d3d_device7_verts[i].r = 1.0f;
			d3d_device7_verts[i].g = 1.0f;
			d3d_device7_verts[i].b = 1.0f;
		}

		if (has_tex) {
			const float *tc = (const float *)(src + i * stride + tex_off);
			d3d_device7_verts[i].u = tc[0];
			d3d_device7_verts[i].v = tc[1];
		} else {
			d3d_device7_verts[i].u = d3d_device7_verts[i].v = 0.0f;
		}
	}

	if (has_rhw) {
		float max_rhw = 0.0f;
		for (DWORD i = 0; i < count; i++) {
			if (d3d_device7_verts[i].rhw > max_rhw) {
				max_rhw = d3d_device7_verts[i].rhw;
			}
		}

		if (max_rhw > 0.0f) {
			const float inv_max = 1.0f / max_rhw;
			for (DWORD i = 0; i < count; i++) {
				d3d_device7_verts[i].rhw *= inv_max;
			}
		}
	}

	const DTTR_PrimitiveType type = d3d_device7_map_primitive_type(prim_type);
	d3d_device7_record_draw(type, d3d_device7_verts, count, has_rhw, has_tex);
	return S_OK;
}

DTTR_COM_NOOP_HRESULT(
	d3ddevice7_drawindexedprimitive,
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
	d3ddevice7_setclipstatus,
	DTTR_Graphics_COM_Direct3DDevice7 *self,
	void *status
)

DTTR_COM_STUB_MEMSET(
	d3ddevice7_getclipstatus,
	DTTR_SIZEOF_D3DCLIPSTATUS,
	void,
	DTTR_Graphics_COM_Direct3DDevice7 *self
)

DTTR_COM_NOOP_HRESULT(
	d3ddevice7_drawprimitivestrided,
	DTTR_Graphics_COM_Direct3DDevice7 *self,
	DWORD prim,
	DWORD fvf,
	void *d,
	DWORD n,
	DWORD f
)

DTTR_COM_NOOP_HRESULT(
	d3ddevice7_drawindexedprimitivestrided,
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
	d3ddevice7_drawprimitivevb,
	DTTR_Graphics_COM_Direct3DDevice7 *self,
	DWORD prim,
	void *vb,
	DWORD st,
	DWORD n,
	DWORD f
)

DTTR_COM_NOOP_HRESULT(
	d3ddevice7_drawindexedprimitivevb,
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
	d3ddevice7_computespherevisibility,
	DWORD,
	0,
	DTTR_Graphics_COM_Direct3DDevice7 *self,
	void *c,
	float *r,
	DWORD n,
	DWORD f
)

DTTR_COM_STUB_SET(
	d3ddevice7_gettexture,
	void *,
	NULL,
	DTTR_Graphics_COM_Direct3DDevice7 *self,
	DWORD stage
)

static HRESULT __stdcall d3ddevice7_settexture(
	DTTR_Graphics_COM_Direct3DDevice7 *self,
	DWORD stage,
	void *texture
) {

	if (!texture) {
		// Unbind texture
		d3d_device7_texture_bind(DTTR_INVALID_TEXTURE);

		return S_OK;
	}

	const DTTR_Graphics_COM_DirectDrawSurface7
		*surf = (const DTTR_Graphics_COM_DirectDrawSurface7 *)texture;
	d3d_device7_texture_bind(surf->dttr_texture);

	return S_OK;
}

static HRESULT __stdcall d3ddevice7_gettexturestagestate(
	DTTR_Graphics_COM_Direct3DDevice7 *self,
	DWORD stage,
	DWORD type,
	DWORD *out
) {
	if (!out)
		return S_OK;

	if (stage != 0) {
		*out = 0;
		return S_OK;
	}

	switch (type) {
	case D3DTSS_COLOROP:
		*out = dttr_backend.stage_color_op;
		break;
	case D3DTSS_COLORARG1:
		*out = dttr_backend.stage_color_arg1;
		break;
	case D3DTSS_COLORARG2:
		*out = dttr_backend.stage_color_arg2;
		break;
	case D3DTSS_ALPHAOP:
		*out = dttr_backend.stage_alpha_op;
		break;
	case D3DTSS_ALPHAARG1:
		*out = dttr_backend.stage_alpha_arg1;
		break;
	case D3DTSS_ALPHAARG2:
		*out = dttr_backend.stage_alpha_arg2;
		break;
	case D3DTSS_ADDRESSU:
		*out = (DWORD)dttr_backend.addr_u;
		break;
	case D3DTSS_ADDRESSV:
		*out = (DWORD)dttr_backend.addr_v;
		break;
	default:
		*out = 0;
		break;
	}

	return S_OK;
}

static HRESULT __stdcall d3ddevice7_settexturestagestate(
	DTTR_Graphics_COM_Direct3DDevice7 *self,
	DWORD stage,
	DWORD type,
	DWORD value
) {
	switch (type) {
	case D3DTSS_COLOROP:
		dttr_backend.stage_color_op = (DWORD)value;
		break;
	case D3DTSS_COLORARG1:
		dttr_backend.stage_color_arg1 = value;
		break;
	case D3DTSS_COLORARG2:
		dttr_backend.stage_color_arg2 = value;
		break;
	case D3DTSS_ALPHAOP:
		dttr_backend.stage_alpha_op = (DWORD)value;
		break;
	case D3DTSS_ALPHAARG1:
		dttr_backend.stage_alpha_arg1 = value;
		break;
	case D3DTSS_ALPHAARG2:
		dttr_backend.stage_alpha_arg2 = value;
		break;
	case D3DTSS_ADDRESS:
		// Legacy combined state sets both texture address axes.
		d3d_device7_set_texture_address_u((DTTR_TextureAddress)value);
		d3d_device7_set_texture_address_v((DTTR_TextureAddress)value);
		break;
	case D3DTSS_ADDRESSU:
		d3d_device7_set_texture_address_u((DTTR_TextureAddress)value);
		break;
	case D3DTSS_ADDRESSV:
		d3d_device7_set_texture_address_v((DTTR_TextureAddress)value);
		break;
	}

	return S_OK;
}

DTTR_COM_STUB_SET(
	d3ddevice7_validatedevice,
	DWORD,
	1,
	DTTR_Graphics_COM_Direct3DDevice7 *self
)

DTTR_COM_NOOP_HRESULT(
	d3ddevice7_applystateblock,
	DTTR_Graphics_COM_Direct3DDevice7 *self,
	DWORD block
)

DTTR_COM_NOOP_HRESULT(
	d3ddevice7_capturestateblock,
	DTTR_Graphics_COM_Direct3DDevice7 *self,
	DWORD block
)

DTTR_COM_NOOP_HRESULT(
	d3ddevice7_deletestateblock,
	DTTR_Graphics_COM_Direct3DDevice7 *self,
	DWORD block
)

DTTR_COM_STUB_SET(
	d3ddevice7_createstateblock,
	DWORD,
	1,
	DTTR_Graphics_COM_Direct3DDevice7 *self,
	DWORD t
)

DTTR_COM_NOOP_HRESULT(
	d3ddevice7_load,
	DTTR_Graphics_COM_Direct3DDevice7 *self,
	void *dst,
	void *dstPt,
	void *src,
	void *srcR,
	DWORD f
)

DTTR_COM_NOOP_HRESULT(
	d3ddevice7_lightenable,
	DTTR_Graphics_COM_Direct3DDevice7 *self,
	DWORD idx,
	BOOL enable
)

DTTR_COM_STUB_SET(
	d3ddevice7_getlightenable,
	BOOL,
	FALSE,
	DTTR_Graphics_COM_Direct3DDevice7 *self,
	DWORD idx
)

DTTR_COM_NOOP_HRESULT(
	d3ddevice7_setclipplane,
	DTTR_Graphics_COM_Direct3DDevice7 *self,
	DWORD idx,
	float *plane
)

DTTR_COM_STUB_MEMSET(
	d3ddevice7_getclipplane,
	16,
	float,
	DTTR_Graphics_COM_Direct3DDevice7 *self,
	DWORD idx
)

static HRESULT __stdcall d3ddevice7_getinfo(
	DTTR_Graphics_COM_Direct3DDevice7 *self,
	DWORD id,
	void *info,
	DWORD sz
) {

	if (info && sz > 0)
		memset(info, 0, sz);
	return S_OK;
}

static DTTR_Graphics_COM_Direct3DDevice7_VT vtbl = {
	.QueryInterface = d3ddevice7_queryinterface,
	.AddRef = d3ddevice7_addref,
	.Release = d3ddevice7_release,
	.GetCaps = d3ddevice7_getcaps,
	.EnumTextureFormats = d3ddevice7_enumtextureformats,
	.BeginScene = d3ddevice7_beginscene,
	.EndScene = d3ddevice7_endscene,
	.GetDirect3D = d3ddevice7_getdirect3d,
	.SetRenderTarget = d3ddevice7_setrendertarget,
	.GetRenderTarget = d3ddevice7_getrendertarget,
	.Clear = d3ddevice7_clear,
	.SetTransform = d3ddevice7_settransform,
	.GetTransform = d3ddevice7_gettransform,
	.SetViewport = d3ddevice7_setviewport,
	.MultiplyTransform = d3ddevice7_multiplytransform,
	.GetViewport = d3ddevice7_getviewport,
	.SetMaterial = d3ddevice7_setmaterial,
	.GetMaterial = d3ddevice7_getmaterial,
	.SetLight = d3ddevice7_setlight,
	.GetLight = d3ddevice7_getlight,
	.SetRenderState = d3ddevice7_setrenderstate,
	.GetRenderState = d3ddevice7_getrenderstate,
	.BeginStateBlock = d3ddevice7_beginstateblock,
	.EndStateBlock = d3ddevice7_endstateblock,
	.PreLoad = d3ddevice7_preload,
	.DrawPrimitive = d3ddevice7_drawprimitive,
	.DrawIndexedPrimitive = d3ddevice7_drawindexedprimitive,
	.SetClipStatus = d3ddevice7_setclipstatus,
	.GetClipStatus = d3ddevice7_getclipstatus,
	.DrawPrimitiveStrided = d3ddevice7_drawprimitivestrided,
	.DrawIndexedPrimitiveStrided = d3ddevice7_drawindexedprimitivestrided,
	.DrawPrimitiveVB = d3ddevice7_drawprimitivevb,
	.DrawIndexedPrimitiveVB = d3ddevice7_drawindexedprimitivevb,
	.ComputeSphereVisibility = d3ddevice7_computespherevisibility,
	.GetTexture = d3ddevice7_gettexture,
	.SetTexture = d3ddevice7_settexture,
	.GetTextureStageState = d3ddevice7_gettexturestagestate,
	.SetTextureStageState = d3ddevice7_settexturestagestate,
	.ValidateDevice = d3ddevice7_validatedevice,
	.ApplyStateBlock = d3ddevice7_applystateblock,
	.CaptureStateBlock = d3ddevice7_capturestateblock,
	.DeleteStateBlock = d3ddevice7_deletestateblock,
	.CreateStateBlock = d3ddevice7_createstateblock,
	.Load = d3ddevice7_load,
	.LightEnable = d3ddevice7_lightenable,
	.GetLightEnable = d3ddevice7_getlightenable,
	.SetClipPlane = d3ddevice7_setclipplane,
	.GetClipPlane = d3ddevice7_getclipplane,
	.GetInfo = d3ddevice7_getinfo,
};

DTTR_Graphics_COM_Direct3DDevice7 *dttr_graphics_com_create_direct3ddevice7() {
	DTTR_Graphics_COM_Direct3DDevice7 *dev = malloc(
		sizeof(DTTR_Graphics_COM_Direct3DDevice7)
	);
	if (dev) {
		dev->vtbl = &vtbl;
	}

	return dev;
}
