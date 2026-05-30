#ifdef DTTR_MODS_ENABLED

#include "../mods/mods_private.h"
#include "imgui_overlay_private.h"

#include <dttr_imgui.h>
#include <dttr_log.h>

#include <stdio.h>
#include <string.h>

static DTTR_BackendType backend_type;
static SDL_Window *window;
static DTTR_ImGuiDesktopScaleState imgui_scale;
static bool initialized;

static const float MODDING_BADGE_FONT_FACTOR = 0.90f;
static const float MODDING_BADGE_MIN_FONT_SIZE = 6.0f;
static const float MODDING_BADGE_DEFAULT_FONT_SIZE = 13.0f;
static const float MODDING_BADGE_LINE_ADVANCE_FACTOR = 0.86f;
static const float MODDING_BADGE_BOLD_OFFSET = 0.5f;
static const ImVec4_c MODDING_BADGE_HEADER_COLOR = {1.0f, 1.0f, 1.0f, 1.0f};
static const ImVec4_c MODDING_BADGE_SECONDS_COLOR = {0.35f, 1.0f, 0.35f, 1.0f};
static const ImVec4_c MODDING_BADGE_HOT_RELOAD_ON_COLOR = {0.35f, 1.0f, 0.35f, 1.0f};
static const ImVec4_c MODDING_BADGE_HOT_RELOAD_OFF_COLOR = {1.0f, 0.35f, 0.35f, 1.0f};
static const char MODDING_BADGE_HOT_RELOAD_LABEL[] = "Hot Reload:";

static float overlay_text_width(const char *text) {
	if (!text) {
		return 0.0f;
	}

	const ImVec2_c size = igCalcTextSize(text, NULL, false, -1.0f);
	return size.x;
}

static float overlay_line_height() { return igGetTextLineHeight(); }

static float overlay_line_advance() {
	return igGetTextLineHeight() * MODDING_BADGE_LINE_ADVANCE_FACTOR;
}

static float max_float(float a, float b) { return a > b ? a : b; }

static float overlay_mod_gap() {
	const ImGuiStyle *style = igGetStyle();
	const float spacing = style ? style->ItemSpacing.x : 4.0f;
	return spacing * 0.5f;
}

static void draw_overlay_text_at(
	ImDrawList *draw_list,
	ImVec2_c pos,
	ImU32 color,
	const char *text
) {
	if (!text) {
		return;
	}

	ImDrawList_AddText_Vec2(draw_list, pos, color, text, text + strlen(text));
}

static void draw_bold_overlay_text(
	ImDrawList *draw_list,
	ImVec2_c pos,
	ImU32 color,
	const char *text
) {
	draw_overlay_text_at(draw_list, pos, color, text);
	draw_overlay_text_at(
		draw_list,
		(ImVec2_c){pos.x + MODDING_BADGE_BOLD_OFFSET, pos.y},
		color,
		text
	);
}

static float hot_reload_width() {
	const char *state = dttr_mods_hot_reload_enabled() ? "on" : "off";
	return overlay_text_width(MODDING_BADGE_HOT_RELOAD_LABEL) + overlay_mod_gap()
		   + overlay_text_width(state);
}

static void draw_hot_reload_header(float width) {
	ImDrawList *draw_list = igGetWindowDrawList();
	if (!draw_list) {
		return;
	}

	const bool hot_reload_enabled = dttr_mods_hot_reload_enabled();
	const char *state = hot_reload_enabled ? "on" : "off";
	const ImVec4_c state_color = hot_reload_enabled ? MODDING_BADGE_HOT_RELOAD_ON_COLOR
													: MODDING_BADGE_HOT_RELOAD_OFF_COLOR;
	const float gap = overlay_mod_gap();
	const float label_width = overlay_text_width(MODDING_BADGE_HOT_RELOAD_LABEL);
	const ImVec2_c cursor = igGetCursorScreenPos();
	const ImU32 label_color = igGetColorU32_Vec4(MODDING_BADGE_HEADER_COLOR);
	const ImU32 state_color_u32 = igGetColorU32_Vec4(state_color);

	draw_bold_overlay_text(draw_list, cursor, label_color, MODDING_BADGE_HOT_RELOAD_LABEL);
	draw_bold_overlay_text(
		draw_list,
		(ImVec2_c){cursor.x + label_width + gap, cursor.y},
		state_color_u32,
		state
	);

	igDummy((ImVec2_c){width, overlay_line_advance()});
}

