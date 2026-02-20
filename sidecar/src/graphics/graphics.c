#include "graphics_internal.h"

#include "log.h"

#include <stddef.h>
#include <windows.h>

#include "dttr_hooks_graphics.h"
#include "dttr_sidecar.h"
#include "sds.h"

#define DTTR_MIN_WINDOW_DIM 64

static sds s_window_title = NULL;

static void s_update_window_title(const DTTR_BackendState *state) {
	int w = 0, h = 0;
	SDL_GetWindowSizeInPixels(state->m_window, &w, &h);
	const char *const driver = SDL_GetGPUDeviceDriver(state->m_device);

	if (!s_window_title)
		s_window_title = sdsempty();

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

// Normalizes a requested dimension to a safe renderable size
static int s_clamp_dim(int value, int fallback) {
	if (value < DTTR_MIN_WINDOW_DIM)
		return fallback;

	return value;
}

// Converts config integer sample count into SDL GPU sample-count enum
static SDL_GPUSampleCount s_msaa_sample_count_from_config(int value) {
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

// Returns readable integer form for logging a sample-count enum
static int s_msaa_sample_count_to_int(SDL_GPUSampleCount value) {
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

// Selects the runtime MSAA sample count based on config and GPU support
static SDL_GPUSampleCount s_select_msaa_sample_count(DTTR_BackendState *state) {
	const SDL_GPUSampleCount requested = s_msaa_sample_count_from_config(
		g_dttr_config.m_msaa_samples
	);
	if (requested == SDL_GPU_SAMPLECOUNT_1)
		return SDL_GPU_SAMPLECOUNT_1;

	const SDL_GPUTextureFormat swapchain_fmt = SDL_GetGPUSwapchainTextureFormat(
		state->m_device,
		state->m_window
	);
	const bool color_supported = SDL_GPUTextureSupportsSampleCount(
		state->m_device,
		swapchain_fmt,
		requested
	);
	const bool depth_supported = SDL_GPUTextureSupportsSampleCount(
		state->m_device,
		SDL_GPU_TEXTUREFORMAT_D32_FLOAT,
		requested
	);

	if (color_supported && depth_supported)
		return requested;

	log_warn(
		"Requested MSAA x%d is unsupported on this device/format. "
		"Falling back to x1.",
		s_msaa_sample_count_to_int(requested)
	);
	return SDL_GPU_SAMPLECOUNT_1;
}

// Selects the actual render target resolution from current config/state
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
			if (window_px_width > target_width)
				target_width = window_px_width;

			if (window_px_height > target_height)
				target_height = window_px_height;
		}

		if (g_dttr_config.m_scaling_fit == DTTR_SCALING_MODE_STRETCH) {
			width = target_width;
			height = target_height;
		} else {
			const int logical_width = s_clamp_dim(state->m_logical_width, WINDOW_WIDTH);
			const int logical_height = s_clamp_dim(state->m_logical_height, WINDOW_HEIGHT);
			const float sx = (float)target_width / (float)logical_width;
			const float sy = (float)target_height / (float)logical_height;
			const float scale = SDL_min(sx, sy);

			width = (int)((float)logical_width * scale);
			height = (int)((float)logical_height * scale);
		}
	}

	*out_width = s_clamp_dim(width, WINDOW_WIDTH);
	*out_height = s_clamp_dim(height, WINDOW_HEIGHT);
}

// Updates render textures when render resolution policy changes
static void s_refresh_render_resolution(DTTR_BackendState *state) {
	int render_width = state->m_width;
	int render_height = state->m_height;
	s_select_render_resolution(state, &render_width, &render_height);

	if (render_width == state->m_width && render_height == state->m_height)
		return;

	if (!dttr_graphics_resize_render_textures(render_width, render_height)) {
		log_warn("Failed to resize render textures to %dx%d", render_width, render_height);
		return;
	}
}

