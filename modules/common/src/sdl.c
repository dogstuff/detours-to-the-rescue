#include <dttr_sdl.h>

#include <dttr_log.h>
#include <dttr_path.h>
#include <windows.h>

typedef bool (*show_simple_message_box_fn)(
	SDL_MessageBoxFlags flags,
	const char *title,
	const char *message,
	SDL_Window *window
);
typedef bool (*show_message_box_fn)(
	const SDL_MessageBoxData *messageboxdata,
	int *buttonid
);
typedef void (*show_open_folder_dialog_fn)(
	SDL_DialogFileCallback callback,
	void *userdata,
	SDL_Window *window,
	const char *default_location,
	bool allow_many
);
typedef void (*show_open_file_dialog_fn)(
	SDL_DialogFileCallback callback,
	void *userdata,
	SDL_Window *window,
	const SDL_DialogFileFilter *filters,
	int nfilters,
	const char *default_location,
	bool allow_many
);
typedef void (*pump_events_fn)();
typedef void (*delay_fn)(Uint32 ms);

#define RESOLVE(module, type, name) ((type)GetProcAddress(module, name))

static const char sdl_module_path[] = "modules\\SDL3.dll";
static const char sdl_module_name[] = "SDL3.dll";

static bool load_attempted;
static show_simple_message_box_fn show_simple_message_box;
static show_message_box_fn show_message_box;
static show_open_folder_dialog_fn show_open_folder_dialog;
static show_open_file_dialog_fn show_open_file_dialog;
static pump_events_fn pump_events;
static delay_fn delay;

static void report_dialog_failure(SDL_DialogFileCallback callback, void *userdata) {
	callback(userdata, NULL, -1);
}

static bool resolve_sdl_dll_path(char *out, size_t out_size) {
	sds path = DTTR_Path_ModuleSibling(NULL, sdl_module_path);
	const bool copied = DTTR_Path_CopySds(out, out_size, path);
	sdsfree(path);
	return copied;
}

static HMODULE get_loaded_sdl_module() {
	HMODULE module = GetModuleHandleA(sdl_module_path);
	return module ? module : GetModuleHandleA(sdl_module_name);
}

static bool resolve_sdl_exports(HMODULE module) {
	show_simple_message_box = RESOLVE(
		module,
		show_simple_message_box_fn,
		"SDL_ShowSimpleMessageBox"
	);
	show_message_box = RESOLVE(module, show_message_box_fn, "SDL_ShowMessageBox");
	show_open_folder_dialog = RESOLVE(
		module,
		show_open_folder_dialog_fn,
		"SDL_ShowOpenFolderDialog"
	);
	show_open_file_dialog = RESOLVE(
		module,
		show_open_file_dialog_fn,
		"SDL_ShowOpenFileDialog"
	);
	pump_events = RESOLVE(module, pump_events_fn, "SDL_PumpEvents");
	delay = RESOLVE(module, delay_fn, "SDL_Delay");

	return show_simple_message_box && show_message_box && show_open_folder_dialog
		   && show_open_file_dialog && pump_events && delay;
}

static bool load_sdl() {
	if (show_message_box) {
		return true;
	}
	if (load_attempted) {
		return false;
	}
	load_attempted = true;

	HMODULE module = get_loaded_sdl_module();
	if (!module) {
		char path[MAX_PATH];
		if (!resolve_sdl_dll_path(path, sizeof(path))) {
			DTTR_LOG_ERROR("Could not resolve SDL3.dll path");
			return false;
		}

		module = LoadLibraryA(path);
	}
	if (!module) {
		DTTR_LOG_ERROR("Could not load SDL3.dll");
		return false;
	}

	return resolve_sdl_exports(module);
}

bool DTTR_SDL_ShowSimpleMessageBox(
	SDL_MessageBoxFlags flags,
	const char *title,
	const char *message,
	SDL_Window *window
) {
	if (!load_sdl()) {
		MessageBoxA(NULL, message, title, MB_OK | MB_ICONERROR);
		return false;
	}

	return show_simple_message_box(flags, title, message, window);
}

bool DTTR_SDL_ShowMessageBox(const SDL_MessageBoxData *messageboxdata, int *buttonid) {
	if (!load_sdl()) {
		return false;
	}

	return show_message_box(messageboxdata, buttonid);
}

void DTTR_SDL_ShowOpenFolderDialog(
	SDL_DialogFileCallback callback,
	void *userdata,
	SDL_Window *window,
	const char *default_location,
	bool allow_many
) {
	if (!load_sdl()) {
		report_dialog_failure(callback, userdata);
		return;
	}

	show_open_folder_dialog(callback, userdata, window, default_location, allow_many);
}

void DTTR_SDL_ShowOpenFileDialog(
	SDL_DialogFileCallback callback,
	void *userdata,
	SDL_Window *window,
	const SDL_DialogFileFilter *filters,
	int nfilters,
	const char *default_location,
	bool allow_many
) {
	if (!load_sdl()) {
		report_dialog_failure(callback, userdata);
		return;
	}

	show_open_file_dialog(
		callback,
		userdata,
		window,
		filters,
		nfilters,
		default_location,
		allow_many
	);
}

void DTTR_SDL_PumpEvents() {
	if (!load_sdl()) {
		return;
	}

	pump_events();
}

void DTTR_SDL_Delay(Uint32 ms) {
	if (!load_sdl()) {
		return;
	}

	delay(ms);
}
