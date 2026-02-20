#ifdef DTTR_COMPONENTS_ENABLED

#include "graphics_internal.h"

#include "log.h"

#include <stdlib.h>
#include <string.h>

#define S_OVERLAY_W 59
#define S_OVERLAY_H 12
#define S_OVERLAY_STRIDE 8

// Embedded bitmap for:
// COMPONENTS
// ENABLED
static const uint8_t S_OVERLAY_BITMAP[] = {
	0x71, 0xC8, 0xBC, 0x72, 0x2F, 0xA2, 0xF9, 0xE0, 0x82, 0x2D, 0xA2, 0x8B, 0x28, 0x32,
	0x22, 0x00, 0x82, 0x2A, 0xBC, 0x8A, 0xAF, 0x2A, 0x21, 0xC0, 0x82, 0x28, 0xA0, 0x8A,
	0x68, 0x26, 0x20, 0x20, 0x71, 0xC8, 0xA0, 0x72, 0x2F, 0xA2, 0x23, 0xC0, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x7D, 0x13, 0x9E, 0x41, 0xF7, 0x80, 0x00, 0x00, 0x41, 0x94, 0x51, 0x41, 0x04,
	0x40, 0x00, 0x00, 0x79, 0x57, 0xDE, 0x41, 0xE4, 0x40, 0x00, 0x00, 0x41, 0x34, 0x51,
	0x41, 0x04, 0x40, 0x00, 0x00, 0x7D, 0x14, 0x5E, 0x7D, 0xF7, 0x80, 0x00,
};

bool dttr_components_overlay_create(DTTR_BackendState *state) {
	const int w = S_OVERLAY_W;
	const int h = S_OVERLAY_H;
	const uint32_t pixel_count = (uint32_t)(w * h);
	const uint32_t byte_count = pixel_count * 4;

	uint8_t *rgba = (uint8_t *)malloc(byte_count);
	if (!rgba)
		return false;

	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			const int bit_index = y * S_OVERLAY_STRIDE * 8 + x;
			const int byte_index = bit_index / 8;
			const int bit_offset = 7 - (bit_index % 8);
			const bool set = (S_OVERLAY_BITMAP[byte_index] >> bit_offset) & 1;
			const int px = (y * w + x) * 4;
			rgba[px + 0] = 255;
			rgba[px + 1] = 255;
			rgba[px + 2] = 255;
			rgba[px + 3] = set ? 255 : 0;
		}
	}

	const SDL_GPUTextureCreateInfo tex_info = {
		.type = SDL_GPU_TEXTURETYPE_2D,
		.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
		.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER,
		.width = (uint32_t)w,
		.height = (uint32_t)h,
		.layer_count_or_depth = 1,
		.num_levels = 1,
		.sample_count = SDL_GPU_SAMPLECOUNT_1,
	};
	state->m_components_overlay_tex = SDL_CreateGPUTexture(state->m_device, &tex_info);

	if (!state->m_components_overlay_tex) {
		free(rgba);
		return false;
	}

	const SDL_GPUTransferBufferCreateInfo tbuf_info = {
		.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
		.size = byte_count,
	};
	SDL_GPUTransferBuffer *tbuf = SDL_CreateGPUTransferBuffer(state->m_device, &tbuf_info);

	if (!tbuf) {
		SDL_ReleaseGPUTexture(state->m_device, state->m_components_overlay_tex);
		state->m_components_overlay_tex = NULL;
		free(rgba);
		return false;
	}

	void *mapped = SDL_MapGPUTransferBuffer(state->m_device, tbuf, false);
	if (!mapped) {
		SDL_ReleaseGPUTransferBuffer(state->m_device, tbuf);
		SDL_ReleaseGPUTexture(state->m_device, state->m_components_overlay_tex);
		state->m_components_overlay_tex = NULL;
		free(rgba);
		return false;
	}

	memcpy(mapped, rgba, byte_count);
	SDL_UnmapGPUTransferBuffer(state->m_device, tbuf);
	free(rgba);

	SDL_GPUCommandBuffer *cmd = SDL_AcquireGPUCommandBuffer(state->m_device);
	if (!cmd) {
		SDL_ReleaseGPUTransferBuffer(state->m_device, tbuf);
		SDL_ReleaseGPUTexture(state->m_device, state->m_components_overlay_tex);
		state->m_components_overlay_tex = NULL;
		return false;
	}

	SDL_GPUCopyPass *copy = SDL_BeginGPUCopyPass(cmd);
	if (copy) {
		const SDL_GPUTextureTransferInfo src = {
			.transfer_buffer = tbuf,
			.pixels_per_row = (uint32_t)w,
			.rows_per_layer = (uint32_t)h,
		};
		const SDL_GPUTextureRegion dst = {
			.texture = state->m_components_overlay_tex,
			.w = (uint32_t)w,
			.h = (uint32_t)h,
			.d = 1,
		};
		SDL_UploadToGPUTexture(copy, &src, &dst, false);
		SDL_EndGPUCopyPass(copy);
	}

	SDL_SubmitGPUCommandBuffer(cmd);
	SDL_WaitForGPUIdle(state->m_device);
	SDL_ReleaseGPUTransferBuffer(state->m_device, tbuf);

	state->m_components_overlay_w = w;
	state->m_components_overlay_h = h;

	log_info("Components overlay texture created (%dx%d)", w, h);
	return true;
}

void dttr_components_overlay_destroy(DTTR_BackendState *state) {
	if (state->m_components_overlay_tex) {
		SDL_ReleaseGPUTexture(state->m_device, state->m_components_overlay_tex);
		state->m_components_overlay_tex = NULL;
	}
}

#endif /* DTTR_COMPONENTS_ENABLED */