// Attempts to create and validate an SDL GPU device for one backend driver name
static bool s_try_create_device_for_driver(
	DTTR_BackendState *state,
	const SDL_GPUShaderFormat requested_formats,
	const char *driver
) {
	state->m_device = SDL_CreateGPUDevice(requested_formats, false, driver);

	if (!state->m_device) {
		log_warn(
			"Failed to create SDL GPU device for driver '%s' "
			"(requested_formats=0x%x): %s",
			driver ? driver : "default",
			(unsigned int)requested_formats,
			SDL_GetError()
		);
		return false;
	}

	if (!SDL_ClaimWindowForGPUDevice(state->m_device, state->m_window)) {
		log_warn(
			"Failed to claim window for SDL GPU driver '%s': %s",
			driver ? driver : "default",
			SDL_GetError()
		);
		SDL_DestroyGPUDevice(state->m_device);
		state->m_device = NULL;
		return false;
	}

	const SDL_GPUShaderFormat available_formats = SDL_GetGPUShaderFormats(state->m_device);
	const char *active_driver = SDL_GetGPUDeviceDriver(state->m_device);
	state->m_shader_format = dttr_graphics_select_shader_format_for_driver(
		active_driver,
		available_formats
	);

	if (state->m_shader_format != SDL_GPU_SHADERFORMAT_INVALID)
		return true;

	log_warn(
		"SDL GPU driver '%s' does not support required shader format. Available "
		"mask=0x%x",
		active_driver ? active_driver : "unknown",
		(unsigned int)available_formats
	);
	SDL_ReleaseWindowFromGPUDevice(state->m_device, state->m_window);
	SDL_DestroyGPUDevice(state->m_device);
	state->m_device = NULL;
	return false;
}

// Maps config graphics API mode to SDL GPU driver name (NULL means auto-select)
static const char *s_graphics_api_driver_name(DTTR_GraphicsApi api) {
	switch (api) {
	case DTTR_GRAPHICS_API_VULKAN:
		return "vulkan";
	case DTTR_GRAPHICS_API_DIRECT3D12:
		return "direct3d12";
	case DTTR_GRAPHICS_API_METAL:
		return "metal";
	default:
		return NULL;
	}
}

// Tries supported backend drivers in order until one produces a usable GPU device
static bool s_create_device(DTTR_BackendState *state) {
	const SDL_GPUShaderFormat requested_formats = dttr_graphics_requested_shader_formats();
	const char *const requested_driver = s_graphics_api_driver_name(
		g_dttr_config.m_graphics_api
	);

	if (requested_driver) {
		if (s_try_create_device_for_driver(state, requested_formats, requested_driver))
			return true;

		log_error(
			"GPU device creation failed for configured graphics_api='%s'; no fallback "
			"APIs "
			"will be attempted",
			requested_driver
		);
		return false;
	}

	const char *const driver_candidates[] = {
		"vulkan",
		"direct3d12",
		"metal",
		NULL, // SDL default
	};

	for (size_t i = 0; i < SDL_arraysize(driver_candidates); i++) {
		if (s_try_create_device_for_driver(state, requested_formats, driver_candidates[i]))
			return true;
	}

	log_error("GPU device creation failed for all supported APIs (d3d12/metal/vulkan)");
	return false;
}

// Initialize SDL, window, GPU device, and backend state; return the native window handle.
HWND dttr_graphics_init(void) {
	DTTR_BackendState *state = &g_dttr_backend;

	if (state->m_initialized && state->m_window) {
		const SDL_PropertiesID props = SDL_GetWindowProperties(state->m_window);
		return (
			HWND
		)SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
	}

	if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
		log_error("SDL_Init failed: %s", SDL_GetError());
		return NULL;
	}

	int initial_window_width = g_dttr_config.m_window_width;
	int initial_window_height = g_dttr_config.m_window_height;

	if (initial_window_width < DTTR_MIN_WINDOW_DIM)
		initial_window_width = WINDOW_WIDTH;

	if (initial_window_height < DTTR_MIN_WINDOW_DIM)
		initial_window_height = WINDOW_HEIGHT;

	state->m_logical_width = WINDOW_WIDTH;
	state->m_logical_height = WINDOW_HEIGHT;
	s_select_render_resolution(state, &state->m_width, &state->m_height);

	state->m_window = SDL_CreateWindow(
		"102 Dalmatians",
		initial_window_width,
		initial_window_height,
		SDL_WINDOW_RESIZABLE
	);

	if (!state->m_window) {
		log_error("Window creation failed: %s", SDL_GetError());
		return NULL;
	}

	if (!s_create_device(state))
		return NULL;

	if (g_dttr_config.m_fullscreen)
		SDL_SetWindowFullscreen(state->m_window, true);

	state->m_msaa_sample_count = s_select_msaa_sample_count(state);
	log_info(
		"MSAA requested: x%d, effective: x%d",
		g_dttr_config.m_msaa_samples,
		s_msaa_sample_count_to_int(state->m_msaa_sample_count)
	);

	log_info(
		"SDL GPU initialized with %s (shaders: %s)",
		SDL_GetGPUDeviceDriver(state->m_device),
		dttr_graphics_shader_format_name(state->m_shader_format)
	);

	s_update_window_title(state);

	if (!dttr_graphics_create_pipelines() || !dttr_graphics_create_resources()) {
		log_error("Failed to create GPU resources");
		return NULL;
	}

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

