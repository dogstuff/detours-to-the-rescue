#include "graphics_internal.h"

#include <string.h>

DTTR_BackendState g_dttr_backend;

// Calculates the total mip level count for a 2D texture size
int dttr_graphics_calc_mip_levels(int w, int h) {
	int d = w > h ? w : h;
	int levels = 1;

	while (d > 1) {
		d >>= 1;
		levels++;
	}

	return levels;
}

// Writes a 4x4 identity matrix into the provided float array
void dttr_graphics_mat4_identity(float *m) {
	memset(m, 0, DTTR_MAT4_SIZE);
	m[0] = m[5] = m[10] = m[15] = 1.0f;
}

// Returns a stable string name for a shader format enum value
const char *dttr_graphics_shader_format_name(SDL_GPUShaderFormat format) {
	switch (format) {
	case SDL_GPU_SHADERFORMAT_SPIRV:
		return "SPIRV";
	case SDL_GPU_SHADERFORMAT_DXIL:
		return "DXIL";
	case SDL_GPU_SHADERFORMAT_MSL:
		return "MSL";
	case SDL_GPU_SHADERFORMAT_METALLIB:
		return "METALLIB";
	case SDL_GPU_SHADERFORMAT_PRIVATE:
		return "PRIVATE";
	default:
		return "UNKNOWN";
	}
}

// Returns the union of shader formats that this build embeds
SDL_GPUShaderFormat dttr_graphics_requested_shader_formats(void) {
	return SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_MSL;
}

// Picks the best supported shader format from an availability bitmask
SDL_GPUShaderFormat dttr_graphics_select_shader_format(SDL_GPUShaderFormat formats) {
	if (formats & SDL_GPU_SHADERFORMAT_DXIL)
		return SDL_GPU_SHADERFORMAT_DXIL;

	if (formats & SDL_GPU_SHADERFORMAT_MSL)
		return SDL_GPU_SHADERFORMAT_MSL;

	if (formats & SDL_GPU_SHADERFORMAT_SPIRV)
		return SDL_GPU_SHADERFORMAT_SPIRV;

	return SDL_GPU_SHADERFORMAT_INVALID;
}

// Maps a known SDL GPU driver name to its preferred shader format
SDL_GPUShaderFormat dttr_graphics_shader_format_for_driver(const char *driver) {
	if (!driver || !driver[0])
		return SDL_GPU_SHADERFORMAT_INVALID;

	if (strcmp(driver, "direct3d12") == 0)
		return SDL_GPU_SHADERFORMAT_DXIL;

	if (strcmp(driver, "metal") == 0)
		return SDL_GPU_SHADERFORMAT_MSL;

	if (strcmp(driver, "vulkan") == 0)
		return SDL_GPU_SHADERFORMAT_SPIRV;

	return SDL_GPU_SHADERFORMAT_INVALID;
}

// Selects a driver-preferred shader format, then falls back to generic selection
SDL_GPUShaderFormat dttr_graphics_select_shader_format_for_driver(const char *driver, SDL_GPUShaderFormat formats) {
	const SDL_GPUShaderFormat preferred = dttr_graphics_shader_format_for_driver(driver);

	if ((preferred != SDL_GPU_SHADERFORMAT_INVALID) && (formats & preferred))
		return preferred;

	return dttr_graphics_select_shader_format(formats);
}

// Reports whether the caller is running on the backend's GPU thread
bool dttr_graphics_is_gpu_thread(void) {
	const DTTR_BackendState *state = &g_dttr_backend;

	if (!state->m_initialized)
		return false;

	if (state->m_gpu_thread_id == 0)
		return true;

	return SDL_GetCurrentThreadID() == state->m_gpu_thread_id;
}
