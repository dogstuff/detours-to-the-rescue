#include "backend_sdl3gpu_internal.h"
#include "graphics_internal.h"

#define S_DRIVER_DISPLAY_VULKAN "Vulkan"
#define S_DRIVER_DISPLAY_DIRECT3D12 "Direct3D 12"

#include "log.h"

#include <dttr_config.h>
#include <dttr_sidecar.h>

#ifdef DTTR_COMPONENTS_ENABLED
#include "../components/components_internal.h"
#include "imgui_overlay_internal.h"
#endif

#include <math.h>
#include <stdlib.h>
#include <string.h>

static const DTTR_RendererVtbl s_renderer;

typedef struct {
	Uint32 x;
	Uint32 y;
	Uint32 w;
	Uint32 h;
} S_GraphicsPresentRect;

// Converts config integer sample count into SDL GPU sample-count enum
static SDL_GPUSampleCount s_msaa_sample_count_from_config(int value) {
	switch (value) {
	case 2:
		return SDL_GPU_SAMPLECOUNT_2;
	case 4:
		return SDL_GPU_SAMPLECOUNT_4;
	case 8:
		return SDL_GPU_SAMPLECOUNT_8;
	default:
		return SDL_GPU_SAMPLECOUNT_1;
	}
}

// Returns readable integer form for logging a sample-count enum
static int s_msaa_sample_count_to_int(SDL_GPUSampleCount value) {
	switch (value) {
	case SDL_GPU_SAMPLECOUNT_2:
		return 2;
	case SDL_GPU_SAMPLECOUNT_4:
		return 4;
	case SDL_GPU_SAMPLECOUNT_8:
		return 8;
	default:
		return 1;
	}
}

// Selects the runtime MSAA sample count based on config and GPU support
static SDL_GPUSampleCount s_select_msaa_sample_count(DTTR_BackendState *state) {
	const SDL_GPUSampleCount requested = s_msaa_sample_count_from_config(
		g_dttr_config.m_msaa_samples
	);
	if (requested == SDL_GPU_SAMPLECOUNT_1) {
		return SDL_GPU_SAMPLECOUNT_1;
	}

	const SDL_GPUTextureFormat swapchain_fmt = SDL_GetGPUSwapchainTextureFormat(
		state->m_device,
		state->m_window
	);
	const bool color_supported = SDL_GPUTextureSupportsSampleCount(
		state->m_device,
		swapchain_fmt,
		requested
	);
	const bool depth_supported = SDL_GPUTextureSupportsSampleCount(
		state->m_device,
		SDL_GPU_TEXTUREFORMAT_D32_FLOAT,
		requested
	);

	if (color_supported && depth_supported) {
		return requested;
	}

	log_warn(
		"Requested MSAA x%d is unsupported on this device/format. "
		"Falling back to x1.",
		s_msaa_sample_count_to_int(requested)
	);
	return SDL_GPU_SAMPLECOUNT_1;
}

static void s_destroy_device(DTTR_BackendState *state) {
	if (!state->m_device) {
		return;
	}

	SDL_DestroyGPUDevice(state->m_device);
	state->m_device = NULL;
}

static void s_release_window_device(DTTR_BackendState *state) {
	if (!state->m_device) {
		return;
	}

	SDL_ReleaseWindowFromGPUDevice(state->m_device, state->m_window);
	s_destroy_device(state);
}

static bool s_try_create_device_for_driver(
	DTTR_BackendState *state,
	const SDL_GPUShaderFormat requested_formats,
	const char *driver
) {
	state->m_device = SDL_CreateGPUDevice(requested_formats, false, driver);

	if (!state->m_device) {
		log_warn(
			"Failed to create SDL GPU device for driver '%s' "
			"(requested_formats=0x%x): %s",
			driver ? driver : "default",
			(unsigned int)requested_formats,
			SDL_GetError()
		);
		return false;
	}

	if (!SDL_ClaimWindowForGPUDevice(state->m_device, state->m_window)) {
		log_warn(
			"Failed to claim window for SDL GPU driver '%s': %s",
			driver ? driver : "default",
			SDL_GetError()
		);
		s_destroy_device(state);
		return false;
	}

	const bool immediate_ok = SDL_WindowSupportsGPUPresentMode(
		state->m_device,
		state->m_window,
		SDL_GPU_PRESENTMODE_IMMEDIATE
	);
	if (!immediate_ok) {
		log_warn(
			"IMMEDIATE present mode unsupported for '%s', falling back to VSYNC",
			driver ? driver : "default"
		);
	}

	if (!SDL_SetGPUSwapchainParameters(
			state->m_device,
			state->m_window,
			SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
			immediate_ok ? SDL_GPU_PRESENTMODE_IMMEDIATE : SDL_GPU_PRESENTMODE_VSYNC
		)) {
		log_error("Failed to set swap chain parameters: %s", SDL_GetError());
	}

	const SDL_GPUShaderFormat available_formats = SDL_GetGPUShaderFormats(state->m_device);
	const char *active_driver = SDL_GetGPUDeviceDriver(state->m_device);
	state->m_shader_format = dttr_graphics_select_shader_format_for_driver(
		active_driver,
		available_formats
	);

	if (state->m_shader_format != SDL_GPU_SHADERFORMAT_INVALID) {
		return true;
	}

	log_warn(
		"SDL GPU driver '%s' does not support required shader format. Available "
		"mask=0x%x",
		active_driver ? active_driver : "unknown",
		(unsigned int)available_formats
	);
	s_release_window_device(state);
	return false;
}

