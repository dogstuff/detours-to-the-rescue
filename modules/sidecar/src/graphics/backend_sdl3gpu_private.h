#ifndef BACKEND_SDL3GPU_PRIVATE_H
#define BACKEND_SDL3GPU_PRIVATE_H

#include "graphics_private.h"

// Pre-touched vertex buffers rotated with fences, avoiding SDL cycle backings
// that fault badly under Wine/MoltenVK.
#define DTTR_VERTEX_RING_DEPTH 3

/// A retired GPU texture kept for reuse by dimension.
typedef struct {
	SDL_GPUTexture *tex;
	Uint32 width;
	Uint32 height;
} dttr_recycled_texture;

// Reuse retired textures; releasing mid-session re-arms SDL Vulkan defrag.
#define DTTR_TEXTURE_RECYCLE_CAP 4096

/// SDL3GPU backend-private deferred texture destroy queue and upload pool.
typedef struct {
	dttr_recycled_texture *deferred_destroys;
	int deferred_destroy_count;
	int deferred_destroy_capacity;

	dttr_recycled_texture *texture_recycle;
	int texture_recycle_count;
	int texture_recycle_capacity;

	// Parked until cleanup to avoid mid-session SDL Vulkan defrag.
	SDL_GPUTexture **graveyard_textures;
	int graveyard_texture_count;
	int graveyard_texture_capacity;
	SDL_GPUTransferBuffer **graveyard_tbufs;
	int graveyard_tbuf_count;
	int graveyard_tbuf_capacity;
	// Persistent transfer buffer for texture/video upload bursts.
	SDL_GPUTransferBuffer *upload_pool;
	Uint32 upload_pool_capacity;
	// Last command buffer that read from upload_pool.
	SDL_GPUFence *upload_pool_fence;

	SDL_GPUTransferBuffer *vertex_ring[DTTR_VERTEX_RING_DEPTH];
	SDL_GPUBuffer *vertex_gpu_ring[DTTR_VERTEX_RING_DEPTH];
	SDL_GPUFence *vertex_ring_fence[DTTR_VERTEX_RING_DEPTH];
	int vertex_ring_index;
} sdl3_gpu_backend_data;

/// Builds all graphics pipelines used by the SDL3 GPU backend.
bool dttr_graphics_sdl3gpu_create_pipelines();
/// Creates shared GPU resources used by the SDL3 GPU backend.
bool dttr_graphics_sdl3gpu_create_resources();
/// Recreates resolution-dependent render textures after updating target size.
bool dttr_graphics_sdl3gpu_resize_render_textures(int width, int height);

/// Pops a recycled GPU texture matching the dimensions, or returns NULL.
SDL_GPUTexture *dttr_graphics_sdl3gpu_take_recycled_texture(
	DTTR_BackendState *state,
	Uint32 width,
	Uint32 height
);

/// Stores a texture until cleanup instead of releasing it mid-session.
void dttr_graphics_sdl3gpu_bury_texture(DTTR_BackendState *state, SDL_GPUTexture *tex);

/// Stores a transfer buffer until cleanup instead of releasing it mid-session.
void dttr_graphics_sdl3gpu_bury_transfer_buffer(
	DTTR_BackendState *state,
	SDL_GPUTransferBuffer *tbuf
);

#endif // BACKEND_SDL3GPU_PRIVATE_H
