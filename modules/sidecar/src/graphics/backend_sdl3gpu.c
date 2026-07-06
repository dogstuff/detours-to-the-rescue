#include "backend_sdl3gpu_private.h"
#include "graphics_private.h"

#include <dttr_log.h>

#include <dttr_config.h>

#define DRIVER_DISPLAY_VULKAN "Vulkan"
#define DRIVER_DISPLAY_DIRECT3D12 "Direct3D 12"

// D3D12 texture-copy offsets need 512-byte alignment; Vulkan/Metal are looser.
#define UPLOAD_POOL_OFFSET_ALIGN 512u

// Per-frame upload cap; overflow stays queued for the next frame.
#define UPLOAD_POOL_MAX_BYTES (256u * 1024u * 1024u)

// Pre-touched at init so load frames do not fault in fresh mapped pages.
#define UPLOAD_POOL_INITIAL_BYTES (16u * 1024u * 1024u)

#ifdef DTTR_MODS_ENABLED
#include "../mods/mods_private.h"
#include "imgui_overlay_private.h"
#endif

#include <math.h>
#include <stdlib.h>
#include <string.h>

static const DTTR_RendererVtbl renderer;
static void cleanup(DTTR_BackendState *state);
static void release_deferred_texture_destroys(DTTR_BackendState *state);
static SDL_GPUTransferBuffer *acquire_upload_pool(
	DTTR_BackendState *state,
	uint32_t bytes
);

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

static void destroy_device(DTTR_BackendState *state) {
	if (!state->device) {
		return;
	}

	SDL_DestroyGPUDevice(state->device);
	state->device = NULL;
}

static void release_window_device(DTTR_BackendState *state) {
	if (!state->device) {
		return;
	}

	SDL_ReleaseWindowFromGPUDevice(state->device, state->window);
	destroy_device(state);
}

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

static const char *graphics_api_driver_name(DTTR_GraphicsAPI api) {
	switch (api) {
	case DTTR_GRAPHICS_API_VULKAN:
		return DTTR_DRIVER_VULKAN;
	case DTTR_GRAPHICS_API_DIRECT3D12:
		return DTTR_DRIVER_DIRECT3D12;
	default:
		return NULL;
	}
}

