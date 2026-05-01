#ifndef DTTR_IMGUI_H
#define DTTR_IMGUI_H

#include <SDL3/SDL.h>
#include <cimgui.h>
#include <cimgui_impl.h>
#include <cimgui_impl_sdlgpu3.h>
#include <stdbool.h>

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
	ImGuiStyle m_base_style;
	float m_current_scale;
	bool m_initialized;
} DTTR_ImGuiDesktopScaleState;

static inline float dttr_imgui_normalize_desktop_scale(float scale) {
	return scale > 0.0f ? scale : 1.0f;
}

static inline float dttr_imgui_get_window_desktop_scale(SDL_Window *window) {
	return dttr_imgui_normalize_desktop_scale(
		window ? SDL_GetWindowDisplayScale(window) : 0.0f
	);
}

static inline float dttr_imgui_get_current_desktop_scale(
	const DTTR_ImGuiDesktopScaleState *state
) {
	return dttr_imgui_normalize_desktop_scale(
		state && state->m_initialized ? state->m_current_scale : 0.0f
	);
}

static inline bool dttr_imgui_scale_changed(float a, float b) {
	const float delta = a > b ? a - b : b - a;
	return delta > DTTR_IMGUI_SCALE_EPSILON;
}

static inline bool dttr_imgui_apply_desktop_scale(
	DTTR_ImGuiDesktopScaleState *state,
	float scale
) {
	if (!state) {
		return false;
	}

	scale = dttr_imgui_normalize_desktop_scale(scale);
	ImGuiStyle *style = igGetStyle();

	if (!state->m_initialized) {
		state->m_base_style = *style;
		state->m_initialized = true;
	} else if (!dttr_imgui_scale_changed(state->m_current_scale, scale)) {
		return false;
	} else {
		*style = state->m_base_style;
	}

	ImGuiStyle_ScaleAllSizes(style, scale);
	style->FontScaleMain = state->m_base_style.FontScaleMain * scale;
	state->m_current_scale = scale;
	return true;
}

static inline bool dttr_imgui_apply_window_desktop_scale(
	DTTR_ImGuiDesktopScaleState *state,
	SDL_Window *window
) {
	return dttr_imgui_apply_desktop_scale(
		state,
		dttr_imgui_get_window_desktop_scale(window)
	);
}

typedef struct DTTR_ImGuiDialogContext {
	SDL_Window *m_window;
	SDL_GLContext m_gl_context;
	DTTR_ImGuiDesktopScaleState m_imgui_scale;
	float m_desktop_scale;
	bool m_imgui_context_ready;
	bool m_imgui_sdl_ready;
	bool m_imgui_gl_ready;
} DTTR_ImGuiDialogContext;

bool dttr_imgui_dialog_begin(
	DTTR_ImGuiDialogContext *ctx,
	const char *title,
	int width,
	int height
);
void dttr_imgui_dialog_end(DTTR_ImGuiDialogContext *ctx);
void dttr_imgui_dialog_shutdown(void);

float dttr_imgui_dialog_scaled_float(const DTTR_ImGuiDialogContext *ctx, float value);
int dttr_imgui_dialog_scaled_int(const DTTR_ImGuiDialogContext *ctx, float value);
bool dttr_imgui_dialog_refresh_scale(DTTR_ImGuiDialogContext *ctx);

void dttr_imgui_dialog_process_events(bool *running);
void dttr_imgui_dialog_new_frame(void);
void dttr_imgui_dialog_render(DTTR_ImGuiDialogContext *ctx);

bool dttr_imgui_dialog_begin_root(DTTR_ImGuiDialogContext *ctx, const char *title);
void dttr_imgui_dialog_end_root(void);

bool dttr_imgui_dialog_button(
	const DTTR_ImGuiDialogContext *ctx,
	const char *id,
	const char *label,
	ImVec2_c size
);
void dttr_imgui_dialog_center_next_item(float item_width);
void dttr_imgui_dialog_draw_header(
	const DTTR_ImGuiDialogContext *ctx,
	const char *title,
	const char *version
);
void dttr_imgui_dialog_draw_padded_text(
	const DTTR_ImGuiDialogContext *ctx,
	const char *message,
	float padding_x,
	float padding_y
);
void dttr_imgui_dialog_fit_window_to_content(
	DTTR_ImGuiDialogContext *ctx,
	int width,
	float padding_y
);

bool dttr_imgui_error_show(const char *title, const char *message);

#ifdef __cplusplus
}
#endif

#endif /* DTTR_IMGUI_H */
