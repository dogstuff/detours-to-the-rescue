#ifdef DTTR_MODDING_ENABLED

#include "../components/components_internal.h"
#include "imgui_overlay_internal.h"

#include <cimgui.h>
#include <cimgui_impl.h>
#include <cimgui_impl_sdlgpu3.h>
#include <dttr_log.h>

static DTTR_BackendType s_backend_type;

// Buffered events are flushed into ImGui before the overlay frame only.
#define MAX_BUFFERED_EVENTS 128
static SDL_Event s_event_buf[MAX_BUFFERED_EVENTS];
static int s_event_count;

void dttr_imgui_init(SDL_Window *window, SDL_GPUDevice *device, DTTR_BackendType backend) {
	s_backend_type = backend;

	igCreateContext(NULL);

	ImGuiIO *io = igGetIO_Nil();
	io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	igStyleColorsDark(NULL);

	ImGuiStyle *style = igGetStyle();
	style->Alpha = 0.9f;
	style->WindowRounding = 4.0f;

	if (backend == DTTR_BACKEND_SDL_GPU) {
		ImGui_ImplSDL3_InitForSDLGPU(window);

		SDL_GPUTextureFormat swapchain_format = SDL_GetGPUSwapchainTextureFormat(
			device,
			window
		);

		CImGui_ImplSDLGPU3_InitInfo info = {
			.Device = device,
			.ColorTargetFormat = swapchain_format,
			.MSAASamples = SDL_GPU_SAMPLECOUNT_1,
			.SwapchainComposition = SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
			.PresentMode = SDL_GPU_PRESENTMODE_VSYNC,
		};
		cImGui_ImplSDLGPU3_Init(&info);
	} else {
		ImGui_ImplSDL3_InitForOpenGL(window, NULL);
		ImGui_ImplOpenGL3_Init("#version 330");
	}

	DTTR_LOG_INFO(
		"ImGui overlay initialized (backend: %s)",
		backend == DTTR_BACKEND_SDL_GPU ? "SDL_GPU" : "OpenGL"
	);
}

void dttr_imgui_cleanup(void) {
	if (s_backend_type == DTTR_BACKEND_SDL_GPU) {
		cImGui_ImplSDLGPU3_Shutdown();
	} else {
		ImGui_ImplOpenGL3_Shutdown();
	}

	ImGui_ImplSDL3_Shutdown();
	igDestroyContext(NULL);
	DTTR_LOG_INFO("ImGui overlay cleaned up");
}

bool dttr_imgui_process_event(const SDL_Event *event) {
	if (s_event_count < MAX_BUFFERED_EVENTS) {
		s_event_buf[s_event_count++] = *event;
	}
	return false;
}

static void s_flush_buffered_events(void) {
	for (int i = 0; i < s_event_count; i++) {
		ImGui_ImplSDL3_ProcessEvent(&s_event_buf[i]);
	}
	s_event_count = 0;
}

static void s_backend_new_frame(void) {
	if (s_backend_type == DTTR_BACKEND_SDL_GPU) {
		cImGui_ImplSDLGPU3_NewFrame();
	} else {
		ImGui_ImplOpenGL3_NewFrame();
	}
}

static void s_new_frame(void) {
	s_backend_new_frame();
	s_flush_buffered_events();
	ImGui_ImplSDL3_NewFrame();
	igNewFrame();
}

// Begins a frame with a custom display size, skipping SDL input processing.
static void s_new_frame_no_input(uint32_t w, uint32_t h) {
	s_backend_new_frame();
	ImGuiIO *io = igGetIO_Nil();
	io->DisplaySize = (ImVec2_c){(float)w, (float)h};
	igNewFrame();
}

static ImDrawData *s_render_game_frame(uint32_t w, uint32_t h) {
	s_new_frame_no_input(w, h);

	const DTTR_RenderGameContext ctx = {
		.m_width = w,
		.m_height = h,
		.m_scale = (float)h / 480.0f,
	};
	dttr_components_render_game(&ctx);

	igRender();
	return igGetDrawData();
}

