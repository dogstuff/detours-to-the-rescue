#include "hooks_private.h"
#include "sidecar_private.h"

#include <dttr_pcdogs.h>

static const DTTR_PCDOGS_T_Patch_Spec game_patches[] = {
	{
		.kind = DTTR_PCDOGS_PATCH_FUNCTION_HOOK,
		.required = true,
		.function = DTTR_PCDOGS_FUNCTION_FILE_OPEN,
		.detour = dttr_crt_hook_open_file_callback,
		.out_original = NULL,
	},
	{
		.kind = DTTR_PCDOGS_PATCH_FUNCTION_HOOK,
		.required = true,
		.function = DTTR_PCDOGS_FUNCTION_TITLE_SCREEN_CLEANUP_RESOURCES,
		.detour = dttr_hook_cleanup_title_resources_callback,
		.out_original = (void **)&dttr_hook_cleanup_title_resources_original,
	},
	DTTR_PCDOGS_PATCH_SPEC_AOB_REL32_JMP(
		false,
		"51 8D 44 24 ?? 57",
		0,
		dttr_hook_resolve_pcdogs_path_callback
	),
};

static DTTR_Core_PatchGroup *game_targets;

// Installs the game-level patch group and clears cleanup state if any required hook fails.
bool dttr_game_hooks_init(const DTTR_Mods_Context *ctx) {
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
void dttr_game_hooks_cleanup(const DTTR_Mods_Context *ctx) {
	DTTR_Core_PatchGroupRelease(&game_targets);
	dttr_hook_cleanup_title_resources_original = NULL;
}