// Maps config graphics API mode to SDL GPU driver name (NULL means auto-select)
static const char *s_graphics_api_driver_name(DTTR_GraphicsApi api) {
	switch (api) {
	case DTTR_GRAPHICS_API_VULKAN:
		return DTTR_DRIVER_VULKAN;
	case DTTR_GRAPHICS_API_DIRECT3D12:
		return DTTR_DRIVER_DIRECT3D12;
	default:
		return NULL;
	}
}

// Tries supported backend drivers in order until one produces a usable GPU device
static bool s_create_device(DTTR_BackendState *state) {
	const SDL_GPUShaderFormat requested_formats = dttr_graphics_requested_shader_formats();
	const char *const requested_driver = s_graphics_api_driver_name(
		g_dttr_config.m_graphics_api
	);

	if (requested_driver) {
		if (s_try_create_device_for_driver(state, requested_formats, requested_driver)) {
			return true;
		}

		log_error(
			"GPU device creation failed for configured graphics_api='%s'; no fallback "
			"APIs "
			"will be attempted",
			requested_driver
		);
		return false;
	}

	const char *const driver_candidates[] = {
		DTTR_DRIVER_VULKAN,
		DTTR_DRIVER_DIRECT3D12,
		NULL, // Falls back to the SDL default driver selection.
	};

	for (size_t i = 0; i < SDL_arraysize(driver_candidates); i++) {
		if (s_try_create_device_for_driver(
				state,
				requested_formats,
				driver_candidates[i]
			)) {
			return true;
		}
	}

	log_error("GPU device creation failed for all supported APIs (d3d12/vulkan)");
	return false;
}

bool dttr_graphics_sdl3gpu_init(DTTR_BackendState *state) {
	if (!s_create_device(state)) {
		return false;
	}

	S_SDL3GPUBackendData *bd = calloc(1, sizeof(S_SDL3GPUBackendData));
	if (!bd) {
		s_release_window_device(state);
		return false;
	}
	state->m_backend_data = bd;
	state->m_backend_type = DTTR_BACKEND_SDL_GPU;
	state->m_renderer = &s_renderer;

	state->m_msaa_sample_count = s_select_msaa_sample_count(state);
	log_info(
		"MSAA requested: x%d, effective: x%d",
		g_dttr_config.m_msaa_samples,
		s_msaa_sample_count_to_int(state->m_msaa_sample_count)
	);

	log_info(
		"SDL GPU initialized with %s (shaders: %s)",
		SDL_GetGPUDeviceDriver(state->m_device),
		dttr_graphics_shader_format_name(state->m_shader_format)
	);

	if (!dttr_graphics_sdl3gpu_create_pipelines()
		|| !dttr_graphics_sdl3gpu_create_resources()) {
		log_error("Failed to create GPU resources");
		s_release_window_device(state);
		return false;
	}

	return true;
}

static void s_cleanup(DTTR_BackendState *state) {
	if (!state->m_device) {
		return;
	}

	for (int i = 0; i < DTTR_SAMPLER_COUNT; i++) {
		if (state->m_samplers[i]) {
			SDL_ReleaseGPUSampler(state->m_device, state->m_samplers[i]);
		}
	}

	if (state->m_dummy_texture) {
		SDL_ReleaseGPUTexture(state->m_device, state->m_dummy_texture);
	}

	if (state->m_depth_texture) {
		SDL_ReleaseGPUTexture(state->m_device, state->m_depth_texture);
	}

	if (state->m_msaa_render_target) {
		SDL_ReleaseGPUTexture(state->m_device, state->m_msaa_render_target);
	}

	if (state->m_render_target) {
		SDL_ReleaseGPUTexture(state->m_device, state->m_render_target);
	}

	if (state->m_transfer_buffer) {
		SDL_ReleaseGPUTransferBuffer(state->m_device, state->m_transfer_buffer);
	}

	if (state->m_vertex_buffer) {
		SDL_ReleaseGPUBuffer(state->m_device, state->m_vertex_buffer);
	}

	for (int i = 0; i < DTTR_UPLOAD_POOL_SIZE; i++) {
		DTTR_UploadPoolSlot *slot = &state->m_upload_pool[i];

		if (slot->m_transfer_buffer) {
			SDL_ReleaseGPUTransferBuffer(state->m_device, slot->m_transfer_buffer);
			slot->m_transfer_buffer = NULL;
		}

		slot->m_capacity = 0;
		slot->m_in_use = false;
	}

	for (int i = 0; i < DTTR_PIPELINE_COUNT; i++) {
		if (state->m_pipelines[i]) {
			SDL_ReleaseGPUGraphicsPipeline(state->m_device, state->m_pipelines[i]);
		}
	}

	if (state->m_buf2tex_pipeline) {
		SDL_ReleaseGPUComputePipeline(state->m_device, state->m_buf2tex_pipeline);
	}

	s_release_window_device(state);
	free(state->m_backend_data);
	state->m_backend_data = NULL;
}

// Tracks one pending texture upload ready for mipmap generation
typedef struct {
	SDL_GPUTexture *tex;
	uint32_t bytes;
	bool generate_mips;
	bool uploaded;
} S_GraphicsPendingUpload;