static void s_draw_components_overlay(const DTTR_RenderContext *ctx) {
	const float scale = ctx->m_scale > 0.0f ? ctx->m_scale : 1.0f;
	const float margin = 4.0f * scale;
	const ImVec2_c pos = {
		(float)ctx->m_game_x + (float)ctx->m_game_w - margin,
		(float)ctx->m_game_y + margin,
	};
	const ImVec2_c pivot = {1.0f, 0.0f};

	igSetNextWindowPos(pos, ImGuiCond_Always, pivot);
	igSetNextWindowBgAlpha(0.3f);

	const ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration
								   | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoNav
								   | ImGuiWindowFlags_NoMove
								   | ImGuiWindowFlags_NoSavedSettings
								   | ImGuiWindowFlags_AlwaysAutoResize;

	igPushFont(NULL, igGetFontSize() * scale);

	if (igBegin("##components_overlay", NULL, flags)) {
		igText("COMPONENTS ENABLED");
	}
	igEnd();

	igPopFont();
}

static ImDrawData *s_render_overlay_frame(
	uint32_t swap_w,
	uint32_t swap_h,
	uint32_t game_x,
	uint32_t game_y,
	uint32_t game_w,
	uint32_t game_h
) {
	const DTTR_RenderContext ctx = {
		.m_window_w = swap_w,
		.m_window_h = swap_h,
		.m_game_x = game_x,
		.m_game_y = game_y,
		.m_game_w = game_w,
		.m_game_h = game_h,
		.m_scale = (float)game_h / 480.0f,
	};

	s_new_frame();
	dttr_components_render(&ctx);
	s_draw_components_overlay(&ctx);

	igRender();
	return igGetDrawData();
}

static void s_submit_sdl3gpu(
	ImDrawData *draw_data,
	SDL_GPUCommandBuffer *cmd,
	SDL_GPUTexture *target
) {
	if (draw_data->CmdListsCount == 0) {
		return;
	}

	cImGui_ImplSDLGPU3_PrepareDrawData(draw_data, cmd);

	SDL_GPUColorTargetInfo color_target = {
		.texture = target,
		.load_op = SDL_GPU_LOADOP_LOAD,
		.store_op = SDL_GPU_STOREOP_STORE,
	};

	SDL_GPURenderPass *pass = SDL_BeginGPURenderPass(cmd, &color_target, 1, NULL);
	if (!pass) {
		return;
	}
	cImGui_ImplSDLGPU3_RenderDrawData(draw_data, cmd, pass);
	SDL_EndGPURenderPass(pass);
}

static void s_submit_opengl(ImDrawData *draw_data) {
	if (draw_data->CmdListsCount == 0) {
		return;
	}

	ImGui_ImplOpenGL3_RenderDrawData(draw_data);
}

void dttr_imgui_render_game_sdl3gpu(
	SDL_GPUCommandBuffer *cmd,
	SDL_GPUTexture *render_target,
	uint32_t w,
	uint32_t h
) {
	if (!dttr_components_has_render_game()) {
		return;
	}
	s_submit_sdl3gpu(s_render_game_frame(w, h), cmd, render_target);
}

void dttr_imgui_render_game_opengl(void) {
	if (!dttr_components_has_render_game()) {
		return;
	}
	ImGuiIO *io = igGetIO_Nil();
	s_submit_opengl(
		s_render_game_frame((uint32_t)io->DisplaySize.x, (uint32_t)io->DisplaySize.y)
	);
}

void dttr_imgui_render_sdl3gpu(
	SDL_GPUCommandBuffer *cmd,
	SDL_GPUTexture *swapchain_tex,
	uint32_t swap_w,
	uint32_t swap_h,
	uint32_t game_x,
	uint32_t game_y,
	uint32_t game_w,
	uint32_t game_h
) {
	s_submit_sdl3gpu(
		s_render_overlay_frame(swap_w, swap_h, game_x, game_y, game_w, game_h),
		cmd,
		swapchain_tex
	);
}

void dttr_imgui_render_opengl(
	uint32_t game_x,
	uint32_t game_y,
	uint32_t game_w,
	uint32_t game_h
) {
	ImGuiIO *io = igGetIO_Nil();
	s_submit_opengl(s_render_overlay_frame(
		(uint32_t)io->DisplaySize.x,
		(uint32_t)io->DisplaySize.y,
		game_x,
		game_y,
		game_w,
		game_h
	));
}

#endif /* DTTR_MODDING_ENABLED */
