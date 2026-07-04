#include <dttr_config.h>
#include <dttr_iso.h>
#include <dttr_loader.h>
#include <dttr_loader_paths.h>
#include <dttr_loader_ui.h>
#include <dttr_log.h>
#include <dttr_path.h>
#include <dttr_sdl.h>

#include <SDL3/SDL.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>

static bool utf8_to_wide_path(WCHAR *out, const char *path) {
	if (MultiByteToWideChar(CP_UTF8, 0, path, -1, out, MAX_PATH) == 0) {
		out[0] = L'\0';
		return false;
	}

	out[MAX_PATH - 1] = L'\0';
	return true;
}

// Resolves user paths before deriving ISO cache keys.
static bool get_full_path(char *out, size_t out_size, const char *path) {
	const DWORD len = GetFullPathNameA(path, (DWORD)out_size, out, NULL);
	return len > 0 && len < out_size;
}

// Uses LOCALAPPDATA, then temp, as the ISO cache base.
static bool get_os_cache_base_dir(char *buf, size_t buf_size) {
	const DWORD len = GetEnvironmentVariableA("LOCALAPPDATA", buf, (DWORD)buf_size);

	if (len > 0 && len < buf_size) {
		return true;
	}

	const DWORD temp_len = GetTempPathA((DWORD)buf_size, buf);
	return temp_len > 0 && temp_len < buf_size;
}

// Tests one known PCDOGS executable subpath.
static bool try_path(WCHAR *out, size_t out_count, const WCHAR *dir, const WCHAR *subpath) {
	WCHAR candidate[MAX_PATH];
	_snwprintf(candidate, MAX_PATH, L"%s\\%s", dir, subpath);
	candidate[MAX_PATH - 1] = L'\0';

	if (GetFileAttributesW(candidate) == INVALID_FILE_ATTRIBUTES) {
		return false;
	}

	if (out_count == 0 || wcslen(candidate) >= out_count) {
		return false;
	}

	wcscpy(out, candidate);
	return true;
}

// Finds a supported PCDOGS executable layout.
static bool try_dir(WCHAR *out, size_t out_count, const WCHAR *dir) {
	const size_t subpath_count = DTTR_Loader_GameSubpathCount();

	for (size_t i = 0; i < subpath_count; i++) {
		if (try_path(out, out_count, dir, DTTR_Loader_GameSubpathAt(i))) {
			return true;
		}
	}

	return false;
}

// Copies optional paths and keeps cancelled selections empty.
static void copy_path(char *out, size_t out_size, const char *path) {
	if (out_size == 0) {
		return;
	}

	if (!path || !DTTR_Path_CopyString(out, out_size, path)) {
		out[0] = '\0';
	}
}

static bool iso_layout_path(
	char *out,
	size_t out_size,
	const char *game_root,
	const char *name
) {
	if (strcmp(game_root, ".") == 0) {
		return DTTR_Path_CopyString(out, out_size, name);
	}

	const int written = snprintf(out, out_size, "%s/%s", game_root, name);
	return written > 0 && (size_t)written < out_size;
}

static bool extract_iso_file(
	DTTR_IsoImage *iso,
	const char *cache_root,
	const char *iso_path,
	char *out_path,
	size_t out_path_size,
	bool *not_found
) {
	*not_found = false;

	if (DTTR_ISO_ExtractFile(iso, iso_path, cache_root, out_path, out_path_size)) {
		return true;
	}

	*not_found = DTTR_ISO_LastErrorWasNotFound();
	if (*not_found) {
		return false;
	}

	DTTR_LOG_ERROR("Could not extract %s (%s)", iso_path, DTTR_ISO_LastError());
	return false;
}

