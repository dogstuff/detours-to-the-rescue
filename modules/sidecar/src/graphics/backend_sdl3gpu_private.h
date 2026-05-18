#ifndef BACKEND_SDL3GPU_PRIVATE_H
#define BACKEND_SDL3GPU_PRIVATE_H

#include "graphics_private.h"

/// SDL3GPU backend-private deferred texture destroy queue.
typedef struct {
	SDL_GPUTexture *deferred_destroys[DTTR_MAX_STAGED_TEXTURES];
	int deferred_destroy_count;
} sdl3_gpu_backend_data;

/// Builds all graphics pipelines used by the SDL3 GPU backend.
bool dttr_graphics_sdl3gpu_create_pipelines();
/// Creates shared GPU resources used by the SDL3 GPU backend.
bool dttr_graphics_sdl3gpu_create_resources();
/// Recreates resolution-dependent render textures after updating target size.
bool dttr_graphics_sdl3gpu_resize_render_textures(int width, int height);

#endif // BACKEND_SDL3GPU_PRIVATE_H
