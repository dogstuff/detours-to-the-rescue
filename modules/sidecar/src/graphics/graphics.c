#include "graphics_private.h"

#include <dttr_errors.h>
#include <dttr_log.h>

#include <stddef.h>
#include <windows.h>

#include "dttr_hooks_graphics.h"
#ifdef DTTR_MODDING_ENABLED
#include "components/components_private.h"
#endif
#include "dttr_sidecar.h"
#include "game_api_private.h"
#include "sds.h"

#define DTTR_MIN_WINDOW_DIM 64
#define S_BACKEND_COUNT SDL_arraysize(s_backend_candidates)

typedef struct {
	Uint64 m_window_flags;
	bool (*m_init)(DTTR_BackendState *state);
} S_BackendCandidate;

static const S_BackendCandidate s_backend_candidates[] = {
	{0, dttr_graphics_sdl3gpu_init},
	{SDL_WINDOW_OPENGL, dttr_graphics_opengl_init},
};

#define S_IDX_SDL_GPU 0
#define S_IDX_OPENGL 1

static sds s_window_title = NULL;

typedef struct {
	int start;
	int end;
} S_BackendRange;

static void s_update_window_title(const DTTR_BackendState *state) {
	int w = 0, h = 0;
	SDL_GetWindowSizeInPixels(state->m_window, &w, &h);

	const char *driver = state->m_renderer ? state->m_renderer->get_driver_name(state)
										   : "unknown";

	if (!s_window_title) {
		s_window_title = sdsempty();
	}

	sdsclear(s_window_title);
	s_window_title = sdscatprintf(
		s_window_title,
#ifdef DTTR_MODDING_ENABLED
		"102 Dalmatians - DttR - Modding - " DTTR_VERSION " - %s - %dx%d",
#else
		"102 Dalmatians - DttR - " DTTR_VERSION " - %s - %dx%d",
#endif
		driver ? driver : "unknown",
		w,
		h
	);

	SDL_SetWindowTitle(state->m_window, s_window_title);
}

static int s_clamp_dim(int value, int fallback) {
	return (value < DTTR_MIN_WINDOW_DIM) ? fallback : value;
}

static void s_select_render_resolution(
	const DTTR_BackendState *state,
	int *out_width,
	int *out_height
) {
	int width = state->m_logical_width;
	int height = state->m_logical_height;

	if (g_dttr_config.m_scaling_method == DTTR_SCALING_METHOD_LOGICAL) {
		int window_px_width = 0;
		int window_px_height = 0;
		int target_width = g_dttr_config.m_window_width;
		int target_height = g_dttr_config.m_window_height;

		if (state->m_window
			&& SDL_GetWindowSizeInPixels(
				state->m_window,
				&window_px_width,
				&window_px_height
			)) {
			if (window_px_width > target_width) {
				target_width = window_px_width;
			}

			if (window_px_height > target_height) {
				target_height = window_px_height;
			}
		}

		if (g_dttr_config.m_scaling_fit == DTTR_SCALING_MODE_STRETCH) {
			width = target_width;
			height = target_height;
		} else {
			const int lw = s_clamp_dim(state->m_logical_width, WINDOW_WIDTH);
			const int lh = s_clamp_dim(state->m_logical_height, WINDOW_HEIGHT);
			const float scale = SDL_min(
				(float)target_width / (float)lw,
				(float)target_height / (float)lh
			);

			width = (int)((float)lw * scale);
			height = (int)((float)lh * scale);
		}
	}

	*out_width = s_clamp_dim(width, WINDOW_WIDTH);
	*out_height = s_clamp_dim(height, WINDOW_HEIGHT);
}

static void s_refresh_render_resolution(DTTR_BackendState *state) {
	int rw = state->m_width;
	int rh = state->m_height;
	s_select_render_resolution(state, &rw, &rh);

	if (rw == state->m_width && rh == state->m_height) {
		return;
	}

	if (!state->m_renderer->resize(state, rw, rh)) {
		DTTR_LOG_WARN("Failed to resize render targets to %dx%d", rw, rh);
	}
}