typedef struct {
	uint32_t draw_count;
	uint32_t clear_count;
	uint32_t pipeline_bind_count;
	uint32_t sampler_bind_count;
} S_GraphicsReplayStats;

typedef struct {
	int last_pipeline_idx;
	SDL_GPUTexture *last_texture;
	SDL_GPUSampler *last_sampler;
} S_GraphicsReplayState;

// Returns true when frame rendering is configured to use multisampled color
static bool s_msaa_enabled(const DTTR_BackendState *state) {
	return state->m_msaa_sample_count != SDL_GPU_SAMPLECOUNT_1
		   && state->m_msaa_render_target != NULL;
}

// Marks a reusable upload slot as available
static void s_release_upload_pool_slot(DTTR_BackendState *state, int pool_slot) {
	if (!state || pool_slot < 0 || pool_slot >= DTTR_UPLOAD_POOL_SIZE) {
		return;
	}

	state->m_upload_pool[pool_slot].m_in_use = false;
}

// Acquires or grows one reusable upload slot that can fit `bytes`
static int s_acquire_upload_pool_slot(DTTR_BackendState *state, uint32_t bytes) {
	if (!state || !state->m_device || bytes == 0) {
		return -1;
	}

	int free_slot = -1;
	int grow_slot = -1;

	for (int i = 0; i < DTTR_UPLOAD_POOL_SIZE; i++) {
		DTTR_UploadPoolSlot *slot = &state->m_upload_pool[i];

		if (slot->m_in_use) {
			continue;
		}

		if (slot->m_transfer_buffer && slot->m_capacity >= bytes) {
			slot->m_in_use = true;
			return i;
		}

		if (free_slot < 0) {
			free_slot = i;
		}

		if (slot->m_transfer_buffer) {
			grow_slot = i;
		}
	}

	const int slot_index = (grow_slot >= 0) ? grow_slot : free_slot;

	if (slot_index < 0) {
		return -1;
	}

	DTTR_UploadPoolSlot *slot = &state->m_upload_pool[slot_index];

	if (slot->m_transfer_buffer) {
		SDL_ReleaseGPUTransferBuffer(state->m_device, slot->m_transfer_buffer);
		slot->m_transfer_buffer = NULL;
	}

	const SDL_GPUTransferBufferCreateInfo tbuf_info = {
		.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
		.size = bytes,
	};
	slot->m_transfer_buffer = SDL_CreateGPUTransferBuffer(state->m_device, &tbuf_info);

	if (!slot->m_transfer_buffer) {
		slot->m_capacity = 0;
		slot->m_in_use = false;
		return -1;
	}

	slot->m_capacity = bytes;
	slot->m_in_use = true;
	return slot_index;
}

// Binds the shared frame vertex buffer to the current render pass when available
static void s_bind_frame_vertex_buffer(
	const DTTR_BackendState *state,
	SDL_GPURenderPass *render_pass
) {
	if (!render_pass) {
		return;
	}

	const SDL_GPUBufferBinding vbuf_binding = {
		.buffer = state->m_vertex_buffer,
	};
	SDL_BindGPUVertexBuffers(render_pass, 0, &vbuf_binding, 1);
}

// Ends the active render pass and clears the pass pointer
static void s_end_render_pass_if_active(DTTR_BackendState *state) {
	if (!state->m_render_pass) {
		return;
	}

	SDL_EndGPURenderPass(state->m_render_pass);
	state->m_render_pass = NULL;
}

// Releases textures deferred for destruction on the GPU thread
static void s_release_deferred_texture_destroys(DTTR_BackendState *state) {
	S_SDL3GPUBackendData *bd = (S_SDL3GPUBackendData *)state->m_backend_data;
	if (!bd) {
		return;
	}

	SDL_LockMutex(state->m_texture_mutex);

	for (int i = 0; i < bd->m_deferred_destroy_count; i++) {
		SDL_ReleaseGPUTexture(state->m_device, bd->m_deferred_destroys[i]);
	}

	bd->m_deferred_destroy_count = 0;
	SDL_UnlockMutex(state->m_texture_mutex);
}

// Queues an SDL GPU texture for deferred deletion on the next frame
static void s_defer_texture_destroy(DTTR_BackendState *state, int texture_index) {
	S_SDL3GPUBackendData *bd = (S_SDL3GPUBackendData *)state->m_backend_data;
	if (!bd || texture_index < 0 || texture_index >= DTTR_MAX_STAGED_TEXTURES) {
		return;
	}

	DTTR_StagedTexture *st = &state->m_staged_textures[texture_index];
	if (st->m_gpu_tex && state->m_device) {
		bd->m_deferred_destroys[bd->m_deferred_destroy_count++] = st->m_gpu_tex;
	}
}

// Lazily creates the GPU texture backing a staged texture slot
static bool s_ensure_staged_texture(DTTR_BackendState *state, DTTR_StagedTexture *st) {
	if (st->m_gpu_tex) {
		return true;
	}

	const SDL_GPUTextureCreateInfo tex_info = {
		.type = SDL_GPU_TEXTURETYPE_2D,
		.format = SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM,
		.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COLOR_TARGET,
		.width = st->m_width,
		.height = st->m_height,
		.layer_count_or_depth = 1,
		.num_levels = dttr_graphics_calc_mip_levels(st->m_width, st->m_height),
	};
	st->m_gpu_tex = SDL_CreateGPUTexture(state->m_device, &tex_info);

	if (!st->m_gpu_tex) {
		log_warn(
			"Failed to create GPU texture %dx%d: %s",
			st->m_width,
			st->m_height,
			SDL_GetError()
		);
		return false;
	}

	return true;
}

