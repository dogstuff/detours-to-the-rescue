#include "hooks_private.h"
#include "sidecar_hook_sigs.h"
#include "sidecar_private.h"

#include <dttr_pcdogs.h>

DTTR_DEFINE_STORAGE(
	DTTR_PCDOGS_F_Title_CleanupScreenResources_proto,
	dttr_hook_cleanup_title_resources_original
)

static DTTR_Core_PatchGroup *game_targets;

// Installs the game-level patch group and clears cleanup state if any required hook
// fails.
bool dttr_game_hooks_init(const DTTR_Mods_Context *ctx) {
	const DTTR_PCDOGS_T_Patch_Spec game_patches[] = {
		DTTR_PCDOGS_F_File_Open->PatchSpec(true, dttr_crt_hook_open_file_callback, NULL),
		DTTR_PCDOGS_F_Title_CleanupScreenResources->PatchSpec(
			true,
			dttr_hook_cleanup_title_resources_callback,
			&dttr_hook_cleanup_title_resources_original
		),
		DTTR_PCDOGS_PATCH_SPEC_AOB_REL32_JMP(
			false,
			DTTR_SIDECAR_AOB_RESOLVE_PCDOGS_PATH,
			0,
			dttr_hook_resolve_pcdogs_path_callback
		),
	};

	if (!dttr_sidecar_install_pcdogs_patch_group(
			ctx,
			"sidecar/game",
			game_patches,
			DTTR_ARRAY_COUNT(game_patches),
			&game_targets
		)) {
		dttr_hook_cleanup_title_resources_original = NULL;
		return false;
	}

	return true;
}

// Releases all game-level patches and drops the saved cleanup callback pointer.
void dttr_game_hooks_cleanup(const DTTR_Mods_Context *) {
	DTTR_Core_PatchGroupRelease(&game_targets);
	dttr_hook_cleanup_title_resources_original = NULL;
}
