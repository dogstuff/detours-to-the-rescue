#ifndef GRAPHICS_PRIVATE_H
#define GRAPHICS_PRIVATE_H

#include <SDL3/SDL.h>
#include <dttr_mods.h>
#include <kvec.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <windows.h>

typedef uint32_t DTTR_Texture;
#define DTTR_INVALID_TEXTURE 0
typedef kvec_t(int) DTTR_IntVector;

typedef enum {
	DTTR_PRIM_POINTLIST = 1,
	DTTR_PRIM_LINELIST = 2,
	DTTR_PRIM_LINESTRIP = 3,
	DTTR_PRIM_TRIANGLELIST = 4,
	DTTR_PRIM_TRIANGLESTRIP = 5,
	DTTR_PRIM_TRIANGLEFAN = 6,
} DTTR_PrimitiveType;

typedef enum {
	DTTR_BLEND_ZERO = 1,
	DTTR_BLEND_ONE = 2,
	DTTR_BLEND_SRCCOLOR = 3,
	DTTR_BLEND_INVSRCCOLOR = 4,
	DTTR_BLEND_SRCALPHA = 5,
	DTTR_BLEND_INVSRCALPHA = 6,
	DTTR_BLEND_DESTALPHA = 7,
	DTTR_BLEND_INVDESTALPHA = 8,
	DTTR_BLEND_DESTCOLOR = 9,
	DTTR_BLEND_INVDESTCOLOR = 10,
} DTTR_BlendFactor;

typedef enum {
	DTTR_CMP_NEVER = 1,
	DTTR_CMP_LESS = 2,
	DTTR_CMP_EQUAL = 3,
	DTTR_CMP_LESSEQUAL = 4,
	DTTR_CMP_GREATER = 5,
	DTTR_CMP_NOTEQUAL = 6,
	DTTR_CMP_GREATEREQUAL = 7,
	DTTR_CMP_ALWAYS = 8,
} DTTR_CompareFunc;

typedef enum {
	DTTR_TEXADDR_WRAP = 1,
	DTTR_TEXADDR_MIRROR = 2,
	DTTR_TEXADDR_CLAMP = 3,
	DTTR_TEXADDR_BORDER = 4,
} DTTR_TextureAddress;

typedef enum {
	DTTR_CULL_NONE = 1,
	DTTR_CULL_CW = 2,
	DTTR_CULL_CCW = 3,
} DTTR_CullMode;

#define DTTR_D3DTOP_DISABLE 1
#define DTTR_D3DTOP_SELECTARG1 2
#define DTTR_D3DTOP_SELECTARG2 3
#define DTTR_D3DTOP_MODULATE 4

#define DTTR_D3DTA_DIFFUSE 0
#define DTTR_D3DTA_CURRENT 1
#define DTTR_D3DTA_TEXTURE 2

typedef struct {
	float x, y, z;
	float rhw;
	float r, g, b, a;
	float u, v;
} DTTR_Vertex;

#define DTTR_CLEAR_COLOR 0x01
#define DTTR_CLEAR_DEPTH 0x02
#define DTTR_CLEAR_STENCIL 0x04

typedef struct {
	SDL_GPUTexture *gpu_tex;
	void *pixels;
	int width;
	int height;
	bool pending_upload;
	uint64_t last_update_frame;
	uint32_t update_streak;
	uint32_t refcount;
	bool cache_key_valid;
	uint64_t cache_key;
} DTTR_StagedTexture;

/// One reusable upload slot holding a transfer buffer for texture uploads.
typedef struct {
	SDL_GPUTransferBuffer *transfer_buffer;
	uint32_t capacity;
	bool in_use;
} DTTR_UploadPoolSlot;

#define DTTR_MAX_STAGED_TEXTURES 4096
#define DTTR_UPLOAD_POOL_SIZE 256

typedef struct {
	float mvp[16];
	float screen_size[2];
	float is_2d;
	float has_texture;
	float color_op;
	float color_arg1;
	float color_arg2;
	float alpha_op;
	float alpha_arg1;
	float alpha_arg2;
} DTTR_Uniforms;

typedef enum {
	DTTR_BACKEND_SDL_GPU,
	DTTR_BACKEND_OPENGL,
} DTTR_BackendType;

/// Game-image placement within the present target, in target pixels.
typedef struct {
	int x;
	int y;
	int w;
	int h;
} DTTR_PresentRect;

typedef struct DTTR_BackendState DTTR_BackendState;

