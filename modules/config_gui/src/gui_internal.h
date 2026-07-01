#ifndef DTTR_CONFIG_TOOL_INTERNAL_H
#define DTTR_CONFIG_TOOL_INTERNAL_H

#include <dttr_config.h>
#include <dttr_imgui.h>
#include <dttr_input_binding.h>
#include <dttr_mods.h>
#include <dttr_path.h>
#include <dttr_sdl.h>

#include <SDL3/SDL.h>

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define DTTR_CONFIG_UI_WINDOW_H 550
#define DTTR_CONFIG_UI_LABEL_W 170.0f
#define DTTR_CONFIG_UI_INPUT_W 285.0f
#define DTTR_CONFIG_UI_PATH_INPUT_W 160.0f
#define DTTR_CONFIG_UI_PATH_BUTTON_W 70.0f
#define DTTR_CONFIG_UI_MOD_BINDING_BUTTON_W 46.0f
#define DTTR_CONFIG_UI_BINDING_VALUE_W 180.0f
#define DTTR_CONFIG_UI_PANEL_PADDING_X 7.5f
#define DTTR_CONFIG_UI_PANEL_PADDING_Y 14.0f
#define DTTR_CONFIG_UI_ITEM_SPACING_X 8.0f
#define DTTR_CONFIG_UI_ITEM_SPACING_Y 6.0f
#define DTTR_CONFIG_UI_TABLE_CELL_PADDING_X 8.0f
#define DTTR_CONFIG_UI_TABLE_CELL_PADDING_Y 4.0f
#define DTTR_CONFIG_UI_HEADER_TEXT_INSET_X 2.0f
#define DTTR_CONFIG_UI_COMBO_POPUP_PADDING_Y 0.0f
#define DTTR_CONFIG_UI_SECTION_SPACING 10.0f
#define DTTR_CONFIG_UI_LABEL_TEXT_COLOR ((ImVec4_c){0.72f, 0.50f, 0.95f, 1.0f})
#define DTTR_CONFIG_UI_CHANGED_LABEL_TEXT_COLOR ((ImVec4_c){1.0f, 0.82f, 0.25f, 1.0f})
#define DTTR_CONFIG_UI_SAVED_CHANGED_LABEL_TEXT_COLOR                                    \
	((ImVec4_c){0.48f, 0.78f, 1.0f, 1.0f})
#define DTTR_CONFIG_UI_TOOLTIP_DEFAULT_TEXT_COLOR ((ImVec4_c){0.48f, 0.90f, 0.48f, 1.0f})
#define DTTR_CONFIG_UI_SELECTED_TAB_BG ((ImVec4_c){0.115f, 0.130f, 0.150f, 1.0f})
#define DTTR_CONFIG_UI_BORDER_COLOR ((ImVec4_c){0.105f, 0.115f, 0.125f, 1.0f})
#define DTTR_CONFIG_UI_TABLE_BORDER_COLOR ((ImVec4_c){0.135f, 0.145f, 0.155f, 1.0f})
#define DTTR_CONFIG_UI_SEPARATOR_COLOR ((ImVec4_c){0.120f, 0.130f, 0.140f, 1.0f})
#define DTTR_CONFIG_UI_PATH_BUTTON_SPACING 4.0f
#define DTTR_CONFIG_UI_STATUS_TEXT_COLOR ((ImVec4_c){1.0f, 0.82f, 0.25f, 1.0f})
#define DTTR_CONFIG_UI_HINT_TEXT_COLOR ((ImVec4_c){0.72f, 0.74f, 0.78f, 1.0f})
#define DTTR_CONFIG_UI_WARNING_TEXT_COLOR ((ImVec4_c){1.0f, 0.55f, 0.18f, 1.0f})
#define DTTR_CONFIG_UI_STATUS_TIMEOUT_MS 5000
#define DTTR_CONFIG_UI_HEADER_TOP_SPACING 0.0f
#define DTTR_CONFIG_UI_TOOLTIP_PADDING_Y 4.0f
#define DTTR_CONFIG_UI_TOOLTIP_WRAP_W 360.0f
#define DTTR_CONFIG_UI_MOD_CONFIG_LABEL_MAX 96
#define DTTR_CONFIG_UI_MOD_CONFIG_TOOLTIP_MAX 256
#define DTTR_CONFIG_UI_MOD_CONFIG_FIELDS_MAX DTTR_CONFIG_MOD_VALUES_MAX
#define DTTR_CONFIG_UI_MOD_CONFIG_CHOICES_MAX DTTR_CONFIG_MOD_VALUES_MAX

#define CONFIG_TABLE_FLAGS                                                               \
	(ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_BordersOuterH                       \
	 | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_NoSavedSettings               \
	 | ImGuiTableFlags_PadOuterX)

#ifndef DTTR_VERSION
#define DTTR_VERSION "unknown"
#endif

typedef bool (*mod_dll_visitor)(const char *dll_name, void *user_data);

typedef enum {
	CONFIG_LABEL_DEFAULT,
	CONFIG_LABEL_SAVED_CHANGED,
	CONFIG_LABEL_UNSAVED,
} config_label_state;

