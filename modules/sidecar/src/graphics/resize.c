#include "resize_private.h"

#define DTTR_MIN_WINDOW_DIM 64

static int clamp_dim(int value, int fallback) {
	return (value < DTTR_MIN_WINDOW_DIM) ? fallback : value;
}

void dttr_window_resize_record(DTTR_WindowResizePending *pending, int width, int height) {
	pending->width = width;
	pending->height = height;
	pending->pending = true;
}

bool dttr_window_resize_take(
	DTTR_WindowResizePending *pending,
	int *out_width,
	int *out_height
) {
	if (!pending->pending) {
		return false;
	}

	pending->pending = false;
	if (pending->width < DTTR_MIN_WINDOW_DIM || pending->height < DTTR_MIN_WINDOW_DIM) {
		return false;
	}

	*out_width = pending->width;
	*out_height = pending->height;
	return true;
}

DTTR_Size dttr_select_render_resolution(
	DTTR_ScalingMethod scaling_method,
	DTTR_ScalingMode scaling_fit,
	DTTR_Size logical,
	DTTR_Size configured_window,
	DTTR_Size window_pixels
) {
	int width = logical.width;
	int height = logical.height;

	if (scaling_method == DTTR_SCALING_METHOD_LOGICAL) {
		int target_width = configured_window.width;
		int target_height = configured_window.height;

		if (window_pixels.width > target_width) {
			target_width = window_pixels.width;
		}
		if (window_pixels.height > target_height) {
			target_height = window_pixels.height;
		}

		if (scaling_fit == DTTR_SCALING_MODE_STRETCH) {
			width = target_width;
			height = target_height;
		} else {
			const int lw = clamp_dim(logical.width, WINDOW_WIDTH);
			const int lh = clamp_dim(logical.height, WINDOW_HEIGHT);
			const float width_scale = (float)target_width / (float)lw;
			const float height_scale = (float)target_height / (float)lh;
			const float scale = width_scale < height_scale ? width_scale : height_scale;

			width = (int)((float)lw * scale);
			height = (int)((float)lh * scale);
		}
	}

	width = clamp_dim(width, WINDOW_WIDTH);
	height = clamp_dim(height, WINDOW_HEIGHT);
	return (DTTR_Size){width, height};
}
