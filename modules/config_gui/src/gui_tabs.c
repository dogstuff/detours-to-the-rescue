#include "gui_internal.h"

typedef void (*S_ConfigTabDrawFn)(
	const DTTR_ImGuiDialogContext *ctx,
	S_ConfigUIState *state
);

typedef struct {
	const char *m_label;
	S_ConfigTabDrawFn m_draw;
} S_ConfigTabSpec;

bool s_begin_tab_settings_table(
	const DTTR_ImGuiDialogContext *ctx,
	const char *id,
	float input_width
) {
	return s_begin_settings_table(ctx, id, DTTR_CONFIG_UI_LABEL_W, input_width);
}

static void s_draw_tab_item(
	const char *label,
	S_ConfigTabDrawFn draw_tab,
	const DTTR_ImGuiDialogContext *ctx,
	S_ConfigUIState *state
) {
	if (!igBeginTabItem(label, NULL, ImGuiTabItemFlags_None)) {
		return;
	}

	draw_tab(ctx, state);
	igEndTabItem();
}

static const S_ConfigTabSpec S_CONFIG_TABS[] = {
	{"General", s_draw_general_tab},
	{"Graphics", s_draw_graphics_tab},
	{"Audio", s_draw_audio_tab},
	{"Gamepad", s_draw_gamepad_tab},
	{"Modding", s_draw_modding_tab},
};

void s_draw_tabs(const DTTR_ImGuiDialogContext *ctx, S_ConfigUIState *state) {
	if (!igBeginTabBar("dttr_config_tabs", ImGuiTabBarFlags_None)) {
		return;
	}

	for (int i = 0; i < (int)SDL_arraysize(S_CONFIG_TABS); i++) {
		s_draw_tab_item(S_CONFIG_TABS[i].m_label, S_CONFIG_TABS[i].m_draw, ctx, state);
	}

	igEndTabBar();
}