// Extracts the ISO files needed for launch.
static bool extract_iso_game_cache(
	DTTR_IsoImage *iso,
	const char *cache_root,
	char *exe_path,
	size_t exe_path_size,
	char *game_root,
	size_t game_root_size
) {
	const size_t layout_count = DTTR_Loader_GameSubpathCount();

	for (size_t i = 0; i < layout_count; i++) {
		const char *root = DTTR_Loader_GameRootAt(i);
		char iso_path[MAX_PATH];

		if (!iso_layout_path(iso_path, sizeof(iso_path), root, "pcdogs.exe")) {
			DTTR_LOG_ERROR("Could not build ISO layout paths for %s", root);
			return false;
		}

		bool not_found;

		if (!extract_iso_file(
				iso,
				cache_root,
				iso_path,
				exe_path,
				exe_path_size,
				&not_found
			)) {
			if (not_found) {
				continue;
			}

			return false;
		}

		char pkg_path[MAX_PATH];

		if (!iso_layout_path(iso_path, sizeof(iso_path), root, "pcdogs.pkg")) {
			DTTR_LOG_ERROR("Could not build ISO layout paths for %s", root);
			return false;
		}

		if (!extract_iso_file(
				iso,
				cache_root,
				iso_path,
				pkg_path,
				sizeof(pkg_path),
				&not_found
			)) {
			if (not_found) {
				continue;
			}

			return false;
		}

		if (!iso_layout_path(iso_path, sizeof(iso_path), root, "data")) {
			DTTR_LOG_ERROR("Could not build ISO layout paths for %s", root);
			return false;
		}

		if (!DTTR_ISO_ExtractTree(iso, iso_path, cache_root)) {
			if (DTTR_ISO_LastErrorWasNotFound()) {
				continue;
			}

			DTTR_LOG_ERROR("Could not extract %s (%s)", iso_path, DTTR_ISO_LastError());
			return false;
		}

		if (!DTTR_Path_CopyString(game_root, game_root_size, root)) {
			DTTR_LOG_ERROR("Could not copy ISO game root: %s", root);
			return false;
		}

		return true;
	}

	DTTR_LOG_ERROR(
		"Could not extract a supported ISO game executable (%s)",
		DTTR_ISO_LastError()
	);
	return false;
}

// Opens an ISO and returns the cached executable path.
static bool resolve_iso_direct(
	WCHAR *out,
	const char *iso_path,
	DTTR_LoaderIsoContext *iso_context
) {
	char full_iso_path[MAX_PATH];

	if (!get_full_path(full_iso_path, sizeof(full_iso_path), iso_path)) {
		DTTR_LOG_ERROR("ISO path is too long: %s", iso_path);
		return false;
	}

	char cache_base_dir[MAX_PATH];

	if (!get_os_cache_base_dir(cache_base_dir, sizeof(cache_base_dir))
		|| !DTTR_LoaderISO_CacheRootForPath(
			cache_base_dir,
			full_iso_path,
			iso_context->cache_root,
			sizeof(iso_context->cache_root)
		)) {
		DTTR_LOG_ERROR("Could not build ISO cache path for %s", full_iso_path);
		return false;
	}

	DTTR_IsoImage iso = {0};
	bool ok = false;

	if (!DTTR_ISO_Open(&iso, full_iso_path)) {
		DTTR_LOG_ERROR(
			"Could not open ISO directly: %s (%s)",
			full_iso_path,
			DTTR_ISO_LastError()
		);
		return false;
	}

	char exe_path[MAX_PATH];

	if (!extract_iso_game_cache(
			&iso,
			iso_context->cache_root,
			exe_path,
			sizeof(exe_path),
			iso_context->game_root,
			sizeof(iso_context->game_root)
		)) {
		DTTR_LOG_ERROR("ISO source: %s", full_iso_path);
		goto done;
	}

	if (!utf8_to_wide_path(out, exe_path)) {
		DTTR_LOG_ERROR("Could not convert cached ISO executable path: %s", exe_path);
		goto done;
	}

	iso_context->is_iso = true;
	DTTR_LOG_INFO("Cached ISO game files under %s", iso_context->cache_root);
	ok = true;

done:
	DTTR_ISO_Close(&iso);
	return ok;
}