static void mod_overlay_elapsed_text(char *out, size_t out_size, size_t mod_index) {
	const unsigned long elapsed_seconds = (unsigned long)(dttr_mods_loaded_elapsed_ms(
															  mod_index
														  )
														  / 1000u);
	snprintf(out, out_size, "(%lus)", elapsed_seconds);
}

static void draw_mod_overlay_row(
	const char *name,
	const char *seconds,
	float gap,
	float total_width
) {
	ImDrawList *draw_list = igGetWindowDrawList();
	if (!draw_list) {
		return;
	}

	const ImVec2_c cursor = igGetCursorScreenPos();
	const ImU32 text_color = igGetColorU32_Col(ImGuiCol_Text, 1.0f);
	const ImU32 seconds_color = igGetColorU32_Vec4(MODDING_BADGE_SECONDS_COLOR);
	const char *mod_name = name ? name : "";
	const float name_width = overlay_text_width(mod_name);

	draw_bold_overlay_text(draw_list, cursor, text_color, mod_name);
	draw_overlay_text_at(
		draw_list,
		(ImVec2_c){cursor.x + name_width + gap, cursor.y},
		seconds_color,
		seconds
	);

	igDummy((ImVec2_c){total_width, overlay_line_advance()});
}

// Selects the SDL GPU ImGui backend only when the active renderer is SDL GPU.
static bool uses_sdl_gpu() { return backend_type == DTTR_BACKEND_SDL_GPU; }

// Returns the ImGui backend label used in overlay startup logs.
static const char *backend_name() { return uses_sdl_gpu() ? "SDL_GPU" : "OpenGL"; }

// Skips backend submission when ImGui produced no command lists for this frame.
static bool has_draw_data(const ImDrawData *draw_data) {
	return draw_data && draw_data->CmdListsCount > 0;
}

// Initializes the ImGui renderer backend that matches the active sidecar graphics API.
static void backend_init(SDL_Window *sdl_window, SDL_GPUDevice *device) {
	if (uses_sdl_gpu()) {
		ImGui_ImplSDL3_InitForSDLGPU(sdl_window);

		SDL_GPUTextureFormat swapchain_format = SDL_GetGPUSwapchainTextureFormat(
			device,
			sdl_window
		);
		CImGui_ImplSDLGPU3_InitInfo info = {
			.Device = device,
			.ColorTargetFormat = swapchain_format,
			.MSAASamples = SDL_GPU_SAMPLECOUNT_1,
			.SwapchainComposition = SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
			.PresentMode = SDL_GPU_PRESENTMODE_VSYNC,
		};

		cImGui_ImplSDLGPU3_Init(&info);
		return;
	}

	ImGui_ImplSDL3_InitForOpenGL(sdl_window, NULL);
	ImGui_ImplOpenGL3_Init("#version 330");
}

// Shuts down the renderer-specific ImGui backend before the shared SDL layer exits.
static void backend_shutdown() {
	if (uses_sdl_gpu()) {
		cImGui_ImplSDLGPU3_Shutdown();
		return;
	}

	ImGui_ImplOpenGL3_Shutdown();
}

// Creates the ImGui context and backend bindings used by mod overlay UI.
void dttr_imgui_init(
	SDL_Window *sdl_window,
	SDL_GPUDevice *device,
	DTTR_BackendType backend
) {
	if (initialized) {
		DTTR_LOG_WARN(
			"ImGui overlay init requested while already initialized; cleaning up stale "
			"backend state"
		);
		dttr_imgui_cleanup();
	}

	backend_type = backend;
	window = sdl_window;
	imgui_scale = (DTTR_ImGuiDesktopScaleState){0};

	igCreateContext(NULL);

	ImGuiIO *io = igGetIO_Nil();
	io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	igStyleColorsDark(NULL);

	ImGuiStyle *style = igGetStyle();
	style->Alpha = 0.9f;
	style->WindowRounding = 4.0f;
	DTTR_ImGui_ApplyWindowDesktopScale(&imgui_scale, window);

	backend_init(window, device);
	initialized = true;
	DTTR_LOG_INFO("ImGui overlay initialized (backend: %s)", backend_name());
}

// Destroys ImGui state and clears cached backend handles during graphics shutdown.
void dttr_imgui_cleanup() {
	if (!initialized) {
		return;
	}

	backend_shutdown();
	ImGui_ImplSDL3_Shutdown();
	igDestroyContext(NULL);
	window = NULL;
	imgui_scale = (DTTR_ImGuiDesktopScaleState){0};
	initialized = false;
	DTTR_LOG_INFO("ImGui overlay cleaned up");
}

