#ifndef DTTR_GRAPHICS_RESIZE_PRIVATE_H
#define DTTR_GRAPHICS_RESIZE_PRIVATE_H

#include <stdbool.h>

#include <dttr_config.h>

typedef struct {
	int width;
	int height;
} DTTR_Size;

typedef struct {
	int width;
	int height;
	bool pending;
} DTTR_WindowResizePending;

void dttr_window_resize_record(DTTR_WindowResizePending *pending, int width, int height);
bool dttr_window_resize_take(
	DTTR_WindowResizePending *pending,
	int *out_width,
	int *out_height
);

DTTR_Size dttr_select_render_resolution(
	DTTR_ScalingMethod scaling_method,
	DTTR_ScalingMode scaling_fit,
	DTTR_Size logical,
	DTTR_Size configured_window,
	DTTR_Size window_pixels
);

#endif
