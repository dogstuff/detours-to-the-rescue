#include "backend_sdl3gpu_private.h"
#include "graphics_private.h"

#include <dttr_log.h>

#include <string.h>

// Creates persistent vertex and transfer buffers used by per-frame uploads.
static void create_frame_buffers(DTTR_BackendState *state) {
	const uint32_t frame_buffer_size = DTTR_MAX_FRAME_VERTICES * DTTR_VERTEX_SIZE;
	const SDL_GPUBufferCreateInfo vbuf_info = {
		.usage = SDL_GPU_BUFFERUSAGE_VERTEX,
		.size = frame_buffer_size,
	};
	state->vertex_buffer = SDL_CreateGPUBuffer(state->device, &vbuf_info);

	const SDL_GPUTransferBufferCreateInfo tbuf_info = {
		.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
		.size = frame_buffer_size,
	};
	state->transfer_buffer = SDL_CreateGPUTransferBuffer(state->device, &tbuf_info);
}

// Creates all sampler variants for wrap/clamp combinations.
static void create_samplers(DTTR_BackendState *state) {
	for (int cu = 0; cu < 2; cu++) {
		for (int cv = 0; cv < 2; cv++) {
			const SDL_GPUSamplerCreateInfo si = {
				.min_filter = SDL_GPU_FILTER_LINEAR,
				.mag_filter = SDL_GPU_FILTER_LINEAR,
				.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR,
				.address_mode_u = cu ? SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE
									 : SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
				.address_mode_v = cv ? SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE
									 : SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
				.enable_anisotropy = true,
				.max_anisotropy = DTTR_MAX_ANISOTROPY,
			};

			state->samplers[cu * 2 + cv] = SDL_CreateGPUSampler(state->device, &si);
		}
	}
}

// Creates render-target and depth textures for the current render resolution.
static void create_render_textures(DTTR_BackendState *state) {
	const SDL_GPUTextureFormat swapchain_fmt = SDL_GetGPUSwapchainTextureFormat(
		state->device,
		state->window
	);
	const SDL_GPUSampleCount sample_count = state->msaa_sample_count;

	const SDL_GPUTextureCreateInfo rt_info = {
		.type = SDL_GPU_TEXTURETYPE_2D,
		.format = swapchain_fmt,
		.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER,
		.width = state->width,
		.height = state->height,
		.layer_count_or_depth = 1,
		.num_levels = 1,
		.sample_count = SDL_GPU_SAMPLECOUNT_1,
	};
	state->render_target = SDL_CreateGPUTexture(state->device, &rt_info);

	state->msaa_render_target = NULL;

	if (sample_count != SDL_GPU_SAMPLECOUNT_1) {
		const SDL_GPUTextureCreateInfo msaa_rt_info = {
			.type = SDL_GPU_TEXTURETYPE_2D,
			.format = swapchain_fmt,
			.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET,
			.width = state->width,
			.height = state->height,
			.layer_count_or_depth = 1,
			.num_levels = 1,
			.sample_count = sample_count,
		};
		state->msaa_render_target = SDL_CreateGPUTexture(state->device, &msaa_rt_info);
	}

	const SDL_GPUTextureCreateInfo depth_tex_info = {
		.type = SDL_GPU_TEXTURETYPE_2D,
		.format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT,
		.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
		.width = state->width,
		.height = state->height,
		.layer_count_or_depth = 1,
		.num_levels = 1,
		.sample_count = sample_count,
	};
	state->depth_texture = SDL_CreateGPUTexture(state->device, &depth_tex_info);
}

// Creates the fallback 1x1 texture used when no texture is bound.
static void create_dummy_texture(DTTR_BackendState *state) {
	const SDL_GPUTextureCreateInfo dummy_tex_info = {
		.type = SDL_GPU_TEXTURETYPE_2D,
		.format = SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM,
		.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER,
		.width = 1,
		.height = 1,
		.layer_count_or_depth = 1,
		.num_levels = 1,
	};
	state->dummy_texture = SDL_CreateGPUTexture(state->device, &dummy_tex_info);
}

