#include <dttr_config.h>
#include <dttr_iso.h>
#include <dttr_loader.h>
#include <dttr_loader_paths.h>
#include <dttr_log.h>
#include <dttr_sdl.h>

#include <SDL3/SDL.h>
#include <string.h>
#include <windows.h>

static void s_utf8_to_wide_path(WCHAR *out, const char *path) {
	MultiByteToWideChar(CP_UTF8, 0, path, -1, out, MAX_PATH);
	out[MAX_PATH - 1] = L'\0';
}

static bool s_get_full_path(char *out, size_t out_size, const char *path) {
	const DWORD len = GetFullPathNameA(path, (DWORD)out_size, out, NULL);
	return len > 0 && len < out_size;
}

static bool s_get_os_cache_base_dir(char *buf, size_t buf_size) {
	const DWORD len = GetEnvironmentVariableA("LOCALAPPDATA", buf, (DWORD)buf_size);
	if (len > 0 && len < buf_size) {
		return true;
	}

	const DWORD temp_len = GetTempPathA((DWORD)buf_size, buf);
	return temp_len > 0 && temp_len < buf_size;
}

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
	for (size_t i = 0; i < dttr_loader_game_subpath_count(); i++) {
		if (s_try_path(out, dir, dttr_loader_game_subpath_at(i))) {
			return true;
		}
	}
	return false;
}

static void s_copy_path(char *out, size_t out_size, const char *path) {
	strncpy(out, path, out_size - 1);
	out[out_size - 1] = '\0';
}

static bool s_extract_iso_file(
	DTTR_IsoImage *iso,
	const char *cache_root,
	const char *iso_path,
	char *out_path,
	size_t out_path_size
) {
	if (dttr_iso_extract_file(iso, iso_path, cache_root, out_path, out_path_size)) {
		return true;
	}

	DTTR_LOG_ERROR("Could not extract %s (%s)", iso_path, dttr_iso_last_error());
	return false;
}

static bool s_extract_iso_support_files(DTTR_IsoImage *iso, const char *cache_root) {
	for (size_t i = 0; i < dttr_loader_iso_support_file_count(); i++) {
		char support_path[MAX_PATH];
		const char *support_file = dttr_loader_iso_support_file_at(i);
		if (!s_extract_iso_file(
				iso,
				cache_root,
				support_file,
				support_path,
				sizeof(support_path)
			)) {
			return false;
		}
	}
	return true;
}

static bool s_resolve_iso_direct(
	WCHAR *out,
	const char *iso_path,
	DTTR_LoaderIsoContext *iso_context
) {
	char full_iso_path[MAX_PATH];
	if (!s_get_full_path(full_iso_path, sizeof(full_iso_path), iso_path)) {
		DTTR_LOG_ERROR("ISO path is too long: %s", iso_path);
		return false;
	}

	char cache_base_dir[MAX_PATH];
	if (!s_get_os_cache_base_dir(cache_base_dir, sizeof(cache_base_dir))
		|| !dttr_loader_iso_cache_root_for_path(
			cache_base_dir,
			full_iso_path,
			iso_context->m_cache_root,
			sizeof(iso_context->m_cache_root)
		)) {
		DTTR_LOG_ERROR("Could not build ISO cache path for %s", full_iso_path);
		return false;
	}

	DTTR_IsoImage iso = {0};
	bool ok = false;
	if (!dttr_iso_open(&iso, full_iso_path)) {
		DTTR_LOG_ERROR(
			"Could not open ISO directly: %s (%s)",
			full_iso_path,
			dttr_iso_last_error()
		);
		return false;
	}

	char exe_path[MAX_PATH];
	if (!s_extract_iso_file(
			&iso,
			iso_context->m_cache_root,
			dttr_loader_iso_game_exe_path(),
			exe_path,
			sizeof(exe_path)
		)) {
		DTTR_LOG_ERROR("ISO source: %s", full_iso_path);
		goto done;
	}

	if (!s_extract_iso_support_files(&iso, iso_context->m_cache_root)) {
		goto done;
	}

	s_utf8_to_wide_path(out, exe_path);
	iso_context->m_is_direct = true;
	s_copy_path(iso_context->m_iso_path, sizeof(iso_context->m_iso_path), full_iso_path);
	s_copy_path(
		iso_context->m_game_root,
		sizeof(iso_context->m_game_root),
		dttr_loader_iso_game_root()
	);
	DTTR_LOG_INFO("Extracted ISO executable to %s", exe_path);
	ok = true;

done:
	dttr_iso_close(&iso);
	return ok;
}