// Lets ImGui consume SDL mouse and keyboard events before they reach game input hooks.
bool dttr_imgui_process_event(const SDL_Event *event) {
	if (!initialized) {
		return false;
	}

	ImGui_ImplSDL3_ProcessEvent(event);
	ImGuiIO *io = igGetIO_Nil();
	switch (event->type) {
	case SDL_EVENT_MOUSE_MOTION:
	case SDL_EVENT_MOUSE_BUTTON_DOWN:
	case SDL_EVENT_MOUSE_BUTTON_UP:
	case SDL_EVENT_MOUSE_WHEEL:
		return io->WantCaptureMouse;
	case SDL_EVENT_KEY_DOWN:
	case SDL_EVENT_KEY_UP:
	case SDL_EVENT_TEXT_INPUT:
		return io->WantCaptureKeyboard;
	default:
		return false;
	}
}

// Starts a renderer-backend ImGui frame for the active graphics API.
static void backend_new_frame() {
	if (uses_sdl_gpu()) {
		cImGui_ImplSDLGPU3_NewFrame();
	} else {
		ImGui_ImplOpenGL3_NewFrame();
	}
}

// Starts a full interactive ImGui frame with SDL input and desktop scaling applied.
static void new_frame() {
	backend_new_frame();
	ImGui_ImplSDL3_NewFrame();
	DTTR_ImGui_ApplyWindowDesktopScale(&imgui_scale, window);
	igNewFrame();
}

// Starts an offscreen ImGui frame for game rendering callbacks that should not read input.
static void new_frame_no_input(uint32_t w, uint32_t h) {
	backend_new_frame();
	DTTR_ImGui_ApplyWindowDesktopScale(&imgui_scale, window);
	ImGuiIO *io = igGetIO_Nil();
	io->DisplaySize = (ImVec2_c){(float)w, (float)h};
	igNewFrame();
}

// Renders game frame for the optional ImGui modding overlay across graphics backends.
static ImDrawData *render_game_frame(uint32_t w, uint32_t h) {
	new_frame_no_input(w, h);

	const DTTR_Mods_RenderGameContext ctx = {
		.width = w,
		.height = h,
		.scale = (float)h / 480.0f,
	};

	dttr_mods_render_game(&ctx);

	igRender();
	return igGetDrawData();
}

// Draws the small modding badge in game coordinates after mod UI has rendered.
static void draw_modding_overlay(const DTTR_Mods_RenderContext *ctx) {
	const float game_scale = ctx->scale > 0.0f ? ctx->scale : 1.0f;
	const float desktop_scale = DTTR_ImGui_GetCurrentDesktopScale(&imgui_scale);
	const float margin = 6.0f * game_scale * desktop_scale;
	const ImVec2_c pos = {
		(float)ctx->game_x + margin,
		(float)ctx->game_y + margin,
	};

	const ImVec2_c pivot = {0.0f, 0.0f};

	igSetNextWindowPos(pos, ImGuiCond_Always, pivot);
	igPushStyleVar_Float(ImGuiStyleVar_WindowBorderSize, 0.0f);
	igPushStyleVar_Float(ImGuiStyleVar_WindowRounding, 0.0f);
	igPushStyleVar_Vec2(ImGuiStyleVar_WindowPadding, (ImVec2_c){0.0f, 0.0f});

	const ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration
								   | ImGuiWindowFlags_NoBackground
								   | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoNav
								   | ImGuiWindowFlags_NoMove
								   | ImGuiWindowFlags_NoSavedSettings
								   | ImGuiWindowFlags_AlwaysAutoResize;

	ImGuiIO *io = igGetIO_Nil();
	ImFont *font = io ? io->FontDefault : NULL;

	if (!font) {
		font = igGetFont();
	}

	if (font) {
		const ImGuiStyle *style = igGetStyle();
		const float style_font_size = style ? style->FontSizeBase : 0.0f;

		const float font_size = font->LegacySize > 0.0f ? font->LegacySize
														: MODDING_BADGE_DEFAULT_FONT_SIZE;

		const float base_font_size = style_font_size > 0.0f ? style_font_size : font_size;

		const float scaled_font_size = base_font_size * MODDING_BADGE_FONT_FACTOR
									   * game_scale;

		const float badge_font_size = scaled_font_size > MODDING_BADGE_MIN_FONT_SIZE
										  ? scaled_font_size
										  : MODDING_BADGE_MIN_FONT_SIZE;

		igPushFont(font, badge_font_size);
	}

	const size_t loaded_count = dttr_mods_loaded_count();
	float rows_width = 0.0f;
	const float gap = overlay_mod_gap();

	for (size_t i = 0; i < loaded_count; i++) {
		char seconds[32];
		mod_overlay_elapsed_text(seconds, sizeof(seconds), i);

		const char *name = dttr_mods_loaded_name(i);
		const float row_width = overlay_text_width(name ? name : "") + gap
								+ overlay_text_width(seconds);
		rows_width = max_float(rows_width, row_width);
	}

	const float text_width = max_float(hot_reload_width(), rows_width);

	igSetNextWindowContentSize((ImVec2_c){text_width, 0.0f});

	if (igBegin("##modding_overlay", NULL, flags)) {
		draw_hot_reload_header(text_width);

		for (size_t i = 0; i < loaded_count; i++) {
			char seconds[32];
			mod_overlay_elapsed_text(seconds, sizeof(seconds), i);

			const char *name = dttr_mods_loaded_name(i);
			draw_mod_overlay_row(name, seconds, gap, text_width);
		}

		igDummy((ImVec2_c){text_width, overlay_line_height() - overlay_line_advance()});
	}

	igEnd();
	igPopStyleVar(3);

	if (font) {
		igPopFont();
	}
}