static void s_init_common_state(DTTR_BackendState *state) {
	dttr_graphics_mat4_identity(state->m_proj);
	dttr_graphics_mat4_identity(state->m_view);
	dttr_graphics_mat4_identity(state->m_model);

	state->m_texture_mutex = SDL_CreateMutex();
	kv_init(state->m_pending_upload_indices);
	kv_init(state->m_batch_records);
	dttr_graphics_surface_texture_cache_reset();

	state->m_clear_color = (SDL_FColor){0, 0, 0, 1};
	state->m_depth_test = true;
	state->m_depth_write = true;
	state->m_blend_enabled = false;
	state->m_addr_u = DTTR_TEXADDR_WRAP;
	state->m_addr_v = DTTR_TEXADDR_WRAP;
	state->m_blend_dst = DTTR_BLEND_ZERO;
	state->m_stage_color_op = DTTR_D3DTOP_MODULATE;
	state->m_stage_color_arg1 = DTTR_D3DTA_TEXTURE;
	state->m_stage_color_arg2 = DTTR_D3DTA_DIFFUSE;
	state->m_stage_alpha_op = DTTR_D3DTOP_SELECTARG1;
	state->m_stage_alpha_arg1 = DTTR_D3DTA_TEXTURE;
	state->m_stage_alpha_arg2 = DTTR_D3DTA_DIFFUSE;

	state->m_viewport_x = 0;
	state->m_viewport_y = 0;
	state->m_viewport_w = state->m_logical_width;
	state->m_viewport_h = state->m_logical_height;
	state->m_viewport_min_z = 0.0f;
	state->m_viewport_max_z = 1.0f;

	state->m_initialized = true;
	state->m_gpu_thread_id = SDL_GetCurrentThreadID();
}

static HWND s_get_hwnd(SDL_Window *window) {
	if (!window) {
		return NULL;
	}
	const SDL_PropertiesID props = SDL_GetWindowProperties(window);
	return (HWND)SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
}

#ifdef DTTR_MODDING_ENABLED
static float s_graphics_scale(uint32_t height) {
	return height > 0 ? (float)height / 480.0f : 1.0f;
}

static void s_graphics_window_size(
	const DTTR_BackendState *state,
	uint32_t *w,
	uint32_t *h
) {
	int win_w = 0;
	int win_h = 0;
	if (state && state->m_window) {
		SDL_GetWindowSizeInPixels(state->m_window, &win_w, &win_h);
	}
	*w = (uint32_t)((win_w > 0) ? win_w : (state ? state->m_width : 0));
	*h = (uint32_t)((win_h > 0) ? win_h : (state ? state->m_height : 0));
}

static DTTR_FrameContext s_graphics_frame_context(const DTTR_BackendState *state) {
	uint32_t window_w = 0;
	uint32_t window_h = 0;
	const uint32_t game_w = (uint32_t)(state ? state->m_width : 0);
	const uint32_t game_h = (uint32_t)(state ? state->m_height : 0);
	s_graphics_window_size(state, &window_w, &window_h);
	return (DTTR_FrameContext){
		.m_frame_index = state ? state->m_frame_index : 0,
		.m_window_w = window_w,
		.m_window_h = window_h,
		.m_game_x = 0,
		.m_game_y = 0,
		.m_game_w = game_w,
		.m_game_h = game_h,
		.m_scale = s_graphics_scale(game_h),
	};
}

static DTTR_PresentContext s_graphics_present_context(
	const DTTR_BackendState *state,
	uint32_t game_x,
	uint32_t game_y,
	uint32_t game_w,
	uint32_t game_h,
	bool imgui_frame_active,
	bool overlay_rendered
) {
	uint32_t window_w = 0;
	uint32_t window_h = 0;
	s_graphics_window_size(state, &window_w, &window_h);
	return (DTTR_PresentContext){
		.m_frame_index = state ? state->m_frame_index : 0,
		.m_window_w = window_w,
		.m_window_h = window_h,
		.m_game_x = game_x,
		.m_game_y = game_y,
		.m_game_w = game_w,
		.m_game_h = game_h,
		.m_scale = s_graphics_scale(game_h),
		.m_imgui_frame_active = imgui_frame_active,
		.m_overlay_rendered = overlay_rendered,
	};
}