#ifdef DTTR_COMPONENTS_ENABLED
	dttr_components_overlay_create(state);
#endif

	const SDL_PropertiesID props = SDL_GetWindowProperties(state->m_window);
	return (HWND)SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
}

// Updates COM-facing logical resolution and reapplies render policy if needed
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

// Refreshes render resolution after a runtime window size change event
void dttr_graphics_handle_window_resize(int width, int height) {
	if (width < DTTR_MIN_WINDOW_DIM || height < DTTR_MIN_WINDOW_DIM)
		return;

	s_refresh_render_resolution(&g_dttr_backend);
	s_update_window_title(&g_dttr_backend);
}

// Releases GPU resources, destroys the SDL window/device pair, and shuts down SDL
void dttr_graphics_cleanup(void) {
	dttr_graphics_hook_cleanup(dttr_game_api_get_ctx());

	DTTR_BackendState *state = &g_dttr_backend;

	if (!state->m_device || !state->m_window) {
		SDL_Quit();
		return;
	}

	for (int i = 0; i < DTTR_SAMPLER_COUNT; i++) {
		if (state->m_samplers[i])
			SDL_ReleaseGPUSampler(state->m_device, state->m_samplers[i]);
	}

#ifdef DTTR_COMPONENTS_ENABLED
	dttr_components_overlay_destroy(state);
#endif

	if (state->m_dummy_texture)
		SDL_ReleaseGPUTexture(state->m_device, state->m_dummy_texture);

	if (state->m_depth_texture)
		SDL_ReleaseGPUTexture(state->m_device, state->m_depth_texture);

	if (state->m_msaa_render_target)
		SDL_ReleaseGPUTexture(state->m_device, state->m_msaa_render_target);

	if (state->m_render_target)
		SDL_ReleaseGPUTexture(state->m_device, state->m_render_target);

	if (state->m_transfer_buffer)
		SDL_ReleaseGPUTransferBuffer(state->m_device, state->m_transfer_buffer);

	if (state->m_vertex_buffer)
		SDL_ReleaseGPUBuffer(state->m_device, state->m_vertex_buffer);

	for (int i = 0; i < DTTR_UPLOAD_POOL_SIZE; i++) {
		DTTR_UploadPoolSlot *slot = &state->m_upload_pool[i];
		if (slot->m_transfer_buffer) {
			SDL_ReleaseGPUTransferBuffer(state->m_device, slot->m_transfer_buffer);
			slot->m_transfer_buffer = NULL;
		}

		if (slot->m_storage_buffer) {
			SDL_ReleaseGPUBuffer(state->m_device, slot->m_storage_buffer);
			slot->m_storage_buffer = NULL;
		}

		slot->m_capacity = 0;
		slot->m_in_use = false;
	}

	for (int i = 0; i < DTTR_PIPELINE_COUNT; i++) {
		if (state->m_pipelines[i])
			SDL_ReleaseGPUGraphicsPipeline(state->m_device, state->m_pipelines[i]);
	}

	if (state->m_buf2tex_pipeline)
		SDL_ReleaseGPUComputePipeline(state->m_device, state->m_buf2tex_pipeline);

	if (state->m_texture_mutex) {
		SDL_DestroyMutex(state->m_texture_mutex);
		state->m_texture_mutex = NULL;
	}

	dttr_graphics_surface_texture_cache_reset();
	kv_destroy(state->m_pending_upload_indices);
	kv_destroy(state->m_batch_records);

	kv_init(state->m_pending_upload_indices);
	kv_init(state->m_batch_records);

	SDL_ReleaseWindowFromGPUDevice(state->m_device, state->m_window);
	SDL_DestroyGPUDevice(state->m_device);
	SDL_DestroyWindow(state->m_window);
	SDL_Quit();
}
