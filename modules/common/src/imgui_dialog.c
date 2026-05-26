#include <dttr_imgui.h>

#include <glad/gl.h>

static const ImGuiWindowFlags ROOT_WINDOW_FLAGS = ImGuiWindowFlags_NoDecoration
												  | ImGuiWindowFlags_NoMove
												  | ImGuiWindowFlags_NoSavedSettings;

static bool sdl_video_ready;

static void use_dialog_imgui_context(const DTTR_ImGuiDialogContext *ctx) {
	if (ctx && ctx->imgui_context) {
		igSetCurrentContext(ctx->imgui_context);
	}
}

static void restore_previous_imgui_context(const DTTR_ImGuiDialogContext *ctx) {
	if (!ctx) {
		return;
	}

	igSetCurrentContext(ctx->previous_imgui_context);
}

static void offset_cursor_y(const DTTR_ImGuiDialogContext *ctx, float amount) {
	igSetCursorPosY(igGetCursorPosY() + DTTR_ImGuiDialog_ScaledFloat(ctx, amount));
}

static float context_scale(const DTTR_ImGuiDialogContext *ctx) {
	return ctx && ctx->desktop_scale > 0.0f ? ctx->desktop_scale : 1.0f;
}

float DTTR_ImGuiDialog_ScaledFloat(const DTTR_ImGuiDialogContext *ctx, float value) {
	return value * context_scale(ctx);
}

int DTTR_ImGuiDialog_ScaledInt(const DTTR_ImGuiDialogContext *ctx, float value) {
	const int scaled = (int)(DTTR_ImGuiDialog_ScaledFloat(ctx, value) + 0.5f);
	return scaled > 0 ? scaled : 1;
}

static void resize_dialog_window_for_scale(DTTR_ImGuiDialogContext *ctx) {
	if (!ctx || !ctx->window || ctx->logical_window_width <= 0
		|| ctx->logical_window_height <= 0) {
		return;
	}

	SDL_SetWindowSize(
		ctx->window,
		DTTR_ImGuiDialog_ScaledInt(ctx, (float)ctx->logical_window_width),
		DTTR_ImGuiDialog_ScaledInt(ctx, (float)ctx->logical_window_height)
	);
}

bool DTTR_ImGuiDialog_RefreshScale(DTTR_ImGuiDialogContext *ctx) {
	if (!ctx || !ctx->window) {
		return false;
	}

	const float old_scale = context_scale(ctx);
	const bool style_changed = DTTR_ImGui_ApplyWindowDesktopScale(
		&ctx->imgui_scale,
		ctx->window
	);
	ctx->desktop_scale = DTTR_ImGui_GetCurrentDesktopScale(&ctx->imgui_scale);
	const bool scale_changed = style_changed
							   || DTTR_ImGui_ScaleChanged(old_scale, ctx->desktop_scale);
	if (scale_changed) {
		resize_dialog_window_for_scale(ctx);
	}

	return scale_changed;
}

static void set_gl_attributes() {
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);
}

static bool init_sdl_video() {
	if (sdl_video_ready) {
		return true;
	}

	if (!SDL_InitSubSystem(SDL_INIT_VIDEO)) {
		return false;
	}

	sdl_video_ready = true;
	return true;
}

static bool init_dialog_imgui(DTTR_ImGuiDialogContext *ctx) {
	ctx->previous_imgui_context = igGetCurrentContext();
	ctx->imgui_context = igCreateContext(NULL);
	use_dialog_imgui_context(ctx);
	ctx->imgui_context_ready = true;

	ImGuiIO *io = igGetIO_Nil();
	io->IniFilename = NULL;
	io->LogFilename = NULL;
	io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	igStyleColorsDark(NULL);
	DTTR_ImGuiDialog_RefreshScale(ctx);
	if (!ImGui_ImplSDL3_InitForOpenGL(ctx->window, ctx->gl_context)) {
		return false;
	}

	ctx->imgui_sdl_ready = true;
	if (!ImGui_ImplOpenGL3_Init("#version 130")) {
		return false;
	}

	ctx->imgui_gl_ready = true;
	return true;
}

