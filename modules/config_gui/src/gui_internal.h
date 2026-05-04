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
	S_CONFIG_LABEL_DEFAULT,
	S_CONFIG_LABEL_SAVED_CHANGED,
	S_CONFIG_LABEL_UNSAVED,
} S_ConfigLabelState;

typedef struct {
	DTTR_Config m_config;
	DTTR_Config m_defaults;
	DTTR_Config m_saved_config;
	char m_path[MAX_PATH];
	char m_components_dir[MAX_PATH];
	char m_status[256];
	Uint64 m_status_expires_at_ms;
	int m_button_sources[DTTR_GAMEPAD_SOURCE_COUNT];
	int m_button_actions[DTTR_GAMEPAD_SOURCE_COUNT];
	int m_binding_row;
	bool m_show_shortcut_debug;
} S_ConfigUIState;

#define S_FIELD_DIFFERS(state, base, field)                                              \
	(memcmp(                                                                             \
		 &(state)->m_config.field,                                                       \
		 &(state)->base.field,                                                           \
		 sizeof((state)->m_config.field)                                                 \
	 )                                                                                   \
	 != 0)
#define S_FIELD_UNSAVED(state, field) S_FIELD_DIFFERS(state, m_saved_config, field)
#define S_FIELD_DEFAULT_CHANGED(state, field) S_FIELD_DIFFERS(state, m_defaults, field)
#define S_PATH_FIELD_DIFFERS(state, base, field)                                         \
	(!dttr_path_matches_normalized((state)->m_config.field, (state)->base.field))
#define S_PATH_FIELD_UNSAVED(state, field)                                               \
	S_PATH_FIELD_DIFFERS(state, m_saved_config, field)
#define S_PATH_FIELD_DEFAULT_CHANGED(state, field)                                       \
	S_PATH_FIELD_DIFFERS(state, m_defaults, field)
#define S_FIELD_LABEL_STATE(state, field)                                                \
	s_config_label_state(                                                                \
		S_FIELD_UNSAVED(state, field),                                                   \
		S_FIELD_DEFAULT_CHANGED(state, field)                                            \
	)
#define S_PATH_FIELD_LABEL_STATE(state, field)                                           \
	s_config_label_state(                                                                \
		S_PATH_FIELD_UNSAVED(state, field),                                              \
		S_PATH_FIELD_DEFAULT_CHANGED(state, field)                                       \
	)

float s_config_standard_input_width(void);
int s_config_window_width(void);
void s_same_path_button_row(const DTTR_ImGuiDialogContext *ctx);
void s_add_scaled_vertical_spacing(const DTTR_ImGuiDialogContext *ctx, float height);
void s_align_next_item_right(float item_width);

void s_set_status(S_ConfigUIState *state, const char *status);
void s_set_components_dir_from_config_path(S_ConfigUIState *state);
void s_sync_rows_from_config(S_ConfigUIState *state);
void s_sync_config_from_rows(S_ConfigUIState *state);
void s_load_config(S_ConfigUIState *state);
void s_save_config(S_ConfigUIState *state);
void s_reset_defaults(S_ConfigUIState *state);
void s_request_reset_defaults(const DTTR_ImGuiDialogContext *ctx, S_ConfigUIState *state);
S_ConfigLabelState s_config_label_state(bool unsaved_changed, bool default_changed);
bool s_gamepad_button_rows_have_unsaved_changes(const S_ConfigUIState *state);
S_ConfigLabelState s_gamepad_button_label_state(
	const S_ConfigUIState *state,
	int source,
	int action
);
bool s_config_has_unsaved_changes(const S_ConfigUIState *state);

