#ifndef BACKEND_SDL3GPU_PRIVATE_H
#define BACKEND_SDL3GPU_PRIVATE_H

#include "graphics_private.h"

// Pre-touched vertex buffers rotated with fences, avoiding mid-frame backing churn.
#define DTTR_VERTEX_RING_DEPTH 3
#define DTTR_UPLOAD_POOL_RING_DEPTH 3
#define DTTR_GRAVEYARD_TEXTURE_CAP 256
#define DTTR_GRAVEYARD_TRANSFER_BUFFER_CAP 32

/// A retired GPU texture kept for reuse by dimension.
typedef struct {
	SDL_GPUTexture *tex;
	Uint32 width;
	Uint32 height;
} dttr_recycled_texture;

// Reuse retired textures without allowing unbounded growth.
#define DTTR_TEXTURE_RECYCLE_CAP 4096

typedef struct {
	SDL_GPUTransferBuffer *buffer;
	Uint32 capacity;
	SDL_GPUFence *fence;
} dttr_upload_pool_slot;

/// SDL3GPU backend-private deferred texture destroy queue and upload pools.
typedef struct {
	dttr_recycled_texture *deferred_destroys;
	int deferred_destroy_count;
	int deferred_destroy_capacity;

	dttr_recycled_texture *texture_recycle;
	int texture_recycle_count;
	int texture_recycle_capacity;

	// Parked until cleanup, bounded so failed reuse cannot grow forever.
	SDL_GPUTexture **graveyard_textures;
	int graveyard_texture_count;
	int graveyard_texture_capacity;
	SDL_GPUTransferBuffer **graveyard_tbufs;
	int graveyard_tbuf_count;
	int graveyard_tbuf_capacity;
	// Persistent transfer buffers for texture/video upload bursts.
	dttr_upload_pool_slot upload_pools[DTTR_UPLOAD_POOL_RING_DEPTH];
	int upload_pool_index;
	int upload_pool_active_index;

	SDL_GPUTransferBuffer *vertex_ring[DTTR_VERTEX_RING_DEPTH];
	SDL_GPUBuffer *vertex_gpu_ring[DTTR_VERTEX_RING_DEPTH];
	bool vertex_ring_complete[DTTR_VERTEX_RING_DEPTH];
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