/// Backend-specific operations dispatched through function pointers.
typedef struct {
	void (*begin_frame)(DTTR_BackendState *state);
	void (*end_frame)(DTTR_BackendState *state);
	bool (*present_video_frame_bgra)(
		DTTR_BackendState *state,
		const uint8_t *pixels,
		int width,
		int height,
		int stride
	);
	bool (*resize)(DTTR_BackendState *state, int width, int height);
	void (*cleanup)(DTTR_BackendState *state);
	const char *(*get_driver_name)(const DTTR_BackendState *state);
	void (*defer_texture_destroy)(DTTR_BackendState *state, int texture_index);
} DTTR_RendererVtbl;

typedef enum { DTTR_BATCH_DRAW, DTTR_BATCH_CLEAR } DTTR_BatchRecordType;

/// A recorded clear or draw command replayed during frame submission.
typedef struct {
	DTTR_BatchRecordType type;

	union {
		struct {
			uint32_t first_vertex;
			uint32_t vertex_count;
			DTTR_Uniforms uniforms;
			SDL_GPUTexture *texture;
			SDL_GPUSampler *sampler;
			uint32_t texture_index;
			int sampler_index;
			int blend_mode;
			bool depth_test;
			bool depth_write;
		} draw;

		struct {
			uint32_t flags;
			SDL_FColor color;
			float depth;
		} clear;
	};
} DTTR_BatchRecord;

typedef kvec_t(DTTR_BatchRecord) DTTR_BatchRecordVector;

#define DTTR_BLEND_OFF 0
#define DTTR_BLEND_ALPHA 1
#define DTTR_BLEND_ADDITIVE 2
#define DTTR_PIPELINE_COUNT 12
#define DTTR_PIPELINE_INDEX(bmode, dtest, dwrite) ((bmode) * 4 + (dtest) * 2 + (dwrite))

#define DTTR_SAMPLER_COUNT 4
#define DTTR_MAX_ANISOTROPY 16.0f
#define DTTR_MAT4_SIZE (sizeof(float) * 16)
#define DTTR_VERTEX_ATTRIBUTE_COUNT 4

#define DTTR_MAX_FRAME_VERTICES 262144
#define DTTR_VERTEX_SIZE ((uint32_t)sizeof(DTTR_Vertex))

struct DTTR_BackendState {
	SDL_ThreadID gpu_thread_id;

	SDL_Window *window;
	SDL_GPUDevice *device;
	SDL_GPUShaderFormat shader_format;
	SDL_GPUSampleCount msaa_sample_count;
	SDL_GPUCommandBuffer *cmd;
	SDL_GPUTexture *swapchain_tex;
	SDL_GPURenderPass *render_pass;

	SDL_GPUGraphicsPipeline *pipelines[DTTR_PIPELINE_COUNT];

	SDL_GPUSampler *samplers[DTTR_SAMPLER_COUNT];
	SDL_GPUTexture *dummy_texture;
	SDL_GPUTexture *depth_texture;
	SDL_GPUTexture *msaa_render_target;
	SDL_GPUTexture *render_target;
	SDL_GPUTexture *video_texture;
	int video_width;
	int video_height;
	SDL_GPUBuffer *vertex_buffer;
	SDL_GPUTransferBuffer *transfer_buffer;
	int logical_width;
	int logical_height;
	int width;
	int height;
	Uint32 swapchain_width;
	Uint32 swapchain_height;

	SDL_FColor clear_color;
	SDL_GPUTexture *bound_texture;
	DTTR_Texture bound_texture_handle;
	bool depth_test;
	bool depth_write;
	bool blend_enabled;
	DTTR_TextureAddress addr_u;
	DTTR_TextureAddress addr_v;
	DTTR_BlendFactor blend_dst;
	DWORD stage_color_op;
	DWORD stage_color_arg1;
	DWORD stage_color_arg2;
	DWORD stage_alpha_op;
	DWORD stage_alpha_arg1;
	DWORD stage_alpha_arg2;
	int viewport_x;
	int viewport_y;
	int viewport_w;
	int viewport_h;
	float viewport_min_z;
	float viewport_max_z;
	float proj[16];
	float view[16];
	float model[16];
	DTTR_BatchRecordVector batch_records;
	uint32_t vertex_offset;
	void *transfer_mapped;
	uint64_t frame_index;

	DTTR_StagedTexture staged_textures[DTTR_MAX_STAGED_TEXTURES];
	int staged_texture_count;
	DTTR_IntVector pending_upload_indices;
	SDL_Mutex *texture_mutex;
	DTTR_UploadPoolSlot upload_pool[DTTR_UPLOAD_POOL_SIZE];

	uint64_t perf_frame_start_ns;
	uint64_t perf_cpu_ns_accum;
	uint64_t perf_upload_bytes_accum;
	uint32_t perf_upload_textures_accum;
	uint32_t perf_mips_generated_accum;
	uint32_t perf_mips_skipped_accum;
	uint32_t perf_draws_accum;
	uint32_t perf_clears_accum;
	uint32_t perf_pipeline_binds_accum;
	uint32_t perf_sampler_binds_accum;
	uint32_t perf_frame_accum_count;

