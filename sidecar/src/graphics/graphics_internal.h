#ifndef DTTR_INTERNAL_H
#define DTTR_INTERNAL_H

#include "dttr_sidecar.h"
#include <SDL3/SDL.h>
#include <dttr_interop.h>
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
	SDL_GPUTexture *m_gpu_tex;
	void *m_pixels;
	int m_width;
	int m_height;
	bool m_pending_upload;
	uint64_t m_last_update_frame;
	uint32_t m_update_streak;
	uint32_t m_refcount;
	bool m_cache_key_valid;
	uint64_t m_cache_key;
} DTTR_StagedTexture;

/// One reusable upload slot holding a paired storage and transfer buffer
typedef struct {
	SDL_GPUBuffer *m_storage_buffer;
	SDL_GPUTransferBuffer *m_transfer_buffer;
	uint32_t m_capacity;
	bool m_in_use;
} DTTR_UploadPoolSlot;

#define DTTR_MAX_STAGED_TEXTURES 4096
#define DTTR_UPLOAD_POOL_SIZE 256

typedef struct {
	float m_mvp[16];
	float m_screen_size[2];
	float m_is_2d;
	float m_has_texture;
} DTTR_Uniforms;

typedef enum { DTTR_BATCH_DRAW, DTTR_BATCH_CLEAR } DTTR_BatchRecordType;