bool DTTR_ImGuiDialog_Begin(
	DTTR_ImGuiDialogContext *ctx,
	const char *title,
	int width,
	int height
) {
	if (!ctx) {
		return false;
	}

	*ctx = (DTTR_ImGuiDialogContext){0};
	if (!init_sdl_video()) {
		return false;
	}

	ctx->logical_window_width = width;
	ctx->logical_window_height = height;

	set_gl_attributes();

	ctx->window = SDL_CreateWindow(
		title,
		width,
		height,
		SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN
	);
	if (!ctx->window) {
		goto fail;
	}

	ctx->gl_context = SDL_GL_CreateContext(ctx->window);
	if (!ctx->gl_context || !SDL_GL_MakeCurrent(ctx->window, ctx->gl_context)) {
		goto fail;
	}

	SDL_GL_SetSwapInterval(1);
	if (!gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress)) {
		goto fail;
	}

	if (!init_dialog_imgui(ctx)) {
		goto fail;
	}

	SDL_ShowWindow(ctx->window);
	return true;

fail:
	DTTR_ImGuiDialog_End(ctx);
	return false;
}

void DTTR_ImGuiDialog_End(DTTR_ImGuiDialogContext *ctx) {
	if (!ctx) {
		return;
	}

	use_dialog_imgui_context(ctx);
	if (ctx->imgui_gl_ready) {
		ImGui_ImplOpenGL3_Shutdown();
	}

	if (ctx->imgui_sdl_ready) {
		ImGui_ImplSDL3_Shutdown();
	}

	if (ctx->imgui_context_ready) {
		igDestroyContext(ctx->imgui_context);
	}

	restore_previous_imgui_context(ctx);
	if (ctx->gl_context) {
		SDL_GL_DestroyContext(ctx->gl_context);
	}

	if (ctx->window) {
		SDL_DestroyWindow(ctx->window);
	}

	*ctx = (DTTR_ImGuiDialogContext){0};
}

void DTTR_ImGuiDialog_Shutdown() {
	if (!sdl_video_ready) {
		return;
	}

	SDL_QuitSubSystem(SDL_INIT_VIDEO);
	SDL_Quit();
	sdl_video_ready = false;
}

void DTTR_ImGuiDialog_NewFrame(const DTTR_ImGuiDialogContext *ctx) {
	use_dialog_imgui_context(ctx);
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL3_NewFrame();
	igNewFrame();
}

void DTTR_ImGuiDialog_Render(DTTR_ImGuiDialogContext *ctx) {
	use_dialog_imgui_context(ctx);
	int width;
	int height;
	SDL_GetWindowSizeInPixels(ctx->window, &width, &height);

	igRender();
	glViewport(0, 0, width, height);
	glClearColor(0.06f, 0.07f, 0.09f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	ImGui_ImplOpenGL3_RenderDrawData(igGetDrawData());
	SDL_GL_SwapWindow(ctx->window);
}

void DTTR_ImGuiDialog_ProcessEvent(
	const DTTR_ImGuiDialogContext *ctx,
	const SDL_Event *event,
	bool *running
) {
	if (!event) {
		return;
	}

	use_dialog_imgui_context(ctx);
	ImGui_ImplSDL3_ProcessEvent(event);
	if (event->type != SDL_EVENT_QUIT) {
		return;
	}

	if (running) {
		*running = false;
	}
}

void DTTR_ImGuiDialog_ProcessEvents(const DTTR_ImGuiDialogContext *ctx, bool *running) {
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		DTTR_ImGuiDialog_ProcessEvent(ctx, &event, running);
		if (running && !*running) {
			break;
		}
	}
}

static void begin_full_window(const DTTR_ImGuiDialogContext *ctx) {
	use_dialog_imgui_context(ctx);
	int width;
	int height;
	SDL_GetWindowSize(ctx->window, &width, &height);
	igSetNextWindowPos((ImVec2_c){0.0f, 0.0f}, ImGuiCond_Always, (ImVec2_c){0.0f, 0.0f});
	igSetNextWindowSize((ImVec2_c){(float)width, (float)height}, ImGuiCond_Always);
}