static bool create_device(DTTR_BackendState *state) {
	const SDL_GPUShaderFormat requested_formats = dttr_graphics_requested_shader_formats();
	const char *requested_driver = graphics_api_driver_name(dttr_config.graphics_api);

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

	const char *driver_candidates[] = {
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

static void push_uniforms(SDL_GPUCommandBuffer *cmd, const DTTR_Uniforms *uniforms) {
	SDL_PushGPUVertexUniformData(cmd, 0, uniforms, sizeof(*uniforms));
	SDL_PushGPUFragmentUniformData(cmd, 0, uniforms, sizeof(*uniforms));
}

static void warm_uniform_pool(SDL_GPUCommandBuffer *cmd, int count) {
	const DTTR_Uniforms uniforms = {0};

	for (int i = 0; i < count; i++) {
		push_uniforms(cmd, &uniforms);
	}
}

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

	// Non-fatal: recreated on first upload if startup preallocation fails.
	if (!acquire_upload_pool(state, UPLOAD_POOL_INITIAL_BYTES)) {
		DTTR_LOG_WARN("Failed to preallocate upload pool");
	}

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

	// Pre-warmup SDL's uniform pool while page faults are cheap.
	SDL_GPUCommandBuffer *warm = SDL_AcquireGPUCommandBuffer(state->device);
	if (warm) {
		warm_uniform_pool(warm, 65536);

		SDL_SubmitGPUCommandBuffer(warm);
		SDL_WaitForGPUIdle(state->device);
	}

	return true;
}

static void cleanup(DTTR_BackendState *state) {
	if (!state->device) {
		return;
	}

	release_deferred_texture_destroys(state);

	sdl3_gpu_backend_data *bd = (sdl3_gpu_backend_data *)state->backend_data;
	if (bd) {
		for (int i = 0; i < bd->texture_recycle_count; i++) {
			SDL_ReleaseGPUTexture(state->device, bd->texture_recycle[i].tex);
		}

		for (int i = 0; i < bd->graveyard_texture_count; i++) {
			SDL_ReleaseGPUTexture(state->device, bd->graveyard_textures[i]);
		}

		for (int i = 0; i < bd->graveyard_tbuf_count; i++) {
			SDL_ReleaseGPUTransferBuffer(state->device, bd->graveyard_tbufs[i]);
		}
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

	if (state->video_texture) {
		SDL_ReleaseGPUTexture(state->device, state->video_texture);

		state->video_texture = NULL;
		state->video_width = 0;
		state->video_height = 0;
	}

	if (bd) {
		for (int i = 0; i < DTTR_VERTEX_RING_DEPTH; i++) {
			if (bd->vertex_ring_fence[i]) {
				SDL_ReleaseGPUFence(state->device, bd->vertex_ring_fence[i]);
			}

			if (bd->vertex_ring[i]) {
				SDL_ReleaseGPUTransferBuffer(state->device, bd->vertex_ring[i]);
			}

			if (bd->vertex_gpu_ring[i]) {
				if (state->vertex_buffer == bd->vertex_gpu_ring[i]) {
					state->vertex_buffer = NULL;
				}

				SDL_ReleaseGPUBuffer(state->device, bd->vertex_gpu_ring[i]);
			}
		}
	} else if (state->transfer_buffer) {
		SDL_ReleaseGPUTransferBuffer(state->device, state->transfer_buffer);
	}

	state->transfer_buffer = NULL;

	if (state->vertex_buffer) {
		SDL_ReleaseGPUBuffer(state->device, state->vertex_buffer);
	}

	for (int i = 0; i < DTTR_PIPELINE_COUNT; i++) {
		if (state->pipelines[i]) {
			SDL_ReleaseGPUGraphicsPipeline(state->device, state->pipelines[i]);
		}
	}

	if (bd && bd->upload_pool_fence) {
		SDL_ReleaseGPUFence(state->device, bd->upload_pool_fence);
	}

	if (bd && bd->upload_pool) {
		SDL_ReleaseGPUTransferBuffer(state->device, bd->upload_pool);
	}

	release_window_device(state);

	if (bd) {
		free(bd->texture_recycle);
		free(bd->graveyard_textures);
		free(bd->graveyard_tbufs);
		free(bd->deferred_destroys);
	}

	free(state->backend_data);
	state->backend_data = NULL;
}

typedef struct {
	SDL_GPUTexture *tex;
	uint32_t bytes;
	bool generate_mips;
	bool uploaded;
} graphics_pending_upload;

static uint32_t align_upload_offset(uint32_t offset) {
	return (offset + (UPLOAD_POOL_OFFSET_ALIGN - 1)) & ~(UPLOAD_POOL_OFFSET_ALIGN - 1);
}

// Shared upload buffer; grows geometrically when a burst exceeds capacity.
static SDL_GPUTransferBuffer *acquire_upload_pool(
	DTTR_BackendState *state,
	uint32_t bytes
) {
	sdl3_gpu_backend_data *bd = (sdl3_gpu_backend_data *)state->backend_data;
	if (!bd || bytes == 0) {
		return NULL;
	}

	if (bd->upload_pool && bd->upload_pool_capacity >= bytes) {
		return bd->upload_pool;
	}

	if (bd->upload_pool_fence) {
		SDL_WaitForGPUFences(state->device, true, &bd->upload_pool_fence, 1);
		SDL_ReleaseGPUFence(state->device, bd->upload_pool_fence);

		bd->upload_pool_fence = NULL;
	}

	uint32_t new_capacity = bd->upload_pool_capacity > 0 ? bd->upload_pool_capacity
														 : bytes;
	while (new_capacity < bytes) {
		if (new_capacity > UINT32_MAX / 2) {
			new_capacity = bytes;
			break;
		}

		new_capacity *= 2;
	}

	// Do not release mid-load: that re-arms SDL's Vulkan defrag.
	if (bd->upload_pool) {
		dttr_graphics_sdl3gpu_bury_transfer_buffer(state, bd->upload_pool);

		bd->upload_pool = NULL;
		bd->upload_pool_capacity = 0;
	}

	const SDL_GPUTransferBufferCreateInfo info = {
		.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
		.size = new_capacity,
	};

	bd->upload_pool = SDL_CreateGPUTransferBuffer(state->device, &info);
	if (!bd->upload_pool) {
		return NULL;
	}

	// Pre-touch now; first touches during Wine/MoltenVK load frames hitch badly.
	void *mapped = SDL_MapGPUTransferBuffer(state->device, bd->upload_pool, false);
	if (mapped) {
		memset(mapped, 0, new_capacity);
		SDL_UnmapGPUTransferBuffer(state->device, bd->upload_pool);
	}

	bd->upload_pool_capacity = new_capacity;
	return bd->upload_pool;
}

// Normally a no-op by the time the next upload burst arrives.
static void wait_upload_pool_fence(DTTR_BackendState *state) {
	sdl3_gpu_backend_data *bd = (sdl3_gpu_backend_data *)state->backend_data;
	if (!bd || !bd->upload_pool_fence) {
		return;
	}

	SDL_WaitForGPUFences(state->device, true, &bd->upload_pool_fence, 1);
	SDL_ReleaseGPUFence(state->device, bd->upload_pool_fence);

	bd->upload_pool_fence = NULL;
}

static void set_upload_pool_fence(DTTR_BackendState *state, SDL_GPUFence *fence) {
	sdl3_gpu_backend_data *bd = (sdl3_gpu_backend_data *)state->backend_data;
	if (!bd) {
		if (fence) {
			SDL_ReleaseGPUFence(state->device, fence);
		}

		return;
	}

	if (bd->upload_pool_fence) {
		SDL_ReleaseGPUFence(state->device, bd->upload_pool_fence);
	}

	bd->upload_pool_fence = fence;
}

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
	DTTR_Uniforms last_uniforms;
	bool uniforms_valid;
} graphics_replay_state;

static bool msaa_enabled(const DTTR_BackendState *state) {
	return state->msaa_sample_count != SDL_GPU_SAMPLECOUNT_1
		   && state->msaa_render_target != NULL;
}

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

static void end_render_pass_if_active(DTTR_BackendState *state) {
	if (!state->render_pass) {
		return;
	}

	SDL_EndGPURenderPass(state->render_pass);
	state->render_pass = NULL;
}

// Moves retired textures into the recycle pool at a GPU-thread safe point.
static void release_deferred_texture_destroys(DTTR_BackendState *state) {
	sdl3_gpu_backend_data *bd = (sdl3_gpu_backend_data *)state->backend_data;
	if (!bd) {
		return;
	}

	if (state->texture_mutex) {
		SDL_LockMutex(state->texture_mutex);
	}

	for (int i = 0; i < bd->deferred_destroy_count; i++) {
		const dttr_recycled_texture entry = bd->deferred_destroys[i];

		if (bd->texture_recycle_count >= DTTR_TEXTURE_RECYCLE_CAP) {
			DTTR_LOG_WARN(
				"Texture recycle pool full (%d); releasing %ux%u",
				bd->texture_recycle_count,
				entry.width,
				entry.height
			);
			SDL_ReleaseGPUTexture(state->device, entry.tex);
			continue;
		}

		if (bd->texture_recycle_count >= bd->texture_recycle_capacity) {
			int new_capacity = bd->texture_recycle_capacity > 0
								   ? bd->texture_recycle_capacity * 2
								   : 64;
			dttr_recycled_texture *grown = realloc(
				bd->texture_recycle,
				(size_t)new_capacity * sizeof(*grown)
			);
			if (!grown) {
				SDL_ReleaseGPUTexture(state->device, entry.tex);
				continue;
			}

			bd->texture_recycle = grown;
			bd->texture_recycle_capacity = new_capacity;
		}

		bd->texture_recycle[bd->texture_recycle_count++] = entry;
	}

	bd->deferred_destroy_count = 0;

	if (state->texture_mutex) {
		SDL_UnlockMutex(state->texture_mutex);
	}
}

void dttr_graphics_sdl3gpu_bury_texture(DTTR_BackendState *state, SDL_GPUTexture *tex) {
	sdl3_gpu_backend_data *bd = (sdl3_gpu_backend_data *)state->backend_data;
	if (!tex) {
		return;
	}

	if (!bd) {
		SDL_ReleaseGPUTexture(state->device, tex);
		return;
	}

	if (bd->graveyard_texture_count >= bd->graveyard_texture_capacity) {
		int new_capacity = bd->graveyard_texture_capacity > 0
							   ? bd->graveyard_texture_capacity * 2
							   : 16;
		SDL_GPUTexture **grown = realloc(
			bd->graveyard_textures,
			(size_t)new_capacity * sizeof(*grown)
		);
		if (!grown) {
			SDL_ReleaseGPUTexture(state->device, tex);
			return;
		}

		bd->graveyard_textures = grown;
		bd->graveyard_texture_capacity = new_capacity;
	}

	bd->graveyard_textures[bd->graveyard_texture_count++] = tex;
}

void dttr_graphics_sdl3gpu_bury_transfer_buffer(
	DTTR_BackendState *state,
	SDL_GPUTransferBuffer *tbuf
) {
	sdl3_gpu_backend_data *bd = (sdl3_gpu_backend_data *)state->backend_data;
	if (!tbuf) {
		return;
	}

	if (!bd) {
		SDL_ReleaseGPUTransferBuffer(state->device, tbuf);
		return;
	}

	if (bd->graveyard_tbuf_count >= bd->graveyard_tbuf_capacity) {
		int new_capacity = bd->graveyard_tbuf_capacity > 0
							   ? bd->graveyard_tbuf_capacity * 2
							   : 8;
		SDL_GPUTransferBuffer **grown = realloc(
			bd->graveyard_tbufs,
			(size_t)new_capacity * sizeof(*grown)
		);
		if (!grown) {
			SDL_ReleaseGPUTransferBuffer(state->device, tbuf);
			return;
		}

		bd->graveyard_tbufs = grown;
		bd->graveyard_tbuf_capacity = new_capacity;
	}

	bd->graveyard_tbufs[bd->graveyard_tbuf_count++] = tbuf;
}

SDL_GPUTexture *dttr_graphics_sdl3gpu_take_recycled_texture(
	DTTR_BackendState *state,
	Uint32 width,
	Uint32 height
) {
	sdl3_gpu_backend_data *bd = (sdl3_gpu_backend_data *)state->backend_data;
	if (!bd || state->backend_type != DTTR_BACKEND_SDL_GPU) {
		return NULL;
	}

	for (int i = 0; i < bd->texture_recycle_count; i++) {
		if (bd->texture_recycle[i].width != width
			|| bd->texture_recycle[i].height != height) {
			continue;
		}

		SDL_GPUTexture *tex = bd->texture_recycle[i].tex;
		bd->texture_recycle[i] = bd->texture_recycle[--bd->texture_recycle_count];
		return tex;
	}

	return NULL;
}

static bool queue_deferred_texture_destroy(
	sdl3_gpu_backend_data *bd,
	SDL_GPUTexture *texture,
	Uint32 width,
	Uint32 height
) {
	if (!bd || !texture) {
		return true;
	}

	if (bd->deferred_destroy_count >= bd->deferred_destroy_capacity) {
		int new_capacity = bd->deferred_destroy_capacity > 0
							   ? bd->deferred_destroy_capacity * 2
							   : 64;
		if (new_capacity <= bd->deferred_destroy_capacity) {
			return false;
		}

		dttr_recycled_texture *new_destroys = realloc(
			bd->deferred_destroys,
			(size_t)new_capacity * sizeof(*new_destroys)
		);
		if (!new_destroys) {
			return false;
		}

		bd->deferred_destroys = new_destroys;
		bd->deferred_destroy_capacity = new_capacity;
	}

	bd->deferred_destroys[bd->deferred_destroy_count++] = (dttr_recycled_texture){
		.tex = texture,
		.width = width,
		.height = height,
	};
	return true;
}

// Queues a staged texture for GPU-thread destruction instead of freeing it from callers.
static bool defer_texture_destroy(DTTR_BackendState *state, int texture_index) {
	sdl3_gpu_backend_data *bd = (sdl3_gpu_backend_data *)state->backend_data;
	if (!bd || texture_index < 0 || texture_index >= DTTR_MAX_STAGED_TEXTURES) {
		return false;
	}

	DTTR_StagedTexture *st = &state->staged_textures[texture_index];
	if (!st->gpu_tex || !state->device) {
		return true;
	}

	if (!queue_deferred_texture_destroy(
			bd,
			st->gpu_tex,
			(Uint32)st->width,
			(Uint32)st->height
		)) {
		DTTR_LOG_WARN("Failed to queue SDL GPU texture destroy");
		return false;
	}

	return true;
}

static bool ensure_sdl_staged_texture(DTTR_BackendState *state, DTTR_StagedTexture *st) {
	if (!st->gpu_tex) {
		st->gpu_tex = dttr_graphics_sdl3gpu_take_recycled_texture(
			state,
			(Uint32)st->width,
			(Uint32)st->height
		);
	}

	return dttr_graphics_ensure_staged_texture(state, st);
}

static bool requeue_detached_upload(
	DTTR_BackendState *state,
	int texture_index,
	void **pixels
) {
	if (!state->texture_mutex || !pixels || !*pixels) {
		return false;
	}

	bool requeued = false;
	SDL_LockMutex(state->texture_mutex);

	if (texture_index >= 0 && texture_index < state->staged_texture_count) {
		DTTR_StagedTexture *st = &state->staged_textures[texture_index];
		if (st->refcount > 0 && !st->pixels) {
			st->pixels = *pixels;
			if (!st->pending_upload) {
				st->pending_upload = true;
				kv_push(int, state->pending_upload_indices, texture_index);
			}
			*pixels = NULL;
			requeued = true;
		}
	}

	SDL_UnlockMutex(state->texture_mutex);
	return requeued;
}

static void discard_detached_pixels(void **pixels) {
	if (!pixels || !*pixels) {
		return;
	}

	free(*pixels);
	*pixels = NULL;
}

// Detaches queued texture uploads under the mutex, uploads them, and keeps failed
// entries queued for retry.
static int collect_and_upload_pending(
	DTTR_BackendState *state,
	SDL_GPUCopyPass *copy,
	graphics_pending_upload *pending_uploads
) {
	if (!state->texture_mutex) {
		return 0;
	}

	int pending_count = 0;
	uint32_t total_bytes = 0;
	SDL_LockMutex(state->texture_mutex);
	const size_t queued_count = kv_size(state->pending_upload_indices);
	size_t deferred_write = 0;

	typedef struct {
		SDL_GPUTexture *tex;
		void *pixels;
		int width;
		int height;
		uint32_t bytes;
		uint32_t pool_offset;
		int texture_index;
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

		const uint32_t bytes = (uint32_t)(st->width * st->height * 4);
		const uint32_t aligned_offset = align_upload_offset(total_bytes);

		// Overflow/budget guard for the shared pool: leave the remainder queued
		// for the next frame rather than wrapping the uint32 offset math.
		if (bytes > UPLOAD_POOL_MAX_BYTES
			|| aligned_offset > UPLOAD_POOL_MAX_BYTES - bytes) {
			kv_A(state->pending_upload_indices, deferred_write++) = idx;
			continue;
		}

		st->pending_upload = false;

		if (!ensure_sdl_staged_texture(state, st)) {
			free(st->pixels);
			st->pixels = NULL;
			continue;
		}

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
			.pool_offset = aligned_offset,
			.texture_index = idx,
			.generate_mips = dttr_config.generate_texture_mipmaps,
		};

		total_bytes = aligned_offset + bytes;
		st->pixels = NULL;
		pending_count++;
	}

	state->pending_upload_indices.n = deferred_write;
	SDL_UnlockMutex(state->texture_mutex);

	SDL_GPUTransferBuffer *pool = NULL;
	void *mapped = NULL;
	if (copy && total_bytes > 0) {
		pool = acquire_upload_pool(state, total_bytes);
		if (pool) {
			wait_upload_pool_fence(state);
			mapped = SDL_MapGPUTransferBuffer(state->device, pool, false);
		}
	}

	if (!mapped && pending_count > 0) {
		DTTR_LOG_WARN("Failed to map upload pool (%u bytes)", total_bytes);
	}

	for (int i = 0; i < pending_count; i++) {
		bool ok = false;
		if (mapped && detached[i].tex) {
			memcpy(
				(uint8_t *)mapped + detached[i].pool_offset,
				detached[i].pixels,
				detached[i].bytes
			);

			const SDL_GPUTextureTransferInfo src = {
				.transfer_buffer = pool,
				.offset = detached[i].pool_offset,
				.pixels_per_row = (Uint32)detached[i].width,
			};

			const SDL_GPUTextureRegion dst = {
				.texture = detached[i].tex,
				.w = (Uint32)detached[i].width,
				.h = (Uint32)detached[i].height,
				.d = 1,
			};

			SDL_UploadToGPUTexture(copy, &src, &dst, false);
			ok = true;
		}

		pending_uploads[i] = (graphics_pending_upload){
			.tex = detached[i].tex,
			.bytes = detached[i].bytes,
			.generate_mips = detached[i].generate_mips,
			.uploaded = ok,
		};

		if (ok) {
			discard_detached_pixels(&detached[i].pixels);
		} else if (!requeue_detached_upload(
					   state,
					   detached[i].texture_index,
					   &detached[i].pixels
				   )) {
			discard_detached_pixels(&detached[i].pixels);
		}
	}

	if (mapped) {
		SDL_UnmapGPUTransferBuffer(state->device, pool);
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

// Runs the pending texture upload copy pass on a dedicated command buffer.
// A dedicated submission (with fence) keeps the pool's in-flight window
// decoupled from the frame command buffer, so the pool can be remapped with
// cycle=false and keep its pre-touched backing.
static void upload_pending_textures(DTTR_BackendState *state) {
	if (kv_size(state->pending_upload_indices) == 0) {
		return;
	}

	SDL_GPUCommandBuffer *ucmd = SDL_AcquireGPUCommandBuffer(state->device);
	if (!ucmd) {
		// Textures stay queued; retried next frame.
		return;
	}

	graphics_pending_upload pending[DTTR_MAX_STAGED_TEXTURES] = {0};

	SDL_GPUCopyPass *copy = SDL_BeginGPUCopyPass(ucmd);
	const int pending_count = collect_and_upload_pending(state, copy, pending);

	if (copy) {
		SDL_EndGPUCopyPass(copy);
	}

	if (pending_count == 0) {
		SDL_CancelGPUCommandBuffer(ucmd);
		return;
	}

	uint32_t uploaded_texture_count = 0;
	uint64_t uploaded_bytes = 0;
	generate_pending_mipmaps(
		state,
		ucmd,
		pending,
		pending_count,
		&uploaded_texture_count,
		&uploaded_bytes
	);

	set_upload_pool_fence(state, SDL_SubmitGPUCommandBufferAndAcquireFence(ucmd));

	state->perf_upload_textures_accum += uploaded_texture_count;
	state->perf_upload_bytes_accum += uploaded_bytes;
}

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

static void reset_replay_state(graphics_replay_state *replay_state) {
	if (!replay_state) {
		return;
	}

	replay_state->last_pipeline_idx = -1;
	replay_state->last_texture = NULL;
	replay_state->last_sampler = NULL;
	replay_state->uniforms_valid = false;
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

	// Uniform pushes are expensive under Wine's Vulkan mapping emulation.
	if (!replay_state || !replay_state->uniforms_valid
		|| memcmp(&replay_state->last_uniforms, &rec->draw.uniforms, sizeof(DTTR_Uniforms))
			   != 0) {
		push_uniforms(state->cmd, &rec->draw.uniforms);

		if (replay_state) {
			replay_state->last_uniforms = rec->draw.uniforms;
			replay_state->uniforms_valid = true;
		}
	}

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

	const bool acquired = SDL_WaitAndAcquireGPUSwapchainTexture(
		state->cmd,
		state->window,
		&state->swapchain_tex,
		&state->swapchain_width,
		&state->swapchain_height
	);

	if (!acquired) {
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
	upload_pending_textures(state);

	state->batch_records.n = 0;
	state->vertex_offset = 0;

	// Rotate pre-touched vertex buffers; avoid SDL cycle allocations mid-frame.
	sdl3_gpu_backend_data *bd = (sdl3_gpu_backend_data *)state->backend_data;
	if (bd) {
		// Let SDL recycle per-command resources as soon as fences signal.
		for (int i = 0; i < DTTR_VERTEX_RING_DEPTH; i++) {
			if (bd->vertex_ring_fence[i]
				&& SDL_QueryGPUFence(state->device, bd->vertex_ring_fence[i])) {
				SDL_ReleaseGPUFence(state->device, bd->vertex_ring_fence[i]);
				bd->vertex_ring_fence[i] = NULL;
			}
		}

		if (bd->upload_pool_fence
			&& SDL_QueryGPUFence(state->device, bd->upload_pool_fence)) {
			SDL_ReleaseGPUFence(state->device, bd->upload_pool_fence);
			bd->upload_pool_fence = NULL;
		}

		bd->vertex_ring_index = (bd->vertex_ring_index + 1) % DTTR_VERTEX_RING_DEPTH;
		SDL_GPUFence **fence = &bd->vertex_ring_fence[bd->vertex_ring_index];
		if (*fence) {
			SDL_WaitForGPUFences(state->device, true, fence, 1);
			SDL_ReleaseGPUFence(state->device, *fence);
			*fence = NULL;
		}

		if (bd->vertex_ring[bd->vertex_ring_index]) {
			state->transfer_buffer = bd->vertex_ring[bd->vertex_ring_index];
		}

		if (bd->vertex_gpu_ring[bd->vertex_ring_index]) {
			state->vertex_buffer = bd->vertex_gpu_ring[bd->vertex_ring_index];
		}
	}

	state->transfer_mapped = SDL_MapGPUTransferBuffer(
		state->device,
		state->transfer_buffer,
		false
	);

	if (!state->transfer_mapped) {
		DTTR_LOG_WARN("BeginFrame: MapGPUTransferBuffer failed");
	}

	state->frame_active = true;
	dttr_graphics_mod_frame_begin(state);
}

// Uploads vertices, replays draw records, blits to the swapchain, and submits the frame.
static void end_frame(DTTR_BackendState *state) {
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

			// Ring fence proves the destination buffer is idle; no cycle needed.
			SDL_UploadToGPUBuffer(copy, &src, &dst, false);
			SDL_EndGPUCopyPass(copy);
		}
	}

	// Draw-light loading frames keep SDL's uniform pool warm under Wine/MoltenVK.
	if (kv_size(state->batch_records) < 32) {
		warm_uniform_pool(state->cmd, 512);
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
	dttr_graphics_mod_after_game_frame();

	DTTR_PresentRect present = {
		.x = 0,
		.y = 0,
		.w = state->width,
		.h = state->height,
	};

	if (state->swapchain_tex) {
		const Uint32 swap_w = (state->swapchain_width > 0) ? state->swapchain_width
														   : (Uint32)state->width;
		const Uint32 swap_h = (state->swapchain_height > 0) ? state->swapchain_height
															: (Uint32)state->height;
		const bool
			is_internal_method = (dttr_config.scaling_method == DTTR_SCALING_METHOD_LOGICAL);
		present = dttr_graphics_compute_present_rect(
			(int)swap_w,
			(int)swap_h,
			state->width,
			state->height,
			dttr_config.scaling_fit == DTTR_SCALING_MODE_STRETCH,
			(!is_internal_method)
				&& (dttr_config.scaling_fit == DTTR_SCALING_MODE_INTEGER),
			1.0f
		);

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
		dttr_graphics_mod_present_rect_before();
	}

	sdl3_gpu_backend_data *bd = (sdl3_gpu_backend_data *)state->backend_data;
	if (bd && bd->vertex_ring[bd->vertex_ring_index] == state->transfer_buffer) {
		SDL_GPUFence *fence = SDL_SubmitGPUCommandBufferAndAcquireFence(state->cmd);
		bd->vertex_ring_fence[bd->vertex_ring_index] = fence;
	} else {
		SDL_SubmitGPUCommandBuffer(state->cmd);
	}

	dttr_graphics_mod_present_rect_after();
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
		dttr_graphics_sdl3gpu_bury_texture(state, state->video_texture);
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
	SDL_GPUTransferBuffer *tbuf = acquire_upload_pool(state, upload_size);

	if (!tbuf) {
		return false;
	}

	wait_upload_pool_fence(state);
	void *mapped = SDL_MapGPUTransferBuffer(state->device, tbuf, false);

	if (!mapped) {
		return false;
	}

	memcpy(mapped, pixels, upload_size);
	SDL_UnmapGPUTransferBuffer(state->device, tbuf);

	SDL_GPUCommandBuffer *cmd = SDL_AcquireGPUCommandBuffer(state->device);

	if (!cmd) {
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
		const DTTR_PresentRect present = dttr_graphics_compute_present_rect(
			(int)swapchain_w,
			(int)swapchain_h,
			width,
			height,
			false,
			false,
			1.0f
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

	set_upload_pool_fence(state, SDL_SubmitGPUCommandBufferAndAcquireFence(cmd));

	return true;
}

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