/// A recorded clear or draw command replayed during frame submission
typedef struct {
	DTTR_BatchRecordType type;

	union {
		struct {
			uint32_t first_vertex;
			uint32_t vertex_count;
			DTTR_Uniforms uniforms;
			SDL_GPUTexture *texture;
			SDL_GPUSampler *sampler;
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

#define DTTR_BLEND_OFF 0
#define DTTR_BLEND_ALPHA 1
#define DTTR_BLEND_ADDITIVE 2
#define DTTR_PIPELINE_COUNT 12
#define DTTR_PIPELINE_INDEX(bmode, dtest, dwrite) ((bmode) * 4 + (dtest) * 2 + (dwrite))

#define DTTR_COMPUTE_WORKGROUP_X 16
#define DTTR_COMPUTE_WORKGROUP_Y 16
#define DTTR_COMPUTE_WORKGROUP_Z 1

#define DTTR_SAMPLER_COUNT 4
#define DTTR_MAX_ANISOTROPY 16.0f
#define DTTR_MAT4_SIZE (sizeof(float) * 16)
#define DTTR_VERTEX_ATTRIBUTE_COUNT 4

#define DTTR_MAX_BATCH_RECORDS 2048
#define DTTR_MAX_FRAME_VERTICES 65536
#define DTTR_VERTEX_SIZE ((uint32_t)sizeof(DTTR_Vertex))

typedef struct {
	SDL_ThreadID m_gpu_thread_id;

	SDL_Window *m_window;
	SDL_GPUDevice *m_device;
	SDL_GPUShaderFormat m_shader_format;
	SDL_GPUSampleCount m_msaa_sample_count;
	SDL_GPUCommandBuffer *m_cmd;
	SDL_GPUTexture *m_swapchain_tex;
	SDL_GPURenderPass *m_render_pass;

	SDL_GPUGraphicsPipeline *m_pipelines[DTTR_PIPELINE_COUNT];
	SDL_GPUComputePipeline *m_buf2tex_pipeline;

	SDL_GPUSampler *m_samplers[DTTR_SAMPLER_COUNT];
	SDL_GPUTexture *m_dummy_texture;
	SDL_GPUTexture *m_depth_texture;
	SDL_GPUTexture *m_msaa_render_target;
	SDL_GPUTexture *m_render_target;
	SDL_GPUTexture *m_video_texture;
	int m_video_width;
	int m_video_height;
	SDL_GPUBuffer *m_vertex_buffer;
	SDL_GPUTransferBuffer *m_transfer_buffer;
	int m_logical_width;
	int m_logical_height;
	int m_width;
	int m_height;
	Uint32 m_swapchain_width;
	Uint32 m_swapchain_height;

	SDL_FColor m_clear_color;
	SDL_GPUTexture *m_bound_texture;
	DTTR_Texture m_bound_texture_handle;
	bool m_depth_test;
	bool m_depth_write;
	bool m_blend_enabled;
	DTTR_TextureAddress m_addr_u;
	DTTR_TextureAddress m_addr_v;
	DTTR_BlendFactor m_blend_dst;
	int m_viewport_x;
	int m_viewport_y;
	int m_viewport_w;
	int m_viewport_h;
	float m_viewport_min_z;
	float m_viewport_max_z;
	float m_proj[16];
	float m_view[16];
	float m_model[16];
	double m_proj_d[16];
	double m_view_d[16];
	double m_model_d[16];

	DTTR_BatchRecord m_batch_records[DTTR_MAX_BATCH_RECORDS];
	uint32_t m_batch_count;
	uint32_t m_vertex_offset;
	void *m_transfer_mapped;
	uint64_t m_frame_index;

	DTTR_StagedTexture m_staged_textures[DTTR_MAX_STAGED_TEXTURES];
	int m_staged_texture_count;
	DTTR_IntVector m_pending_upload_indices;
	SDL_Mutex *m_texture_mutex;
	SDL_GPUTexture *m_deferred_destroys[DTTR_MAX_STAGED_TEXTURES];
	int m_deferred_destroy_count;
	DTTR_UploadPoolSlot m_upload_pool[DTTR_UPLOAD_POOL_SIZE];

	uint64_t m_perf_frame_start_ns;
	uint64_t m_perf_cpu_ns_accum;
	uint64_t m_perf_upload_bytes_accum;
	uint32_t m_perf_upload_textures_accum;
	uint32_t m_perf_mips_generated_accum;
	uint32_t m_perf_mips_skipped_accum;
	uint32_t m_perf_draws_accum;
	uint32_t m_perf_clears_accum;
	uint32_t m_perf_pipeline_binds_accum;
	uint32_t m_perf_sampler_binds_accum;
	uint32_t m_perf_frame_accum_count;

	bool m_initialized;
	bool m_frame_active;
} DTTR_BackendState;

extern DTTR_BackendState g_dttr_backend;

/// Begins a new GPU frame and prepares transfer resources
void dttr_graphics_begin_frame(void);
/// Submits queued GPU work and presents the current frame
void dttr_graphics_end_frame(void);
/// Uploads and presents one BGRA video frame directly to the swapchain
bool dttr_graphics_present_video_frame_bgra(
	const uint8_t *pixels,
	int width,
	int height,
	int stride
);

/// Returns true when called from the renderer's GPU thread
bool dttr_graphics_is_gpu_thread(void);

/// Computes the number of mip levels needed for a texture size
int dttr_graphics_calc_mip_levels(int w, int h);
/// Writes an identity matrix into a 4x4 float matrix buffer
void dttr_graphics_mat4_identity(float *m);

/// Returns a readable name for an SDL shader-format bit
const char *dttr_graphics_shader_format_name(SDL_GPUShaderFormat format);
/// Returns the shader formats embedded in this build
SDL_GPUShaderFormat dttr_graphics_requested_shader_formats(void);
/// Chooses the preferred shader format from an available format mask
SDL_GPUShaderFormat dttr_graphics_select_shader_format(SDL_GPUShaderFormat formats);
/// Returns the preferred shader format for a specific backend driver
SDL_GPUShaderFormat dttr_graphics_shader_format_for_driver(const char *driver);
/// Chooses a shader format using driver preference with mask validation
SDL_GPUShaderFormat dttr_graphics_select_shader_format_for_driver(
	const char *driver,
	SDL_GPUShaderFormat formats
);

/// Builds all graphics pipelines used by the renderer
bool dttr_graphics_create_pipelines(void);
/// Creates shared GPU resources used by the renderer
bool dttr_graphics_create_resources(void);
/// Recreates resolution-dependent render textures after updating target size
bool dttr_graphics_resize_render_textures(int width, int height);
/// Updates logical resolution and optionally render size based on scaling method
void dttr_graphics_set_logical_resolution(int width, int height);
/// Applies runtime window resize to rendering policy
void dttr_graphics_handle_window_resize(int width, int height);

/// Clears shared surface-texture cache state
void dttr_graphics_surface_texture_cache_reset(void);

#endif
