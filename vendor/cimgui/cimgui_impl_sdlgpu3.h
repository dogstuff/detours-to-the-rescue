// C wrapper for imgui_impl_sdlgpu3 backend
// Generated for DttR project - wraps the C++ SDL GPU3 renderer backend in extern "C"

#ifndef CIMGUI_IMPL_SDLGPU3_H
#define CIMGUI_IMPL_SDLGPU3_H

#include "cimgui.h"
#include <SDL3/SDL_gpu.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	SDL_GPUDevice *Device;
	SDL_GPUTextureFormat ColorTargetFormat;
	SDL_GPUSampleCount MSAASamples;
	SDL_GPUSwapchainComposition SwapchainComposition;
	SDL_GPUPresentMode PresentMode;
} CImGui_ImplSDLGPU3_InitInfo;

CIMGUI_API bool cImGui_ImplSDLGPU3_Init(CImGui_ImplSDLGPU3_InitInfo *info);
CIMGUI_API void cImGui_ImplSDLGPU3_Shutdown(void);
CIMGUI_API void cImGui_ImplSDLGPU3_NewFrame(void);
CIMGUI_API void cImGui_ImplSDLGPU3_PrepareDrawData(
	ImDrawData *draw_data,
	SDL_GPUCommandBuffer *command_buffer
);
CIMGUI_API void cImGui_ImplSDLGPU3_RenderDrawData(
	ImDrawData *draw_data,
	SDL_GPUCommandBuffer *command_buffer,
	SDL_GPURenderPass *render_pass
);
CIMGUI_API void cImGui_ImplSDLGPU3_CreateDeviceObjects(void);
CIMGUI_API void cImGui_ImplSDLGPU3_DestroyDeviceObjects(void);
CIMGUI_API void cImGui_ImplSDLGPU3_UpdateTexture(ImTextureData *tex);

#ifdef __cplusplus
}
#endif

#endif /* CIMGUI_IMPL_SDLGPU3_H */
