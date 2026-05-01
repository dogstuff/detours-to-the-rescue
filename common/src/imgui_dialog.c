#include <dttr_imgui.h>

#include <glad/gl.h>

static const ImGuiWindowFlags S_ROOT_WINDOW_FLAGS = ImGuiWindowFlags_NoDecoration
													| ImGuiWindowFlags_NoMove
													| ImGuiWindowFlags_NoSavedSettings;

static bool s_sdl_video_ready;

static float s_context_scale(const DTTR_ImGuiDialogContext *ctx) {
	return ctx && ctx->m_desktop_scale > 0.0f ? ctx->m_desktop_scale : 1.0f;
}

float dttr_imgui_dialog_scaled_float(const DTTR_ImGuiDialogContext *ctx, float value) {
	return value * s_context_scale(ctx);
}

int dttr_imgui_dialog_scaled_int(const DTTR_ImGuiDialogContext *ctx, float value) {
	const int scaled = (int)(dttr_imgui_dialog_scaled_float(ctx, value) + 0.5f);
	return scaled > 0 ? scaled : 1;
}

bool dttr_imgui_dialog_refresh_scale(DTTR_ImGuiDialogContext *ctx) {
	if (!ctx || !ctx->m_window) {
		return false;
	}

	const float old_scale = s_context_scale(ctx);
	const bool style_changed = dttr_imgui_apply_window_desktop_scale(
		&ctx->m_imgui_scale,
		ctx->m_window
	);
	ctx->m_desktop_scale = dttr_imgui_get_current_desktop_scale(&ctx->m_imgui_scale);
	return style_changed || dttr_imgui_scale_changed(old_scale, ctx->m_desktop_scale);
}

static bool s_init_sdl_video(void) {
	if (s_sdl_video_ready) {
		return true;
	}

	if (!SDL_InitSubSystem(SDL_INIT_VIDEO)) {
		return false;
	}

	s_sdl_video_ready = true;
	return true;
}

bool dttr_imgui_dialog_begin(
	DTTR_ImGuiDialogContext *ctx,
	const char *title,
	int width,
	int height
) {
	*ctx = (DTTR_ImGuiDialogContext){0};
	if (!s_init_sdl_video()) {
		return false;
	}

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);

	ctx->m_window = SDL_CreateWindow(
		title,
		width,
		height,
		SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN
	);
	if (!ctx->m_window) {
		goto fail;
	}

	ctx->m_gl_context = SDL_GL_CreateContext(ctx->m_window);
	if (!ctx->m_gl_context || !SDL_GL_MakeCurrent(ctx->m_window, ctx->m_gl_context)) {
		goto fail;
	}

	SDL_GL_SetSwapInterval(1);
	if (!gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress)) {
		goto fail;
	}

	igCreateContext(NULL);
	ctx->m_imgui_context_ready = true;
	ImGuiIO *io = igGetIO_Nil();
	io->IniFilename = NULL;
	io->LogFilename = NULL;
	io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	igStyleColorsDark(NULL);
	dttr_imgui_dialog_refresh_scale(ctx);
	if (!ImGui_ImplSDL3_InitForOpenGL(ctx->m_window, ctx->m_gl_context)) {
		goto fail;
	}
	ctx->m_imgui_sdl_ready = true;
	if (!ImGui_ImplOpenGL3_Init("#version 130")) {
		goto fail;
	}
	ctx->m_imgui_gl_ready = true;

	SDL_ShowWindow(ctx->m_window);
	return true;

fail:
	dttr_imgui_dialog_end(ctx);
	return false;
}

void dttr_imgui_dialog_end(DTTR_ImGuiDialogContext *ctx) {
	if (ctx->m_imgui_gl_ready) {
		ImGui_ImplOpenGL3_Shutdown();
	}
	if (ctx->m_imgui_sdl_ready) {
		ImGui_ImplSDL3_Shutdown();
	}
	if (ctx->m_imgui_context_ready) {
		igDestroyContext(NULL);
	}
	if (ctx->m_gl_context) {
		SDL_GL_DestroyContext(ctx->m_gl_context);
	}
	if (ctx->m_window) {
		SDL_DestroyWindow(ctx->m_window);
	}
	*ctx = (DTTR_ImGuiDialogContext){0};
}

void dttr_imgui_dialog_shutdown(void) {
	if (!s_sdl_video_ready) {
		return;
	}

	SDL_QuitSubSystem(SDL_INIT_VIDEO);
	SDL_Quit();
	s_sdl_video_ready = false;
}

void dttr_imgui_dialog_new_frame(void) {
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL3_NewFrame();
	igNewFrame();
}

void dttr_imgui_dialog_render(DTTR_ImGuiDialogContext *ctx) {
	int width;
	int height;
	SDL_GetWindowSizeInPixels(ctx->m_window, &width, &height);

	igRender();
	glViewport(0, 0, width, height);
	glClearColor(0.06f, 0.07f, 0.09f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	ImGui_ImplOpenGL3_RenderDrawData(igGetDrawData());
	SDL_GL_SwapWindow(ctx->m_window);
}

void dttr_imgui_dialog_process_events(bool *running) {
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		ImGui_ImplSDL3_ProcessEvent(&event);
		if (event.type == SDL_EVENT_QUIT) {
			*running = false;
			return;
		}
		if (event.type == SDL_EVENT_KEY_DOWN
			&& event.key.scancode == SDL_SCANCODE_ESCAPE) {
			*running = false;
			return;
		}
	}
}