// Clears ISO context before direct ISO resolution.
static bool resolve_iso(
	WCHAR *out,
	const char *iso_path,
	DTTR_LoaderIsoContext *iso_context
) {
	if (!iso_context) {
		DTTR_LOG_ERROR("ISO path provided without context");
		return false;
	}

	memset(iso_context, 0, sizeof(*iso_context));

	if (resolve_iso_direct(out, iso_path, iso_context)) {
		return true;
	}

	DTTR_LoaderUI_ShowError(
		"DttR: ISO Load Failed",
		"DttR could not read the selected ISO. Consider using the extracted game files "
		"instead."
	);
	return false;
}

// Tries the saved ISO or game folder before prompting.
static bool try_configured_path(
	WCHAR *out,
	const char *configured_path,
	DTTR_LoaderIsoContext *iso_context
) {
	WCHAR wide_path[MAX_PATH];
	if (!utf8_to_wide_path(wide_path, configured_path)) {
		DTTR_LOG_ERROR("Could not convert configured path: %s", configured_path);
		return false;
	}

	if (DTTR_LoaderPath_IsISOW(wide_path)) {
		DTTR_LOG_INFO("Using configured ISO path: %s", configured_path);
		return resolve_iso(out, configured_path, iso_context);
	}

	if (try_dir(out, MAX_PATH, wide_path)) {
		DTTR_LOG_INFO("Using configured PCDOGS path: %s", configured_path);
		return true;
	}

	return false;
}

static char browse_result[MAX_PATH];
static HANDLE browse_event;

// Builds the chooser label and root path for a disc.
static void fill_disc_candidate(DTTR_LoaderUIDiscCandidate *candidate, char drive) {
	snprintf(candidate->label, sizeof(candidate->label), "Open Disc %c:", drive);
	snprintf(candidate->path, sizeof(candidate->path), "%c:\\", drive);
}

// Stores the SDL dialog result and wakes the browse loop.
static void SDLCALL browse_callback(void *, const char *const *filelist, int) {
	copy_path(
		browse_result,
		sizeof(browse_result),
		(filelist && filelist[0]) ? filelist[0] : NULL
	);

	if (browse_event) {
		SetEvent(browse_event);
	}
}

// Pumps SDL while the native browse dialog is open.
static bool wait_for_browse_result() {
	while (WaitForSingleObject(browse_event, 0) == WAIT_TIMEOUT) {
		DTTR_SDL_PumpEvents();
		DTTR_SDL_Delay(10);
	}

	return browse_result[0] != '\0';
}

// Saves the chosen source for the next launch without blocking this launch.
static void save_selected_path(const char *path) {
	copy_path(dttr_config.pcdogs_path, sizeof(dttr_config.pcdogs_path), path);
	if (!DTTR_Config_Save(dttr_config_path, &dttr_config)) {
		DTTR_LOG_ERROR(
			"Could not save selected game path to %s; continuing for this launch",
			dttr_config_path
		);
	}
}

// Finds mounted discs that contain a known game layout.
static void scan_disc_candidates(
	DTTR_LoaderUIDiscCandidate *candidates,
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

		if (!try_dir(game_path, MAX_PATH, root_w)) {
			continue;
		}

		DTTR_LoaderUIDiscCandidate *candidate = &candidates[*candidate_count];
		fill_disc_candidate(candidate, drive);
		(*candidate_count)++;

		if (*candidate_count >= DTTR_LOADER_UI_MAX_DISC_CANDIDATES) {
			return;
		}
	}
}

// Revalidates a disc before saving it.
static bool try_disc_candidate(WCHAR *out, const DTTR_LoaderUIDiscCandidate *candidate) {
	WCHAR wide_path[MAX_PATH];
	if (!utf8_to_wide_path(wide_path, candidate->path)
		|| !try_dir(out, MAX_PATH, wide_path)) {
		DTTR_LoaderUI_ShowError(
			"DttR: Disc Not Found",
			"The selected disc no longer contains pcdogs.exe."
		);
		return false;
	}

	DTTR_LOG_INFO("Selected game disc: %s", candidate->path);
	save_selected_path(candidate->path);
	return true;
}

