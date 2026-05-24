#include "gui_internal.h"

static const char *const LOG_LEVEL_TOOLTIPS[] = {
	"Logs every message, including detailed trace output.",
	"Logs debug, info, warning, error, and fatal messages.",
	"Logs normal runtime messages and higher-severity messages.",
	"Logs warnings, errors, and fatal messages.",
	"Logs errors and fatal messages only.",
	"Logs only fatal messages.",
};

static const char *const MINIDUMP_TYPE_TOOLTIPS[] = {
	"Writes a standard crash minidump.",
	"Writes a larger crash minidump with additional details.",
};

static const char *TOOLTIP_PCDOGS_PATH = "Installed game directory or original ISO.";
static const char *TOOLTIP_SAVES_PATH = "Redirect saves and logs under the DttR "
										"directory. Use null or \"\" to disable. "
										"Default: saves.";
static const char *TOOLTIP_LOG_LEVEL = "Minimum log level. Default: info (Release), "
									   "debug (Debug).";
static const char *TOOLTIP_LOG_FILE_PATH = "DttR log file path. Default: dttr.log.";
static const char *TOOLTIP_MINIDUMP_TYPE = "Crash dump detail. Default: normal "
										   "(Release), detailed (Debug).";
static const char *TOOLTIP_SKIP_INTRO_MOVIES = "Skip intro movies at launch. Default: "
											   "false.";

void draw_general_tab(const DTTR_ImGuiDialogContext *ctx, config_ui_state *state) {
	if (!begin_tab_settings_table(
			ctx,
			"##general_settings_table",
			config_standard_input_width()
		)) {
		return;
	}

	labeled_path_picker(
		ctx,
		state,
		"Game directory or ISO",
		"##pcdogs_path",
		state->config.pcdogs_path,
		sizeof(state->config.pcdogs_path),
		TOOLTIP_PCDOGS_PATH,
		PATH_FIELD_LABEL_STATE(state, pcdogs_path)
	);
	labeled_input_text(
		ctx,
		"Saves path",
		"##saves_path",
		state->config.saves_path,
		sizeof(state->config.saves_path),
		TOOLTIP_SAVES_PATH,
		PATH_FIELD_LABEL_STATE(state, saves_path)
	);
	labeled_log_path_picker(
		ctx,
		state,
		"Log file path",
		"##log_file_path",
		state->config.log_file_path,
		sizeof(state->config.log_file_path),
		TOOLTIP_LOG_FILE_PATH,
		PATH_FIELD_LABEL_STATE(state, log_file_path)
	);
	labeled_choice_combo(
		ctx,
		"Log level",
		"##log_level",
		&state->config.log_level,
		DTTR_CONFIG_CHOICES_LOG_LEVEL,
		LOG_LEVEL_TOOLTIPS,
		TOOLTIP_LOG_LEVEL,
		FIELD_LABEL_STATE(state, log_level)
	);
	labeled_choice_combo(
		ctx,
		"Minidump type",
		"##minidump_type",
		(int *)&state->config.minidump_type,
		DTTR_CONFIG_CHOICES_MINIDUMP_TYPE,
		MINIDUMP_TYPE_TOOLTIPS,
		TOOLTIP_MINIDUMP_TYPE,
		FIELD_LABEL_STATE(state, minidump_type)
	);
	labeled_checkbox(
		ctx,
		"Skip intro movies",
		"##skip_intro_movies",
		&state->config.skip_intro_movies,
		TOOLTIP_SKIP_INTRO_MOVIES,
		FIELD_LABEL_STATE(state, skip_intro_movies)
	);
	end_settings_table();
}
