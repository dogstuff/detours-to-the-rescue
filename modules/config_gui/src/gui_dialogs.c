#include "gui_internal.h"

static const SDL_DialogFileFilter S_ISO_FILE_FILTERS[] = {
	{"ISO images", "iso"},
};

static const SDL_DialogFileFilter S_LOG_FILE_FILTERS[] = {
	{"Log files", "log"},
	{"Text files", "txt"},
};

static SDL_Window *s_dialog_parent_window(const DTTR_ImGuiDialogContext *ctx) {
	return ctx ? ctx->m_window : NULL;
}

static const char *s_optional_path(const char *path) {
	return path && path[0] ? path : NULL;
}

static void s_open_file_dialog(
	const DTTR_ImGuiDialogContext *ctx,
	S_ConfigUIState *state,
	SDL_DialogFileCallback callback,
	const SDL_DialogFileFilter *filters,
	int filter_count,
	const char *path
) {
	dttr_sdl_show_open_file_dialog(
		callback,
		state,
		s_dialog_parent_window(ctx),
		filters,
		filter_count,
		path,
		false
	);
}

static void s_apply_dialog_selection(
	S_ConfigUIState *state,
	char *path,
	size_t path_size,
	const char *const *filelist,
	const char *open_failure_status,
	const char *selected_status
) {
	if (!state) {
		return;
	}

	if (!filelist) {
		s_set_status(state, open_failure_status);
		return;
	}

	if (!filelist[0]) {
		return;
	}

	if (!path || path_size == 0) {
		return;
	}

	if (!dttr_path_copy_string(path, path_size, filelist[0])) {
		s_set_status(state, "Selected path is too long.");
		return;
	}

	s_set_status(state, selected_status);
}

static void SDLCALL
s_pcdogs_path_dialog_callback(void *userdata, const char *const *filelist, int filter) {
	(void)filter;
	S_ConfigUIState *state = (S_ConfigUIState *)userdata;
	s_apply_dialog_selection(
		state,
		state ? state->m_config.m_pcdogs_path : NULL,
		state ? sizeof(state->m_config.m_pcdogs_path) : 0,
		filelist,
		"Failed to open path dialog.",
		"Selected game path."
	);
}

void s_open_pcdogs_dir_dialog(const DTTR_ImGuiDialogContext *ctx, S_ConfigUIState *state) {
	const char *path = state ? s_optional_path(state->m_config.m_pcdogs_path) : NULL;
	dttr_sdl_show_open_folder_dialog(
		s_pcdogs_path_dialog_callback,
		state,
		s_dialog_parent_window(ctx),
		path,
		false
	);
}

void s_open_pcdogs_iso_dialog(const DTTR_ImGuiDialogContext *ctx, S_ConfigUIState *state) {
	const char *path = state ? s_optional_path(state->m_config.m_pcdogs_path) : NULL;
	s_open_file_dialog(
		ctx,
		state,
		s_pcdogs_path_dialog_callback,
		S_ISO_FILE_FILTERS,
		(int)SDL_arraysize(S_ISO_FILE_FILTERS),
		path
	);
}

static void SDLCALL
s_log_file_path_dialog_callback(void *userdata, const char *const *filelist, int filter) {
	(void)filter;
	S_ConfigUIState *state = (S_ConfigUIState *)userdata;
	s_apply_dialog_selection(
		state,
		state ? state->m_config.m_log_file_path : NULL,
		state ? sizeof(state->m_config.m_log_file_path) : 0,
		filelist,
		"Failed to open log file dialog.",
		"Selected log file path."
	);
}

void s_open_log_file_dialog(const DTTR_ImGuiDialogContext *ctx, S_ConfigUIState *state) {
	const char *path = state ? s_optional_path(state->m_config.m_log_file_path) : NULL;
	s_open_file_dialog(
		ctx,
		state,
		s_log_file_path_dialog_callback,
		S_LOG_FILE_FILTERS,
		(int)SDL_arraysize(S_LOG_FILE_FILTERS),
		path
	);
}