// Opens the requested native picker.
static bool run_browse_dialog(DTTR_LoaderUIChoice choice) {
	if (!browse_event) {
		browse_event = CreateEventW(NULL, TRUE, FALSE, NULL);

		if (!browse_event) {
			DTTR_LOG_ERROR("Could not create browse completion event");
			return false;
		}
	}

	browse_result[0] = '\0';
	ResetEvent(browse_event);

	if (choice == DTTR_LOADER_UI_CHOICE_BROWSE_FOLDER) {
		DTTR_SDL_ShowOpenFolderDialog(browse_callback, NULL, NULL, NULL, false);
		return wait_for_browse_result();
	}

	const SDL_DialogFileFilter filters[] = {{"ISO images", "iso"}};
	DTTR_SDL_ShowOpenFileDialog(browse_callback, NULL, NULL, filters, 1, NULL, false);
	return wait_for_browse_result();
}

static bool try_browsed_path(
	WCHAR *out,
	DTTR_LoaderUIChoice choice,
	DTTR_LoaderIsoContext *iso_context
);

// Resolves the path returned by the requested picker.
static bool try_browse_choice(
	WCHAR *out,
	DTTR_LoaderUIChoice choice,
	DTTR_LoaderIsoContext *iso_context
) {
	return run_browse_dialog(choice) && try_browsed_path(out, choice, iso_context);
}

// Validates the latest browse result before saving it.
static bool try_browsed_path(
	WCHAR *out,
	DTTR_LoaderUIChoice choice,
	DTTR_LoaderIsoContext *iso_context
) {
	WCHAR wide_path[MAX_PATH];
	if (!utf8_to_wide_path(wide_path, browse_result)) {
		DTTR_LoaderUI_ShowError(
			"DttR: Game Not Found",
			"The selected folder does not contain pcdogs.exe."
		);
		return false;
	}

	if (choice == DTTR_LOADER_UI_CHOICE_BROWSE_ISO || DTTR_LoaderPath_IsISOW(wide_path)) {
		return resolve_iso(out, browse_result, iso_context);
	}

	if (try_dir(out, MAX_PATH, wide_path)) {
		return true;
	}

	DTTR_LoaderUI_ShowError(
		"DttR: Game Not Found",
		"The selected folder does not contain pcdogs.exe."
	);
	return false;
}

// Prompts until a source resolves to an executable.
static bool prompt_browse_for_path(WCHAR *out, DTTR_LoaderIsoContext *iso_context) {
	DTTR_LoaderUIDiscCandidate disc_candidates[DTTR_LOADER_UI_MAX_DISC_CANDIDATES];
	size_t disc_candidate_count = 0;
	scan_disc_candidates(disc_candidates, &disc_candidate_count);

	for (;;) {
		const DTTR_LoaderUIChoice choice = DTTR_LoaderUI_ChooseGameSource(
			disc_candidates,
			disc_candidate_count
		);

		size_t disc_index = 0;

		if (DTTR_LoaderUI_ChoiceIsDisc(choice, &disc_index)) {
			if (disc_index < disc_candidate_count
				&& try_disc_candidate(out, &disc_candidates[disc_index])) {
				return true;
			}

			scan_disc_candidates(disc_candidates, &disc_candidate_count);
			continue;
		}

		if (!DTTR_LoaderUI_ChoiceIsBrowse(choice)) {
			return false;
		}

		if (!try_browse_choice(out, choice, iso_context)) {
			continue;
		}

		DTTR_LOG_INFO("Selected game path: %s", browse_result);
		save_selected_path(browse_result);
		return true;
	}
}

// Uses the saved source when valid, otherwise prompts.
bool DTTR_Loader_ResolveEXEPath(
	WCHAR *out,
	const char *configured_path,
	DTTR_LoaderIsoContext *iso_context
) {
	if (configured_path && configured_path[0]
		&& try_configured_path(out, configured_path, iso_context)) {
		return true;
	}

	return prompt_browse_for_path(out, iso_context);
}