// Uploads one pending pixel payload directly to a GPU texture via transfer buffer
static bool s_upload_texture_data(
	DTTR_BackendState *state,
	SDL_GPUCopyPass *copy,
	SDL_GPUTexture *tex,
	void *pixels,
	int width,
	int height,
	uint32_t bytes
) {
	if (!copy || !tex || !pixels || bytes == 0) {
		free(pixels);
		return false;
	}

	SDL_GPUTransferBuffer *tbuf = NULL;
	bool from_pool = false;
	int pool_slot = -1;

	if (g_dttr_config.m_texture_upload_sync) {
		pool_slot = s_acquire_upload_pool_slot(state, bytes);
	}

	if (pool_slot >= 0) {
		tbuf = state->m_upload_pool[pool_slot].m_transfer_buffer;
		from_pool = true;
	} else {
		const SDL_GPUTransferBufferCreateInfo tbuf_info = {
			.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
			.size = bytes,
		};
		tbuf = SDL_CreateGPUTransferBuffer(state->m_device, &tbuf_info);

		if (!tbuf) {
			free(pixels);
			return false;
		}
	}

	void *mapped = SDL_MapGPUTransferBuffer(state->m_device, tbuf, false);

	if (!mapped) {
		if (from_pool) {
			s_release_upload_pool_slot(state, pool_slot);
		} else {
			SDL_ReleaseGPUTransferBuffer(state->m_device, tbuf);
		}
		free(pixels);
		return false;
	}

	memcpy(mapped, pixels, bytes);
	SDL_UnmapGPUTransferBuffer(state->m_device, tbuf);

	const SDL_GPUTextureTransferInfo src = {
		.transfer_buffer = tbuf,
		.pixels_per_row = (Uint32)width,
	};
	const SDL_GPUTextureRegion dst = {
		.texture = tex,
		.w = (Uint32)width,
		.h = (Uint32)height,
		.d = 1,
	};
	SDL_UploadToGPUTexture(copy, &src, &dst, false);

	if (from_pool) {
		s_release_upload_pool_slot(state, pool_slot);
	} else {
		SDL_ReleaseGPUTransferBuffer(state->m_device, tbuf);
	}

	free(pixels);
	return true;
}

// Detaches staged pixel payloads and uploads them directly to GPU textures
static int s_collect_and_upload_pending(
	DTTR_BackendState *state,
	SDL_GPUCopyPass *copy,
	S_GraphicsPendingUpload *pending_uploads,
	int max_uploads
) {
	if (!state->m_texture_mutex) {
		return 0;
	}

	int pending_count = 0;
	SDL_LockMutex(state->m_texture_mutex);
	const size_t queued_count = kv_size(state->m_pending_upload_indices);
	size_t deferred_write = 0;

	// Detach pixel payloads under the mutex, collecting texture/size info
	typedef struct {
		SDL_GPUTexture *tex;
		void *pixels;
		int width;
		int height;
		uint32_t bytes;
		bool generate_mips;
	} S_DetachedUpload;

	S_DetachedUpload detached[DTTR_MAX_STAGED_TEXTURES];

	for (size_t q = 0; q < queued_count; q++) {
		const int idx = kv_A(state->m_pending_upload_indices, q);

		if (idx < 0 || idx >= state->m_staged_texture_count) {
			continue;
		}

		DTTR_StagedTexture *st = &state->m_staged_textures[idx];

		if (!st->m_pixels) {
			st->m_pending_upload = false;
			continue;
		}

		if (max_uploads > 0 && pending_count >= max_uploads) {
			kv_A(state->m_pending_upload_indices, deferred_write++) = idx;
			continue;
		}

		st->m_pending_upload = false;

		if (!s_ensure_staged_texture(state, st)) {
			free(st->m_pixels);
			st->m_pixels = NULL;
			continue;
		}

		const uint32_t bytes = (uint32_t)(st->m_width * st->m_height * 4);

		if (bytes == 0) {
			free(st->m_pixels);
			st->m_pixels = NULL;
			continue;
		}

		if (pending_count >= DTTR_MAX_STAGED_TEXTURES) {
			free(st->m_pixels);
			st->m_pixels = NULL;
			continue;
		}

		detached[pending_count] = (S_DetachedUpload){
			.tex = st->m_gpu_tex,
			.pixels = st->m_pixels,
			.width = st->m_width,
			.height = st->m_height,
			.bytes = bytes,
			.generate_mips = g_dttr_config.m_generate_texture_mipmaps,
		};
		st->m_pixels = NULL;
		pending_count++;
	}
	state->m_pending_upload_indices.n = deferred_write;
	SDL_UnlockMutex(state->m_texture_mutex);

	// Upload each detached payload directly to its texture
	for (int i = 0; i < pending_count; i++) {
		const bool ok = s_upload_texture_data(
			state,
			copy,
			detached[i].tex,
			detached[i].pixels,
			detached[i].width,
			detached[i].height,
			detached[i].bytes
		);
		pending_uploads[i] = (S_GraphicsPendingUpload){
			.tex = detached[i].tex,
			.bytes = detached[i].bytes,
			.generate_mips = detached[i].generate_mips,
			.uploaded = ok,
		};
	}

	return pending_count;
}

