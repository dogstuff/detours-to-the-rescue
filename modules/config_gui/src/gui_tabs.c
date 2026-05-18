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
	{"Gamepad", draw_gamepad_tab},
#ifdef DTTR_MODS_ENABLED
	{"Modding", draw_modding_tab},
#endif
};

void draw_tabs(const DTTR_ImGuiDialogContext *ctx, config_ui_state *state) {
	if (!igBeginTabBar("dttr_config_tabs", ImGuiTabBarFlags_None)) {
		return;
	}

	for (int i = 0; i < (int)SDL_arraysize(CONFIG_TABS); i++) {
		const config_tab_spec *tab = &CONFIG_TABS[i];
		if (!igBeginTabItem(tab->label, NULL, ImGuiTabItemFlags_None)) {
			continue;
		}

		tab->draw(ctx, state);
		igEndTabItem();
	}

	igEndTabBar();
}
