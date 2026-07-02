#ifndef DTTR_IMGUI_H
#define DTTR_IMGUI_H

#include <SDL3/SDL.h>
#include <cimgui.h>
#include <cimgui_impl.h>
#include <cimgui_impl_sdlgpu3.h>
#include <stdbool.h>
#include <stddef.h>

#define DTTR_IMGUI_COLOR_LINK ((ImVec4_c){0.33f, 0.63f, 1.0f, 1.0f})
#define DTTR_IMGUI_COLOR_STACK_FRAME_BG ((ImVec4_c){0.10f, 0.11f, 0.12f, 1.0f})
#define DTTR_IMGUI_COLOR_BUTTON_BG ((ImVec4_c){0.184f, 0.204f, 0.227f, 1.0f})
#define DTTR_IMGUI_COLOR_BUTTON_BG_HOVERED ((ImVec4_c){0.235f, 0.267f, 0.298f, 1.0f})
#define DTTR_IMGUI_COLOR_BUTTON_BG_ACTIVE ((ImVec4_c){0.145f, 0.165f, 0.188f, 1.0f})
#define DTTR_IMGUI_COLOR_BUTTON_TEXT ((ImVec4_c){0.91f, 0.90f, 0.87f, 1.0f})
#define DTTR_IMGUI_SCALE_EPSILON 0.001f

#ifdef __cplusplus
extern "C" {
#endif

typedef struct DTTR_ImGuiDesktopScaleState {
	ImGuiStyle base_style;
	float current_scale;
	bool initialized;
} DTTR_ImGuiDesktopScaleState;

static inline float DTTR_ImGui_NormalizeDesktopScale(float scale) {
	return scale > 0.0f ? scale : 1.0f;
}

static inline float DTTR_ImGui_GetWindowDesktopScale(SDL_Window *window) {
	return DTTR_ImGui_NormalizeDesktopScale(
		window ? SDL_GetWindowDisplayScale(window) : 0.0f
	);
}

static inline float DTTR_ImGui_GetCurrentDesktopScale(
	const DTTR_ImGuiDesktopScaleState *state
) {
	return DTTR_ImGui_NormalizeDesktopScale(
		state && state->initialized ? state->current_scale : 0.0f
	);
}

static inline bool DTTR_ImGui_ScaleChanged(float a, float b) {
	const float delta = a > b ? a - b : b - a;
	return delta > DTTR_IMGUI_SCALE_EPSILON;
}

static inline bool DTTR_ImGui_ApplyDesktopScale(
	DTTR_ImGuiDesktopScaleState *state,
	float scale
) {
	if (!state) {
		return false;
	}

	scale = DTTR_ImGui_NormalizeDesktopScale(scale);
	ImGuiStyle *style = igGetStyle();

	if (!state->initialized) {
		state->base_style = *style;
		state->initialized = true;
	} else if (!DTTR_ImGui_ScaleChanged(state->current_scale, scale)) {
		return false;
	} else {
		*style = state->base_style;
	}

	ImGuiStyle_ScaleAllSizes(style, scale);
	style->FontScaleMain = state->base_style.FontScaleMain * scale;
	state->current_scale = scale;
	return true;
}

static inline bool DTTR_ImGui_ApplyWindowDesktopScale(
	DTTR_ImGuiDesktopScaleState *state,
	SDL_Window *window
) {
	return DTTR_ImGui_ApplyDesktopScale(state, DTTR_ImGui_GetWindowDesktopScale(window));
}

typedef struct DTTR_ImGuiDialogContext {
	SDL_Window *window;
	SDL_GLContext gl_context;
	ImGuiContext *imgui_context;
	ImGuiContext *previous_imgui_context;
	DTTR_ImGuiDesktopScaleState imgui_scale;
	float desktop_scale;
	int logical_window_width;
	int logical_window_height;
	int logical_min_window_width;
	int logical_min_window_height;
	bool resizable;
	bool scaled_initial_size_applied;
	bool imgui_context_ready;
	bool imgui_sdl_ready;
	bool imgui_gl_ready;
} DTTR_ImGuiDialogContext;

bool DTTR_ImGuiDialog_Begin(
	DTTR_ImGuiDialogContext *ctx,
	const char *title,
	int width,
	int height
);
bool DTTR_ImGuiDialog_BeginResizable(
	DTTR_ImGuiDialogContext *ctx,
	const char *title,
	int width,
	int height,
	int min_width,
	int min_height
);
void DTTR_ImGuiDialog_SetWindowIconPNG(const unsigned char *png, size_t png_size);
void DTTR_ImGuiDialog_End(DTTR_ImGuiDialogContext *ctx);
void DTTR_ImGuiDialog_Shutdown();

float DTTR_ImGuiDialog_ScaledFloat(const DTTR_ImGuiDialogContext *ctx, float value);
void DTTR_ImGuiDialog_OffsetCursorY(const DTTR_ImGuiDialogContext *ctx, float amount);
bool DTTR_ImGuiDialog_RefreshScale(DTTR_ImGuiDialogContext *ctx);

void DTTR_ImGuiDialog_ProcessEvent(
	const DTTR_ImGuiDialogContext *ctx,
	const SDL_Event *event,
	bool *running
);
void DTTR_ImGuiDialog_ProcessEvents(const DTTR_ImGuiDialogContext *ctx, bool *running);
void DTTR_ImGuiDialog_NewFrame(const DTTR_ImGuiDialogContext *ctx);
void DTTR_ImGuiDialog_Render(DTTR_ImGuiDialogContext *ctx);

bool DTTR_ImGuiDialog_BeginRoot(
	DTTR_ImGuiDialogContext *ctx,
	const char *title,
	ImGuiWindowFlags flags
);
void DTTR_ImGuiDialog_EndRoot();

bool DTTR_ImGuiDialog_Button(
	const DTTR_ImGuiDialogContext *ctx,
	const char *id,
	const char *label,
	ImVec2_c size
);
void DTTR_ImGuiDialog_CenterNextItem(float item_width);
void DTTR_ImGuiDialog_DrawHeader(
	const DTTR_ImGuiDialogContext *ctx,
	const char *title,
	const char *version
);
void DTTR_ImGuiDialog_DrawPaddedText(
	const DTTR_ImGuiDialogContext *ctx,
	const char *message,
	float padding_x,
	float padding_y
);
void DTTR_ImGuiDialog_FitWindowToContent(
	DTTR_ImGuiDialogContext *ctx,
	int width,
	float padding_y
);

bool DTTR_ImGui_ErrorShow(const char *title, const char *message);

#ifdef __cplusplus
}

#endif

#endif // DTTR_IMGUI_H
