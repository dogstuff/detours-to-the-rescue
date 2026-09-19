#include "graphics_private.h"

#include <dttr_config.h>
#include <dttr_log.h>
#include <math.h>
#include <string.h>

DTTR_BackendState dttr_backend;

bool dttr_graphics_ensure_staged_texture(DTTR_BackendState *state, DTTR_StagedTexture *st) {
	if (!st || !state->device) {
		return st && st->gpu_tex != NULL;
	}

	if (st->gpu_tex) {
		return true;
	}

	const SDL_GPUTextureCreateInfo tex_info = {
		.type = SDL_GPU_TEXTURETYPE_2D,
		.format = SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM,
		.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COLOR_TARGET,
		.width = st->width,
		.height = st->height,
		.layer_count_or_depth = 1,
		.num_levels = dttr_graphics_calc_mip_levels(st->width, st->height),
	};

	st->gpu_tex = SDL_CreateGPUTexture(state->device, &tex_info);

	if (!st->gpu_tex) {
		DTTR_LOG_WARN(
			"Failed to create GPU texture %dx%d: %s",
			st->width,
			st->height,
			SDL_GetError()
		);
		return false;
	}

	return true;
}

DTTR_PresentRect dttr_graphics_compute_present_rect(
	int dst_w,
	int dst_h,
	int src_w,
	int src_h,
	bool stretch,
	bool integer_fit,
	float fallback_scale
) {
	DTTR_PresentRect rect = {
		.x = 0,
		.y = 0,
		.w = dst_w,
		.h = dst_h,
	};

	if (stretch || src_w <= 0 || src_h <= 0) {
		return rect;
	}

	const float sx = (float)dst_w / (float)src_w;
	const float sy = (float)dst_h / (float)src_h;
	float scale = sx < sy ? sx : sy;

	if (scale < 0.001f) {
		scale = fallback_scale;
	}

	if (integer_fit && scale >= 1.0f) {
		scale = floorf(scale);
	}

	rect.w = (int)((float)src_w * scale);
	rect.h = (int)((float)src_h * scale);

	if (rect.w < 1) {
		rect.w = 1;
	}

	if (rect.h < 1) {
		rect.h = 1;
	}

	if (rect.w > dst_w) {
		rect.w = dst_w;
	}

	if (rect.h > dst_h) {
		rect.h = dst_h;
	}

	rect.x = (dst_w - rect.w) / 2;
	rect.y = (dst_h - rect.h) / 2;
	return rect;
}

int dttr_graphics_calc_mip_levels(int w, int h) {
	int d = w > h ? w : h;
	int levels = 1;

	while (d > 1) {
		d >>= 1;
		levels++;
	}

	return levels;
}

void dttr_graphics_mat4_identity(float *m) {
	memset(m, 0, DTTR_MAT4_SIZE);
	m[0] = m[5] = m[10] = m[15] = 1.0f;
}

const char *dttr_graphics_shader_format_name(SDL_GPUShaderFormat format) {
	switch (format) {
	case SDL_GPU_SHADERFORMAT_SPIRV:
		return "SPIRV";
	case SDL_GPU_SHADERFORMAT_DXBC:
		return "DXBC";
	case SDL_GPU_SHADERFORMAT_DXIL:
		return "DXIL";
	case SDL_GPU_SHADERFORMAT_METALLIB:
		return "METALLIB";
	case SDL_GPU_SHADERFORMAT_PRIVATE:
		return "PRIVATE";
	default:
		return "UNKNOWN";
	}
}

SDL_GPUShaderFormat dttr_graphics_requested_shader_formats() {
	return SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXBC
		   | SDL_GPU_SHADERFORMAT_DXIL;
}

SDL_GPUShaderFormat dttr_graphics_select_shader_format(SDL_GPUShaderFormat formats) {
	if (formats & SDL_GPU_SHADERFORMAT_DXIL) {
		return SDL_GPU_SHADERFORMAT_DXIL;
	}

	if (formats & SDL_GPU_SHADERFORMAT_DXBC) {
		return SDL_GPU_SHADERFORMAT_DXBC;
	}

	if (formats & SDL_GPU_SHADERFORMAT_SPIRV) {
		return SDL_GPU_SHADERFORMAT_SPIRV;
	}

	return SDL_GPU_SHADERFORMAT_INVALID;
}

// Selects a driver-preferred shader format, then falls back to generic selection.
SDL_GPUShaderFormat dttr_graphics_select_shader_format_for_driver(
	const char *driver,
	SDL_GPUShaderFormat formats
) {
	if (driver && strcmp(driver, DTTR_DRIVER_DIRECT3D12) == 0) {
		if (formats & SDL_GPU_SHADERFORMAT_DXIL) {
			return SDL_GPU_SHADERFORMAT_DXIL;
		}

		if (formats & SDL_GPU_SHADERFORMAT_DXBC) {
			return SDL_GPU_SHADERFORMAT_DXBC;
		}

		return SDL_GPU_SHADERFORMAT_INVALID;
	}

	if (driver && strcmp(driver, DTTR_DRIVER_VULKAN) == 0) {
		if (formats & SDL_GPU_SHADERFORMAT_SPIRV) {
			return SDL_GPU_SHADERFORMAT_SPIRV;
		}

		return SDL_GPU_SHADERFORMAT_INVALID;
	}

	return dttr_graphics_select_shader_format(formats);
}

bool dttr_graphics_is_gpu_thread() {
	const DTTR_BackendState *state = &dttr_backend;

	if (!state->initialized) {
		return false;
	}

	if (state->gpu_thread_id == 0) {
		return true;
	}

	return SDL_GetCurrentThreadID() == state->gpu_thread_id;
}