static DTTR_WindowContext s_graphics_window_context(const DTTR_BackendState *state) {
	uint32_t window_w = 0;
	uint32_t window_h = 0;
	s_graphics_window_size(state, &window_w, &window_h);
	return (DTTR_WindowContext){
		.m_window = state ? state->m_window : NULL,
		.m_hwnd = state ? s_get_hwnd(state->m_window) : NULL,
		.m_window_w = window_w,
		.m_window_h = window_h,
	};
}

static DTTR_GraphicsBackend s_graphics_backend(const DTTR_BackendState *state) {
	if (!state) {
		return DTTR_GRAPHICS_BACKEND_UNKNOWN;
	}
	switch (state->m_backend_type) {
	case DTTR_BACKEND_SDL_GPU:
		return DTTR_GRAPHICS_BACKEND_SDL_GPU;
	case DTTR_BACKEND_OPENGL:
		return DTTR_GRAPHICS_BACKEND_OPENGL;
	default:
		return DTTR_GRAPHICS_BACKEND_UNKNOWN;
	}
}

static DTTR_GraphicsContext s_graphics_context(const DTTR_BackendState *state) {
	return (DTTR_GraphicsContext){
		.m_window = state ? state->m_window : NULL,
		.m_hwnd = state ? s_get_hwnd(state->m_window) : NULL,
		.m_backend = s_graphics_backend(state),
		.m_driver_name = (state && state->m_renderer)
							 ? state->m_renderer->get_driver_name(state)
							 : NULL,
		.m_render_w = (uint32_t)(state ? state->m_width : 0),
		.m_render_h = (uint32_t)(state ? state->m_height : 0),
	};
}

static void s_call_frame_component(
	DTTR_BackendState *state,
	DTTR_ComponentFrameBeginFn callback
) {
	const DTTR_FrameContext ctx = s_graphics_frame_context(state);
	callback(&ctx);
}

static void s_call_present_component(
	DTTR_BackendState *state,
	uint32_t game_x,
	uint32_t game_y,
	uint32_t game_w,
	uint32_t game_h,
	bool imgui_frame_active,
	bool overlay_rendered,
	DTTR_ComponentBeforePresentFn callback
) {
	const DTTR_PresentContext ctx = s_graphics_present_context(
		state,
		game_x,
		game_y,
		game_w,
		game_h,
		imgui_frame_active,
		overlay_rendered
	);
	callback(&ctx);
}

static void s_call_window_component(
	DTTR_BackendState *state,
	DTTR_ComponentWindowCreatedFn callback
) {
	const DTTR_WindowContext ctx = s_graphics_window_context(state);
	callback(&ctx);
}

static void s_call_graphics_component(
	DTTR_BackendState *state,
	DTTR_ComponentGraphicsDeviceCreatedFn callback
) {
	const DTTR_GraphicsContext ctx = s_graphics_context(state);
	callback(&ctx);
}

void dttr_graphics_component_frame_begin(DTTR_BackendState *state) {
	s_call_frame_component(state, dttr_components_frame_begin);
}

void dttr_graphics_component_before_game_frame(DTTR_BackendState *state) {
	s_call_frame_component(state, dttr_components_before_game_frame);
}

void dttr_graphics_component_after_game_frame(DTTR_BackendState *state) {
	s_call_frame_component(state, dttr_components_after_game_frame);
}

