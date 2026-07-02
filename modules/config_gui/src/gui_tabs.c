#include "gui_internal.h"

typedef void (*config_tab_draw_fn)(
	const DTTR_ImGuiDialogContext *ctx,
	config_ui_state *state
);

typedef struct {
	const char *label;
	config_tab_draw_fn draw;
} config_tab_spec;

bool begin_tab_settings_table(
	const DTTR_ImGuiDialogContext *ctx,
	const char *id,
	float input_width
) {
	return begin_settings_table(ctx, id, DTTR_CONFIG_UI_LABEL_W, input_width);
}

static const config_tab_spec CONFIG_TABS[] = {
	{"General", draw_general_tab},
	{"Graphics", draw_graphics_tab},
	{"Audio", draw_audio_tab},
	{"Controls", draw_controls_tab},
#ifdef DTTR_MODS_ENABLED
	{"Modding", draw_modding_tab},
#endif
};

static void draw_selected_tab_accent(const DTTR_ImGuiDialogContext *ctx) {
	const ImVec2_c tab_min = igGetItemRectMin();
	const ImVec2_c tab_max = igGetItemRectMax();
	const float accent_h = DTTR_ImGuiDialog_ScaledFloat(ctx, DTTR_CONFIG_UI_TAB_ACCENT_H);

	ImDrawList_AddRectFilled(
		igGetWindowDrawList(),
		(ImVec2_c){tab_min.x, tab_max.y - accent_h},
		tab_max,
		igGetColorU32_Vec4(DTTR_CONFIG_UI_TAB_ACCENT_COLOR),
		0.0f,
		ImDrawFlags_None
	);
}

void draw_tabs(const DTTR_ImGuiDialogContext *ctx, config_ui_state *state) {
	if (!igBeginTabBar("dttr_config_tabs", ImGuiTabBarFlags_None)) {
		return;
	}

	for (int i = 0; i < (int)SDL_arraysize(CONFIG_TABS); i++) {
		const config_tab_spec *tab = &CONFIG_TABS[i];
		const bool open = igBeginTabItem(tab->label, NULL, ImGuiTabItemFlags_None);
		if (!open) {
			continue;
		}

		draw_selected_tab_accent(ctx);
		tab->draw(ctx, state);
		igEndTabItem();
	}

	igEndTabBar();
}
