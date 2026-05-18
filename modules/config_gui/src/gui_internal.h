#ifndef DTTR_CONFIG_TOOL_INTERNAL_H
#define DTTR_CONFIG_TOOL_INTERNAL_H

#include <dttr_config.h>
#include <dttr_imgui.h>
#include <dttr_path.h>
#include <dttr_sdl.h>

#include <SDL3/SDL.h>

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define DTTR_CONFIG_UI_WINDOW_H 467
#define DTTR_CONFIG_UI_LABEL_W 185.0f
#define DTTR_CONFIG_UI_INPUT_W 300.0f
#define DTTR_CONFIG_UI_PATH_INPUT_W 180.0f
#define DTTR_CONFIG_UI_PATH_BUTTON_W 74.0f
#define DTTR_CONFIG_UI_GAMEPAD_SOURCE_W 145.0f
#define DTTR_CONFIG_UI_GAMEPAD_ACTION_W 155.0f
#define DTTR_CONFIG_UI_GAMEPAD_BUTTON_W 58.0f
#define DTTR_CONFIG_UI_PANEL_PADDING_X 18.0f
#define DTTR_CONFIG_UI_PANEL_PADDING_Y 10.0f
#define DTTR_CONFIG_UI_LABEL_TEXT_COLOR ((ImVec4_c){0.72f, 0.50f, 0.95f, 1.0f})
#define DTTR_CONFIG_UI_CHANGED_LABEL_TEXT_COLOR ((ImVec4_c){1.0f, 0.82f, 0.25f, 1.0f})
#define DTTR_CONFIG_UI_SAVED_CHANGED_LABEL_TEXT_COLOR                                    \
	((ImVec4_c){0.48f, 0.78f, 1.0f, 1.0f})
#define DTTR_CONFIG_UI_TOOLTIP_DEFAULT_TEXT_COLOR ((ImVec4_c){0.48f, 0.90f, 0.48f, 1.0f})
#define DTTR_CONFIG_UI_SELECTED_TAB_BG ((ImVec4_c){0.115f, 0.130f, 0.150f, 1.0f})
#define DTTR_CONFIG_UI_PATH_BUTTON_SPACING 4.0f
#define DTTR_CONFIG_UI_SPIN_BUTTON_SPACING 1.0f
#define DTTR_CONFIG_UI_ROW_MARGIN_X 1.0f
#define DTTR_CONFIG_UI_TABLE_CELL_PADDING_Y 2.0f
#define DTTR_CONFIG_UI_STATUS_TEXT_COLOR ((ImVec4_c){1.0f, 0.82f, 0.25f, 1.0f})
#define DTTR_CONFIG_UI_WARNING_TEXT_COLOR ((ImVec4_c){1.0f, 0.55f, 0.18f, 1.0f})
#define DTTR_CONFIG_UI_STATUS_TIMEOUT_MS 5000
#define DTTR_CONFIG_UI_STATUS_X 8.0f
#define DTTR_CONFIG_UI_STATUS_BOTTOM_MARGIN 8.0f
#define DTTR_CONFIG_UI_SCROLLBAR_WIDTH_SCALE 0.5f
#define DTTR_CONFIG_UI_HEADER_TOP_SPACING 0.0f
#define DTTR_CONFIG_UI_TOOLTIP_WRAP_W 360.0f

#ifndef DTTR_VERSION
#define DTTR_VERSION "unknown"
#endif

typedef enum {
	CONFIG_LABEL_DEFAULT,
	CONFIG_LABEL_SAVED_CHANGED,
	CONFIG_LABEL_UNSAVED,
} config_label_state;

typedef struct {
	DTTR_Config config;
	DTTR_Config defaults;
	DTTR_Config saved_config;
	char path[MAX_PATH];
	char mods_dir[MAX_PATH];
	char status[256];
	Uint64 status_expires_at_ms;
	int button_sources[DTTR_GAMEPAD_SOURCE_COUNT];
	int button_actions[DTTR_GAMEPAD_SOURCE_COUNT];
	int binding_row;
	bool show_shortcut_debug;
} config_ui_state;

#define FIELD_DIFFERS(state, base, field)                                                \
	(memcmp(&(state)->config.field, &(state)->base.field, sizeof((state)->config.field)) \
	 != 0)
#define FIELD_UNSAVED(state, field) FIELD_DIFFERS(state, saved_config, field)
#define FIELD_DEFAULT_CHANGED(state, field) FIELD_DIFFERS(state, defaults, field)
#define PATH_FIELD_DIFFERS(state, base, field)                                           \
	(!DTTR_Path_MatchesNormalized((state)->config.field, (state)->base.field))
#define PATH_FIELD_UNSAVED(state, field) PATH_FIELD_DIFFERS(state, saved_config, field)
#define PATH_FIELD_DEFAULT_CHANGED(state, field)                                         \
	PATH_FIELD_DIFFERS(state, defaults, field)
#define FIELD_LABEL_STATE(state, field)                                                  \
	make_config_label_state(                                                             \
		FIELD_UNSAVED(state, field),                                                     \
		FIELD_DEFAULT_CHANGED(state, field)                                              \
	)
#define PATH_FIELD_LABEL_STATE(state, field)                                             \
	make_config_label_state(                                                             \
		PATH_FIELD_UNSAVED(state, field),                                                \
		PATH_FIELD_DEFAULT_CHANGED(state, field)                                         \
	)

float config_standard_input_width();
int config_window_width();
void same_path_button_row(const DTTR_ImGuiDialogContext *ctx);
void add_scaled_vertical_spacing(const DTTR_ImGuiDialogContext *ctx, float height);
void align_next_item_right(float item_width);