static bool s_resolve_iso(
	WCHAR *out,
	const char *iso_path,
	DTTR_LoaderIsoContext *iso_context
) {
	if (!iso_context) {
		DTTR_LOG_ERROR("ISO path provided without context");
		return false;
	}

	memset(iso_context, 0, sizeof(*iso_context));
	if (s_resolve_iso_direct(out, iso_path, iso_context)) {
		return true;
	}

	dttr_sdl_show_simple_message_box(
		SDL_MESSAGEBOX_ERROR,
		"DttR: ISO Load Failed",
		"DttR could not read the selected ISO. Consider using the extracted game files "
		"instead.",
		NULL
	);
	return false;
}

static bool s_try_configured_path(
	WCHAR *out,
	const char *configured_path,
	DTTR_LoaderIsoContext *iso_context
) {
	WCHAR wide_path[MAX_PATH];
	s_utf8_to_wide_path(wide_path, configured_path);

	if (dttr_loader_path_is_iso_w(wide_path)) {
		DTTR_LOG_INFO("Using configured ISO path: %s", configured_path);
		return s_resolve_iso(out, configured_path, iso_context);
	}

	if (s_try_dir(out, wide_path)) {
		DTTR_LOG_INFO("Using configured PCDogs path: %s", configured_path);
		return true;
	}

	return false;
}

static char s_browse_result[MAX_PATH];
static volatile bool s_browse_chosen;

static void SDLCALL
s_browse_callback(void *userdata, const char *const *filelist, int filter) {
	(void)userdata;
	(void)filter;

	if (!filelist || !filelist[0]) {
		s_browse_result[0] = '\0';
		s_browse_chosen = true;
		return;
	}

	strncpy(s_browse_result, filelist[0], MAX_PATH - 1);
	s_browse_result[MAX_PATH - 1] = '\0';
	s_browse_chosen = true;
}

static bool s_wait_for_browse_result(void) {
	while (!s_browse_chosen) {
		dttr_sdl_pump_events();
		dttr_sdl_delay(10);
	}
	return s_browse_result[0] != '\0';
}

static bool s_prompt_browse_for_path(WCHAR *out, DTTR_LoaderIsoContext *iso_context) {
	const SDL_MessageBoxButtonData buttons[] = {
		{SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 0, "Exit"},
		{0, 1, "Use Directory"},
		{SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 2, "Use ISO"},
	};

	const SDL_MessageBoxData msgbox = {
		.flags = SDL_MESSAGEBOX_INFORMATION,
		.window = NULL,
		.title = "DttR: Specify Game Files",
		.message = "Select either the folder containing the 102 Dalmatians: Puppies "
				   "to the Rescue files, the original game disc, or an ISO image.\n\n"
				   "Installed folders should contain pcdogs.exe. Disc roots and ISO "
				   "images should contain Setup\\102Dalms\\pcdogs.exe.",
		.numbuttons = 3,
		.buttons = buttons,
	};

	int button_id = 0;
	dttr_sdl_show_message_box(&msgbox, &button_id);

	if (button_id != 1 && button_id != 2) {
		return false;
	}

	s_browse_chosen = false;
	if (button_id == 1) {
		dttr_sdl_show_open_folder_dialog(s_browse_callback, NULL, NULL, NULL, false);
	} else {
		const SDL_DialogFileFilter filters[] = {{"ISO images", "iso"}};
		dttr_sdl_show_open_file_dialog(
			s_browse_callback,
			NULL,
			NULL,
			filters,
			1,
			NULL,
			false
		);
	}

	if (!s_wait_for_browse_result()) {
		return false;
	}

	WCHAR wide_path[MAX_PATH];
	s_utf8_to_wide_path(wide_path, s_browse_result);

	bool resolved = false;
	if (button_id == 2 || dttr_loader_path_is_iso_w(wide_path)) {
		resolved = s_resolve_iso(out, s_browse_result, iso_context);
	} else {
		resolved = s_try_dir(out, wide_path);
		if (!resolved) {
			dttr_sdl_show_simple_message_box(
				SDL_MESSAGEBOX_ERROR,
				"DttR: Game Not Found",
				"The selected folder does not contain pcdogs.exe.",
				NULL
			);
		}
	}

	if (!resolved) {
		return false;
	}

	DTTR_LOG_INFO("Selected game path: %s", s_browse_result);

	s_copy_path(
		g_dttr_config.m_pcdogs_path,
		sizeof(g_dttr_config.m_pcdogs_path),
		s_browse_result
	);
	dttr_config_save(g_dttr_config_path, &g_dttr_config);

	return true;
}

bool dttr_loader_resolve_exe_path(
	WCHAR *out,
	const char *configured_path,
	DTTR_LoaderIsoContext *iso_context
) {
	if (configured_path && configured_path[0]
		&& s_try_configured_path(out, configured_path, iso_context)) {
		return true;
	}

	return s_prompt_browse_for_path(out, iso_context);
}