bool DTTR_ImGuiDialog_BeginRoot(
	DTTR_ImGuiDialogContext *ctx,
	const char *title,
	ImGuiWindowFlags flags
) {
	begin_full_window(ctx);
	igPushStyleVar_Vec2(ImGuiStyleVar_WindowPadding, (ImVec2_c){0.0f, 0.0f});
	return igBegin(title, NULL, ROOT_WINDOW_FLAGS | flags);
}

void DTTR_ImGuiDialog_EndRoot() {
	igEnd();
	igPopStyleVar(1);
}

bool DTTR_ImGuiDialog_Button(
	const DTTR_ImGuiDialogContext *ctx,
	const char *id,
	const char *label,
	ImVec2_c size
) {
	igPushID_Str(id);
	igPushStyleColor_Vec4(ImGuiCol_Text, DTTR_IMGUI_COLOR_BUTTON_TEXT);
	igPushStyleColor_Vec4(ImGuiCol_Button, DTTR_IMGUI_COLOR_BUTTON_BG);
	igPushStyleColor_Vec4(ImGuiCol_ButtonHovered, DTTR_IMGUI_COLOR_BUTTON_BG_HOVERED);
	igPushStyleColor_Vec4(ImGuiCol_ButtonActive, DTTR_IMGUI_COLOR_BUTTON_BG_ACTIVE);
	igPushStyleVar_Float(
		ImGuiStyleVar_FrameRounding,
		DTTR_ImGuiDialog_ScaledFloat(ctx, 2.0f)
	);
	const bool clicked = igButton(label, size);
	igPopStyleVar(1);
	igPopStyleColor(4);
	igPopID();
	return clicked;
}

void DTTR_ImGuiDialog_CenterNextItem(float item_width) {
	const float window_width = igGetWindowWidth();
	if (window_width > item_width) {
		igSetCursorPosX((window_width - item_width) * 0.5f);
	}
}

static void draw_centered_text(const char *text) {
	const ImVec2_c size = igCalcTextSize(text, NULL, false, -1.0f);
	DTTR_ImGuiDialog_CenterNextItem(size.x);
	igTextUnformatted(text, NULL);
}

void DTTR_ImGuiDialog_DrawHeader(
	const DTTR_ImGuiDialogContext *ctx,
	const char *title,
	const char *version
) {
	offset_cursor_y(ctx, 3.0f);
	const ImGuiStyle *style = igGetStyle();
	const float title_font_size = (style ? style->FontSizeBase : 0.0f) * 1.35f;
	igPushFont(NULL, title_font_size);
	draw_centered_text(title);
	igPopFont();
	offset_cursor_y(ctx, 4.0f);
	draw_centered_text(version);
	offset_cursor_y(ctx, 2.0f);
}

void DTTR_ImGuiDialog_DrawPaddedText(
	const DTTR_ImGuiDialogContext *ctx,
	const char *message,
	float padding_x,
	float padding_y
) {
	igSetCursorPosX(DTTR_ImGuiDialog_ScaledFloat(ctx, padding_x));
	offset_cursor_y(ctx, padding_y);
	igPushTextWrapPos(igGetWindowWidth() - DTTR_ImGuiDialog_ScaledFloat(ctx, padding_x));
	igTextWrapped("%s", message ? message : "");
	igPopTextWrapPos();
	offset_cursor_y(ctx, padding_y);
}

void DTTR_ImGuiDialog_FitWindowToContent(
	DTTR_ImGuiDialogContext *ctx,
	int width,
	float padding_y
) {
	const int scaled_width = DTTR_ImGuiDialog_ScaledInt(ctx, (float)width);
	const int height = (int)(igGetCursorPosY()
							 + DTTR_ImGuiDialog_ScaledFloat(ctx, padding_y) + 0.5f);
	SDL_SetWindowSize(ctx->window, scaled_width, height);
}