// Uploads a single white pixel directly into the dummy texture.
static void upload_dummy_white_pixel(DTTR_BackendState *state) {
	const uint32_t white_pixel = 0xFFFFFFFF;
	const uint32_t buf_size = sizeof(white_pixel);

	const SDL_GPUTransferBufferCreateInfo tbuf_info = {
		.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
		.size = buf_size,
	};
	SDL_GPUTransferBuffer *tbuf = SDL_CreateGPUTransferBuffer(state->device, &tbuf_info);

	if (!tbuf) {
		return;
	}

	void *mapped = SDL_MapGPUTransferBuffer(state->device, tbuf, false);

	if (!mapped) {
		SDL_ReleaseGPUTransferBuffer(state->device, tbuf);
		return;
	}

	memcpy(mapped, &white_pixel, buf_size);
	SDL_UnmapGPUTransferBuffer(state->device, tbuf);

	SDL_GPUCommandBuffer *cmd = SDL_AcquireGPUCommandBuffer(state->device);

	if (!cmd) {
		SDL_ReleaseGPUTransferBuffer(state->device, tbuf);
		return;
	}

	SDL_GPUCopyPass *copy = SDL_BeginGPUCopyPass(cmd);

	if (copy) {
		const SDL_GPUTextureTransferInfo src = {
			.transfer_buffer = tbuf,
			.pixels_per_row = 1,
		};
		const SDL_GPUTextureRegion dst = {
			.texture = state->dummy_texture,
			.w = 1,
			.h = 1,
			.d = 1,
		};
		SDL_UploadToGPUTexture(copy, &src, &dst, false);
		SDL_EndGPUCopyPass(copy);
	}

	SDL_SubmitGPUCommandBuffer(cmd);
	SDL_ReleaseGPUTransferBuffer(state->device, tbuf);
}

// Creates persistent GPU buffers, samplers, and textures required for rendering.
bool dttr_graphics_sdl3gpu_create_resources() {
	DTTR_BackendState *state = &dttr_backend;
	const bool needs_msaa_target = state->msaa_sample_count != SDL_GPU_SAMPLECOUNT_1;

	create_frame_buffers(state);
	create_samplers(state);
	create_render_textures(state);
	create_dummy_texture(state);

	if (!state->vertex_buffer || !state->transfer_buffer) {
		DTTR_LOG_ERROR("Failed to create frame buffers");
		return false;
	}

	for (int i = 0; i < DTTR_SAMPLER_COUNT; i++) {
		if (state->samplers[i]) {
			continue;
		}

		DTTR_LOG_ERROR("Failed to create samplers");
		return false;
	}

	if (!state->render_target || !state->depth_texture) {
		DTTR_LOG_ERROR("Failed to create render textures");
		return false;
	}

	if (!state->dummy_texture) {
		DTTR_LOG_ERROR("Failed to create dummy texture");
		return false;
	}

	if (needs_msaa_target && !state->msaa_render_target) {
		DTTR_LOG_ERROR("Failed to create MSAA render target");
		return false;
	}

	upload_dummy_white_pixel(state);
	return true;
}

// Recreates render-target and depth textures to match a new resolution.
bool dttr_graphics_sdl3gpu_resize_render_textures(int width, int height) {
	DTTR_BackendState *state = &dttr_backend;
	const bool needs_msaa_target = state->msaa_sample_count != SDL_GPU_SAMPLECOUNT_1;

	if (!state->device || !state->window || width <= 0 || height <= 0) {
		return false;
	}

	if (state->render_target) {
		SDL_ReleaseGPUTexture(state->device, state->render_target);
		state->render_target = NULL;
	}

	if (state->msaa_render_target) {
		SDL_ReleaseGPUTexture(state->device, state->msaa_render_target);
		state->msaa_render_target = NULL;
	}

	if (state->depth_texture) {
		SDL_ReleaseGPUTexture(state->device, state->depth_texture);
		state->depth_texture = NULL;
	}

	state->width = width;
	state->height = height;
	create_render_textures(state);

	if (!state->render_target || !state->depth_texture) {
		DTTR_LOG_ERROR("Failed to recreate render textures at %dx%d", width, height);
		return false;
	}

	if (needs_msaa_target && !state->msaa_render_target) {
		DTTR_LOG_ERROR("Failed to recreate MSAA render target at %dx%d", width, height);
		return false;
	}

	return true;
}
