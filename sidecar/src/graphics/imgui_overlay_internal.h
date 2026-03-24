#ifndef DTTR_IMGUI_OVERLAY_INTERNAL_H
#define DTTR_IMGUI_OVERLAY_INTERNAL_H

#include "graphics_internal.h"

#include <SDL3/SDL.h>
#include <stdbool.h>

void dttr_imgui_init(SDL_Window *window, SDL_GPUDevice *device, DTTR_BackendType backend);
void dttr_imgui_cleanup(void);
bool dttr_imgui_process_event(const SDL_Event *event);

void dttr_imgui_render_game_sdl3gpu(
	SDL_GPUCommandBuffer *cmd,
	SDL_GPUTexture *render_target,
	uint32_t w,
	uint32_t h
);

void dttr_imgui_render_game_opengl(void);

void dttr_imgui_render_sdl3gpu(
	SDL_GPUCommandBuffer *cmd,
	SDL_GPUTexture *swapchain_tex,
	uint32_t swap_w,
	uint32_t swap_h,
	uint32_t game_x,
	uint32_t game_y,
	uint32_t game_w,
	uint32_t game_h
);

void dttr_imgui_render_opengl(
	uint32_t game_x,
	uint32_t game_y,
	uint32_t game_w,
	uint32_t game_h
);

#endif /* DTTR_IMGUI_OVERLAY_INTERNAL_H */