typedef struct {
	const char *label;
	const char *tooltip;
	config_label_state label_state;
	const char *display;
	const char *capture_display;
	bool capturing;
	bool show_clear_button;
	bool show_reset_button;
	const char *bind_tooltip;
	const char *clear_tooltip;
	const char *reset_tooltip;
} config_binding_row_spec;

typedef struct {
	bool bind_clicked;
	bool clear_clicked;
	bool reset_clicked;
} config_binding_row_result;

typedef struct {
	char value[DTTR_CONFIG_MOD_STRING_MAX];
	char label[DTTR_CONFIG_UI_MOD_CONFIG_LABEL_MAX];
	char tooltip[DTTR_CONFIG_UI_MOD_CONFIG_TOOLTIP_MAX];
} config_mod_choice_ui;

typedef struct {
	int mod_index;
	char id[DTTR_CONFIG_MOD_FIELD_ID_MAX];
	char label[DTTR_CONFIG_UI_MOD_CONFIG_LABEL_MAX];
	char tooltip[DTTR_CONFIG_UI_MOD_CONFIG_TOOLTIP_MAX];
	DTTR_Mods_ConfigFieldType type;
	DTTR_Mods_ConfigDefaultValue default_value;
	char default_string[DTTR_CONFIG_MOD_STRING_MAX];
	int int_min;
	int int_max;
	float float_min;
	float float_max;
	int first_choice;
	int choice_count;
} config_mod_field_ui;

typedef struct {
	char dll_name[MAX_PATH];
	char mod_id[DTTR_CONFIG_MOD_ID_MAX];
	char label[DTTR_CONFIG_UI_MOD_CONFIG_LABEL_MAX];
	uint32_t schema_version;
	int first_field;
	int field_count;
} config_mod_ui;

typedef struct {
	DTTR_Config config;
	DTTR_Config defaults;
	DTTR_Config saved_config;
	char path[MAX_PATH];
	char mods_dir[MAX_PATH];
	int mod_config_count;
	config_mod_ui mod_configs[DTTR_CONFIG_MOD_CONFIGS_MAX];
	int mod_config_field_count;
	config_mod_field_ui mod_config_fields[DTTR_CONFIG_UI_MOD_CONFIG_FIELDS_MAX];
	int mod_config_choice_count;
	config_mod_choice_ui mod_config_choices[DTTR_CONFIG_UI_MOD_CONFIG_CHOICES_MAX];
	char status[256];
	Uint64 status_expires_at_ms;
	SDL_Gamepad *preview_gamepad;
	int preview_gamepad_index;
	bool input_binding_capturing;
	char input_binding_mod_id[DTTR_CONFIG_MOD_ID_MAX];
	char input_binding_field_id[DTTR_CONFIG_MOD_FIELD_ID_MAX];
	int control_binding_action;
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

void for_each_mod_dll(const char *mods_dir, mod_dll_visitor visit, void *user_data);

float config_standard_input_width();
int config_window_width();
void add_scaled_vertical_spacing(const DTTR_ImGuiDialogContext *ctx, float height);

void set_status(config_ui_state *state, const char *status);
void set_mods_dir_from_config_path(config_ui_state *state);
void load_config(config_ui_state *state);
void save_config(config_ui_state *state);
void reset_defaults(config_ui_state *state);
void reload_mod_config_specs(config_ui_state *state);
void close_gamepad_preview(config_ui_state *state);
void request_reset_defaults(const DTTR_ImGuiDialogContext *ctx, config_ui_state *state);
config_label_state make_config_label_state(bool unsaved_changed, bool default_changed);
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
bool begin_config_content_region(
	const DTTR_ImGuiDialogContext *ctx,
	const config_ui_state *state
);
void end_config_content_region();
void draw_footer_text(const DTTR_ImGuiDialogContext *ctx, const config_ui_state *state);
bool begin_padded_panel(const DTTR_ImGuiDialogContext *ctx);
void end_padded_panel();
bool begin_settings_table_with_width(
	const DTTR_ImGuiDialogContext *ctx,
	const char *id,
	float label_width,
	float input_width,
	float table_width
);
bool begin_settings_table(
	const DTTR_ImGuiDialogContext *ctx,
	const char *id,
	float label_width,
	float input_width
);
void end_settings_table();
void begin_config_table_row();
void begin_setting_row();
float table_input_width(const DTTR_ImGuiDialogContext *ctx, float input_width);
float path_text_input_width(const DTTR_ImGuiDialogContext *ctx, int button_count);
void draw_config_label(
	const char *label,
	const char *tooltip,
	config_label_state label_state
);
config_binding_row_result draw_config_binding_row(
	const DTTR_ImGuiDialogContext *ctx,
	const config_binding_row_spec *spec
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

bool event_cancels_binding(const SDL_Event *event);
void begin_input_binding_capture(
	config_ui_state *state,
	const char *mod_id,
	const char *field_id
);
void cancel_input_binding_capture(config_ui_state *state);
bool capture_input_binding_event(config_ui_state *state, const SDL_Event *event);
void begin_control_binding_capture(config_ui_state *state, int action);
bool input_binding_field_capturing(
	const config_ui_state *state,
	const char *mod_id,
	const char *field_id
);
bool control_binding_field_capturing(const config_ui_state *state, int action);
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
