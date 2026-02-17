#include <dttr_config.h>
#include <dttr_loader.h>
#include <log.h>

#include <SDL3/SDL.h>
#include <windows.h>

static const WCHAR *PCDOGS_EXE_NAME = L"pcdogs.exe";
static const WCHAR *PCDOGS_DISC_SUBPATH = L"Setup\\102Dalms\\pcdogs.exe";

static bool s_try_path(WCHAR *out, const WCHAR *dir, const WCHAR *subpath) {
	WCHAR candidate[MAX_PATH];
	_snwprintf(candidate, MAX_PATH, L"%s\\%s", dir, subpath);
	candidate[MAX_PATH - 1] = L'\0';

	if (GetFileAttributesW(candidate) == INVALID_FILE_ATTRIBUTES) {
		return false;
	}

	wcscpy(out, candidate);
	return true;
}

static bool s_try_dir(WCHAR *out, const WCHAR *dir) {
	return s_try_path(out, dir, PCDOGS_EXE_NAME)
		   || s_try_path(out, dir, PCDOGS_DISC_SUBPATH);
}

static char s_browse_result[MAX_PATH];
static volatile bool s_browse_chosen;

static void SDLCALL
s_browse_callback(void *userdata, const char *const *filelist, int filter) {
	if (!filelist || !filelist[0]) {
		return;
	}

	strncpy(s_browse_result, filelist[0], MAX_PATH - 1);
	s_browse_result[MAX_PATH - 1] = '\0';
	s_browse_chosen = true;
}

static bool s_prompt_browse_for_dir(WCHAR *out) {
	const SDL_MessageBoxButtonData buttons[] = {
		{SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 0, "Exit"},
		{SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 1, "Browse"},
	};

	const SDL_MessageBoxData msgbox = {
		.flags = SDL_MESSAGEBOX_INFORMATION,
		.window = NULL,
		.title = "DttR: Specify Game Directory",
		.message = "Select the folder containing the 102 Dalmatians: Puppies to the "
				   "Rescue files.\n"
				   "This directory should contain a pcdogs.exe file.",
		.numbuttons = 2,
		.buttons = buttons,
	};

	int button_id = 0;
	SDL_ShowMessageBox(&msgbox, &button_id);

	if (button_id != 1) {
		return false;
	}

	s_browse_chosen = false;
	SDL_ShowOpenFolderDialog(s_browse_callback, NULL, NULL, NULL, false);

	while (!s_browse_chosen) {
		SDL_PumpEvents();
		SDL_Delay(10);
	}

	WCHAR wide_dir[MAX_PATH];
	MultiByteToWideChar(CP_UTF8, 0, s_browse_result, -1, wide_dir, MAX_PATH);

	if (!s_try_dir(out, wide_dir)) {
		SDL_ShowSimpleMessageBox(
			SDL_MESSAGEBOX_ERROR,
			"DttR: Game Not Found",
			"The selected folder does not contain pcdogs.exe.",
			NULL
		);
		return false;
	}

	log_info("Selected directory: %s", s_browse_result);

	strncpy(
		g_dttr_config.m_pcdogs_path,
		s_browse_result,
		sizeof(g_dttr_config.m_pcdogs_path) - 1
	);
	g_dttr_config.m_pcdogs_path[sizeof(g_dttr_config.m_pcdogs_path) - 1] = '\0';
	dttr_config_save(g_dttr_config_path, &g_dttr_config);

	return true;
}

bool dttr_loader_resolve_exe_path(WCHAR *out, const char *configured_dir) {
	if (!configured_dir || !configured_dir[0]) {
		return s_prompt_browse_for_dir(out);
	}

	WCHAR wide_dir[MAX_PATH];
	MultiByteToWideChar(CP_UTF8, 0, configured_dir, -1, wide_dir, MAX_PATH);

	if (s_try_dir(out, wide_dir)) {
		log_info("Using configured PCDogs path: %s", configured_dir);
		return true;
	}

	return s_prompt_browse_for_dir(out);
}