void set_status(config_ui_state *state, const char *status);
void set_mods_dir_from_config_path(config_ui_state *state);
void sync_rows_from_config(config_ui_state *state);
void sync_config_from_rows(config_ui_state *state);
int gamepad_button_row_count();
int gamepad_button_row_action(int row);
const char *gamepad_button_row_label(int row);
int gamepad_default_source_for_action(const config_ui_state *state, int action);
const char *game_action_tooltip(int action);
void load_config(config_ui_state *state);
void save_config(config_ui_state *state);
void reset_defaults(config_ui_state *state);
void request_reset_defaults(const DTTR_ImGuiDialogContext *ctx, config_ui_state *state);
config_label_state make_config_label_state(bool unsaved_changed, bool default_changed);
bool gamepad_button_rows_have_unsaved_changes(const config_ui_state *state);
config_label_state gamepad_button_label_state(
	const config_ui_state *state,
	int source,
	int action
);
bool config_has_unsaved_changes(const config_ui_state *state);

bool choice_combo(
	const char *label,
	int *value,
	DTTR_ConfigChoiceList choices,
	const char *const *tooltips
);
void show_tooltip(const char *text);
bool themed_row_button(
	const DTTR_ImGuiDialogContext *ctx,
	const char *id,
	const char *label,
	float width
);
void push_config_theme();
void pop_config_theme();
void draw_bottom_status_text(
	const DTTR_ImGuiDialogContext *ctx,
	const config_ui_state *state
);
bool begin_padded_panel(const DTTR_ImGuiDialogContext *ctx, float width);
void end_padded_panel();
bool begin_settings_table_with_cell_padding_and_margins(
	const DTTR_ImGuiDialogContext *ctx,
	const char *id,
	float label_width,
	float input_width,
	float table_width,
	float cell_padding_x,
	float left_margin_width,
	float right_margin_width
);
bool begin_settings_table_with_width(
	const DTTR_ImGuiDialogContext *ctx,
	const char *id,
	float label_width,
	float input_width,
	float table_width
);
bool begin_full_width_settings_table(
	const DTTR_ImGuiDialogContext *ctx,
	const char *id,
	float label_width,
	float input_width
);
bool begin_settings_table(
	const DTTR_ImGuiDialogContext *ctx,
	const char *id,
	float label_width,
	float input_width
);
void end_settings_table();
float table_width_ignoring_scrollbar();
bool begin_gamepad_button_table(const DTTR_ImGuiDialogContext *ctx);
void begin_setting_row();
float table_input_width(const DTTR_ImGuiDialogContext *ctx, float input_width);
float path_text_input_width(const DTTR_ImGuiDialogContext *ctx, int button_count);
void draw_config_label(
	const char *label,
	const char *tooltip,
	config_label_state label_state
);
bool labeled_input_text(
	const DTTR_ImGuiDialogContext *ctx,
	const char *label,
	const char *id,
	char *buf,
	size_t buf_size,
	const char *tooltip,
	config_label_state label_state
);
bool labeled_path_picker(
	const DTTR_ImGuiDialogContext *ctx,
	config_ui_state *state,
	const char *label,
	const char *id,
	char *buf,
	size_t buf_size,
	const char *tooltip,
	config_label_state label_state
);
bool labeled_log_path_picker(
	const DTTR_ImGuiDialogContext *ctx,
	config_ui_state *state,
	const char *label,
	const char *id,
	char *buf,
	size_t buf_size,
	const char *tooltip,
	config_label_state label_state
);
bool labeled_input_int(
	const DTTR_ImGuiDialogContext *ctx,
	const char *label,
	const char *id,
	int *value,
	int step,
	int step_fast,
	const char *tooltip,
	config_label_state label_state
);
bool labeled_input_float(
	const DTTR_ImGuiDialogContext *ctx,
	const char *label,
	const char *id,
	float *value,
	const char *tooltip,
	config_label_state label_state
);
bool labeled_checkbox(
	const DTTR_ImGuiDialogContext *ctx,
	const char *label,
	const char *id,
	bool *value,
	const char *tooltip,
	config_label_state label_state
);
bool labeled_choice_combo(
	const DTTR_ImGuiDialogContext *ctx,
	const char *label,
	const char *id,
	int *value,
	DTTR_ConfigChoiceList choices,
	const char *const *tooltips,
	const char *tooltip,
	config_label_state label_state
);

void open_pcdogs_dir_dialog(const DTTR_ImGuiDialogContext *ctx, config_ui_state *state);
void open_pcdogs_iso_dialog(const DTTR_ImGuiDialogContext *ctx, config_ui_state *state);
void open_log_file_dialog(const DTTR_ImGuiDialogContext *ctx, config_ui_state *state);

const char *source_label(int source);
const char *source_tooltip(int source);
int source_from_event(const SDL_Event *event);
bool event_cancels_binding(const SDL_Event *event);
void cancel_binding(config_ui_state *state);
void capture_source(config_ui_state *state, int new_source);
bool begin_tab_settings_table(
	const DTTR_ImGuiDialogContext *ctx,
	const char *id,
	float input_width
);
void draw_general_tab(const DTTR_ImGuiDialogContext *ctx, config_ui_state *state);
void draw_graphics_tab(const DTTR_ImGuiDialogContext *ctx, config_ui_state *state);
void draw_audio_tab(const DTTR_ImGuiDialogContext *ctx, config_ui_state *state);
void draw_gamepad_tab(const DTTR_ImGuiDialogContext *ctx, config_ui_state *state);
void draw_modding_tab(const DTTR_ImGuiDialogContext *ctx, config_ui_state *state);
void draw_tabs(const DTTR_ImGuiDialogContext *ctx, config_ui_state *state);

#endif
