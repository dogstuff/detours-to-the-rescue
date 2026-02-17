#include "dttr_sidecar.h"
#include "graphics_internal.h"
#include "log.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

// Tracks one temporary storage buffer upload destined for a staged texture
typedef struct {
	SDL_GPUBuffer *buf;
	SDL_GPUTexture *tex;
	void *pixels;
	int width;
	int height;
	uint32_t bytes;
	bool generate_mips;
	bool from_pool;
	int pool_slot;
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
	if (!state || pool_slot < 0 || pool_slot >= DTTR_UPLOAD_POOL_SIZE)
		return;

	state->m_upload_pool[pool_slot].m_in_use = false;
}

// Acquires or grows one reusable upload slot that can fit `bytes`
static int s_acquire_upload_pool_slot(DTTR_BackendState *state, uint32_t bytes) {
	if (!state || !state->m_device || bytes == 0)
		return -1;

	int free_slot = -1;
	int grow_slot = -1;

	for (int i = 0; i < DTTR_UPLOAD_POOL_SIZE; i++) {
		DTTR_UploadPoolSlot *slot = &state->m_upload_pool[i];
		if (slot->m_in_use)
			continue;

		if (slot->m_storage_buffer && slot->m_transfer_buffer
			&& slot->m_capacity >= bytes) {
			slot->m_in_use = true;
			return i;
		}

		if (free_slot < 0)
			free_slot = i;

		if (slot->m_storage_buffer && slot->m_transfer_buffer)
			grow_slot = i;
	}

	const int slot_index = (grow_slot >= 0) ? grow_slot : free_slot;
	if (slot_index < 0)
		return -1;

	DTTR_UploadPoolSlot *slot = &state->m_upload_pool[slot_index];

	if (slot->m_storage_buffer) {
		SDL_ReleaseGPUBuffer(state->m_device, slot->m_storage_buffer);
		slot->m_storage_buffer = NULL;
	}

	if (slot->m_transfer_buffer) {
		SDL_ReleaseGPUTransferBuffer(state->m_device, slot->m_transfer_buffer);
		slot->m_transfer_buffer = NULL;
	}

	const SDL_GPUBufferCreateInfo sbuf_info = {
		.usage = SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ,
		.size = bytes,
	};
	slot->m_storage_buffer = SDL_CreateGPUBuffer(state->m_device, &sbuf_info);

	if (!slot->m_storage_buffer) {
		slot->m_capacity = 0;
		slot->m_in_use = false;
		return -1;
	}

	const SDL_GPUTransferBufferCreateInfo tbuf_info = {
		.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
		.size = bytes,
	};
	slot->m_transfer_buffer = SDL_CreateGPUTransferBuffer(state->m_device, &tbuf_info);

	if (!slot->m_transfer_buffer) {
		SDL_ReleaseGPUBuffer(state->m_device, slot->m_storage_buffer);
		slot->m_storage_buffer = NULL;
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
	if (!render_pass)
		return;

	const SDL_GPUBufferBinding vbuf_binding = {
		.buffer = state->m_vertex_buffer,
	};
	SDL_BindGPUVertexBuffers(render_pass, 0, &vbuf_binding, 1);
}

// Ends the active render pass and clears the pass pointer
static void s_end_render_pass_if_active(DTTR_BackendState *state) {
	if (!state->m_render_pass)
		return;

	SDL_EndGPURenderPass(state->m_render_pass);
	state->m_render_pass = NULL;
}

// Releases textures deferred for destruction on the GPU thread
static void s_release_deferred_texture_destroys(DTTR_BackendState *state) {
	SDL_LockMutex(state->m_texture_mutex);

	for (int i = 0; i < state->m_deferred_destroy_count; i++) {
		SDL_ReleaseGPUTexture(state->m_device, state->m_deferred_destroys[i]);
	}

	state->m_deferred_destroy_count = 0;
	SDL_UnlockMutex(state->m_texture_mutex);
}

// Lazily creates the GPU texture backing a staged texture slot
static bool s_ensure_staged_texture(DTTR_BackendState *state, DTTR_StagedTexture *st) {
	if (st->m_gpu_tex)
		return true;

	const SDL_GPUTextureCreateInfo tex_info = {
		.type = SDL_GPU_TEXTURETYPE_2D,
		.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
		.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER
				 | SDL_GPU_TEXTUREUSAGE_COMPUTE_STORAGE_SIMULTANEOUS_READ_WRITE
				 | SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COLOR_TARGET,
		.width = st->m_width,
		.height = st->m_height,
		.layer_count_or_depth = 1,
		.num_levels = dttr_graphics_calc_mip_levels(st->m_width, st->m_height),
	};
	st->m_gpu_tex = SDL_CreateGPUTexture(state->m_device, &tex_info);
	return st->m_gpu_tex != NULL;
}

// Detaches staged pixel payloads for upload, minimizing time spent holding the
// shared texture mutex
static int s_collect_pending_uploads(
	DTTR_BackendState *state,
	S_GraphicsPendingUpload *pending_uploads,
	int max_uploads
) {
	if (!state->m_texture_mutex)
		return 0;

	int pending_count = 0;
	SDL_LockMutex(state->m_texture_mutex);
	const size_t queued_count = kv_size(state->m_pending_upload_indices);
	size_t deferred_write = 0;

	for (size_t q = 0; q < queued_count; q++) {
		const int idx = kv_A(state->m_pending_upload_indices, q);

		if (idx < 0 || idx >= state->m_staged_texture_count)
			continue;

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

		pending_uploads[pending_count] = (S_GraphicsPendingUpload){
			.buf = NULL,
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

	return pending_count;
}

// Uploads one pending pixel payload to a temporary storage buffer
static void s_stage_upload_data(
	DTTR_BackendState *state,
	SDL_GPUCopyPass *copy,
	S_GraphicsPendingUpload *upload
) {
	if (!upload || !upload->pixels || !upload->tex || upload->bytes == 0 || !copy) {
		free(upload ? upload->pixels : NULL);

		if (upload)
			upload->pixels = NULL;

		return;
	}

	const SDL_GPUBufferCreateInfo sbuf_info = {
		.usage = SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ, // used as a temporary fallback
														   // path
		.size = upload->bytes, // used as a temporary fallback path
	};
	SDL_GPUBuffer *pixel_buf = NULL;
	SDL_GPUTransferBuffer *tbuf = NULL;
	bool from_pool = false;
	// Reusing pool slots is only safe when we block for GPU completion before slot reuse.
	// In async mode, pool reuse can race outstanding GPU reads and corrupt uploads.
	int pool_slot = -1;

	if (g_dttr_config.m_texture_upload_sync) {
		pool_slot = s_acquire_upload_pool_slot(state, upload->bytes);
	}

	if (pool_slot >= 0) {
		DTTR_UploadPoolSlot *slot = &state->m_upload_pool[pool_slot];
		pixel_buf = slot->m_storage_buffer;
		tbuf = slot->m_transfer_buffer;
		from_pool = true;
	} else {
		pixel_buf = SDL_CreateGPUBuffer(state->m_device, &sbuf_info);

		if (!pixel_buf) {
			free(upload->pixels);
			upload->pixels = NULL;
			return;
		}

		const SDL_GPUTransferBufferCreateInfo tbuf_info = {
			.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
			.size = upload->bytes,
		};
		tbuf = SDL_CreateGPUTransferBuffer(state->m_device, &tbuf_info);

		if (!tbuf) {
			SDL_ReleaseGPUBuffer(state->m_device, pixel_buf);
			free(upload->pixels);
			upload->pixels = NULL;
			return;
		}
	}

	void *mapped = SDL_MapGPUTransferBuffer(state->m_device, tbuf, false);

	if (!mapped) {
		if (from_pool) {
			s_release_upload_pool_slot(state, pool_slot);
		} else {
			SDL_ReleaseGPUTransferBuffer(state->m_device, tbuf);
			SDL_ReleaseGPUBuffer(state->m_device, pixel_buf);
		}
		free(upload->pixels);
		upload->pixels = NULL;
		return;
	}

	memcpy(mapped, upload->pixels, upload->bytes);
	SDL_UnmapGPUTransferBuffer(state->m_device, tbuf);

	const SDL_GPUTransferBufferLocation src = {
		.transfer_buffer = tbuf,
	};
	const SDL_GPUBufferRegion dst = {
		.buffer = pixel_buf,
		.size = upload->bytes,
	};
	SDL_UploadToGPUBuffer(copy, &src, &dst, false);

	if (!from_pool)
		SDL_ReleaseGPUTransferBuffer(state->m_device, tbuf);

	upload->buf = pixel_buf;
	upload->from_pool = from_pool;
	upload->pool_slot = from_pool ? pool_slot : -1;
	free(upload->pixels);
	upload->pixels = NULL;
}

// Runs compute conversion and mip generation for all queued texture uploads
static void s_dispatch_pending_uploads(
	DTTR_BackendState *state,
	SDL_GPUCommandBuffer *cmd,
	const S_GraphicsPendingUpload *pending,
	int pending_count,
	uint32_t *uploaded_texture_count,
	uint64_t *uploaded_bytes
) {
	for (int p = 0; p < pending_count; p++) {
		if (!pending[p].buf || !pending[p].tex)
			continue;

		const SDL_GPUStorageTextureReadWriteBinding tex_bind = {
			.texture = pending[p].tex,
		};
		SDL_GPUComputePass *comp = SDL_BeginGPUComputePass(cmd, &tex_bind, 1, NULL, 0);

		if (comp) {
			SDL_BindGPUComputePipeline(comp, state->m_buf2tex_pipeline);
			SDL_BindGPUComputeStorageBuffers(comp, 0, &pending[p].buf, 1);
			const uint32_t pc[2] = {
				(uint32_t)pending[p].width,
				(uint32_t)pending[p].height,
			};
			SDL_PushGPUComputeUniformData(cmd, 0, pc, sizeof(pc));
			const uint32_t gx = (uint32_t)((pending[p].width + DTTR_COMPUTE_WORKGROUP_X
											- 1)
										   / DTTR_COMPUTE_WORKGROUP_X);
			const uint32_t gy = (uint32_t)((pending[p].height + DTTR_COMPUTE_WORKGROUP_Y
											- 1)
										   / DTTR_COMPUTE_WORKGROUP_Y);
			SDL_DispatchGPUCompute(comp, gx, gy, DTTR_COMPUTE_WORKGROUP_Z);
			SDL_EndGPUComputePass(comp);
		}

		if (pending[p].generate_mips) {
			SDL_GenerateMipmapsForGPUTexture(cmd, pending[p].tex);
			state->m_perf_mips_generated_accum++;
		} else {
			state->m_perf_mips_skipped_accum++;
		}

		if (uploaded_texture_count)
			(*uploaded_texture_count)++;

		if (uploaded_bytes)
			(*uploaded_bytes) += pending[p].bytes;
	}
}

// Releases temporary storage buffers used for one upload batch
static void s_release_pending_upload_buffers(
	DTTR_BackendState *state,
	const S_GraphicsPendingUpload *pending,
	int pending_count
) {
	for (int p = 0; p < pending_count; p++) {
		if (!pending[p].buf)
			continue;

		if (pending[p].from_pool) {
			s_release_upload_pool_slot(state, pending[p].pool_slot);
		} else {
			SDL_ReleaseGPUBuffer(state->m_device, pending[p].buf);
		}
	}
}

// Flushes all pending staged textures to GPU textures
static void s_upload_pending_textures(DTTR_BackendState *state, SDL_GPUCommandBuffer *cmd) {
	if (!cmd)
		return;

	S_GraphicsPendingUpload pending[DTTR_MAX_STAGED_TEXTURES] = {0};
	const int pending_count = s_collect_pending_uploads(state, pending, 0);

	if (pending_count == 0)
		return;

	SDL_GPUCopyPass *copy = SDL_BeginGPUCopyPass(cmd);

	for (int i = 0; i < pending_count; i++) {
		s_stage_upload_data(state, copy, &pending[i]);
	}

	if (copy) {
		SDL_EndGPUCopyPass(copy);
	}

	int prepared_count = 0;

	for (int i = 0; i < pending_count; i++) {
		if (pending[i].buf)
			prepared_count++;
	}

	if (prepared_count == 0)
		return;

	uint32_t uploaded_texture_count = 0;
	uint64_t uploaded_bytes = 0;
	s_dispatch_pending_uploads(
		state,
		cmd,
		pending,
		pending_count,
		&uploaded_texture_count,
		&uploaded_bytes
	);
	s_release_pending_upload_buffers(state, pending, pending_count);

	state->m_perf_upload_textures_accum += uploaded_texture_count;
	state->m_perf_upload_bytes_accum += uploaded_bytes;
}

// Opens a standard draw pass when no render pass is active
static bool s_begin_draw_pass_if_needed(DTTR_BackendState *state) {
	if (state->m_render_pass)
		return false;

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
	s_bind_frame_vertex_buffer(state, state->m_render_pass);
	return state->m_render_pass != NULL;
}

static void s_reset_replay_state(S_GraphicsReplayState *replay_state) {
	if (!replay_state)
		return;

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
		.store_op = SDL_GPU_STOREOP_DONT_CARE,
	};

	state->m_render_pass = SDL_BeginGPURenderPass(
		state->m_cmd,
		&color_target,
		1,
		&depth_target
	);
	s_bind_frame_vertex_buffer(state, state->m_render_pass);
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

	if (!state->m_render_pass)
		return;

	const int pidx = DTTR_PIPELINE_INDEX(
		rec->draw.blend_mode,
		rec->draw.depth_test,
		rec->draw.depth_write
	);
	if (!replay_state || replay_state->last_pipeline_idx != pidx) {
		SDL_BindGPUGraphicsPipeline(state->m_render_pass, state->m_pipelines[pidx]);

		if (replay_state)
			replay_state->last_pipeline_idx = pidx;

		if (replay_stats)
			replay_stats->pipeline_bind_count++;
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

		if (replay_stats)
			replay_stats->sampler_bind_count++;
	}

	SDL_DrawGPUPrimitives(
		state->m_render_pass,
		rec->draw.vertex_count,
		1,
		rec->draw.first_vertex,
		0
	);

	if (replay_stats)
		replay_stats->draw_count++;
}

// Replays all recorded batch operations for the frame
static S_GraphicsReplayStats s_replay_batch_records(DTTR_BackendState *state) {
	S_GraphicsReplayStats replay_stats = {0};

	if (state->m_batch_count == 0)
		return replay_stats;

	S_GraphicsReplayState replay_state = {0};
	s_reset_replay_state(&replay_state);
	state->m_render_pass = NULL;

	for (uint32_t i = 0; i < state->m_batch_count; i++) {
		const DTTR_BatchRecord *rec = &state->m_batch_records[i];

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
void dttr_graphics_begin_frame(void) {
	DTTR_BackendState *state = &g_dttr_backend;
	if (state->m_frame_active)
		return;

	if (!state->m_device || !state->m_window || !dttr_graphics_is_gpu_thread())
		return;

	state->m_frame_index++;

	state->m_cmd = SDL_AcquireGPUCommandBuffer(state->m_device);

	if (!state->m_cmd) {
		log_error("Failed to acquire GPU command buffer");
		return;
	}

	s_release_deferred_texture_destroys(state);
	s_upload_pending_textures(state, state->m_cmd);

	SDL_WaitAndAcquireGPUSwapchainTexture(
		state->m_cmd,
		state->m_window,
		&state->m_swapchain_tex,
		&state->m_swapchain_width,
		&state->m_swapchain_height
	);

	state->m_batch_count = 0;
	state->m_vertex_offset = 0;
	state->m_transfer_mapped = SDL_MapGPUTransferBuffer(
		state->m_device,
		state->m_transfer_buffer,
		true
	);

	if (!state->m_transfer_mapped)
		log_warn("BeginFrame: MapGPUTransferBuffer failed");

	state->m_frame_active = true;
}

// Finalizes GPU uploads and batch replay, then submits the frame command buffer
void dttr_graphics_end_frame(void) {
	DTTR_BackendState *state = &g_dttr_backend;

	if (!state->m_frame_active)
		return;

	state->m_frame_active = false;

	if (state->m_transfer_mapped) {
		SDL_UnmapGPUTransferBuffer(state->m_device, state->m_transfer_buffer);
		state->m_transfer_mapped = NULL;
	}

	if (!state->m_cmd)
		return;

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

	if (state->m_swapchain_tex) {
		const Uint32 swap_w = (state->m_swapchain_width > 0) ? state->m_swapchain_width
															 : (Uint32)state->m_width;
		const Uint32 swap_h = (state->m_swapchain_height > 0) ? state->m_swapchain_height
															  : (Uint32)state->m_height;
		Uint32 present_x = 0;
		Uint32 present_y = 0;
		Uint32 present_w = swap_w;
		Uint32 present_h = swap_h;

		const bool is_internal_method
			= (g_dttr_config.m_scaling_method == DTTR_SCALING_METHOD_LOGICAL);
		const bool is_stretch_fit
			= (g_dttr_config.m_scaling_fit == DTTR_SCALING_MODE_STRETCH);
		const bool is_integer_fit = (!is_internal_method)
									&& (g_dttr_config.m_scaling_fit
										== DTTR_SCALING_MODE_INTEGER);

		if (!is_stretch_fit) {
			const float sx = (float)swap_w / (float)state->m_width;
			const float sy = (float)swap_h / (float)state->m_height;
			float scale = SDL_min(sx, sy);
			if (is_integer_fit && scale >= 1.0f) {
				scale = floorf(scale);
			}

			present_w = (Uint32)((float)state->m_width * scale);
			present_h = (Uint32)((float)state->m_height * scale);
			if (present_w == 0)
				present_w = 1;

			if (present_h == 0)
				present_h = 1;

			present_w = SDL_min(present_w, swap_w);
			present_h = SDL_min(present_h, swap_h);
			present_x = (swap_w - present_w) / 2;
			present_y = (swap_h - present_h) / 2;
		}

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
					.x = present_x,
					.y = present_y,
					.w = present_w,
					.h = present_h,
				},
			.clear_color = {0.0f, 0.0f, 0.0f, 1.0f},
			.load_op = SDL_GPU_LOADOP_CLEAR,
			.filter = g_dttr_config.m_present_filter,
		};
		SDL_BlitGPUTexture(state->m_cmd, &blit);
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
		&& state->m_video_height == height)
		return true;

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

	if (!state->m_video_texture)
		return false;

	state->m_video_width = width;
	state->m_video_height = height;
	return true;
}

bool dttr_graphics_present_video_frame_bgra(
	const uint8_t *pixels,
	int width,
	int height,
	int stride
) {
	DTTR_BackendState *state = &g_dttr_backend;

	if (!pixels || width <= 0 || height <= 0 || stride < (width * 4))
		return false;

	if (!state->m_device || !state->m_window || !dttr_graphics_is_gpu_thread())
		return false;

	if (state->m_frame_active) {
		// This path assumes ownership of the command buffer lifecycle
		return false;
	}

	if (!s_ensure_video_texture(state, width, height))
		return false;

	const Uint32 upload_size = (Uint32)(stride * height);
	const SDL_GPUTransferBufferCreateInfo tbuf_info = {
		.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
		.size = upload_size,
	};
	SDL_GPUTransferBuffer *tbuf = SDL_CreateGPUTransferBuffer(state->m_device, &tbuf_info);

	if (!tbuf)
		return false;

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
		Uint32 dst_w = swapchain_w;
		Uint32 dst_h = swapchain_h;
		Uint32 dst_x = 0;
		Uint32 dst_y = 0;

		const float sx = (float)swapchain_w / (float)width;
		const float sy = (float)swapchain_h / (float)height;
		const float scale = SDL_min(sx, sy);
		dst_w = (Uint32)((float)width * scale);
		dst_h = (Uint32)((float)height * scale);

		if (dst_w == 0)
			dst_w = 1;

		if (dst_h == 0)
			dst_h = 1;

		dst_x = (swapchain_w - dst_w) / 2;
		dst_y = (swapchain_h - dst_h) / 2;

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
					.x = dst_x,
					.y = dst_y,
					.w = dst_w,
					.h = dst_h,
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
