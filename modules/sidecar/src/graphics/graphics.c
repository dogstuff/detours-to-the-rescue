#include "graphics_private.h"
#include "hooks_private.h"
#include "resize_private.h"

#include <dttr_config.h>
#include <dttr_errors.h>
#include <dttr_log.h>
#include <dttr_sdl.h>

#include <stddef.h>
#include <windows.h>

#ifdef DTTR_MODS_ENABLED
#include "../timing_private.h"
#include "mods/mods_private.h"
#endif
#include "sds.h"
#include "sidecar_private.h"

#define DTTR_MIN_WINDOW_DIM 64
#define BACKEND_COUNT SDL_arraysize(backend_candidates)

typedef struct {
	Uint64 window_flags;
	bool (*init)(DTTR_BackendState *state);
} backend_candidate;

static const backend_candidate backend_candidates[] = {
	{0, dttr_graphics_sdl3gpu_init},
	{SDL_WINDOW_OPENGL, dttr_graphics_opengl_init},
};

#define IDX_SDL_GPU 0
#define IDX_OPENGL 1

static sds window_title = NULL;

typedef struct {
	int start;
	int end;
} backend_range;

// Show renderer and pixel size in the window title.
static void update_window_title(const DTTR_BackendState *state) {
	int w = 0, h = 0;
	SDL_GetWindowSizeInPixels(state->window, &w, &h);

	const char *driver = state->renderer ? state->renderer->get_driver_name(state)
										 : "unknown";

	if (!window_title) {
		window_title = sdsempty();
	}

	sdsclear(window_title);
	window_title = sdscatprintf(
		window_title,
#ifdef DTTR_MODS_ENABLED
		"102 Dalmatians - DttR - Modding - " DTTR_VERSION " - %s - %dx%d",
#else
		"102 Dalmatians - DttR - " DTTR_VERSION " - %s - %dx%d",
#endif
		driver ? driver : "unknown",
		w,
		h
	);

	SDL_SetWindowTitle(state->window, window_title);
}

// Clamp bogus render sizes to the game default.
static int clamp_dim(int value, int fallback) {
	return (value < DTTR_MIN_WINDOW_DIM) ? fallback : value;
}

// Derives the internal render target size from logical-scaling settings and a pixel size.
static DTTR_Size select_render_resolution(
	const DTTR_BackendState *state,
	DTTR_Size window_pixels
) {
	return dttr_select_render_resolution(
		dttr_config.scaling_method,
		dttr_config.scaling_fit,
		(DTTR_Size){state->logical_width, state->logical_height},
		(DTTR_Size){dttr_config.window_width, dttr_config.window_height},
		window_pixels
	);
}

// Resizes backend render targets only when the selected logical resolution has changed.
static void refresh_render_resolution(DTTR_BackendState *state, DTTR_Size window_pixels) {
	const DTTR_Size size = select_render_resolution(state, window_pixels);

	if (size.width == state->width && size.height == state->height) {
		return;
	}

	if (!state->renderer->resize(state, size.width, size.height)) {
		DTTR_LOG_WARN("Failed to resize render targets to %dx%d", size.width, size.height);
	}
}

// Seeds renderer-independent state before either SDL GPU or OpenGL owns the backend.
static void init_common_state(DTTR_BackendState *state) {
	dttr_graphics_mat4_identity(state->proj);
	dttr_graphics_mat4_identity(state->view);
	dttr_graphics_mat4_identity(state->model);

	state->texture_mutex = SDL_CreateMutex();
	kv_init(state->pending_upload_indices);
	kv_init(state->batch_records);
	dttr_graphics_surface_texture_cache_reset();

	state->clear_color = (SDL_FColor){0, 0, 0, 1};
	state->depth_test = true;
	state->depth_write = true;
	state->blend_enabled = false;
	state->addr_u = DTTR_TEXADDR_WRAP;
	state->addr_v = DTTR_TEXADDR_WRAP;
	state->blend_src = DTTR_BLEND_ONE;
	state->blend_dst = DTTR_BLEND_ZERO;
	state->stage_color_op = DTTR_D3DTOP_MODULATE;
	state->stage_color_arg1 = DTTR_D3DTA_TEXTURE;
	state->stage_color_arg2 = DTTR_D3DTA_DIFFUSE;
	state->stage_alpha_op = DTTR_D3DTOP_SELECTARG1;
	state->stage_alpha_arg1 = DTTR_D3DTA_TEXTURE;
	state->stage_alpha_arg2 = DTTR_D3DTA_DIFFUSE;

	state->viewport_x = 0;
	state->viewport_y = 0;
	state->viewport_w = state->logical_width;
	state->viewport_h = state->logical_height;
	state->viewport_min_z = 0.0f;
	state->viewport_max_z = 1.0f;

	state->initialized = true;
	state->gpu_thread_id = SDL_GetCurrentThreadID();
}