void dttr_graphics_component_before_present(
	DTTR_BackendState *state,
	uint32_t game_x,
	uint32_t game_y,
	uint32_t game_w,
	uint32_t game_h,
	bool imgui_frame_active,
	bool overlay_rendered
) {
	s_call_present_component(
		state,
		game_x,
		game_y,
		game_w,
		game_h,
		imgui_frame_active,
		overlay_rendered,
		dttr_components_before_present
	);
}

void dttr_graphics_component_after_present(
	DTTR_BackendState *state,
	uint32_t game_x,
	uint32_t game_y,
	uint32_t game_w,
	uint32_t game_h,
	bool imgui_frame_active,
	bool overlay_rendered
) {
	s_call_present_component(
		state,
		game_x,
		game_y,
		game_w,
		game_h,
		imgui_frame_active,
		overlay_rendered,
		dttr_components_after_present
	);
}

void dttr_graphics_component_frame_end(DTTR_BackendState *state) {
	s_call_frame_component(state, dttr_components_frame_end);
}

void dttr_graphics_component_window_created(DTTR_BackendState *state) {
	s_call_window_component(state, dttr_components_window_created);
}

void dttr_graphics_component_window_resized(DTTR_BackendState *state) {
	s_call_window_component(state, dttr_components_window_resized);
}

void dttr_graphics_component_window_destroying(DTTR_BackendState *state) {
	s_call_window_component(state, dttr_components_window_destroying);
}

void dttr_graphics_component_device_created(DTTR_BackendState *state) {
	s_call_graphics_component(state, dttr_components_graphics_device_created);
}

void dttr_graphics_component_device_lost(DTTR_BackendState *state) {
	s_call_graphics_component(state, dttr_components_graphics_device_lost);
}

void dttr_graphics_component_device_restored(DTTR_BackendState *state) {
	s_call_graphics_component(state, dttr_components_graphics_device_restored);
}

void dttr_graphics_component_device_destroying(DTTR_BackendState *state) {
	s_call_graphics_component(state, dttr_components_graphics_device_destroying);
}
#endif

static void s_destroy_window(DTTR_BackendState *state) {
	if (!state->m_window) {
		return;
	}

	dttr_graphics_component_window_destroying(state);
	SDL_DestroyWindow(state->m_window);
	state->m_window = NULL;
}

static S_BackendRange s_select_backend_range(DTTR_GraphicsApi api) {
	switch (api) {
	case DTTR_GRAPHICS_API_OPENGL:
		return (S_BackendRange){S_IDX_OPENGL, S_IDX_OPENGL};
	case DTTR_GRAPHICS_API_AUTO:
		return (S_BackendRange){S_IDX_SDL_GPU, S_BACKEND_COUNT - 1};
	default:
		return (S_BackendRange){S_IDX_SDL_GPU, S_IDX_SDL_GPU};
	}
}

HWND dttr_graphics_init(void) {
	DTTR_BackendState *state = &g_dttr_backend;

	if (state->m_initialized && state->m_window) {
		return s_get_hwnd(state->m_window);
	}

	if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
		DTTR_LOG_ERROR("SDL_Init failed: %s", SDL_GetError());
		return NULL;
	}

	int win_w = s_clamp_dim(g_dttr_config.m_window_width, WINDOW_WIDTH);
	int win_h = s_clamp_dim(g_dttr_config.m_window_height, WINDOW_HEIGHT);

	state->m_logical_width = WINDOW_WIDTH;
	state->m_logical_height = WINDOW_HEIGHT;
	s_select_render_resolution(state, &state->m_width, &state->m_height);

	const S_BackendRange backend_range = s_select_backend_range(
		g_dttr_config.m_graphics_api
	);

	for (int i = backend_range.start; i <= backend_range.end; i++) {
		s_destroy_window(state);

		state->m_window = SDL_CreateWindow(
			"102 Dalmatians",
			win_w,
			win_h,
			SDL_WINDOW_RESIZABLE | s_backend_candidates[i].m_window_flags
		);

		if (!state->m_window) {
			continue;
		}

		if (!s_backend_candidates[i].m_init(state)) {
			continue;
		}

		dttr_graphics_component_window_created(state);
		dttr_graphics_component_device_created(state);
		dttr_graphics_component_device_restored(state);

		if (g_dttr_config.m_fullscreen) {
			SDL_SetWindowFullscreen(state->m_window, true);
		}

		s_init_common_state(state);
		s_update_window_title(state);

		return s_get_hwnd(state->m_window);
	}

	{
		sds msg;
		if (g_dttr_config.m_graphics_api == DTTR_GRAPHICS_API_AUTO) {
			msg = sdsnew("All graphics backends failed to initialize");
		} else {
			msg = sdscatprintf(
				sdsempty(),
				"Graphics backend '%s' failed to initialize. "
				"It may not be supported on this system.",
				dttr_config_graphics_api_name(g_dttr_config.m_graphics_api)
			);
		}

		DTTR_LOG_ERROR("%s", msg);
		dttr_errors_show_message("DttR: Error", msg);
		sdsfree(msg);
	}

	s_destroy_window(state);

	return NULL;
}

