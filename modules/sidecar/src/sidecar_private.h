#ifndef DTTR_SIDECAR_PRIVATE_H
#define DTTR_SIDECAR_PRIVATE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <windows.h>

#include <dttr_mods.h>
#include <dttr_pcdogs.h>
#include <dttr_runtime.h>

#include <sds.h>

#include <SDL3/SDL.h>

enum { DTTR_EXE_HASH_LENGTH = 16 };

extern HINSTANCE dttr_sidecar_module;
extern char dttr_loader_dir[MAX_PATH];
extern char dttr_exe_hash[DTTR_EXE_HASH_LENGTH + 1];

DTTR_HOOK_STORAGE_SLOT(dttr_hook_win_main)
DTTR_STORAGE_SLOT(
	DTTR_PCDOGS_F_Title_CleanupScreenResources_proto,
	dttr_hook_cleanup_title_resources_original
)

void dttr_game_data_init();
void dttr_game_data_cleanup();
bool dttr_game_data_resolve_existing_read_path(
	const char *path,
	char *out_path,
	size_t out_path_size
);
bool dttr_game_data_resolve_read_path(
	const char *path,
	char *out_path,
	size_t out_path_size
);
const char *dttr_game_data_find_data_segment(const char *path);
sds dttr_game_data_resolve_media_path(const char *relative);

void dttr_pcdogs_crash_symbols_register(const DTTR_Core_Context *runtime);
void dttr_pcdogs_crash_symbols_clear();

const DTTR_Mods_Context *dttr_sidecar_context();
const DTTR_Core_Context *dttr_sidecar_runtime_context();
void dttr_sidecar_handle_sdl_event(const SDL_Event *event);
void dttr_sidecar_poll_sdl_events();

static inline const char *dttr_sidecar_result_detail(DTTR_Result result) {
	return result.message ? result.message : DTTR_StatusName(result.status);
}

// Installs a PCDogs patch group with common sidecar logging and cleanup.
static inline bool dttr_sidecar_install_pcdogs_patch_group(
	const DTTR_Mods_Context *ctx,
	const char *label,
	const DTTR_PCDOGS_T_Patch_Spec *patches,
	size_t patch_count,
	DTTR_Core_PatchGroup **group
) {
	DTTR_PCDOGS_T_Patch_Report report = {0};
	DTTR_Result result = DTTR_PCDOGS_PatchGroup_Install(
		&ctx->runtime,
		patches,
		patch_count,
		group,
		&report
	);
	if (!DTTR_ResultOK(result)) {
		DTTR_MODS_LOG_ERROR(
			ctx,
			"%s: patch %u failed: %s",
			label,
			(unsigned)report.failed_index,
			dttr_sidecar_result_detail(result)
		);
		DTTR_Core_PatchGroupRelease(group);
		return false;
	}

	DTTR_MODS_LOG_DEBUG(
		ctx,
		"Installed %u %s patches (%u optional skipped)",
		(unsigned)report.installed,
		label,
		(unsigned)report.skipped_optional
	);
	return true;
}

#endif // DTTR_SIDECAR_PRIVATE_H
