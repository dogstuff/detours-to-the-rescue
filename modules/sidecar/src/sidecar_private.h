#ifndef DTTR_SIDECAR_PRIVATE_H
#define DTTR_SIDECAR_PRIVATE_H

#include <stdbool.h>
#include <stddef.h>

#include <dttr_mods.h>
#include <dttr_pcdogs.h>
#include <dttr_runtime.h>

#include <SDL3/SDL.h>

DTTR_HOOK_STORAGE_SLOT(dttr_hook_win_main)
DTTR_STORAGE_SLOT(
	DTTR_PCDOGS_F_TitleScreenCleanupResources_proto,
	dttr_hook_cleanup_title_resources_original
)

const DTTR_Mods_Context *dttr_sidecar_context();
const DTTR_Core_Context *dttr_sidecar_runtime_context();
// Routes SDL events through the shared sidecar runtime path.
void dttr_sidecar_handle_sdl_event(const SDL_Event *event);
// Pumps SDL events through the shared sidecar runtime path.
void dttr_sidecar_poll_sdl_events();

// Installs a PCDogs patch group with common sidecar logging and cleanup.
static inline bool dttr_sidecar_install_pcdogs_patch_group(
	const DTTR_Mods_Context *ctx,
	const char *label,
	const DTTR_PCDOGS_T_Patch_Spec *patches,
	size_t patch_count,
	DTTR_Core_PatchGroup **group
) {
	DTTR_PCDOGS_T_Patch_Report report = {0};
	DTTR_Core_Result result = DTTR_PCDOGS_PatchGroup_Install(
		&ctx->runtime,
		patches,
		patch_count,
		group,
		&report
	);
	if (!DTTR_Core_ResultOk(result)) {
		DTTR_MODS_LOG_ERROR(
			ctx,
			"%s: patch %u failed: %s",
			label,
			(unsigned)report.failed_index,
			result.message
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
