#ifndef DTTR_BACKEND_SDL3GPU_INTERNAL_H
#define DTTR_BACKEND_SDL3GPU_INTERNAL_H

#include "graphics_internal.h"

// Builds all graphics pipelines used by the SDL3 GPU backend
bool dttr_graphics_sdl3gpu_create_pipelines(void);
// Creates shared GPU resources used by the SDL3 GPU backend
bool dttr_graphics_sdl3gpu_create_resources(void);
// Recreates resolution-dependent render textures after updating target size
bool dttr_graphics_sdl3gpu_resize_render_textures(int width, int height);

#endif /* DTTR_BACKEND_SDL3GPU_INTERNAL_H */
