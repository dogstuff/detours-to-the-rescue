#include "backend_sdl3gpu_private.h"
#include "graphics_private.h"

#include <dttr_log.h>

#include <dttr_config.h>

#define DRIVER_DISPLAY_VULKAN "Vulkan"
#define DRIVER_DISPLAY_DIRECT3D12 "Direct3D 12"

#ifdef DTTR_MODS_ENABLED
#include "../mods/mods_private.h"
#include "imgui_overlay_private.h"
#endif

#include <math.h>
#include <stdlib.h>
#include <string.h>

static const DTTR_RendererVtbl renderer;
static void cleanup(DTTR_BackendState *state);

typedef struct {
	Uint32 x;
	Uint32 y;
	Uint32 w;
	Uint32 h;
} graphics_present_rect;

// Converts the configured MSAA sample count into SDL's GPU enum value.
static SDL_GPUSampleCount msaa_sample_count_from_config(int value) {
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

// Converts SDL's GPU sample-count enum back to the integer used in logs and config.
static int msaa_sample_count_to_int(SDL_GPUSampleCount value) {
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

// Uses the requested MSAA count only when both swapchain and depth formats support it.
static SDL_GPUSampleCount select_msaa_sample_count(DTTR_BackendState *state) {
	const SDL_GPUSampleCount requested = msaa_sample_count_from_config(
		dttr_config.msaa_samples
	);
	if (requested == SDL_GPU_SAMPLECOUNT_1) {
		return SDL_GPU_SAMPLECOUNT_1;
	}

	const SDL_GPUTextureFormat swapchain_fmt = SDL_GetGPUSwapchainTextureFormat(
		state->device,
		state->window
	);
	const bool color_supported = SDL_GPUTextureSupportsSampleCount(
		state->device,
		swapchain_fmt,
		requested
	);
	const bool depth_supported = SDL_GPUTextureSupportsSampleCount(
		state->device,
		SDL_GPU_TEXTUREFORMAT_D32_FLOAT,
		requested
	);

	if (color_supported && depth_supported) {
		return requested;
	}

	DTTR_LOG_WARN(
		"Requested MSAA x%d is unsupported on this device/format. "
		"Falling back to x1.",
		msaa_sample_count_to_int(requested)
	);
	return SDL_GPU_SAMPLECOUNT_1;
}

// Releases the SDL GPU device and clears the backend pointer after ownership ends.
static void destroy_device(DTTR_BackendState *state) {
	if (!state->device) {
		return;
	}

	SDL_DestroyGPUDevice(state->device);
	state->device = NULL;
}

// Unclaims the SDL window before destroying the GPU device bound to it.
static void release_window_device(DTTR_BackendState *state) {
	if (!state->device) {
		return;
	}

	SDL_ReleaseWindowFromGPUDevice(state->device, state->window);
	destroy_device(state);
}

// Attempts one SDL GPU driver and only keeps it after the game window can be claimed.
static bool try_create_device_for_driver(
	DTTR_BackendState *state,
	const SDL_GPUShaderFormat requested_formats,
	const char *driver
) {
	state->device = SDL_CreateGPUDevice(requested_formats, false, driver);

	if (!state->device) {
		DTTR_LOG_WARN(
			"Failed to create SDL GPU device for driver '%s' "
			"(requested_formats=0x%x): %s",
			driver ? driver : "default",
			(unsigned int)requested_formats,
			SDL_GetError()
		);
		return false;
	}

	if (!SDL_ClaimWindowForGPUDevice(state->device, state->window)) {
		DTTR_LOG_WARN(
			"Failed to claim window for SDL GPU driver '%s': %s",
			driver ? driver : "default",
			SDL_GetError()
		);
		destroy_device(state);
		return false;
	}

	const bool immediate_ok = SDL_WindowSupportsGPUPresentMode(
		state->device,
		state->window,
		SDL_GPU_PRESENTMODE_IMMEDIATE
	);
	if (!immediate_ok) {
		DTTR_LOG_WARN(
			"IMMEDIATE present mode unsupported for '%s', falling back to VSYNC",
			driver ? driver : "default"
		);
	}

	if (!SDL_SetGPUSwapchainParameters(
			state->device,
			state->window,
			SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
			immediate_ok ? SDL_GPU_PRESENTMODE_IMMEDIATE : SDL_GPU_PRESENTMODE_VSYNC
		)) {
		DTTR_LOG_ERROR("Failed to set swap chain parameters: %s", SDL_GetError());
	}

	const SDL_GPUShaderFormat available_formats = SDL_GetGPUShaderFormats(state->device);
	const char *active_driver = SDL_GetGPUDeviceDriver(state->device);
	state->shader_format = dttr_graphics_select_shader_format_for_driver(
		active_driver,
		available_formats
	);

	if (state->shader_format != SDL_GPU_SHADERFORMAT_INVALID) {
		return true;
	}

	DTTR_LOG_WARN(
		"SDL GPU driver '%s' does not support required shader format. Available "
		"mask=0x%x",
		active_driver ? active_driver : "unknown",
		(unsigned int)available_formats
	);
	release_window_device(state);
	return false;
}

// Maps a configured graphics API to the SDL GPU driver name requested at device creation.
static const char *graphics_api_driver_name(DTTR_GraphicsApi api) {
	switch (api) {
	case DTTR_GRAPHICS_API_VULKAN:
		return DTTR_DRIVER_VULKAN;
	case DTTR_GRAPHICS_API_DIRECT3D12:
		return DTTR_DRIVER_DIRECT3D12;
	default:
		return NULL;
	}
}

// Creates an SDL GPU device using the configured driver or the supported fallback order.
static bool create_device(DTTR_BackendState *state) {
	const SDL_GPUShaderFormat requested_formats = dttr_graphics_requested_shader_formats();
	const char *const requested_driver = graphics_api_driver_name(
		dttr_config.graphics_api
	);

	if (requested_driver) {
		if (try_create_device_for_driver(state, requested_formats, requested_driver)) {
			return true;
		}

		DTTR_LOG_ERROR(
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
		if (try_create_device_for_driver(state, requested_formats, driver_candidates[i])) {
			return true;
		}
	}

	DTTR_LOG_ERROR("GPU device creation failed for all supported APIs (d3d12/vulkan)");
	return false;
}

// Initializes SDL GPU backend state after device creation succeeds.
bool dttr_graphics_sdl3gpu_init(DTTR_BackendState *state) {
	if (!create_device(state)) {
		return false;
	}

	sdl3_gpu_backend_data *bd = calloc(1, sizeof(sdl3_gpu_backend_data));
	if (!bd) {
		release_window_device(state);
		return false;
	}

	state->backend_data = bd;
	state->backend_type = DTTR_BACKEND_SDL_GPU;
	state->renderer = &renderer;

	state->msaa_sample_count = select_msaa_sample_count(state);
	DTTR_LOG_INFO(
		"MSAA requested: x%d, effective: x%d",
		dttr_config.msaa_samples,
		msaa_sample_count_to_int(state->msaa_sample_count)
	);

	DTTR_LOG_INFO(
		"SDL GPU initialized with %s (shaders: %s)",
		SDL_GetGPUDeviceDriver(state->device),
		dttr_graphics_shader_format_name(state->shader_format)
	);

	if (!dttr_graphics_sdl3gpu_create_pipelines()
		|| !dttr_graphics_sdl3gpu_create_resources()) {
		DTTR_LOG_ERROR("Failed to create GPU resources");
		cleanup(state);
		state->renderer = NULL;
		return false;
	}

	return true;
}

// Releases all SDL GPU resources owned by the backend before the window/device go away.
static void cleanup(DTTR_BackendState *state) {
	if (!state->device) {
		return;
	}

	for (int i = 0; i < DTTR_SAMPLER_COUNT; i++) {
		if (state->samplers[i]) {
			SDL_ReleaseGPUSampler(state->device, state->samplers[i]);
		}
	}

	if (state->dummy_texture) {
		SDL_ReleaseGPUTexture(state->device, state->dummy_texture);
	}

	if (state->depth_texture) {
		SDL_ReleaseGPUTexture(state->device, state->depth_texture);
	}

	if (state->msaa_render_target) {
		SDL_ReleaseGPUTexture(state->device, state->msaa_render_target);
	}

	if (state->render_target) {
		SDL_ReleaseGPUTexture(state->device, state->render_target);
	}

	if (state->transfer_buffer) {
		SDL_ReleaseGPUTransferBuffer(state->device, state->transfer_buffer);
	}

	if (state->vertex_buffer) {
		SDL_ReleaseGPUBuffer(state->device, state->vertex_buffer);
	}

	for (int i = 0; i < DTTR_UPLOAD_POOL_SIZE; i++) {
		DTTR_UploadPoolSlot *slot = &state->upload_pool[i];

		if (slot->transfer_buffer) {
			SDL_ReleaseGPUTransferBuffer(state->device, slot->transfer_buffer);
			slot->transfer_buffer = NULL;
		}

		slot->capacity = 0;
		slot->in_use = false;
	}

	for (int i = 0; i < DTTR_PIPELINE_COUNT; i++) {
		if (state->pipelines[i]) {
			SDL_ReleaseGPUGraphicsPipeline(state->device, state->pipelines[i]);
		}
	}

	if (state->buf2tex_pipeline) {
		SDL_ReleaseGPUComputePipeline(state->device, state->buf2tex_pipeline);
	}

	release_window_device(state);
	free(state->backend_data);
	state->backend_data = NULL;
}

typedef struct {
	SDL_GPUTexture *tex;
	uint32_t bytes;
	bool generate_mips;
	bool uploaded;
} graphics_pending_upload;

typedef struct {
	uint32_t draw_count;
	uint32_t clear_count;
	uint32_t pipeline_bind_count;
	uint32_t sampler_bind_count;
} graphics_replay_stats;

typedef struct {
	int last_pipeline_idx;
	SDL_GPUTexture *last_texture;
	SDL_GPUSampler *last_sampler;
} graphics_replay_state;

// Treats MSAA as active only after the sample count and render target are both ready.
static bool msaa_enabled(const DTTR_BackendState *state) {
	return state->msaa_sample_count != SDL_GPU_SAMPLECOUNT_1
		   && state->msaa_render_target != NULL;
}

static SDL_GPUTransferBuffer *create_upload_buffer(
	DTTR_BackendState *state,
	uint32_t bytes
) {
	const SDL_GPUTransferBufferCreateInfo info = {
		.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
		.size = bytes,
	};

	return SDL_CreateGPUTransferBuffer(state->device, &info);
}

// Marks a transient upload buffer slot as reusable after texture upload completes.
static void release_upload_pool_slot(DTTR_BackendState *state, int pool_slot) {
	if (!state || pool_slot < 0 || pool_slot >= DTTR_UPLOAD_POOL_SIZE) {
		return;
	}

	state->upload_pool[pool_slot].in_use = false;
}

// Reuses or grows a transfer-buffer slot large enough for the pending texture upload.
static int acquire_upload_pool_slot(DTTR_BackendState *state, uint32_t bytes) {
	if (!state || !state->device || bytes == 0) {
		return -1;
	}

	int free_slot = -1;
	int grow_slot = -1;

	for (int i = 0; i < DTTR_UPLOAD_POOL_SIZE; i++) {
		DTTR_UploadPoolSlot *slot = &state->upload_pool[i];

		if (slot->in_use) {
			continue;
		}

		if (slot->transfer_buffer && slot->capacity >= bytes) {
			slot->in_use = true;
			return i;
		}

		if (free_slot < 0) {
			free_slot = i;
		}

		if (slot->transfer_buffer) {
			grow_slot = i;
		}
	}

	const int slot_index = (grow_slot >= 0) ? grow_slot : free_slot;

	if (slot_index < 0) {
		return -1;
	}

	DTTR_UploadPoolSlot *slot = &state->upload_pool[slot_index];

	if (slot->transfer_buffer) {
		SDL_ReleaseGPUTransferBuffer(state->device, slot->transfer_buffer);
		slot->transfer_buffer = NULL;
	}

	slot->transfer_buffer = create_upload_buffer(state, bytes);

	if (!slot->transfer_buffer) {
		slot->capacity = 0;
		slot->in_use = false;
		return -1;
	}

	slot->capacity = bytes;
	slot->in_use = true;
	return slot_index;
}

// Binds the shared quad vertex buffer used by replayed DirectDraw-style draw calls.
static void bind_frame_vertex_buffer(
	const DTTR_BackendState *state,
	SDL_GPURenderPass *render_pass
) {
	if (!render_pass) {
		return;
	}

	const SDL_GPUBufferBinding vbuf_binding = {
		.buffer = state->vertex_buffer,
	};

	SDL_BindGPUVertexBuffers(render_pass, 0, &vbuf_binding, 1);
}

// Closes the current SDL GPU render pass before commands switch to copy or compute work.
static void end_render_pass_if_active(DTTR_BackendState *state) {
	if (!state->render_pass) {
		return;
	}

	SDL_EndGPURenderPass(state->render_pass);
	state->render_pass = NULL;
}

// Frees textures queued from non-render threads once the GPU thread reaches a safe point.
static void release_deferred_texture_destroys(DTTR_BackendState *state) {
	sdl3_gpu_backend_data *bd = (sdl3_gpu_backend_data *)state->backend_data;
	if (!bd) {
		return;
	}

	SDL_LockMutex(state->texture_mutex);

	for (int i = 0; i < bd->deferred_destroy_count; i++) {
		SDL_ReleaseGPUTexture(state->device, bd->deferred_destroys[i]);
	}

	bd->deferred_destroy_count = 0;
	SDL_UnlockMutex(state->texture_mutex);
}

// Queues a staged texture for GPU-thread destruction instead of freeing it from callers.
static void defer_texture_destroy(DTTR_BackendState *state, int texture_index) {
	sdl3_gpu_backend_data *bd = (sdl3_gpu_backend_data *)state->backend_data;
	if (!bd || texture_index < 0 || texture_index >= DTTR_MAX_STAGED_TEXTURES) {
		return;
	}

	DTTR_StagedTexture *st = &state->staged_textures[texture_index];
	if (st->gpu_tex && state->device) {
		bd->deferred_destroys[bd->deferred_destroy_count++] = st->gpu_tex;
	}
}

// Creates the GPU texture backing a staged DirectDraw surface the first time it is used.
static bool ensure_staged_texture(DTTR_BackendState *state, DTTR_StagedTexture *st) {
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

// Copies one detached pixel buffer into a GPU texture using either the upload pool or a
// temporary transfer buffer.
static bool upload_texture_data(
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

	if (dttr_config.texture_upload_sync) {
		pool_slot = acquire_upload_pool_slot(state, bytes);
	}

	if (pool_slot >= 0) {
		tbuf = state->upload_pool[pool_slot].transfer_buffer;
		from_pool = true;
	} else {
		tbuf = create_upload_buffer(state, bytes);

		if (!tbuf) {
			free(pixels);
			return false;
		}
	}

	void *mapped = SDL_MapGPUTransferBuffer(state->device, tbuf, false);

	if (!mapped) {
		if (from_pool) {
			release_upload_pool_slot(state, pool_slot);
		} else {
			SDL_ReleaseGPUTransferBuffer(state->device, tbuf);
		}

		free(pixels);
		return false;
	}

	memcpy(mapped, pixels, bytes);
	SDL_UnmapGPUTransferBuffer(state->device, tbuf);

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
		release_upload_pool_slot(state, pool_slot);
	} else {
		SDL_ReleaseGPUTransferBuffer(state->device, tbuf);
	}

	free(pixels);
	return true;
}

// Detaches queued texture uploads under the mutex, uploads them, and keeps failed
// entries queued for retry.
static int collect_and_upload_pending(
	DTTR_BackendState *state,
	SDL_GPUCopyPass *copy,
	graphics_pending_upload *pending_uploads,
	int max_uploads
) {
	if (!state->texture_mutex) {
		return 0;
	}

	int pending_count = 0;
	SDL_LockMutex(state->texture_mutex);
	const size_t queued_count = kv_size(state->pending_upload_indices);
	size_t deferred_write = 0;

	typedef struct {
		SDL_GPUTexture *tex;
		void *pixels;
		int width;
		int height;
		uint32_t bytes;
		bool generate_mips;
	} detached_upload;

	detached_upload detached[DTTR_MAX_STAGED_TEXTURES];

	for (size_t q = 0; q < queued_count; q++) {
		const int idx = kv_A(state->pending_upload_indices, q);

		if (idx < 0 || idx >= state->staged_texture_count) {
			continue;
		}

		DTTR_StagedTexture *st = &state->staged_textures[idx];

		if (!st->pixels) {
			st->pending_upload = false;
			continue;
		}

		if (max_uploads > 0 && pending_count >= max_uploads) {
			kv_A(state->pending_upload_indices, deferred_write++) = idx;
			continue;
		}

		st->pending_upload = false;

		if (!ensure_staged_texture(state, st)) {
			free(st->pixels);
			st->pixels = NULL;
			continue;
		}

		const uint32_t bytes = (uint32_t)(st->width * st->height * 4);

		if (bytes == 0) {
			free(st->pixels);
			st->pixels = NULL;
			continue;
		}

		if (pending_count >= DTTR_MAX_STAGED_TEXTURES) {
			free(st->pixels);
			st->pixels = NULL;
			continue;
		}

		detached[pending_count] = (detached_upload){
			.tex = st->gpu_tex,
			.pixels = st->pixels,
			.width = st->width,
			.height = st->height,
			.bytes = bytes,
			.generate_mips = dttr_config.generate_texture_mipmaps,
		};

		st->pixels = NULL;
		pending_count++;
	}

	state->pending_upload_indices.n = deferred_write;
	SDL_UnlockMutex(state->texture_mutex);

	for (int i = 0; i < pending_count; i++) {
		const bool ok = upload_texture_data(
			state,
			copy,
			detached[i].tex,
			detached[i].pixels,
			detached[i].width,
			detached[i].height,
			detached[i].bytes
		);
		pending_uploads[i] = (graphics_pending_upload){
			.tex = detached[i].tex,
			.bytes = detached[i].bytes,
			.generate_mips = detached[i].generate_mips,
			.uploaded = ok,
		};
	}

	return pending_count;
}

// Generates mipmaps for uploaded textures that requested them and records upload stats.
static void generate_pending_mipmaps(
	DTTR_BackendState *state,
	SDL_GPUCommandBuffer *cmd,
	const graphics_pending_upload *pending,
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
			state->perf_mips_generated_accum++;
		} else {
			state->perf_mips_skipped_accum++;
		}

		if (uploaded_texture_count) {
			(*uploaded_texture_count)++;
		}

		if (uploaded_bytes) {
			(*uploaded_bytes) += pending[p].bytes;
		}
	}
}

// Runs the pending texture upload copy pass and updates per-frame upload counters.
static void upload_pending_textures(DTTR_BackendState *state, SDL_GPUCommandBuffer *cmd) {
	if (!cmd) {
		return;
	}

	graphics_pending_upload pending[DTTR_MAX_STAGED_TEXTURES] = {0};

	SDL_GPUCopyPass *copy = SDL_BeginGPUCopyPass(cmd);
	const int pending_count = collect_and_upload_pending(state, copy, pending, 0);

	if (copy) {
		SDL_EndGPUCopyPass(copy);
	}

	if (pending_count == 0) {
		return;
	}

	uint32_t uploaded_texture_count = 0;
	uint64_t uploaded_bytes = 0;
	generate_pending_mipmaps(
		state,
		cmd,
		pending,
		pending_count,
		&uploaded_texture_count,
		&uploaded_bytes
	);

	state->perf_upload_textures_accum += uploaded_texture_count;
	state->perf_upload_bytes_accum += uploaded_bytes;
}

// Fits the game render target into the swapchain according to stretch, fit, and integer
// scaling settings.
static graphics_present_rect compute_present_rect(
	Uint32 dst_w,
	Uint32 dst_h,
	int src_w,
	int src_h,
	bool stretch,
	bool integer_fit
) {
	graphics_present_rect rect = {
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

// Notifies mods after SDL GPU backend draw/blit work is queued and before submit.
static void mod_before_present(
	DTTR_BackendState *state,
	const graphics_present_rect *present
) {
	dttr_graphics_mod_before_present(
		state,
		present->x,
		present->y,
		present->w,
		present->h,
		false,
		true
	);
}

// Notifies mods after SDL GPU presentation using the same game viewport payload.
static void mod_after_present(
	DTTR_BackendState *state,
	const graphics_present_rect *present,
	bool overlay_rendered
) {
	dttr_graphics_mod_after_present(
		state,
		present->x,
		present->y,
		present->w,
		present->h,
		false,
		overlay_rendered
	);
}

// Restores full-target viewport and scissor after custom game viewport changes.
static void set_default_viewport(const DTTR_BackendState *state) {
	if (!state->render_pass) {
		return;
	}

	const SDL_GPUViewport viewport = {
		.x = 0.0f,
		.y = 0.0f,
		.w = (float)state->width,
		.h = (float)state->height,
		.min_depth = 0.0f,
		.max_depth = 1.0f,
	};

	SDL_SetGPUViewport(state->render_pass, &viewport);

	const SDL_Rect scissor = {
		.x = 0,
		.y = 0,
		.w = state->width,
		.h = state->height,
	};

	SDL_SetGPUScissor(state->render_pass, &scissor);
}

// Opens a draw render pass lazily so queued clear and draw records can share command
// buffers.
static bool begin_draw_pass_if_needed(DTTR_BackendState *state) {
	if (state->render_pass) {
		return false;
	}

	const bool use_msaa = msaa_enabled(state);
	const SDL_GPUColorTargetInfo color_target = {
		.texture = use_msaa ? state->msaa_render_target : state->render_target,
		.load_op = SDL_GPU_LOADOP_LOAD,
		.store_op = use_msaa ? SDL_GPU_STOREOP_RESOLVE_AND_STORE : SDL_GPU_STOREOP_STORE,
		.resolve_texture = use_msaa ? state->render_target : NULL,
		.resolve_mip_level = 0,
		.resolve_layer = 0,
	};

	const SDL_GPUDepthStencilTargetInfo depth_target = {
		.texture = state->depth_texture,
		.load_op = SDL_GPU_LOADOP_LOAD,
		.store_op = SDL_GPU_STOREOP_DONT_CARE,
	};

	state->render_pass = SDL_BeginGPURenderPass(
		state->cmd,
		&color_target,
		1,
		&depth_target
	);

	if (!state->render_pass) {
		DTTR_LOG_WARN("Failed to begin render pass");
		return false;
	}

	bind_frame_vertex_buffer(state, state->render_pass);
	set_default_viewport(state);
	return true;
}

// Clears cached pipeline and sampler bindings between replay passes.
static void reset_replay_state(graphics_replay_state *replay_state) {
	if (!replay_state) {
		return;
	}

	replay_state->last_pipeline_idx = -1;
	replay_state->last_texture = NULL;
	replay_state->last_sampler = NULL;
}

// Starts a render pass configured for the clear flags recorded by the DirectDraw replay
// layer.
static void begin_clear_pass(
	DTTR_BackendState *state,
	const DTTR_BatchRecord *rec,
	graphics_replay_state *replay_state
) {
	end_render_pass_if_active(state);

	const bool use_msaa = msaa_enabled(state);
	const SDL_GPUColorTargetInfo color_target = {
		.texture = use_msaa ? state->msaa_render_target : state->render_target,
		.clear_color = rec->clear.color,
		.load_op = (rec->clear.flags & DTTR_CLEAR_COLOR) ? SDL_GPU_LOADOP_CLEAR
														 : SDL_GPU_LOADOP_LOAD,
		.store_op = use_msaa ? SDL_GPU_STOREOP_RESOLVE_AND_STORE : SDL_GPU_STOREOP_STORE,
		.resolve_texture = use_msaa ? state->render_target : NULL,
		.resolve_mip_level = 0,
		.resolve_layer = 0,
	};

	const SDL_GPUDepthStencilTargetInfo depth_target = {
		.texture = state->depth_texture,
		.clear_depth = rec->clear.depth,
		.load_op = (rec->clear.flags & DTTR_CLEAR_DEPTH) ? SDL_GPU_LOADOP_CLEAR
														 : SDL_GPU_LOADOP_LOAD,
		.store_op = SDL_GPU_STOREOP_STORE,
	};

	state->render_pass = SDL_BeginGPURenderPass(
		state->cmd,
		&color_target,
		1,
		&depth_target
	);
	bind_frame_vertex_buffer(state, state->render_pass);
	set_default_viewport(state);
	reset_replay_state(replay_state);
}

// Replays one recorded draw call while avoiding redundant pipeline and sampler binds.
static void draw_batch_record(
	DTTR_BackendState *state,
	const DTTR_BatchRecord *rec,
	graphics_replay_state *replay_state,
	graphics_replay_stats *replay_stats
) {
	const bool began_pass = begin_draw_pass_if_needed(state);

	if (began_pass) {
		reset_replay_state(replay_state);
	}

	if (!state->render_pass) {
		return;
	}

	const int pidx = DTTR_PIPELINE_INDEX(
		rec->draw.blend_mode,
		rec->draw.depth_test,
		rec->draw.depth_write
	);

	if (!replay_state || replay_state->last_pipeline_idx != pidx) {
		SDL_BindGPUGraphicsPipeline(state->render_pass, state->pipelines[pidx]);

		if (replay_state) {
			replay_state->last_pipeline_idx = pidx;
		}

		if (replay_stats) {
			replay_stats->pipeline_bind_count++;
		}
	}

	SDL_PushGPUVertexUniformData(
		state->cmd,
		0,
		&rec->draw.uniforms,
		sizeof(DTTR_Uniforms)
	);
	SDL_PushGPUFragmentUniformData(
		state->cmd,
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

		SDL_BindGPUFragmentSamplers(state->render_pass, 0, &tex_binding, 1);

		if (replay_state) {
			replay_state->last_texture = rec->draw.texture;
			replay_state->last_sampler = rec->draw.sampler;
		}

		if (replay_stats) {
			replay_stats->sampler_bind_count++;
		}
	}

	SDL_DrawGPUPrimitives(
		state->render_pass,
		rec->draw.vertex_count,
		1,
		rec->draw.first_vertex,
		0
	);

	if (replay_stats) {
		replay_stats->draw_count++;
	}
}

// Replays queued clear and draw records into SDL GPU commands for the current frame.
static graphics_replay_stats replay_batch_records(DTTR_BackendState *state) {
	graphics_replay_stats replay_stats = {0};

	if (kv_size(state->batch_records) == 0) {
		return replay_stats;
	}

	graphics_replay_state replay_state = {0};
	reset_replay_state(&replay_state);
	state->render_pass = NULL;

	for (size_t i = 0; i < kv_size(state->batch_records); i++) {
		const DTTR_BatchRecord *rec = &kv_A(state->batch_records, i);

		if (rec->type == DTTR_BATCH_CLEAR) {
			begin_clear_pass(state, rec, &replay_state);
			replay_stats.clear_count++;
			continue;
		}

		draw_batch_record(state, rec, &replay_state, &replay_stats);
	}

	end_render_pass_if_active(state);
	return replay_stats;
}

// Acquires the frame command buffer and swapchain texture before uploads and replay work.
static void begin_frame(DTTR_BackendState *state) {
	if (!state->device || !state->window || !dttr_graphics_is_gpu_thread()) {
		return;
	}

	state->frame_index++;

	state->cmd = SDL_AcquireGPUCommandBuffer(state->device);

	if (!state->cmd) {
		DTTR_LOG_ERROR("Failed to acquire GPU command buffer");
		return;
	}

	release_deferred_texture_destroys(state);

	if (!SDL_WaitAndAcquireGPUSwapchainTexture(
			state->cmd,
			state->window,
			&state->swapchain_tex,
			&state->swapchain_width,
			&state->swapchain_height
		)) {
		DTTR_LOG_WARN("Failed to acquire swapchain texture: %s", SDL_GetError());
		SDL_CancelGPUCommandBuffer(state->cmd);
		state->cmd = NULL;
		return;
	}

	// No swapchain image available, skip this frame.
	if (!state->swapchain_tex) {
		SDL_CancelGPUCommandBuffer(state->cmd);
		state->cmd = NULL;
		return;
	}

	// Textures must be uploaded after swapchain acquire for Vulkan.
	upload_pending_textures(state, state->cmd);

	state->batch_records.n = 0;
	state->vertex_offset = 0;
	state->transfer_mapped = SDL_MapGPUTransferBuffer(
		state->device,
		state->transfer_buffer,
		true
	);

	if (!state->transfer_mapped) {
		DTTR_LOG_WARN("BeginFrame: MapGPUTransferBuffer failed");
	}

	state->frame_active = true;
	dttr_graphics_mod_frame_begin(state);
}

// Uploads vertices, replays draw records, blits to the swapchain, and submits the frame.
static void end_frame(DTTR_BackendState *state) {
	state->frame_active = false;

	dttr_graphics_mod_before_game_frame(state);

	if (state->transfer_mapped) {
		SDL_UnmapGPUTransferBuffer(state->device, state->transfer_buffer);
		state->transfer_mapped = NULL;
	}

	if (!state->cmd) {
		return;
	}

	if (state->vertex_offset > 0) {
		SDL_GPUCopyPass *copy = SDL_BeginGPUCopyPass(state->cmd);

		if (copy) {
			const SDL_GPUTransferBufferLocation src = {
				.transfer_buffer = state->transfer_buffer,
			};

			const SDL_GPUBufferRegion dst = {
				.buffer = state->vertex_buffer,
				.size = state->vertex_offset * DTTR_VERTEX_SIZE,
			};

			SDL_UploadToGPUBuffer(copy, &src, &dst, true);
			SDL_EndGPUCopyPass(copy);
		}
	}

	const graphics_replay_stats replay_stats = replay_batch_records(state);
	state->perf_draws_accum += replay_stats.draw_count;
	state->perf_clears_accum += replay_stats.clear_count;
	state->perf_pipeline_binds_accum += replay_stats.pipeline_bind_count;
	state->perf_sampler_binds_accum += replay_stats.sampler_bind_count;

#ifdef DTTR_MODS_ENABLED
	dttr_imgui_render_game_sdl3gpu(
		state->cmd,
		state->render_target,
		(uint32_t)state->width,
		(uint32_t)state->height
	);
#endif
	dttr_graphics_mod_after_game_frame(state);

	graphics_present_rect present = {
		.x = 0,
		.y = 0,
		.w = (Uint32)state->width,
		.h = (Uint32)state->height,
	};

	bool overlay_rendered = false;
	if (state->swapchain_tex) {
		const Uint32 swap_w = (state->swapchain_width > 0) ? state->swapchain_width
														   : (Uint32)state->width;
		const Uint32 swap_h = (state->swapchain_height > 0) ? state->swapchain_height
															: (Uint32)state->height;
		const bool
			is_internal_method = (dttr_config.scaling_method == DTTR_SCALING_METHOD_LOGICAL);
		present = compute_present_rect(
			swap_w,
			swap_h,
			state->width,
			state->height,
			dttr_config.scaling_fit == DTTR_SCALING_MODE_STRETCH,
			(!is_internal_method)
				&& (dttr_config.scaling_fit == DTTR_SCALING_MODE_INTEGER)
		);
		overlay_rendered = true;

		const SDL_GPUBlitInfo blit = {
			.source =
				{
					.texture = state->render_target,
					.w = state->width,
					.h = state->height,
				},
				.destination =
					{
						.texture = state->swapchain_tex,
						.x = present.x,
						.y = present.y,
						.w = present.w,
						.h = present.h,
					},
				.clear_color = {0.0f, 0.0f, 0.0f, 1.0f},
				.load_op = SDL_GPU_LOADOP_CLEAR,
			.filter = dttr_config.present_filter,
		};

		SDL_BlitGPUTexture(state->cmd, &blit);

#ifdef DTTR_MODS_ENABLED
		dttr_imgui_render_sdl3gpu(
			state->cmd,
			state->swapchain_tex,
			swap_w,
			swap_h,
			present.x,
			present.y,
			present.w,
			present.h
		);
#endif
		mod_before_present(state, &present);
	}

	SDL_SubmitGPUCommandBuffer(state->cmd);

	if (dttr_config.texture_upload_sync) {
		SDL_WaitForGPUIdle(state->device);
	}

	mod_after_present(state, &present, overlay_rendered);
	dttr_graphics_mod_frame_end(state);
	state->cmd = NULL;
}

// Recreates the movie texture only when decoded frame dimensions change.
static bool ensure_video_texture(DTTR_BackendState *state, int width, int height) {
	if (state->video_texture && state->video_width == width
		&& state->video_height == height) {
		return true;
	}

	if (state->video_texture) {
		SDL_ReleaseGPUTexture(state->device, state->video_texture);
		state->video_texture = NULL;
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

	state->video_texture = SDL_CreateGPUTexture(state->device, &tex_info);

	if (!state->video_texture) {
		return false;
	}

	state->video_width = width;
	state->video_height = height;
	return true;
}

// Uploads a BGRA movie frame and presents it directly while normal frame rendering is
// idle.
static bool present_video_frame_bgra(
	DTTR_BackendState *state,
	const uint8_t *pixels,
	int width,
	int height,
	int stride
) {
	if (!state->device || !state->window || !dttr_graphics_is_gpu_thread()) {
		return false;
	}

	if (state->frame_active) {
		// Video presentation assumes sole ownership of the command buffer.
		return false;
	}

	if (!ensure_video_texture(state, width, height)) {
		return false;
	}

	const Uint32 upload_size = (Uint32)(stride * height);
	SDL_GPUTransferBuffer *tbuf = create_upload_buffer(state, upload_size);

	if (!tbuf) {
		return false;
	}

	void *mapped = SDL_MapGPUTransferBuffer(state->device, tbuf, false);

	if (!mapped) {
		SDL_ReleaseGPUTransferBuffer(state->device, tbuf);
		return false;
	}

	memcpy(mapped, pixels, upload_size);
	SDL_UnmapGPUTransferBuffer(state->device, tbuf);

	SDL_GPUCommandBuffer *cmd = SDL_AcquireGPUCommandBuffer(state->device);

	if (!cmd) {
		SDL_ReleaseGPUTransferBuffer(state->device, tbuf);
		return false;
	}

	SDL_GPUTexture *swapchain_tex = NULL;
	Uint32 swapchain_w = 0;
	Uint32 swapchain_h = 0;
	SDL_WaitAndAcquireGPUSwapchainTexture(
		cmd,
		state->window,
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
			.texture = state->video_texture,
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
		const graphics_present_rect present = compute_present_rect(
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
					.texture = state->video_texture,
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
			.filter = dttr_config.present_filter,
			.cycle = false,
		};

		SDL_BlitGPUTexture(cmd, &blit);
	}

	SDL_SubmitGPUCommandBuffer(cmd);
	SDL_ReleaseGPUTransferBuffer(state->device, tbuf);
	return true;
}

// Recreates SDL GPU render targets for the requested game-space resolution.
static bool resize(DTTR_BackendState *state, int width, int height) {
	return dttr_graphics_sdl3gpu_resize_render_textures(width, height);
}

// Converts SDL GPU driver identifiers into labels suitable for the window title.
static const char *driver_display_name(const char *driver) {
	if (strcmp(driver, DTTR_DRIVER_VULKAN) == 0) {
		return DRIVER_DISPLAY_VULKAN;
	}

	if (strcmp(driver, DTTR_DRIVER_DIRECT3D12) == 0) {
		return DRIVER_DISPLAY_DIRECT3D12;
	}

	return driver;
}

// Reports the SDL GPU driver label shown in the window title and mod context.
static const char *get_driver_name(const DTTR_BackendState *state) {
	return driver_display_name(SDL_GetGPUDeviceDriver(state->device));
}

static const DTTR_RendererVtbl renderer = {
	.begin_frame = begin_frame,
	.end_frame = end_frame,
	.present_video_frame_bgra = present_video_frame_bgra,
	.resize = resize,
	.cleanup = cleanup,
	.get_driver_name = get_driver_name,
	.defer_texture_destroy = defer_texture_destroy,
};
