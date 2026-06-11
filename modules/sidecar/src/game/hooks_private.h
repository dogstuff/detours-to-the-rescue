#ifndef DTTR_GAME_HOOKS_PRIVATE_H
#define DTTR_GAME_HOOKS_PRIVATE_H

#include <stdint.h>

#include <dttr_mods.h>
#include <dttr_pcdogs.h>
#include <dttr_runtime.h>
#include <windows.h>

DTTR_STORAGE_SLOT(
	DTTR_PCDOGS_F_Title_CleanupScreenResources_proto,
	dttr_hook_cleanup_title_resources_original
)

// Replacement file-open callback that supports redirected saves and cached game data.
DTTR_PCDOGS_T_File_Handle *__cdecl dttr_crt_hook_open_file_callback(
	const char *path,
	const char *mode
);
// Replacement bootstrap callback that points the game at the resolved PCDogs path.
uint32_t __cdecl dttr_hook_resolve_pcdogs_path_callback();
// Replacement title-screen cleanup callback that clears stale resource globals.
BOOL __cdecl dttr_hook_cleanup_title_resources_callback();

#ifdef DTTR_MODS_ENABLED
extern DTTR_PCDOGS_F_Model_AdvanceAnimation_proto
	dttr_game_hook_model_advance_animation_original;
extern DTTR_PCDOGS_F_Scene_UpdateNodeAnimation_proto
	dttr_game_hook_scene_update_node_animation_original;

int32_t __cdecl dttr_game_hook_model_advance_animation_callback(
	DTTR_PCDOGS_T_Actor_State *actor
);

void __cdecl dttr_game_hook_scene_update_node_animation_callback(
	DTTR_PCDOGS_T_Actor_State *actor,
	DTTR_PCDOGS_T_Scene_Node *parent_node,
	DTTR_PCDOGS_T_Scene_Node *node
);
#endif

// Installs file, path, and title-resource cleanup hooks as one game patch group.
bool dttr_game_hooks_init(const DTTR_Mods_Context *ctx);
// Releases the game patch group and saved callback pointers.
void dttr_game_hooks_cleanup(const DTTR_Mods_Context *ctx);

#endif // DTTR_GAME_HOOKS_PRIVATE_H
