#include "gui_internal.h"

static const SDL_DialogFileFilter ISO_FILE_FILTERS[] = {
	{"ISO images", "iso"},
};

static const SDL_DialogFileFilter LOG_FILE_FILTERS[] = {
	{"Log files", "log"},
	{"Text files", "txt"},
};

static SDL_Window *dialog_parent_window(const DTTR_ImGuiDialogContext *ctx) {
	return ctx ? ctx->window : NULL;
}

static const char *optional_path(const char *path) {
	return path && path[0] ? path : NULL;
}

static void open_file_dialog(
	const DTTR_ImGuiDialogContext *ctx,
	config_ui_state *state,
	SDL_DialogFileCallback callback,
	const SDL_DialogFileFilter *filters,
	int filter_count,
	const char *path
) {
	DTTR_SDL_ShowOpenFileDialog(
		callback,
		state,
		dialog_parent_window(ctx),
		filters,
		filter_count,
		path,
		false
	);
}

static void apply_dialog_selection(
	config_ui_state *state,
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
		set_status(state, open_failure_status);
		return;
	}

	if (!filelist[0]) {
		return;
	}

	if (!path || path_size == 0) {
		return;
	}

	if (!DTTR_Path_CopyString(path, path_size, filelist[0])) {
		set_status(state, "Selected path is too long.");
		return;
	}

	set_status(state, selected_status);
}

static void SDLCALL
pcdogs_path_dialog_callback(void *userdata, const char *const *filelist, int) {
	config_ui_state *state = (config_ui_state *)userdata;
	apply_dialog_selection(
		state,
		state ? state->config.pcdogs_path : NULL,
		state ? sizeof(state->config.pcdogs_path) : 0,
		filelist,
		"Failed to open path dialog.",
		"Selected game path."
	);
}

void open_pcdogs_dir_dialog(const DTTR_ImGuiDialogContext *ctx, config_ui_state *state) {
	const char *path = state ? optional_path(state->config.pcdogs_path) : NULL;
	DTTR_SDL_ShowOpenFolderDialog(
		pcdogs_path_dialog_callback,
		state,
		dialog_parent_window(ctx),
		path,
		false
	);
}

void open_pcdogs_iso_dialog(const DTTR_ImGuiDialogContext *ctx, config_ui_state *state) {
	const char *path = state ? optional_path(state->config.pcdogs_path) : NULL;
	open_file_dialog(
		ctx,
		state,
		pcdogs_path_dialog_callback,
		ISO_FILE_FILTERS,
		(int)SDL_arraysize(ISO_FILE_FILTERS),
		path
	);
}

static void SDLCALL
log_file_path_dialog_callback(void *userdata, const char *const *filelist, int) {
	config_ui_state *state = (config_ui_state *)userdata;
	apply_dialog_selection(
		state,
		state ? state->config.log_file_path : NULL,
		state ? sizeof(state->config.log_file_path) : 0,
		filelist,
		"Failed to open log file dialog.",
		"Selected log file path."
	);
}

void open_log_file_dialog(const DTTR_ImGuiDialogContext *ctx, config_ui_state *state) {
	const char *path = state ? optional_path(state->config.log_file_path) : NULL;
	open_file_dialog(
		ctx,
		state,
		log_file_path_dialog_callback,
		LOG_FILE_FILTERS,
		(int)SDL_arraysize(LOG_FILE_FILTERS),
		path
	);
}