// Generates mipmaps and tallies stats for successfully uploaded textures
static void s_generate_pending_mipmaps(
	DTTR_BackendState *state,
	SDL_GPUCommandBuffer *cmd,
	const S_GraphicsPendingUpload *pending,
	int pending_count,
	uint32_t *uploaded_texture_count,
	uint64_t *uploaded_bytes
) {
	for (int p = 0; p < pending_count; p++) {
		if (!pending[p].uploaded || !pending[p].tex) {
			continue;
		}

		if (pending[p].generate_mips) {
			SDL_GenerateMipmapsForGPUTexture(cmd, pending[p].tex);
			state->m_perf_mips_generated_accum++;
		} else {
			state->m_perf_mips_skipped_accum++;
		}

		if (uploaded_texture_count) {
			(*uploaded_texture_count)++;
		}

		if (uploaded_bytes) {
			(*uploaded_bytes) += pending[p].bytes;
		}
	}
}

// Flushes all pending staged textures to GPU textures
static void s_upload_pending_textures(DTTR_BackendState *state, SDL_GPUCommandBuffer *cmd) {
	if (!cmd) {
		return;
	}

	S_GraphicsPendingUpload pending[DTTR_MAX_STAGED_TEXTURES] = {0};

	SDL_GPUCopyPass *copy = SDL_BeginGPUCopyPass(cmd);
	const int pending_count = s_collect_and_upload_pending(state, copy, pending, 0);

	if (copy) {
		SDL_EndGPUCopyPass(copy);
	}

	if (pending_count == 0) {
		return;
	}

	uint32_t uploaded_texture_count = 0;
	uint64_t uploaded_bytes = 0;
	s_generate_pending_mipmaps(
		state,
		cmd,
		pending,
		pending_count,
		&uploaded_texture_count,
		&uploaded_bytes
	);

	state->m_perf_upload_textures_accum += uploaded_texture_count;
	state->m_perf_upload_bytes_accum += uploaded_bytes;
}

static S_GraphicsPresentRect s_compute_present_rect(
	Uint32 dst_w,
	Uint32 dst_h,
	int src_w,
	int src_h,
	bool stretch,
	bool integer_fit
) {
	S_GraphicsPresentRect rect = {
		.x = 0,
		.y = 0,
		.w = dst_w,
		.h = dst_h,
	};

	if (stretch) {
		return rect;
	}

	const float sx = (float)dst_w / (float)src_w;
	const float sy = (float)dst_h / (float)src_h;
	float scale = SDL_min(sx, sy);

	if (integer_fit && scale >= 1.0f) {
		scale = floorf(scale);
	}

	rect.w = (Uint32)((float)src_w * scale);
	rect.h = (Uint32)((float)src_h * scale);

	if (rect.w == 0) {
		rect.w = 1;
	}

	if (rect.h == 0) {
		rect.h = 1;
	}

	rect.w = SDL_min(rect.w, dst_w);
	rect.h = SDL_min(rect.h, dst_h);
	rect.x = (dst_w - rect.w) / 2;
	rect.y = (dst_h - rect.h) / 2;
	return rect;
}

static void s_set_default_viewport(const DTTR_BackendState *state) {
	if (!state->m_render_pass) {
		return;
	}

	const SDL_GPUViewport viewport = {
		.x = 0.0f,
		.y = 0.0f,
		.w = (float)state->m_width,
		.h = (float)state->m_height,
		.min_depth = 0.0f,
		.max_depth = 1.0f,
	};
	SDL_SetGPUViewport(state->m_render_pass, &viewport);

	const SDL_Rect scissor = {
		.x = 0,
		.y = 0,
		.w = state->m_width,
		.h = state->m_height,
	};
	SDL_SetGPUScissor(state->m_render_pass, &scissor);
}

// Opens a standard draw pass when no render pass is active
static bool s_begin_draw_pass_if_needed(DTTR_BackendState *state) {
	if (state->m_render_pass) {
		return false;
	}

	const bool use_msaa = s_msaa_enabled(state);
	const SDL_GPUColorTargetInfo color_target = {
		.texture = use_msaa ? state->m_msaa_render_target : state->m_render_target,
		.load_op = SDL_GPU_LOADOP_LOAD,
		.store_op = use_msaa ? SDL_GPU_STOREOP_RESOLVE_AND_STORE : SDL_GPU_STOREOP_STORE,
		.resolve_texture = use_msaa ? state->m_render_target : NULL,
		.resolve_mip_level = 0,
		.resolve_layer = 0,
	};
	const SDL_GPUDepthStencilTargetInfo depth_target = {
		.texture = state->m_depth_texture,
		.load_op = SDL_GPU_LOADOP_LOAD,
		.store_op = SDL_GPU_STOREOP_DONT_CARE,
	};
	state->m_render_pass = SDL_BeginGPURenderPass(
		state->m_cmd,
		&color_target,
		1,
		&depth_target
	);

	if (!state->m_render_pass) {
		log_warn("Failed to begin render pass");
		return false;
	}

	s_bind_frame_vertex_buffer(state, state->m_render_pass);
	s_set_default_viewport(state);
	return true;
}

static void s_reset_replay_state(S_GraphicsReplayState *replay_state) {
	if (!replay_state) {
		return;
	}

	replay_state->last_pipeline_idx = -1;
	replay_state->last_texture = NULL;
	replay_state->last_sampler = NULL;
}