// Retrieves the native Win32 handle that the original game and modding callbacks expect.
static HWND get_hwnd(SDL_Window *window) {
	if (!window) {
		return NULL;
	}

	const SDL_PropertiesID props = SDL_GetWindowProperties(window);
	return (HWND)SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
}

#ifdef DTTR_MODS_ENABLED
// Scales mod coordinates from the original 480-line game space into the active
// render target.
static float graphics_scale(uint32_t height) {
	return height > 0 ? (float)height / 480.0f : 1.0f;
}

// Reads the real SDL window size and falls back to render-target dimensions during
// early startup.
static void graphics_window_size(const DTTR_BackendState *state, uint32_t *w, uint32_t *h) {
	int win_w = 0;
	int win_h = 0;
	if (state && state->window) {
		SDL_GetWindowSizeInPixels(state->window, &win_w, &win_h);
	}

	*w = (uint32_t)((win_w > 0) ? win_w : (state ? state->width : 0));
	*h = (uint32_t)((win_h > 0) ? win_h : (state ? state->height : 0));
}

// Builds the per-frame mod payload from the current game and window dimensions.
static DTTR_Mods_FrameContext graphics_frame_context(const DTTR_BackendState *state) {
	uint32_t window_w = 0;
	uint32_t window_h = 0;
	const uint32_t game_w = (uint32_t)(state ? state->width : 0);
	const uint32_t game_h = (uint32_t)(state ? state->height : 0);
	graphics_window_size(state, &window_w, &window_h);
	return (DTTR_Mods_FrameContext){
		.frame_index = state ? state->frame_index : 0,
		.window_w = window_w,
		.window_h = window_h,
		.game_x = 0,
		.game_y = 0,
		.game_w = game_w,
		.game_h = game_h,
		.scale = graphics_scale(game_h),
	};
}

// Converts the active SDL window into the mod window payload used by lifecycle
// callbacks.
static DTTR_Mods_WindowContext graphics_window_context(const DTTR_BackendState *state) {
	uint32_t window_w = 0;
	uint32_t window_h = 0;
	graphics_window_size(state, &window_w, &window_h);
	return (DTTR_Mods_WindowContext){
		.window = state ? state->window : NULL,
		.hwnd = state ? get_hwnd(state->window) : NULL,
		.window_w = window_w,
		.window_h = window_h,
	};
}

// Maps the selected renderer implementation to the public mod backend enum.
static DTTR_Mods_GraphicsBackend graphics_backend(const DTTR_BackendState *state) {
	if (!state) {
		return DTTR_MODS_GRAPHICS_BACKEND_UNKNOWN;
	}

	switch (state->backend_type) {
	case DTTR_BACKEND_SDL_GPU:
		return DTTR_MODS_GRAPHICS_BACKEND_SDL_GPU;
	case DTTR_BACKEND_OPENGL:
		return DTTR_MODS_GRAPHICS_BACKEND_OPENGL;
	default:
		return DTTR_MODS_GRAPHICS_BACKEND_UNKNOWN;
	}
}

static DTTR_Mods_GraphicsContext graphics_context(const DTTR_BackendState *state) {
	return (DTTR_Mods_GraphicsContext){
		.window = state ? state->window : NULL,
		.hwnd = state ? get_hwnd(state->window) : NULL,
		.backend = graphics_backend(state),
		.driver_name = (state && state->renderer)
						   ? state->renderer->get_driver_name(state)
						   : NULL,
		.render_w = (uint32_t)(state ? state->width : 0),
		.render_h = (uint32_t)(state ? state->height : 0),
	};
}

static void call_frame_mod(DTTR_BackendState *state, DTTR_Mods_FrameBeginFn callback) {
	const DTTR_Mods_FrameContext ctx = graphics_frame_context(state);
	callback(&ctx);
}

static void call_window_mod(DTTR_BackendState *state, DTTR_Mods_WindowCreatedFn callback) {
	const DTTR_Mods_WindowContext ctx = graphics_window_context(state);
	callback(&ctx);
}

static void call_graphics_mod(
	DTTR_BackendState *state,
	DTTR_Mods_GraphicsDeviceCreatedFn callback
) {
	const DTTR_Mods_GraphicsContext ctx = graphics_context(state);
	callback(&ctx);
}