	DTTR_BackendType backend_type;
	const DTTR_RendererVtbl *renderer;
	void *backend_data;

	bool initialized;
	bool frame_active;

	/// True while end_frame dispatches the BEFORE_RENDER mod callback.
	bool in_frame_callback;
};

extern DTTR_BackendState dttr_backend;

HWND dttr_graphics_init();
void dttr_graphics_cleanup();
SDL_Window *dttr_graphics_get_window();
SDL_GPUDevice *dttr_graphics_get_device();
void dttr_graphics_handle_window_resize(int width, int height);
bool dttr_graphics_present_video_frame_bgra(
	const uint8_t *pixels,
	int width,
	int height,
	int stride
);

void dttr_graphics_begin_frame();
void dttr_graphics_end_frame();

bool dttr_graphics_is_gpu_thread();
/// Returns true when a draw call should receive subpixel logical-scaling seam fill.
bool dttr_graphics_should_fill_mesh_seams(
	DTTR_PrimitiveType type,
	bool transformed,
	bool depth_test,
	bool blend_enabled
);
/// Expands triangle-list vertices by a tiny physical-pixel amount to hide mesh cracks.
void dttr_graphics_fill_mesh_seams(
	DTTR_Vertex *verts,
	uint32_t count,
	int logical_width,
	int logical_height,
	int render_width,
	int render_height
);

#ifdef DTTR_MODS_ENABLED
void dttr_graphics_mod_frame_begin(DTTR_BackendState *state);
void dttr_graphics_mod_before_game_frame(void);
void dttr_graphics_mod_after_game_frame(void);
void dttr_graphics_mod_before_present(void);
void dttr_graphics_mod_after_present(void);
void dttr_graphics_mod_frame_end(DTTR_BackendState *state);
void dttr_graphics_mod_window_created(DTTR_BackendState *state);
void dttr_graphics_mod_window_resized(DTTR_BackendState *state);
void dttr_graphics_mod_window_destroying(DTTR_BackendState *state);
void dttr_graphics_mod_device_created(DTTR_BackendState *state);
void dttr_graphics_mod_device_lost(DTTR_BackendState *state);
void dttr_graphics_mod_device_restored(DTTR_BackendState *state);
void dttr_graphics_mod_device_destroying(DTTR_BackendState *state);
#else
static inline void dttr_graphics_mod_frame_begin(DTTR_BackendState *) {}

static inline void dttr_graphics_mod_before_game_frame() {}

static inline void dttr_graphics_mod_after_game_frame() {}

static inline void dttr_graphics_mod_before_present() {}

static inline void dttr_graphics_mod_after_present() {}

static inline void dttr_graphics_mod_frame_end(DTTR_BackendState *) {}

static inline void dttr_graphics_mod_window_created(DTTR_BackendState *) {}

static inline void dttr_graphics_mod_window_resized(DTTR_BackendState *) {}

static inline void dttr_graphics_mod_window_destroying(DTTR_BackendState *) {}

static inline void dttr_graphics_mod_device_created(DTTR_BackendState *) {}

static inline void dttr_graphics_mod_device_lost(DTTR_BackendState *) {}

static inline void dttr_graphics_mod_device_restored(DTTR_BackendState *) {}

static inline void dttr_graphics_mod_device_destroying(DTTR_BackendState *) {}
#endif

DTTR_PresentRect dttr_graphics_compute_present_rect(
	int dst_w,
	int dst_h,
	int src_w,
	int src_h,
	bool stretch,
	bool integer_fit,
	float fallback_scale
);

void dttr_graphics_mod_present_rect_before();
void dttr_graphics_mod_present_rect_after();

bool dttr_graphics_ensure_staged_texture(DTTR_BackendState *state, DTTR_StagedTexture *st);

int dttr_graphics_calc_mip_levels(int w, int h);
void dttr_graphics_mat4_identity(float *m);

const char *dttr_graphics_shader_format_name(SDL_GPUShaderFormat format);
SDL_GPUShaderFormat dttr_graphics_requested_shader_formats();
SDL_GPUShaderFormat dttr_graphics_select_shader_format(SDL_GPUShaderFormat formats);
SDL_GPUShaderFormat dttr_graphics_shader_format_for_driver(const char *driver);
SDL_GPUShaderFormat dttr_graphics_select_shader_format_for_driver(
	const char *driver,
	SDL_GPUShaderFormat formats
);

void dttr_graphics_set_logical_resolution(int width, int height);
void dttr_graphics_surface_texture_cache_reset();

bool dttr_graphics_sdl3gpu_init(DTTR_BackendState *state);
bool dttr_graphics_opengl_init(DTTR_BackendState *state);

#endif
