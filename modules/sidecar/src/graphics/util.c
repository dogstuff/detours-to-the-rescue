#include "graphics_private.h"

#include <dttr_config.h>
#include <math.h>
#include <string.h>

DTTR_BackendState dttr_backend;

#define DTTR_MESH_SEAM_FILL_PHYSICAL_PX 0.5f
#define DTTR_MESH_SEAM_MAX_VERTEX_NUDGE_PHYSICAL_PX 0.75f
#define DTTR_MESH_SEAM_MIN_AREA_PX 1.0e-4f

int dttr_graphics_calc_mip_levels(int w, int h) {
	int d = w > h ? w : h;
	int levels = 1;

	while (d > 1) {
		d >>= 1;
		levels++;
	}

	return levels;
}

void dttr_graphics_mat4_identity(float *m) {
	memset(m, 0, DTTR_MAT4_SIZE);
	m[0] = m[5] = m[10] = m[15] = 1.0f;
}

const char *dttr_graphics_shader_format_name(SDL_GPUShaderFormat format) {
	switch (format) {
	case SDL_GPU_SHADERFORMAT_SPIRV:
		return "SPIRV";
	case SDL_GPU_SHADERFORMAT_DXIL:
		return "DXIL";
	case SDL_GPU_SHADERFORMAT_METALLIB:
		return "METALLIB";
	case SDL_GPU_SHADERFORMAT_PRIVATE:
		return "PRIVATE";
	default:
		return "UNKNOWN";
	}
}

SDL_GPUShaderFormat dttr_graphics_requested_shader_formats() {
	return SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL;
}

SDL_GPUShaderFormat dttr_graphics_select_shader_format(SDL_GPUShaderFormat formats) {
	if (formats & SDL_GPU_SHADERFORMAT_DXIL) {
		return SDL_GPU_SHADERFORMAT_DXIL;
	}

	if (formats & SDL_GPU_SHADERFORMAT_SPIRV) {
		return SDL_GPU_SHADERFORMAT_SPIRV;
	}

	return SDL_GPU_SHADERFORMAT_INVALID;
}

SDL_GPUShaderFormat dttr_graphics_shader_format_for_driver(const char *driver) {
	if (!driver || !driver[0]) {
		return SDL_GPU_SHADERFORMAT_INVALID;
	}

	if (strcmp(driver, DTTR_DRIVER_DIRECT3D12) == 0) {
		return SDL_GPU_SHADERFORMAT_DXIL;
	}

	if (strcmp(driver, DTTR_DRIVER_VULKAN) == 0) {
		return SDL_GPU_SHADERFORMAT_SPIRV;
	}

	return SDL_GPU_SHADERFORMAT_INVALID;
}

// Selects a driver-preferred shader format, then falls back to generic selection.
SDL_GPUShaderFormat dttr_graphics_select_shader_format_for_driver(
	const char *driver,
	SDL_GPUShaderFormat formats
) {
	const SDL_GPUShaderFormat preferred = dttr_graphics_shader_format_for_driver(driver);

	if ((preferred != SDL_GPU_SHADERFORMAT_INVALID) && (formats & preferred)) {
		return preferred;
	}

	return dttr_graphics_select_shader_format(formats);
}

bool dttr_graphics_is_gpu_thread() {
	const DTTR_BackendState *state = &dttr_backend;

	if (!state->initialized) {
		return false;
	}

	if (state->gpu_thread_id == 0) {
		return true;
	}

	return SDL_GetCurrentThreadID() == state->gpu_thread_id;
}

// Restricts seam fill to the rendering mode and draw states that expose solid mesh cracks.
bool dttr_graphics_should_fill_mesh_seams(
	DTTR_PrimitiveType type,
	bool transformed,
	bool depth_test,
	bool blend_enabled
) {
	return dttr_config.scaling_method == DTTR_SCALING_METHOD_LOGICAL
		   && dttr_config.vertex_precision == DTTR_VERTEX_PRECISION_SUBPIXEL
		   && type == DTTR_PRIM_TRIANGLELIST && transformed && depth_test
		   && !blend_enabled;
}

// Expands one triangle in physical-pixel space while preserving all non-position attrs.
static void fill_mesh_seam_triangle(
	DTTR_Vertex *tri,
	float logical_to_px_x,
	float logical_to_px_y,
	float px_to_logical_x,
	float px_to_logical_y
) {
	float x[3];
	float y[3];
	float nx[3];
	float ny[3];
	float c[3];

	for (int i = 0; i < 3; i++) {
		x[i] = tri[i].x * logical_to_px_x;
		y[i] = tri[i].y * logical_to_px_y;
	}

	const float area_px = (x[1] - x[0]) * (y[2] - y[0]) - (y[1] - y[0]) * (x[2] - x[0]);
	if (!isfinite(area_px) || fabsf(area_px) <= DTTR_MESH_SEAM_MIN_AREA_PX) {
		return;
	}

	const float winding = area_px > 0.0f ? 1.0f : -1.0f;
	for (int i = 0; i < 3; i++) {
		const int j = (i + 1) % 3;
		const float dx = x[j] - x[i];
		const float dy = y[j] - y[i];
		const float len = sqrtf(dx * dx + dy * dy);

		if (!isfinite(len) || len <= 1.0e-6f) {
			return;
		}

		nx[i] = winding * dy / len;
		ny[i] = -winding * dx / len;
		c[i] = nx[i] * x[i] + ny[i] * y[i] + DTTR_MESH_SEAM_FILL_PHYSICAL_PX;
	}

	for (int i = 0; i < 3; i++) {
		const int prev = (i + 2) % 3;
		const float det = nx[prev] * ny[i] - ny[prev] * nx[i];

		if (!isfinite(det) || fabsf(det) <= 1.0e-6f) {
			continue;
		}

		const float expanded_x = (c[prev] * ny[i] - ny[prev] * c[i]) / det;
		const float expanded_y = (nx[prev] * c[i] - c[prev] * nx[i]) / det;

		if (isfinite(expanded_x) && isfinite(expanded_y)) {
			float dx = expanded_x - x[i];
			float dy = expanded_y - y[i];
			const float dist2 = dx * dx + dy * dy;
			const float max_dist = DTTR_MESH_SEAM_MAX_VERTEX_NUDGE_PHYSICAL_PX;

			if (isfinite(dist2) && dist2 > max_dist * max_dist) {
				const float scale = max_dist / sqrtf(dist2);
				dx *= scale;
				dy *= scale;
			}

			tri[i].x = (x[i] + dx) * px_to_logical_x;
			tri[i].y = (y[i] + dy) * px_to_logical_y;
		}
	}
}

// Applies conservative overlap to a triangle list in-place.
void dttr_graphics_fill_mesh_seams(
	DTTR_Vertex *verts,
	uint32_t count,
	int logical_width,
	int logical_height,
	int render_width,
	int render_height
) {
	if (!verts || count < 3 || logical_width <= 0 || logical_height <= 0
		|| render_width <= 0 || render_height <= 0) {
		return;
	}

	const float logical_to_px_x = (float)render_width / (float)logical_width;
	const float logical_to_px_y = (float)render_height / (float)logical_height;
	const float px_to_logical_x = (float)logical_width / (float)render_width;
	const float px_to_logical_y = (float)logical_height / (float)render_height;

	for (uint32_t i = 0; i + 2 < count; i += 3) {
		fill_mesh_seam_triangle(
			&verts[i],
			logical_to_px_x,
			logical_to_px_y,
			px_to_logical_x,
			px_to_logical_y
		);
	}
}
