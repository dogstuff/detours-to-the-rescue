#include "graphics_internal.h"

#include "log.h"

#include <stddef.h>
#include <windows.h>

#include "dttr_hooks_graphics.h"
#include "dttr_sidecar.h"
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
#ifdef DTTR_COMPONENTS_ENABLED
		"102 Dalmatians - DttR - Components - " DTTR_VERSION " - %s - %dx%d",
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
		log_warn("Failed to resize render targets to %dx%d", rw, rh);
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
	const SDL_PropertiesID props = SDL_GetWindowProperties(window);
	return (HWND)SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
}

HWND dttr_graphics_init(void) {
	DTTR_BackendState *state = &g_dttr_backend;

	if (state->m_initialized && state->m_window) {
		return s_get_hwnd(state->m_window);
	}

	if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
		log_error("SDL_Init failed: %s", SDL_GetError());
		return NULL;
	}

	int win_w = s_clamp_dim(g_dttr_config.m_window_width, WINDOW_WIDTH);
	int win_h = s_clamp_dim(g_dttr_config.m_window_height, WINDOW_HEIGHT);

	state->m_logical_width = WINDOW_WIDTH;
	state->m_logical_height = WINDOW_HEIGHT;
	s_select_render_resolution(state, &state->m_width, &state->m_height);

	int start = S_IDX_SDL_GPU;
	int end = S_IDX_SDL_GPU;

	switch (g_dttr_config.m_graphics_api) {
	case DTTR_GRAPHICS_API_OPENGL:
		start = end = S_IDX_OPENGL;
		break;
	case DTTR_GRAPHICS_API_AUTO:
		end = S_BACKEND_COUNT - 1;
		break;
	default:
		break;
	}

	for (int i = start; i <= end; i++) {
		if (state->m_window) {
			SDL_DestroyWindow(state->m_window);
			state->m_window = NULL;
		}

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

		log_error("%s", msg);

		// We don't use SDL_ShowSimpleMessage here because
		// it doesn't wanna work here?? (idk bro)
		MessageBoxA(NULL, msg, "DttR: Error", MB_OK | MB_ICONERROR);
		sdsfree(msg);
	}

	if (state->m_window) {
		SDL_DestroyWindow(state->m_window);
		state->m_window = NULL;
	}

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
	dttr_graphics_hooks_cleanup(dttr_game_api_get_ctx());

	DTTR_BackendState *state = &g_dttr_backend;

	state->m_renderer->cleanup(state);

	if (state->m_texture_mutex) {
		SDL_DestroyMutex(state->m_texture_mutex);
		state->m_texture_mutex = NULL;
	}

	dttr_graphics_surface_texture_cache_reset();
	kv_destroy(state->m_pending_upload_indices);
	kv_destroy(state->m_batch_records);
	kv_init(state->m_pending_upload_indices);
	kv_init(state->m_batch_records);

	if (state->m_window) {
		SDL_DestroyWindow(state->m_window);
	}

	SDL_Quit();
}