// Opens a render pass configured for the recorded clear command
static void s_begin_clear_pass(
	DTTR_BackendState *state,
	const DTTR_BatchRecord *rec,
	S_GraphicsReplayState *replay_state
) {
	s_end_render_pass_if_active(state);

	const bool use_msaa = s_msaa_enabled(state);
	const SDL_GPUColorTargetInfo color_target = {
		.texture = use_msaa ? state->m_msaa_render_target : state->m_render_target,
		.clear_color = rec->clear.color,
		.load_op = (rec->clear.flags & DTTR_CLEAR_COLOR) ? SDL_GPU_LOADOP_CLEAR
														 : SDL_GPU_LOADOP_LOAD,
		.store_op = use_msaa ? SDL_GPU_STOREOP_RESOLVE_AND_STORE : SDL_GPU_STOREOP_STORE,
		.resolve_texture = use_msaa ? state->m_render_target : NULL,
		.resolve_mip_level = 0,
		.resolve_layer = 0,
	};
	const SDL_GPUDepthStencilTargetInfo depth_target = {
		.texture = state->m_depth_texture,
		.clear_depth = rec->clear.depth,
		.load_op = (rec->clear.flags & DTTR_CLEAR_DEPTH) ? SDL_GPU_LOADOP_CLEAR
														 : SDL_GPU_LOADOP_LOAD,
		.store_op = SDL_GPU_STOREOP_STORE,
	};

	state->m_render_pass = SDL_BeginGPURenderPass(
		state->m_cmd,
		&color_target,
		1,
		&depth_target
	);
	s_bind_frame_vertex_buffer(state, state->m_render_pass);
	s_set_default_viewport(state);
	s_reset_replay_state(replay_state);
}

// Executes one recorded draw command in the active frame command buffer
static void s_draw_batch_record(
	DTTR_BackendState *state,
	const DTTR_BatchRecord *rec,
	S_GraphicsReplayState *replay_state,
	S_GraphicsReplayStats *replay_stats
) {
	const bool began_pass = s_begin_draw_pass_if_needed(state);

	if (began_pass) {
		s_reset_replay_state(replay_state);
	}

	if (!state->m_render_pass) {
		return;
	}

	const int pidx = DTTR_PIPELINE_INDEX(
		rec->draw.blend_mode,
		rec->draw.depth_test,
		rec->draw.depth_write
	);

	if (!replay_state || replay_state->last_pipeline_idx != pidx) {
		SDL_BindGPUGraphicsPipeline(state->m_render_pass, state->m_pipelines[pidx]);

		if (replay_state) {
			replay_state->last_pipeline_idx = pidx;
		}

		if (replay_stats) {
			replay_stats->pipeline_bind_count++;
		}
	}

	SDL_PushGPUVertexUniformData(
		state->m_cmd,
		0,
		&rec->draw.uniforms,
		sizeof(DTTR_Uniforms)
	);
	SDL_PushGPUFragmentUniformData(
		state->m_cmd,
		0,
		&rec->draw.uniforms,
		sizeof(DTTR_Uniforms)
	);

	if (!replay_state || replay_state->last_texture != rec->draw.texture
		|| replay_state->last_sampler != rec->draw.sampler) {
		const SDL_GPUTextureSamplerBinding tex_binding = {
			.texture = rec->draw.texture,
			.sampler = rec->draw.sampler,
		};
		SDL_BindGPUFragmentSamplers(state->m_render_pass, 0, &tex_binding, 1);

		if (replay_state) {
			replay_state->last_texture = rec->draw.texture;
			replay_state->last_sampler = rec->draw.sampler;
		}

		if (replay_stats) {
			replay_stats->sampler_bind_count++;
		}
	}

	SDL_DrawGPUPrimitives(
		state->m_render_pass,
		rec->draw.vertex_count,
		1,
		rec->draw.first_vertex,
		0
	);

	if (replay_stats) {
		replay_stats->draw_count++;
	}
}

// Replays all recorded batch operations for the frame
static S_GraphicsReplayStats s_replay_batch_records(DTTR_BackendState *state) {
	S_GraphicsReplayStats replay_stats = {0};

	if (kv_size(state->m_batch_records) == 0) {
		return replay_stats;
	}

	S_GraphicsReplayState replay_state = {0};
	s_reset_replay_state(&replay_state);
	state->m_render_pass = NULL;

	for (size_t i = 0; i < kv_size(state->m_batch_records); i++) {
		const DTTR_BatchRecord *rec = &kv_A(state->m_batch_records, i);

		if (rec->type == DTTR_BATCH_CLEAR) {
			s_begin_clear_pass(state, rec, &replay_state);
			replay_stats.clear_count++;
			continue;
		}
		s_draw_batch_record(state, rec, &replay_state, &replay_stats);
	}

	s_end_render_pass_if_active(state);
	return replay_stats;
}