bool s_choice_combo(
	const char *label,
	int *value,
	DTTR_ConfigChoiceList choices,
	const char *const *tooltips
);
void s_show_tooltip(const char *text);
bool s_themed_row_button(
	const DTTR_ImGuiDialogContext *ctx,
	const char *id,
	const char *label,
	float width
);
void s_push_config_theme(void);
void s_pop_config_theme(void);
void s_draw_bottom_status_text(
	const DTTR_ImGuiDialogContext *ctx,
	const S_ConfigUIState *state
);
bool s_begin_padded_panel(const DTTR_ImGuiDialogContext *ctx, float width);
void s_end_padded_panel(void);
bool s_begin_settings_table_with_cell_padding_and_margins(
	const DTTR_ImGuiDialogContext *ctx,
	const char *id,
	float label_width,
	float input_width,
	float table_width,
	float cell_padding_x,
	float left_margin_width,
	float right_margin_width
);
bool s_begin_settings_table_with_width(
	const DTTR_ImGuiDialogContext *ctx,
	const char *id,
	float label_width,
	float input_width,
	float table_width
);
bool s_begin_full_width_settings_table(
	const DTTR_ImGuiDialogContext *ctx,
	const char *id,
	float label_width,
	float input_width
);
bool s_begin_settings_table(
	const DTTR_ImGuiDialogContext *ctx,
	const char *id,
	float label_width,
	float input_width
);
void s_end_settings_table(void);
float s_table_width_ignoring_scrollbar(void);
bool s_begin_gamepad_button_table(const DTTR_ImGuiDialogContext *ctx);
void s_begin_setting_row(void);
float s_table_input_width(const DTTR_ImGuiDialogContext *ctx, float input_width);
float s_path_text_input_width(const DTTR_ImGuiDialogContext *ctx, int button_count);
void s_draw_config_label(
	const char *label,
	const char *tooltip,
	S_ConfigLabelState label_state
);
bool s_labeled_input_text(
	const DTTR_ImGuiDialogContext *ctx,
	const char *label,
	const char *id,
	char *buf,
	size_t buf_size,
	const char *tooltip,
	S_ConfigLabelState label_state
);
bool s_labeled_path_picker(
	const DTTR_ImGuiDialogContext *ctx,
	S_ConfigUIState *state,
	const char *label,
	const char *id,
	char *buf,
	size_t buf_size,
	const char *tooltip,
	S_ConfigLabelState label_state
);
bool s_labeled_log_path_picker(
	const DTTR_ImGuiDialogContext *ctx,
	S_ConfigUIState *state,
	const char *label,
	const char *id,
	char *buf,
	size_t buf_size,
	const char *tooltip,
	S_ConfigLabelState label_state
);
bool s_labeled_input_int(
	const DTTR_ImGuiDialogContext *ctx,
	const char *label,
	const char *id,
	int *value,
	int step,
	int step_fast,
	const char *tooltip,
	S_ConfigLabelState label_state
);
bool s_labeled_input_float(
	const DTTR_ImGuiDialogContext *ctx,
	const char *label,
	const char *id,
	float *value,
	const char *tooltip,
	S_ConfigLabelState label_state
);
bool s_labeled_checkbox(
	const DTTR_ImGuiDialogContext *ctx,
	const char *label,
	const char *id,
	bool *value,
	const char *tooltip,
	S_ConfigLabelState label_state
);
bool s_labeled_choice_combo(
	const DTTR_ImGuiDialogContext *ctx,
	const char *label,
	const char *id,
	int *value,
	DTTR_ConfigChoiceList choices,
	const char *const *tooltips,
	const char *tooltip,
	S_ConfigLabelState label_state
);

void s_open_pcdogs_dir_dialog(const DTTR_ImGuiDialogContext *ctx, S_ConfigUIState *state);
void s_open_pcdogs_iso_dialog(const DTTR_ImGuiDialogContext *ctx, S_ConfigUIState *state);
void s_open_log_file_dialog(const DTTR_ImGuiDialogContext *ctx, S_ConfigUIState *state);

const char *s_source_label(int source);
const char *s_source_tooltip(int source);
int s_source_from_event(const SDL_Event *event);
bool s_event_cancels_binding(const SDL_Event *event);
void s_cancel_binding(S_ConfigUIState *state);
void s_capture_source(S_ConfigUIState *state, int new_source);
bool s_begin_tab_settings_table(
	const DTTR_ImGuiDialogContext *ctx,
	const char *id,
	float input_width
);
void s_draw_general_tab(const DTTR_ImGuiDialogContext *ctx, S_ConfigUIState *state);
void s_draw_graphics_tab(const DTTR_ImGuiDialogContext *ctx, S_ConfigUIState *state);
void s_draw_audio_tab(const DTTR_ImGuiDialogContext *ctx, S_ConfigUIState *state);
void s_draw_gamepad_tab(const DTTR_ImGuiDialogContext *ctx, S_ConfigUIState *state);
void s_draw_modding_tab(const DTTR_ImGuiDialogContext *ctx, S_ConfigUIState *state);
void s_draw_tabs(const DTTR_ImGuiDialogContext *ctx, S_ConfigUIState *state);

#endif