void dttr_graphics_set_logical_resolution(int width, int height) {
	DTTR_BackendState *state = &g_dttr_backend;

	state->m_logical_width = s_clamp_dim(width, WINDOW_WIDTH);
	state->m_logical_height = s_clamp_dim(height, WINDOW_HEIGHT);

	if (state->m_viewport_w <= 0 || state->m_viewport_h <= 0
		|| (state->m_viewport_x == 0 && state->m_viewport_y == 0)) {
		state->m_viewport_w = state->m_logical_width;
		state->m_viewport_h = state->m_logical_height;
	}

	s_refresh_render_resolution(state);
}

SDL_Window *dttr_graphics_get_window(void) { return g_dttr_backend.m_window; }

SDL_GPUDevice *dttr_graphics_get_device(void) { return g_dttr_backend.m_device; }

void dttr_graphics_handle_window_resize(int width, int height) {
	if (width < DTTR_MIN_WINDOW_DIM || height < DTTR_MIN_WINDOW_DIM) {
		return;
	}

	s_refresh_render_resolution(&g_dttr_backend);
	s_update_window_title(&g_dttr_backend);
	dttr_graphics_component_window_resized(&g_dttr_backend);
}

void dttr_graphics_begin_frame(void) {
	DTTR_BackendState *state = &g_dttr_backend;

	if (state->m_frame_active) {
		return;
	}

	state->m_renderer->begin_frame(state);
}

void dttr_graphics_end_frame(void) {
	DTTR_BackendState *state = &g_dttr_backend;

	if (!state->m_frame_active) {
		return;
	}

	state->m_renderer->end_frame(state);
}

bool dttr_graphics_present_video_frame_bgra(
	const uint8_t *pixels,
	int width,
	int height,
	int stride
) {
	DTTR_BackendState *state = &g_dttr_backend;

	if (!pixels || width <= 0 || height <= 0 || stride < (width * 4)) {
		return false;
	}

	return state->m_renderer
		->present_video_frame_bgra(state, pixels, width, height, stride);
}

void dttr_graphics_cleanup(void) {
	dttr_graphics_component_device_lost(&g_dttr_backend);
	dttr_graphics_component_device_destroying(&g_dttr_backend);
	dttr_graphics_hooks_cleanup(dttr_game_api_get_ctx());

	DTTR_BackendState *state = &g_dttr_backend;

	if (state->m_renderer) {
		state->m_renderer->cleanup(state);
		state->m_renderer = NULL;
	}

	if (state->m_texture_mutex) {
		SDL_DestroyMutex(state->m_texture_mutex);
		state->m_texture_mutex = NULL;
	}

	dttr_graphics_surface_texture_cache_reset();
	kv_destroy(state->m_pending_upload_indices);
	kv_destroy(state->m_batch_records);
	kv_init(state->m_pending_upload_indices);
	kv_init(state->m_batch_records);

	s_destroy_window(state);

	SDL_Quit();
}