// Starts a frame, acquires command/swapchain resources, and maps the upload buffer
static void s_begin_frame(DTTR_BackendState *state) {
	if (!state->m_device || !state->m_window || !dttr_graphics_is_gpu_thread()) {
		return;
	}

	state->m_frame_index++;

	state->m_cmd = SDL_AcquireGPUCommandBuffer(state->m_device);

	if (!state->m_cmd) {
		log_error("Failed to acquire GPU command buffer");
		return;
	}

	s_release_deferred_texture_destroys(state);

	if (!SDL_WaitAndAcquireGPUSwapchainTexture(
			state->m_cmd,
			state->m_window,
			&state->m_swapchain_tex,
			&state->m_swapchain_width,
			&state->m_swapchain_height
		)) {
		log_warn("Failed to acquire swapchain texture: %s", SDL_GetError());
		SDL_CancelGPUCommandBuffer(state->m_cmd);
		state->m_cmd = NULL;
		return;
	}

	// No swapchain image available, skip this frame.
	if (!state->m_swapchain_tex) {
		SDL_CancelGPUCommandBuffer(state->m_cmd);
		state->m_cmd = NULL;
		return;
	}

	// Textures must be uploaded after swapchain acquire for Vulkan.
	s_upload_pending_textures(state, state->m_cmd);

	state->m_batch_records.n = 0;
	state->m_vertex_offset = 0;
	state->m_transfer_mapped = SDL_MapGPUTransferBuffer(
		state->m_device,
		state->m_transfer_buffer,
		true
	);

	if (!state->m_transfer_mapped) {
		log_warn("BeginFrame: MapGPUTransferBuffer failed");
	}

	state->m_frame_active = true;
}

// Finalizes GPU uploads and batch replay, then submits the frame command buffer
static void s_end_frame(DTTR_BackendState *state) {
	state->m_frame_active = false;

	if (state->m_transfer_mapped) {
		SDL_UnmapGPUTransferBuffer(state->m_device, state->m_transfer_buffer);
		state->m_transfer_mapped = NULL;
	}

	if (!state->m_cmd) {
		return;
	}

	if (state->m_vertex_offset > 0) {
		SDL_GPUCopyPass *copy = SDL_BeginGPUCopyPass(state->m_cmd);

		if (copy) {
			const SDL_GPUTransferBufferLocation src = {
				.transfer_buffer = state->m_transfer_buffer,
			};
			const SDL_GPUBufferRegion dst = {
				.buffer = state->m_vertex_buffer,
				.size = state->m_vertex_offset * DTTR_VERTEX_SIZE,
			};
			SDL_UploadToGPUBuffer(copy, &src, &dst, true);
			SDL_EndGPUCopyPass(copy);
		}
	}

	const S_GraphicsReplayStats replay_stats = s_replay_batch_records(state);
	state->m_perf_draws_accum += replay_stats.draw_count;
	state->m_perf_clears_accum += replay_stats.clear_count;
	state->m_perf_pipeline_binds_accum += replay_stats.pipeline_bind_count;
	state->m_perf_sampler_binds_accum += replay_stats.sampler_bind_count;

#ifdef DTTR_COMPONENTS_ENABLED
	dttr_imgui_render_game_sdl3gpu(
		state->m_cmd,
		state->m_render_target,
		(uint32_t)state->m_width,
		(uint32_t)state->m_height
	);
#endif

	if (state->m_swapchain_tex) {
		const Uint32 swap_w = (state->m_swapchain_width > 0) ? state->m_swapchain_width
															 : (Uint32)state->m_width;
		const Uint32 swap_h = (state->m_swapchain_height > 0) ? state->m_swapchain_height
															  : (Uint32)state->m_height;
		const bool is_internal_method
			= (g_dttr_config.m_scaling_method == DTTR_SCALING_METHOD_LOGICAL);
		const S_GraphicsPresentRect present = s_compute_present_rect(
			swap_w,
			swap_h,
			state->m_width,
			state->m_height,
			g_dttr_config.m_scaling_fit == DTTR_SCALING_MODE_STRETCH,
			(!is_internal_method)
				&& (g_dttr_config.m_scaling_fit == DTTR_SCALING_MODE_INTEGER)
		);

		const SDL_GPUBlitInfo blit = {
			.source =
				{
					.texture = state->m_render_target,
					.w = state->m_width,
					.h = state->m_height,
				},
				.destination =
					{
						.texture = state->m_swapchain_tex,
						.x = present.x,
						.y = present.y,
						.w = present.w,
						.h = present.h,
					},
				.clear_color = {0.0f, 0.0f, 0.0f, 1.0f},
				.load_op = SDL_GPU_LOADOP_CLEAR,
			.filter = g_dttr_config.m_present_filter,
		};
		SDL_BlitGPUTexture(state->m_cmd, &blit);

#ifdef DTTR_COMPONENTS_ENABLED
		dttr_imgui_render_sdl3gpu(
			state->m_cmd,
			state->m_swapchain_tex,
			swap_w,
			swap_h,
			present.x,
			present.y,
			present.w,
			present.h
		);
#endif
	}

	SDL_SubmitGPUCommandBuffer(state->m_cmd);

	if (g_dttr_config.m_texture_upload_sync) {
		SDL_WaitForGPUIdle(state->m_device);
	}

	state->m_cmd = NULL;
}

