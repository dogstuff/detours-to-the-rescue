#include "graphics_internal.h"

#include "log.h"

#include <string.h>

// Creates persistent vertex and transfer buffers used by per-frame uploads
static void s_create_frame_buffers(DTTR_BackendState *state) {
	const SDL_GPUBufferCreateInfo vbuf_info = {
		.usage = SDL_GPU_BUFFERUSAGE_VERTEX,
		.size = DTTR_MAX_FRAME_VERTICES * DTTR_VERTEX_SIZE,
	};
	state->m_vertex_buffer = SDL_CreateGPUBuffer(state->m_device, &vbuf_info);

	const SDL_GPUTransferBufferCreateInfo tbuf_info = {
		.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
		.size = DTTR_MAX_FRAME_VERTICES * DTTR_VERTEX_SIZE,
	};
	state->m_transfer_buffer = SDL_CreateGPUTransferBuffer(state->m_device, &tbuf_info);
}

// Creates all sampler variants for wrap/clamp combinations
static void s_create_samplers(DTTR_BackendState *state) {
	for (int cu = 0; cu < 2; cu++) {
		for (int cv = 0; cv < 2; cv++) {
			const SDL_GPUSamplerCreateInfo si = {
				.min_filter = SDL_GPU_FILTER_LINEAR,
				.mag_filter = SDL_GPU_FILTER_LINEAR,
				.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR,
				.address_mode_u
				= cu ? SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE : SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
				.address_mode_v
				= cv ? SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE : SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
				.enable_anisotropy = true,
				.max_anisotropy = DTTR_MAX_ANISOTROPY,
			};

			state->m_samplers[cu * 2 + cv] = SDL_CreateGPUSampler(state->m_device, &si);
		}
	}
}

// Creates render-target and depth textures for the current render resolution
static void s_create_render_textures(DTTR_BackendState *state) {
	const SDL_GPUTextureFormat swapchain_fmt
		= SDL_GetGPUSwapchainTextureFormat(state->m_device, state->m_window);
	const SDL_GPUSampleCount sample_count = state->m_msaa_sample_count;

	const SDL_GPUTextureCreateInfo rt_info = {
		.type = SDL_GPU_TEXTURETYPE_2D,
		.format = swapchain_fmt,
		.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER,
		.width = state->m_width,
		.height = state->m_height,
		.layer_count_or_depth = 1,
		.num_levels = 1,
		.sample_count = SDL_GPU_SAMPLECOUNT_1,
	};
	state->m_render_target = SDL_CreateGPUTexture(state->m_device, &rt_info);

	state->m_msaa_render_target = NULL;

	if (sample_count != SDL_GPU_SAMPLECOUNT_1) {
		const SDL_GPUTextureCreateInfo msaa_rt_info = {
			.type = SDL_GPU_TEXTURETYPE_2D,
			.format = swapchain_fmt,
			.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET,
			.width = state->m_width,
			.height = state->m_height,
			.layer_count_or_depth = 1,
			.num_levels = 1,
			.sample_count = sample_count,
		};
		state->m_msaa_render_target = SDL_CreateGPUTexture(state->m_device, &msaa_rt_info);
	}

	const SDL_GPUTextureCreateInfo depth_tex_info = {
		.type = SDL_GPU_TEXTURETYPE_2D,
		.format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT,
		.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
		.width = state->m_width,
		.height = state->m_height,
		.layer_count_or_depth = 1,
		.num_levels = 1,
		.sample_count = sample_count,
	};
	state->m_depth_texture = SDL_CreateGPUTexture(state->m_device, &depth_tex_info);
}

// Creates the fallback 1x1 texture used when no texture is bound
static void s_create_dummy_texture(DTTR_BackendState *state) {
	const SDL_GPUTextureCreateInfo dummy_tex_info = {
		.type = SDL_GPU_TEXTURETYPE_2D,
		.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
		.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER
				 | SDL_GPU_TEXTUREUSAGE_COMPUTE_STORAGE_SIMULTANEOUS_READ_WRITE,
		.width = 1,
		.height = 1,
		.layer_count_or_depth = 1,
		.num_levels = 1,
	};
	state->m_dummy_texture = SDL_CreateGPUTexture(state->m_device, &dummy_tex_info);
}

