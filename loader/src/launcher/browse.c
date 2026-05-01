#include <dttr_config.h>
#include <dttr_iso.h>
#include <dttr_loader.h>
#include <dttr_loader_paths.h>
#include <dttr_loader_ui.h>
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

static bool s_extract_iso_game_cache(
	DTTR_IsoImage *iso,
	const char *cache_root,
	char *exe_path,
	size_t exe_path_size
) {
	if (!s_extract_iso_file(
			iso,
			cache_root,
			dttr_loader_iso_game_exe_path(),
			exe_path,
			exe_path_size
		)) {
		return false;
	}

	char pkg_path[MAX_PATH];
	if (!s_extract_iso_file(
			iso,
			cache_root,
			dttr_loader_iso_game_pkg_path(),
			pkg_path,
			sizeof(pkg_path)
		)) {
		return false;
	}

	const char *data_path = dttr_loader_iso_game_data_path();
	if (dttr_iso_extract_tree(iso, data_path, cache_root)) {
		return true;
	}

	DTTR_LOG_ERROR("Could not extract %s (%s)", data_path, dttr_iso_last_error());
	return false;
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
	if (!s_extract_iso_game_cache(
			&iso,
			iso_context->m_cache_root,
			exe_path,
			sizeof(exe_path)
		)) {
		DTTR_LOG_ERROR("ISO source: %s", full_iso_path);
		goto done;
	}

	s_utf8_to_wide_path(out, exe_path);
	iso_context->m_is_iso = true;
	s_copy_path(
		iso_context->m_game_root,
		sizeof(iso_context->m_game_root),
		dttr_loader_iso_game_root()
	);
	DTTR_LOG_INFO("Cached ISO game files under %s", iso_context->m_cache_root);
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

	dttr_loader_ui_show_error(
		"DttR: ISO Load Failed",
		"DttR could not read the selected ISO. Consider using the extracted game files "
		"instead."
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
static HANDLE s_browse_event;

static void SDLCALL s_browse_callback(void *, const char *const *filelist, int) {
	if (!filelist || !filelist[0]) {
		s_browse_result[0] = '\0';
	} else {
		s_copy_path(s_browse_result, sizeof(s_browse_result), filelist[0]);
	}

	if (s_browse_event) {
		SetEvent(s_browse_event);
	}
}

static bool s_wait_for_browse_result(void) {
	while (WaitForSingleObject(s_browse_event, 0) == WAIT_TIMEOUT) {
		dttr_sdl_pump_events();
		dttr_sdl_delay(10);
	}
	return s_browse_result[0] != '\0';
}

static void s_save_selected_path(const char *path) {
	s_copy_path(g_dttr_config.m_pcdogs_path, sizeof(g_dttr_config.m_pcdogs_path), path);
	dttr_config_save(g_dttr_config_path, &g_dttr_config);
}

static void s_scan_disc_candidates(
	DTTR_LoaderUiDiscCandidate *candidates,
	size_t *candidate_count
) {
	*candidate_count = 0;
	const DWORD drives = GetLogicalDrives();
	for (char drive = 'A'; drive <= 'Z'; drive++) {
		const DWORD bit = 1u << (drive - 'A');
		if ((drives & bit) == 0) {
			continue;
		}

		WCHAR root_w[] = {drive, L':', L'\\', L'\0'};
		const UINT drive_type = GetDriveTypeW(root_w);
		if (drive_type == DRIVE_UNKNOWN || drive_type == DRIVE_NO_ROOT_DIR) {
			continue;
		}

		WCHAR game_path[MAX_PATH];
		if (!s_try_dir(game_path, root_w)) {
			continue;
		}

		DTTR_LoaderUiDiscCandidate *candidate = &candidates[*candidate_count];
		snprintf(candidate->m_label, sizeof(candidate->m_label), "Open Disc %c:", drive);
		snprintf(candidate->m_path, sizeof(candidate->m_path), "%c:\\", drive);
		(*candidate_count)++;
		if (*candidate_count >= DTTR_LOADER_UI_MAX_DISC_CANDIDATES) {
			return;
		}
	}
}

static bool s_try_disc_candidate(WCHAR *out, const DTTR_LoaderUiDiscCandidate *candidate) {
	WCHAR wide_path[MAX_PATH];
	s_utf8_to_wide_path(wide_path, candidate->m_path);
	if (!s_try_dir(out, wide_path)) {
		dttr_loader_ui_show_error(
			"DttR: Disc Not Found",
			"The selected disc no longer contains pcdogs.exe."
		);
		return false;
	}

	DTTR_LOG_INFO("Selected game disc: %s", candidate->m_path);
	s_save_selected_path(candidate->m_path);
	return true;
}

static bool s_run_browse_dialog(DTTR_LoaderUiChoice choice) {
	if (!s_browse_event) {
		s_browse_event = CreateEventW(NULL, TRUE, FALSE, NULL);
		if (!s_browse_event) {
			DTTR_LOG_ERROR("Could not create browse completion event");
			return false;
		}
	}

	s_browse_result[0] = '\0';
	ResetEvent(s_browse_event);
	if (choice == DTTR_LOADER_UI_CHOICE_BROWSE_FOLDER) {
		dttr_sdl_show_open_folder_dialog(s_browse_callback, NULL, NULL, NULL, false);
		return s_wait_for_browse_result();
	}

	const SDL_DialogFileFilter filters[] = {{"ISO images", "iso"}};
	dttr_sdl_show_open_file_dialog(s_browse_callback, NULL, NULL, filters, 1, NULL, false);
	return s_wait_for_browse_result();
}

static bool s_try_browsed_path(
	WCHAR *out,
	DTTR_LoaderUiChoice choice,
	DTTR_LoaderIsoContext *iso_context
) {
	WCHAR wide_path[MAX_PATH];
	s_utf8_to_wide_path(wide_path, s_browse_result);

	if (choice == DTTR_LOADER_UI_CHOICE_BROWSE_ISO
		|| dttr_loader_path_is_iso_w(wide_path)) {
		return s_resolve_iso(out, s_browse_result, iso_context);
	}

	if (s_try_dir(out, wide_path)) {
		return true;
	}

	dttr_loader_ui_show_error(
		"DttR: Game Not Found",
		"The selected folder does not contain pcdogs.exe."
	);
	return false;
}

static bool s_prompt_browse_for_path(WCHAR *out, DTTR_LoaderIsoContext *iso_context) {
	DTTR_LoaderUiDiscCandidate disc_candidates[DTTR_LOADER_UI_MAX_DISC_CANDIDATES];
	size_t disc_candidate_count = 0;
	s_scan_disc_candidates(disc_candidates, &disc_candidate_count);

	for (;;) {
		const DTTR_LoaderUiChoice choice = dttr_loader_ui_choose_game_source(
			disc_candidates,
			disc_candidate_count
		);

		size_t disc_index = 0;
		if (dttr_loader_ui_choice_is_disc(choice, &disc_index)) {
			if (disc_index < disc_candidate_count
				&& s_try_disc_candidate(out, &disc_candidates[disc_index])) {
				return true;
			}

			s_scan_disc_candidates(disc_candidates, &disc_candidate_count);
			continue;
		}

		if (choice != DTTR_LOADER_UI_CHOICE_BROWSE_FOLDER
			&& choice != DTTR_LOADER_UI_CHOICE_BROWSE_ISO) {
			return false;
		}

		if (!s_run_browse_dialog(choice)
			|| !s_try_browsed_path(out, choice, iso_context)) {
			continue;
		}

		DTTR_LOG_INFO("Selected game path: %s", s_browse_result);
		s_save_selected_path(s_browse_result);
		return true;
	}
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