void dttr_graphics_mod_frame_begin(DTTR_BackendState *state) {
	call_frame_mod(state, dttr_mods_frame_begin);
}

void dttr_graphics_mod_before_game_frame() {
	// frame_active remains true so native mod draws join the open frame.
	dttr_timing_before_render_frame(
		dttr_timing_render_reuses_previous_sim_state(),
		dttr_backend.frame_active
	);
}

void dttr_graphics_mod_after_game_frame() {
	dttr_timing_after_render_frame(dttr_timing_render_reuses_previous_sim_state());
}

void dttr_graphics_mod_before_present() {
	dttr_timing_before_present_frame();
}

void dttr_graphics_mod_after_present() {
	dttr_timing_after_present_frame();
}

void dttr_graphics_mod_frame_end(DTTR_BackendState *state) {
	call_frame_mod(state, dttr_mods_frame_end);
}

void dttr_graphics_mod_window_created(DTTR_BackendState *state) {
	call_window_mod(state, dttr_mods_window_created);
}

void dttr_graphics_mod_window_resized(DTTR_BackendState *state) {
	call_window_mod(state, dttr_mods_window_resized);
}

void dttr_graphics_mod_window_destroying(DTTR_BackendState *state) {
	call_window_mod(state, dttr_mods_window_destroying);
}

void dttr_graphics_mod_device_created(DTTR_BackendState *state) {
	call_graphics_mod(state, dttr_mods_graphics_device_created);
}

void dttr_graphics_mod_device_lost(DTTR_BackendState *state) {
	call_graphics_mod(state, dttr_mods_graphics_device_lost);
}

void dttr_graphics_mod_device_restored(DTTR_BackendState *state) {
	call_graphics_mod(state, dttr_mods_graphics_device_restored);
}

void dttr_graphics_mod_device_destroying(DTTR_BackendState *state) {
	call_graphics_mod(state, dttr_mods_graphics_device_destroying);
}
#endif

void dttr_graphics_mod_present_rect_before() {
	dttr_graphics_mod_before_present();
}

void dttr_graphics_mod_present_rect_after() {
	dttr_graphics_mod_after_present();
}

// Notify mods before the SDL handle is invalid.
static void destroy_window(DTTR_BackendState *state) {
	if (!state->window) {
		return;
	}

	dttr_graphics_mod_window_destroying(state);
	SDL_DestroyWindow(state->window);
	state->window = NULL;
}

// AUTO keeps fallback order; explicit APIs probe one backend.
static backend_range select_backend_range(DTTR_GraphicsAPI api) {
	switch (api) {
	case DTTR_GRAPHICS_API_OPENGL:
		return (backend_range){IDX_OPENGL, IDX_OPENGL};
	case DTTR_GRAPHICS_API_AUTO:
		return (backend_range){IDX_SDL_GPU, BACKEND_COUNT - 1};
	default:
		return (backend_range){IDX_SDL_GPU, IDX_SDL_GPU};
	}
}

HWND dttr_graphics_init() {
	DTTR_BackendState *state = &dttr_backend;

	if (state->initialized && state->window) {
		return get_hwnd(state->window);
	}

	if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
		DTTR_LOG_ERROR("SDL_Init failed: %s", SDL_GetError());
		return NULL;
	}

	int win_w = clamp_dim(dttr_config.window_width, WINDOW_WIDTH);
	int win_h = clamp_dim(dttr_config.window_height, WINDOW_HEIGHT);

	state->logical_width = WINDOW_WIDTH;
	state->logical_height = WINDOW_HEIGHT;
	const DTTR_Size render_size = select_render_resolution(state, (DTTR_Size){0, 0});
	state->width = render_size.width;
	state->height = render_size.height;

	const backend_range backend_range = select_backend_range(dttr_config.graphics_api);

	for (int i = backend_range.start; i <= backend_range.end; i++) {
		destroy_window(state);

		state->window = SDL_CreateWindow(
			"102 Dalmatians",
			win_w,
			win_h,
			SDL_WINDOW_RESIZABLE | backend_candidates[i].window_flags
		);

		if (!state->window) {
			continue;
		}

		if (!backend_candidates[i].init(state)) {
			continue;
		}

		dttr_graphics_mod_window_created(state);
		dttr_graphics_mod_device_created(state);
		dttr_graphics_mod_device_restored(state);

		if (dttr_config.fullscreen) {
			if (!SDL_SetWindowFullscreen(state->window, true)) {
				DTTR_LOG_WARN("SDL_SetWindowFullscreen failed: %s", SDL_GetError());
			}
		}

		init_common_state(state);
		update_window_title(state);

		return get_hwnd(state->window);
	}

	{
		sds msg;
		if (dttr_config.graphics_api == DTTR_GRAPHICS_API_AUTO) {
			msg = sdsnew("All graphics backends failed to initialize");
		} else {
			msg = sdscatprintf(
				sdsempty(),
				"Graphics backend '%s' failed to initialize. "
				"It may not be supported on this system.",
				DTTR_Config_GraphicsAPIName(dttr_config.graphics_api)
			);
		}

		DTTR_LOG_ERROR("%s", msg);
		DTTR_Errors_ShowMessage("DttR: Error", msg);
		sdsfree(msg);
	}

	destroy_window(state);

	return NULL;
}

