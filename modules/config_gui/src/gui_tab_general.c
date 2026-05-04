#include "gui_internal.h"

static const char *const S_LOG_LEVEL_TOOLTIPS[] = {
	"Logs every message, including detailed trace output.",
	"Logs debug, info, warning, error, and fatal messages.",
	"Logs normal runtime messages and higher-severity messages.",
	"Logs warnings, errors, and fatal messages.",
	"Logs errors and fatal messages only.",
	"Logs only fatal messages.",
};

static const char *const S_MINIDUMP_TYPE_TOOLTIPS[] = {
	"Writes a standard crash minidump.",
	"Writes a larger crash minidump with additional details.",
};

static const char *S_TOOLTIP_PCDOGS_PATH = "The extracted/installed game directory or "
										   "original game ISO.";
static const char *S_TOOLTIP_SAVES_PATH
	= "Path for redirected saves/logs reads and writes, relative to the DttR directory. "
	  "Files are stored in subdirectories unique to the version of the game being run. "
	  "Set to null or \"\" to disable redirection. Default: saves.";
static const char *S_TOOLTIP_LOG_LEVEL = "The minimum log level to output. Default: "
										 "info (Release), debug (Debug).";
static const char *S_TOOLTIP_LOG_FILE_PATH = "Path to the DttR log file. Default: "
											 "dttr.log.";
static const char *S_TOOLTIP_MINIDUMP_TYPE = "The type of minidump to write on crash. "
											 "Default: normal (Release), detailed "
											 "(Debug).";

void s_draw_general_tab(const DTTR_ImGuiDialogContext *ctx, S_ConfigUIState *state) {
	if (!s_begin_tab_settings_table(
			ctx,
			"##general_settings_table",
			s_config_standard_input_width()
		)) {
		return;
	}

	s_labeled_path_picker(
		ctx,
		state,
		"Game directory or ISO",
		"##pcdogs_path",
		state->m_config.m_pcdogs_path,
		sizeof(state->m_config.m_pcdogs_path),
		S_TOOLTIP_PCDOGS_PATH,
		S_PATH_FIELD_LABEL_STATE(state, m_pcdogs_path)
	);
	s_labeled_input_text(
		ctx,
		"Saves path",
		"##saves_path",
		state->m_config.m_saves_path,
		sizeof(state->m_config.m_saves_path),
		S_TOOLTIP_SAVES_PATH,
		S_PATH_FIELD_LABEL_STATE(state, m_saves_path)
	);
	s_labeled_log_path_picker(
		ctx,
		state,
		"Log file path",
		"##log_file_path",
		state->m_config.m_log_file_path,
		sizeof(state->m_config.m_log_file_path),
		S_TOOLTIP_LOG_FILE_PATH,
		S_PATH_FIELD_LABEL_STATE(state, m_log_file_path)
	);
	s_labeled_choice_combo(
		ctx,
		"Log level",
		"##log_level",
		&state->m_config.m_log_level,
		DTTR_CONFIG_CHOICES_LOG_LEVEL,
		S_LOG_LEVEL_TOOLTIPS,
		S_TOOLTIP_LOG_LEVEL,
		S_FIELD_LABEL_STATE(state, m_log_level)
	);
	s_labeled_choice_combo(
		ctx,
		"Minidump type",
		"##minidump_type",
		(int *)&state->m_config.m_minidump_type,
		DTTR_CONFIG_CHOICES_MINIDUMP_TYPE,
		S_MINIDUMP_TYPE_TOOLTIPS,
		S_TOOLTIP_MINIDUMP_TYPE,
		S_FIELD_LABEL_STATE(state, m_minidump_type)
	);
	s_end_settings_table();
}
