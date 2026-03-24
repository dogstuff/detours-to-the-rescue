// C wrapper for imgui_impl_sdlgpu3 backend

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "imgui/backends/imgui_impl_sdlgpu3.h"
#include "cimgui_impl_sdlgpu3.h"

extern "C" {

bool cImGui_ImplSDLGPU3_Init(CImGui_ImplSDLGPU3_InitInfo *info) {
	ImGui_ImplSDLGPU3_InitInfo cpp_info = {};
	cpp_info.Device = info->Device;
	cpp_info.ColorTargetFormat = info->ColorTargetFormat;
	cpp_info.MSAASamples = info->MSAASamples;
	cpp_info.SwapchainComposition = info->SwapchainComposition;
	cpp_info.PresentMode = info->PresentMode;
	return ImGui_ImplSDLGPU3_Init(&cpp_info);
}

void cImGui_ImplSDLGPU3_Shutdown(void) {
	ImGui_ImplSDLGPU3_Shutdown();
}

void cImGui_ImplSDLGPU3_NewFrame(void) {
	ImGui_ImplSDLGPU3_NewFrame();
}

void cImGui_ImplSDLGPU3_PrepareDrawData(
	ImDrawData *draw_data,
	SDL_GPUCommandBuffer *command_buffer
) {
	ImGui_ImplSDLGPU3_PrepareDrawData(draw_data, command_buffer);
}

void cImGui_ImplSDLGPU3_RenderDrawData(
	ImDrawData *draw_data,
	SDL_GPUCommandBuffer *command_buffer,
	SDL_GPURenderPass *render_pass
) {
	ImGui_ImplSDLGPU3_RenderDrawData(draw_data, command_buffer, render_pass, nullptr);
}

void cImGui_ImplSDLGPU3_CreateDeviceObjects(void) {
	ImGui_ImplSDLGPU3_CreateDeviceObjects();
}

void cImGui_ImplSDLGPU3_DestroyDeviceObjects(void) {
	ImGui_ImplSDLGPU3_DestroyDeviceObjects();
}

void cImGui_ImplSDLGPU3_UpdateTexture(ImTextureData *tex) {
	ImGui_ImplSDLGPU3_UpdateTexture(tex);
}

} // extern "C"