// Updates the game-space resolution and resizes backend targets when scaling settings
// require it.
void dttr_graphics_set_logical_resolution(int width, int height) {
	DTTR_BackendState *state = &dttr_backend;

	state->logical_width = clamp_dim(width, WINDOW_WIDTH);
	state->logical_height = clamp_dim(height, WINDOW_HEIGHT);

	if (state->viewport_w <= 0 || state->viewport_h <= 0
		|| (state->viewport_x == 0 && state->viewport_y == 0)) {
		state->viewport_w = state->logical_width;
		state->viewport_h = state->logical_height;
	}

	DTTR_Size window_pixels = {0};
	if (state->window) {
		SDL_GetWindowSizeInPixels(
			state->window,
			&window_pixels.width,
			&window_pixels.height
		);
	}

	refresh_render_resolution(state, window_pixels);
}

// Returns the SDL window owned by the active graphics backend.
SDL_Window *dttr_graphics_get_window() {
	return dttr_backend.window;
}

// Returns the SDL GPU device when the active backend owns one.
SDL_GPUDevice *dttr_graphics_get_device() {
	return dttr_backend.device;
}

// Applies a user window resize to render targets before notifying mods.
void dttr_graphics_handle_window_resize(int width, int height) {
	if (width < DTTR_MIN_WINDOW_DIM || height < DTTR_MIN_WINDOW_DIM) {
		return;
	}

	refresh_render_resolution(&dttr_backend, (DTTR_Size){width, height});
	update_window_title(&dttr_backend);
	dttr_graphics_mod_window_resized(&dttr_backend);
}

// Starts backend frame rendering after mod frame-begin callbacks run.
void dttr_graphics_begin_frame() {
	DTTR_BackendState *state = &dttr_backend;

	if (state->frame_active || state->in_frame_callback) {
		return;
	}

	state->renderer->begin_frame(state);
}

// Finishes backend frame rendering before mod frame-end callbacks run.
void dttr_graphics_end_frame() {
	DTTR_BackendState *state = &dttr_backend;

	// in_frame_callback: a mod callback running inside end_frame re-entered the
	// present path (e.g. injected game code reached Flip); the outer end_frame
	// finishes this frame, so the re-entrant call must not.
	if (!state->frame_active || state->in_frame_callback) {
		return;
	}

	// Keep the frame open across the BEFORE_RENDER mod callback so native draws
	// injected by mods.
	state->in_frame_callback = true;
	dttr_graphics_mod_before_game_frame();
	state->in_frame_callback = false;
	state->frame_active = false;

	state->renderer->end_frame(state);
}

// Uploads a decoded BGRA movie frame through the active renderer for fullscreen playback.
bool dttr_graphics_present_video_frame_bgra(
	const uint8_t *pixels,
	int width,
	int height,
	int stride
) {
	DTTR_BackendState *state = &dttr_backend;

	if (!pixels || width <= 0 || height <= 0 || stride < (width * 4)) {
		return false;
	}

	return state->renderer->present_video_frame_bgra(state, pixels, width, height, stride);
}

// Tears down graphics lifecycle callbacks, backend resources, and the SDL window in
// order.
void dttr_graphics_cleanup() {
	dttr_graphics_mod_device_lost(&dttr_backend);
	dttr_graphics_mod_device_destroying(&dttr_backend);

	DTTR_BackendState *state = &dttr_backend;

	if (state->renderer) {
		state->renderer->cleanup(state);
		state->renderer = NULL;
	}

	if (state->texture_mutex) {
		SDL_DestroyMutex(state->texture_mutex);
		state->texture_mutex = NULL;
	}

	dttr_graphics_surface_texture_cache_reset();
	kv_destroy(state->pending_upload_indices);
	kv_destroy(state->batch_records);
	kv_init(state->pending_upload_indices);
	kv_init(state->batch_records);

	destroy_window(state);
	state->initialized = false;

	SDL_Quit();
}
