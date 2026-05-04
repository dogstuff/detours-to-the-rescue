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

static const S_ConfigTabSpec S_CONFIG_TABS[] = {
	{"General", s_draw_general_tab},
	{"Graphics", s_draw_graphics_tab},
	{"Audio", s_draw_audio_tab},
	{"Gamepad", s_draw_gamepad_tab},
#ifdef DTTR_MODDING_ENABLED
	{"Modding", s_draw_modding_tab},
#endif
};

void s_draw_tabs(const DTTR_ImGuiDialogContext *ctx, S_ConfigUIState *state) {
	if (!igBeginTabBar("dttr_config_tabs", ImGuiTabBarFlags_None)) {
		return;
	}

	for (int i = 0; i < (int)SDL_arraysize(S_CONFIG_TABS); i++) {
		const S_ConfigTabSpec *tab = &S_CONFIG_TABS[i];
		if (!igBeginTabItem(tab->m_label, NULL, ImGuiTabItemFlags_None)) {
			continue;
		}

		tab->m_draw(ctx, state);
		igEndTabItem();
	}

	igEndTabBar();
}