// Uploads a single white pixel into the dummy texture via the compute path
static void s_upload_dummy_pixel_to_texture(
	const DTTR_BackendState *state, SDL_GPUBuffer *pixel_buf, SDL_GPUTransferBuffer *tbuf
) {
	const uint32_t white_pixel = 0xFFFFFFFF;
	const uint32_t buf_size = sizeof(white_pixel);

	void *mapped = SDL_MapGPUTransferBuffer(state->m_device, tbuf, false);

	if (!mapped)
		return;

	memcpy(mapped, &white_pixel, buf_size);
	SDL_UnmapGPUTransferBuffer(state->m_device, tbuf);

	SDL_GPUCommandBuffer *cmd = SDL_AcquireGPUCommandBuffer(state->m_device);

	if (!cmd)
		return;

	SDL_GPUCopyPass *copy = SDL_BeginGPUCopyPass(cmd);

	if (copy) {
		const SDL_GPUTransferBufferLocation src = {
			.transfer_buffer = tbuf,
		};
		const SDL_GPUBufferRegion dst = {
			.buffer = pixel_buf,
			.size = buf_size,
		};
		SDL_UploadToGPUBuffer(copy, &src, &dst, false);
		SDL_EndGPUCopyPass(copy);
	}

	SDL_GPUComputePass *comp = SDL_BeginGPUComputePass(
		cmd,
		&(SDL_GPUStorageTextureReadWriteBinding){
			.texture = state->m_dummy_texture,
		},
		1,
		NULL,
		0
	);

	if (comp) {
		SDL_BindGPUComputePipeline(comp, state->m_buf2tex_pipeline);
		SDL_BindGPUComputeStorageBuffers(comp, 0, &pixel_buf, 1);
		const uint32_t pc[2] = {1, 1}; // width, height
		SDL_PushGPUComputeUniformData(cmd, 0, pc, sizeof(pc));
		SDL_DispatchGPUCompute(comp, 1, 1, 1);
		SDL_EndGPUComputePass(comp);
	}

	SDL_SubmitGPUCommandBuffer(cmd);
}

// Allocates temporary upload buffers and seeds the fallback dummy texture
static void s_upload_dummy_white_pixel(DTTR_BackendState *state) {
	const uint32_t buf_size = sizeof(uint32_t);

	const SDL_GPUBufferCreateInfo sbuf_info = {
		.usage = SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ,
		.size = buf_size,
	};
	SDL_GPUBuffer *pixel_buf = SDL_CreateGPUBuffer(state->m_device, &sbuf_info);

	if (!pixel_buf)
		return;

	const SDL_GPUTransferBufferCreateInfo tbuf_info = {
		.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
		.size = buf_size,
	};
	SDL_GPUTransferBuffer *tbuf = SDL_CreateGPUTransferBuffer(state->m_device, &tbuf_info);

	if (!tbuf) {
		SDL_ReleaseGPUBuffer(state->m_device, pixel_buf);
		return;
	}

	s_upload_dummy_pixel_to_texture(state, pixel_buf, tbuf);
	SDL_ReleaseGPUTransferBuffer(state->m_device, tbuf);
	SDL_ReleaseGPUBuffer(state->m_device, pixel_buf);
}

// Creates persistent GPU buffers, samplers, and textures required for rendering
bool dttr_graphics_create_resources(void) {
	DTTR_BackendState *state = &g_dttr_backend;

	s_create_frame_buffers(state);
	s_create_samplers(state);
	s_create_render_textures(state);
	s_create_dummy_texture(state);

	if (!state->m_vertex_buffer || !state->m_transfer_buffer) {
		log_error("graphics: Failed to create frame buffers");
		return false;
	}

	if (!state->m_samplers[0] || !state->m_samplers[1] || !state->m_samplers[2]
		|| !state->m_samplers[3]) {
		log_error("graphics: Failed to create samplers");
		return false;
	}

	if (!state->m_render_target || !state->m_depth_texture) {
		log_error("graphics: Failed to create render textures");
		return false;
	}

	if (!state->m_dummy_texture) {
		log_error("graphics: Failed to create dummy texture");
		return false;
	}

	if (state->m_msaa_sample_count != SDL_GPU_SAMPLECOUNT_1 && !state->m_msaa_render_target) {
		log_error("graphics: Failed to create MSAA render target");
		return false;
	}

	// initialize the 1x1 fallback texture via compute shader
	s_upload_dummy_white_pixel(state);
	return true;
}

// Recreates render-target and depth textures to match a new resolution
bool dttr_graphics_resize_render_textures(int width, int height) {
	DTTR_BackendState *state = &g_dttr_backend;

	if (!state->m_device || !state->m_window || width <= 0 || height <= 0)
		return false;

	if (state->m_render_target) {
		SDL_ReleaseGPUTexture(state->m_device, state->m_render_target);
		state->m_render_target = NULL;
	}

	if (state->m_msaa_render_target) {
		SDL_ReleaseGPUTexture(state->m_device, state->m_msaa_render_target);
		state->m_msaa_render_target = NULL;
	}

	if (state->m_depth_texture) {
		SDL_ReleaseGPUTexture(state->m_device, state->m_depth_texture);
		state->m_depth_texture = NULL;
	}

	state->m_width = width;
	state->m_height = height;
	s_create_render_textures(state);

	if (!state->m_render_target || !state->m_depth_texture) {
		log_error("graphics: Failed to recreate render textures at %dx%d", width, height);
		return false;
	}

	if (state->m_msaa_sample_count != SDL_GPU_SAMPLECOUNT_1 && !state->m_msaa_render_target) {
		log_error("graphics: Failed to recreate MSAA render target at %dx%d", width, height);
		return false;
	}

	return true;
}
