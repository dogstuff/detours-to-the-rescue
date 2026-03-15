#ifdef DTTR_COMPONENTS_ENABLED

#include "graphics_internal.h"

#include <stdlib.h>

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

void dttr_components_overlay_build_vertices(
	DTTR_Vertex *out,
	int render_w,
	int render_h,
	int overlay_w,
	int overlay_h
) {
	const float scale = render_h / 320.0f;
	const float ow = overlay_w * scale;
	const float oh = overlay_h * scale;
	const float margin = 4.0f * scale;
	const float alpha = 0.15f;
	const float x0 = render_w - ow - margin;
	const float x1 = x0 + ow;
	const float y1 = margin + oh;

	out[0] = (DTTR_Vertex){x0, margin, 0, 1, 1, 1, 1, alpha, 0, 0};
	out[1] = (DTTR_Vertex){x1, margin, 0, 1, 1, 1, 1, alpha, 1, 0};
	out[2] = (DTTR_Vertex){x0, y1, 0, 1, 1, 1, 1, alpha, 0, 1};
	out[3] = (DTTR_Vertex){x0, y1, 0, 1, 1, 1, 1, alpha, 0, 1};
	out[4] = (DTTR_Vertex){x1, margin, 0, 1, 1, 1, 1, alpha, 1, 0};
	out[5] = (DTTR_Vertex){x1, y1, 0, 1, 1, 1, 1, alpha, 1, 1};
}

uint8_t *dttr_components_overlay_decode_bitmap(int *out_w, int *out_h) {
	const int w = S_OVERLAY_W;
	const int h = S_OVERLAY_H;
	const uint32_t pixel_count = (uint32_t)(w * h);
	const uint32_t byte_count = pixel_count * 4;

	uint8_t *rgba = (uint8_t *)malloc(byte_count);
	if (!rgba)
		return NULL;

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

	*out_w = w;
	*out_h = h;
	return rgba;
}

#endif /* DTTR_COMPONENTS_ENABLED */
