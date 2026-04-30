#include <dttr_sdl.h>

#include <dttr_log.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>

typedef bool (*S_ShowSimpleMessageBox)(
	SDL_MessageBoxFlags flags,
	const char *title,
	const char *message,
	SDL_Window *window
);
typedef bool (*S_ShowMessageBox)(const SDL_MessageBoxData *messageboxdata, int *buttonid);
typedef void (*S_ShowOpenFolderDialog)(
	SDL_DialogFileCallback callback,
	void *userdata,
	SDL_Window *window,
	const char *default_location,
	bool allow_many
);
typedef void (*S_ShowOpenFileDialog)(
	SDL_DialogFileCallback callback,
	void *userdata,
	SDL_Window *window,
	const SDL_DialogFileFilter *filters,
	int nfilters,
	const char *default_location,
	bool allow_many
);
typedef void (*S_PumpEvents)(void);
typedef void (*S_Delay)(Uint32 ms);

#define S_RESOLVE(module, type, name) ((type)GetProcAddress(module, name))

static bool s_load_attempted;
static S_ShowSimpleMessageBox s_show_simple_message_box;
static S_ShowMessageBox s_show_message_box;
static S_ShowOpenFolderDialog s_show_open_folder_dialog;
static S_ShowOpenFileDialog s_show_open_file_dialog;
static S_PumpEvents s_pump_events;
static S_Delay s_delay;

static bool s_resolve_sdl_dll_path(char *out, size_t out_size) {
	const DWORD len = GetModuleFileNameA(NULL, out, (DWORD)out_size);
	if (!len || len >= out_size) {
		return false;
	}

	char *const last_sep = strrchr(out, '\\');
	if (!last_sep) {
		return false;
	}

	const size_t dir_len = (size_t)(last_sep - out) + 1;
	const int written = snprintf(out + dir_len, out_size - dir_len, "modules\\SDL3.dll");
	return written > 0 && (size_t)written < out_size - dir_len;
}

static bool s_load_sdl(void) {
	if (s_show_message_box) {
		return true;
	}
	if (s_load_attempted) {
		return false;
	}
	s_load_attempted = true;

	char path[MAX_PATH];
	if (!s_resolve_sdl_dll_path(path, sizeof(path))) {
		DTTR_LOG_ERROR("Could not resolve SDL3.dll path");
		return false;
	}

	const HMODULE module = LoadLibraryA(path);
	if (!module) {
		DTTR_LOG_ERROR("Could not load SDL3.dll");
		return false;
	}

	s_show_simple_message_box = S_RESOLVE(
		module,
		S_ShowSimpleMessageBox,
		"SDL_ShowSimpleMessageBox"
	);
	s_show_message_box = S_RESOLVE(module, S_ShowMessageBox, "SDL_ShowMessageBox");
	s_show_open_folder_dialog = S_RESOLVE(
		module,
		S_ShowOpenFolderDialog,
		"SDL_ShowOpenFolderDialog"
	);
	s_show_open_file_dialog = S_RESOLVE(
		module,
		S_ShowOpenFileDialog,
		"SDL_ShowOpenFileDialog"
	);
	s_pump_events = S_RESOLVE(module, S_PumpEvents, "SDL_PumpEvents");
	s_delay = S_RESOLVE(module, S_Delay, "SDL_Delay");

	return s_show_simple_message_box && s_show_message_box && s_show_open_folder_dialog
		   && s_show_open_file_dialog && s_pump_events && s_delay;
}

bool dttr_sdl_show_simple_message_box(
	SDL_MessageBoxFlags flags,
	const char *title,
	const char *message,
	SDL_Window *window
) {
	if (!s_load_sdl()) {
		MessageBoxA(NULL, message, title, MB_OK | MB_ICONERROR);
		return false;
	}

	return s_show_simple_message_box(flags, title, message, window);
}

bool dttr_sdl_show_message_box(const SDL_MessageBoxData *messageboxdata, int *buttonid) {
	if (!s_load_sdl()) {
		return false;
	}

	return s_show_message_box(messageboxdata, buttonid);
}

void dttr_sdl_show_open_folder_dialog(
	SDL_DialogFileCallback callback,
	void *userdata,
	SDL_Window *window,
	const char *default_location,
	bool allow_many
) {
	if (!s_load_sdl()) {
		callback(userdata, NULL, -1);
		return;
	}

	s_show_open_folder_dialog(callback, userdata, window, default_location, allow_many);
}

void dttr_sdl_show_open_file_dialog(
	SDL_DialogFileCallback callback,
	void *userdata,
	SDL_Window *window,
	const SDL_DialogFileFilter *filters,
	int nfilters,
	const char *default_location,
	bool allow_many
) {
	if (!s_load_sdl()) {
		callback(userdata, NULL, -1);
		return;
	}

	s_show_open_file_dialog(
		callback,
		userdata,
		window,
		filters,
		nfilters,
		default_location,
		allow_many
	);
}

void dttr_sdl_pump_events(void) {
	if (!s_load_sdl()) {
		return;
	}

	s_pump_events();
}

void dttr_sdl_delay(Uint32 ms) {
	if (!s_load_sdl()) {
		return;
	}

	s_delay(ms);
}
