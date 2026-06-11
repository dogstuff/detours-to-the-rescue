#include "hooks_private.h"
#include "sidecar_hook_sigs.h"
#include "sidecar_private.h"

#include <dttr_pcdogs.h>

#ifdef DTTR_MODS_ENABLED
#include "frame_pacing_private.h"
#endif

DTTR_DEFINE_STORAGE(
	DTTR_PCDOGS_F_Title_CleanupScreenResources_proto,
	dttr_hook_cleanup_title_resources_original
)

#ifdef DTTR_MODS_ENABLED
DTTR_DEFINE_STORAGE(
	DTTR_PCDOGS_F_Model_AdvanceAnimation_proto,
	dttr_game_hook_model_advance_animation_original
)

DTTR_DEFINE_STORAGE(
	DTTR_PCDOGS_F_Scene_UpdateNodeAnimation_proto,
	dttr_game_hook_scene_update_node_animation_original
)

int32_t __cdecl dttr_game_hook_model_advance_animation_callback(
	DTTR_PCDOGS_T_Actor_State *actor
) {
	if (dttr_game_render_only_scene_replay_active()) {
		return 0;
	}

	return dttr_game_hook_model_advance_animation_original
			   ? dttr_game_hook_model_advance_animation_original(actor)
			   : 0;
}

void __cdecl dttr_game_hook_scene_update_node_animation_callback(
	DTTR_PCDOGS_T_Actor_State *actor,
	DTTR_PCDOGS_T_Scene_Node *parent_node,
	DTTR_PCDOGS_T_Scene_Node *node
) {
	if (dttr_game_render_only_scene_replay_active()) {
		return;
	}

	if (dttr_game_hook_scene_update_node_animation_original) {
		dttr_game_hook_scene_update_node_animation_original(actor, parent_node, node);
	}
}
#endif

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

#ifdef DTTR_MODS_ENABLED
#define SIDECAR_GAME_REPLAY_MUTATION_GATE(accessor, callback, original_ptr)              \
	accessor->PatchSpec(true, callback, original_ptr),
#include "sidecar_game_replay_mutation_gates.def"
#undef SIDECAR_GAME_REPLAY_MUTATION_GATE
#endif
	};

	if (!dttr_sidecar_install_pcdogs_patch_group(
			ctx,
			"sidecar/game",
			game_patches,
			DTTR_ARRAY_COUNT(game_patches),
			&game_targets
		)) {
		dttr_hook_cleanup_title_resources_original = NULL;

#ifdef DTTR_MODS_ENABLED
#define SIDECAR_GAME_REPLAY_MUTATION_GATE(accessor, callback, original_ptr)              \
	*(original_ptr) = NULL;
#include "sidecar_game_replay_mutation_gates.def"
#undef SIDECAR_GAME_REPLAY_MUTATION_GATE
#endif

		return false;
	}

	return true;
}

// Releases all game-level patches and drops the saved cleanup callback pointer.
void dttr_game_hooks_cleanup(const DTTR_Mods_Context *) {
	DTTR_Core_PatchGroupRelease(&game_targets);
	dttr_hook_cleanup_title_resources_original = NULL;

#ifdef DTTR_MODS_ENABLED
#define SIDECAR_GAME_REPLAY_MUTATION_GATE(accessor, callback, original_ptr)              \
	*(original_ptr) = NULL;
#include "sidecar_game_replay_mutation_gates.def"
#undef SIDECAR_GAME_REPLAY_MUTATION_GATE
#endif
}