// Renders overlay frame for the optional ImGui modding overlay across graphics backends.
static ImDrawData *render_overlay_frame(
	uint32_t swap_w,
	uint32_t swap_h,
	uint32_t game_x,
	uint32_t game_y,
	uint32_t game_w,
	uint32_t game_h
) {
	const DTTR_Mods_RenderContext ctx = {
		.window_w = swap_w,
		.window_h = swap_h,
		.game_x = game_x,
		.game_y = game_y,
		.game_w = game_w,
		.game_h = game_h,
		.scale = (float)game_h / 480.0f,
	};

	new_frame();
	dttr_mods_imgui_begin(&ctx);
	dttr_mods_render(&ctx);
	draw_modding_overlay(&ctx);
	dttr_mods_imgui_end(&ctx);

	igRender();
	return igGetDrawData();
}

// Reads ImGui's current display size for OpenGL paths that present to the default target.
static void current_display_size(uint32_t *width, uint32_t *height) {
	ImGuiIO *io = igGetIO_Nil();
	*width = (uint32_t)io->DisplaySize.x;
	*height = (uint32_t)io->DisplaySize.y;
}

// Submits SDL3 GPU for the optional ImGui modding overlay across graphics backends.
static void submit_sdl3gpu(
	ImDrawData *draw_data,
	SDL_GPUCommandBuffer *cmd,
	SDL_GPUTexture *target
) {
	if (!has_draw_data(draw_data)) {
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

// Submits OpenGL for the optional ImGui modding overlay across graphics backends.
static void submit_opengl(ImDrawData *draw_data) {
	if (!has_draw_data(draw_data)) {
		return;
	}

	ImGui_ImplOpenGL3_RenderDrawData(draw_data);
}

// Renders mod-provided game content into an SDL GPU render target.
void dttr_imgui_render_game_sdl3gpu(
	SDL_GPUCommandBuffer *cmd,
	SDL_GPUTexture *render_target,
	uint32_t w,
	uint32_t h
) {
	if (!dttr_mods_has_render_game()) {
		return;
	}

	submit_sdl3gpu(render_game_frame(w, h), cmd, render_target);
}

// Renders mod-provided game content through the OpenGL ImGui backend.
void dttr_imgui_render_game_opengl() {
	if (!dttr_mods_has_render_game()) {
		return;
	}

	uint32_t width = 0;
	uint32_t height = 0;
	current_display_size(&width, &height);
	submit_opengl(render_game_frame(width, height));
}

// Renders the interactive overlay into the SDL GPU swapchain target.
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
	submit_sdl3gpu(
		render_overlay_frame(swap_w, swap_h, game_x, game_y, game_w, game_h),
		cmd,
		swapchain_tex
	);
}

// Renders the interactive overlay through OpenGL after the game viewport is known.
void dttr_imgui_render_opengl(
	uint32_t game_x,
	uint32_t game_y,
	uint32_t game_w,
	uint32_t game_h
) {
	uint32_t width = 0;
	uint32_t height = 0;
	current_display_size(&width, &height);
	submit_opengl(render_overlay_frame(width, height, game_x, game_y, game_w, game_h));
}

#endif // DTTR_MODS_ENABLED