static void s_begin_full_window(DTTR_ImGuiDialogContext *ctx) {
	int width;
	int height;
	SDL_GetWindowSize(ctx->m_window, &width, &height);
	igSetNextWindowPos((ImVec2_c){0.0f, 0.0f}, ImGuiCond_Always, (ImVec2_c){0.0f, 0.0f});
	igSetNextWindowSize((ImVec2_c){(float)width, (float)height}, ImGuiCond_Always);
}

bool dttr_imgui_dialog_begin_root(DTTR_ImGuiDialogContext *ctx, const char *title) {
	s_begin_full_window(ctx);
	igPushStyleVar_Vec2(ImGuiStyleVar_WindowPadding, (ImVec2_c){0.0f, 0.0f});
	return igBegin(title, NULL, S_ROOT_WINDOW_FLAGS);
}

void dttr_imgui_dialog_end_root(void) {
	igEnd();
	igPopStyleVar(1);
}

bool dttr_imgui_dialog_button(
	const DTTR_ImGuiDialogContext *ctx,
	const char *id,
	const char *label,
	ImVec2_c size
) {
	const bool clicked = igInvisibleButton(id, size, ImGuiButtonFlags_None);
	const ImVec2_c min = igGetItemRectMin();
	const ImVec2_c max = igGetItemRectMax();
	const bool active = igIsItemActive();
	const bool hovered = igIsItemHovered(ImGuiHoveredFlags_None);

	const ImVec4_c bg = active	  ? DTTR_IMGUI_COLOR_BUTTON_BG_ACTIVE
						: hovered ? DTTR_IMGUI_COLOR_BUTTON_BG_HOVERED
								  : DTTR_IMGUI_COLOR_BUTTON_BG;
	const ImU32 bg_col = igGetColorU32_Vec4(bg);
	const ImU32 text_col = igGetColorU32_Vec4(DTTR_IMGUI_COLOR_BUTTON_TEXT);

	ImDrawList *draw_list = igGetWindowDrawList();
	ImDrawList_AddRectFilled(
		draw_list,
		min,
		max,
		bg_col,
		dttr_imgui_dialog_scaled_float(ctx, 2.0f),
		0
	);
	const ImVec2_c text_size = igCalcTextSize(label, NULL, false, -1.0f);
	const ImVec2_c text_pos = {
		min.x + (size.x - text_size.x) * 0.5f,
		min.y + (size.y - text_size.y) * 0.5f,
	};

	ImDrawList_AddText_Vec2(draw_list, text_pos, text_col, label, NULL);
	return clicked;
}

void dttr_imgui_dialog_center_next_item(float item_width) {
	const float window_width = igGetWindowWidth();
	if (window_width > item_width) {
		igSetCursorPosX((window_width - item_width) * 0.5f);
	}
}

static void s_draw_centered_text(const char *text) {
	const ImVec2_c size = igCalcTextSize(text, NULL, false, -1.0f);
	dttr_imgui_dialog_center_next_item(size.x);
	igTextUnformatted(text, NULL);
}

void dttr_imgui_dialog_draw_header(
	const DTTR_ImGuiDialogContext *ctx,
	const char *title,
	const char *version
) {
	igSetCursorPosY(dttr_imgui_dialog_scaled_float(ctx, 18.0f));
	const ImGuiStyle *style = igGetStyle();
	igPushFont(NULL, style->FontSizeBase * 1.35f);
	s_draw_centered_text(title);
	igPopFont();
	igSetCursorPosY(igGetCursorPosY() + dttr_imgui_dialog_scaled_float(ctx, 4.0f));
	s_draw_centered_text(version);
	igSetCursorPosY(igGetCursorPosY() + dttr_imgui_dialog_scaled_float(ctx, 2.0f));
}

void dttr_imgui_dialog_draw_padded_text(
	const DTTR_ImGuiDialogContext *ctx,
	const char *message,
	float padding_x,
	float padding_y
) {
	igSetCursorPosX(dttr_imgui_dialog_scaled_float(ctx, padding_x));
	igSetCursorPosY(igGetCursorPosY() + dttr_imgui_dialog_scaled_float(ctx, padding_y));
	igPushTextWrapPos(igGetWindowWidth() - dttr_imgui_dialog_scaled_float(ctx, padding_x));
	igTextWrapped("%s", message ? message : "");
	igPopTextWrapPos();
	igSetCursorPosY(igGetCursorPosY() + dttr_imgui_dialog_scaled_float(ctx, padding_y));
}

void dttr_imgui_dialog_fit_window_to_content(
	DTTR_ImGuiDialogContext *ctx,
	int width,
	float padding_y
) {
	const int scaled_width = dttr_imgui_dialog_scaled_int(ctx, (float)width);
	const int height = (int)(igGetCursorPosY()
							 + dttr_imgui_dialog_scaled_float(ctx, padding_y) + 0.5f);
	SDL_SetWindowSize(ctx->m_window, scaled_width, height);
}