// Ensures the dedicated video texture exists and matches the decoded frame size
static bool s_ensure_video_texture(DTTR_BackendState *state, int width, int height) {
	if (state->m_video_texture && state->m_video_width == width
		&& state->m_video_height == height) {
		return true;
	}

	if (state->m_video_texture) {
		SDL_ReleaseGPUTexture(state->m_device, state->m_video_texture);
		state->m_video_texture = NULL;
	}

	const SDL_GPUTextureCreateInfo tex_info = {
		.type = SDL_GPU_TEXTURETYPE_2D,
		.format = SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM,
		.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER,
		.width = width,
		.height = height,
		.layer_count_or_depth = 1,
		.num_levels = 1,
		.sample_count = SDL_GPU_SAMPLECOUNT_1,
	};
	state->m_video_texture = SDL_CreateGPUTexture(state->m_device, &tex_info);

	if (!state->m_video_texture) {
		return false;
	}

	state->m_video_width = width;
	state->m_video_height = height;
	return true;
}

static bool s_present_video_frame_bgra(
	DTTR_BackendState *state,
	const uint8_t *pixels,
	int width,
	int height,
	int stride
) {
	if (!state->m_device || !state->m_window || !dttr_graphics_is_gpu_thread()) {
		return false;
	}

	if (state->m_frame_active) {
		// Video presentation assumes sole ownership of the command buffer.
		return false;
	}

	if (!s_ensure_video_texture(state, width, height)) {
		return false;
	}

	const Uint32 upload_size = (Uint32)(stride * height);
	const SDL_GPUTransferBufferCreateInfo tbuf_info = {
		.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
		.size = upload_size,
	};
	SDL_GPUTransferBuffer *tbuf = SDL_CreateGPUTransferBuffer(state->m_device, &tbuf_info);

	if (!tbuf) {
		return false;
	}

	void *mapped = SDL_MapGPUTransferBuffer(state->m_device, tbuf, false);

	if (!mapped) {
		SDL_ReleaseGPUTransferBuffer(state->m_device, tbuf);
		return false;
	}

	memcpy(mapped, pixels, upload_size);
	SDL_UnmapGPUTransferBuffer(state->m_device, tbuf);

	SDL_GPUCommandBuffer *cmd = SDL_AcquireGPUCommandBuffer(state->m_device);

	if (!cmd) {
		SDL_ReleaseGPUTransferBuffer(state->m_device, tbuf);
		return false;
	}

	SDL_GPUTexture *swapchain_tex = NULL;
	Uint32 swapchain_w = 0;
	Uint32 swapchain_h = 0;
	SDL_WaitAndAcquireGPUSwapchainTexture(
		cmd,
		state->m_window,
		&swapchain_tex,
		&swapchain_w,
		&swapchain_h
	);

	SDL_GPUCopyPass *copy = SDL_BeginGPUCopyPass(cmd);

	if (copy) {
		const SDL_GPUTextureTransferInfo src = {
			.transfer_buffer = tbuf,
			.offset = 0,
			.pixels_per_row = (Uint32)(stride / 4),
			.rows_per_layer = (Uint32)height,
		};
		const SDL_GPUTextureRegion dst = {
			.texture = state->m_video_texture,
			.mip_level = 0,
			.layer = 0,
			.x = 0,
			.y = 0,
			.z = 0,
			.w = (Uint32)width,
			.h = (Uint32)height,
			.d = 1,
		};
		SDL_UploadToGPUTexture(copy, &src, &dst, false);
		SDL_EndGPUCopyPass(copy);
	}

	if (swapchain_tex) {
		const S_GraphicsPresentRect present = s_compute_present_rect(
			swapchain_w,
			swapchain_h,
			width,
			height,
			false,
			false
		);

		const SDL_GPUBlitInfo blit = {
			.source =
				{
					.texture = state->m_video_texture,
					.mip_level = 0,
					.layer_or_depth_plane = 0,
					.x = 0,
					.y = 0,
					.w = (Uint32)width,
					.h = (Uint32)height,
				},
			.destination =
				{
					.texture = swapchain_tex,
					.mip_level = 0,
					.layer_or_depth_plane = 0,
					.x = present.x,
					.y = present.y,
					.w = present.w,
					.h = present.h,
				},
			.load_op = SDL_GPU_LOADOP_CLEAR,
			.clear_color = (SDL_FColor){0.0f, 0.0f, 0.0f, 1.0f},
			.flip_mode = SDL_FLIP_NONE,
			.filter = g_dttr_config.m_present_filter,
			.cycle = false,
		};
		SDL_BlitGPUTexture(cmd, &blit);
	}

	SDL_SubmitGPUCommandBuffer(cmd);
	SDL_ReleaseGPUTransferBuffer(state->m_device, tbuf);
	return true;
}

static bool s_resize(DTTR_BackendState *state, int width, int height) {
	return dttr_graphics_sdl3gpu_resize_render_textures(width, height);
}

static const char *s_driver_display_name(const char *driver) {
	if (strcmp(driver, DTTR_DRIVER_VULKAN) == 0) {
		return S_DRIVER_DISPLAY_VULKAN;
	}

	if (strcmp(driver, DTTR_DRIVER_DIRECT3D12) == 0) {
		return S_DRIVER_DISPLAY_DIRECT3D12;
	}

	return driver;
}

static const char *s_get_driver_name(const DTTR_BackendState *state) {
	return s_driver_display_name(SDL_GetGPUDeviceDriver(state->m_device));
}

static const DTTR_RendererVtbl s_renderer = {
	.begin_frame = s_begin_frame,
	.end_frame = s_end_frame,
	.present_video_frame_bgra = s_present_video_frame_bgra,
	.resize = s_resize,
	.cleanup = s_cleanup,
	.get_driver_name = s_get_driver_name,
	.defer_texture_destroy = s_defer_texture_destroy,
};
