#ifndef DTTR_GAME_HOOKS_PRIVATE_H
#define DTTR_GAME_HOOKS_PRIVATE_H

#include <stdint.h>

#include <dttr_mods.h>
#include <dttr_pcdogs.h>
#include <dttr_runtime.h>
#include <windows.h>

// Redirects file opens through saved-game and cached-data paths.
DTTR_PCDOGS_T_File_Handle *__cdecl dttr_crt_hook_open_file_callback(
	const char *path,
	const char *mode
);

// Clears stale title-screen resource globals during cleanup.
DTTR_STORAGE_SLOT(
	DTTR_PCDOGS_F_Title_CleanupScreenResources_proto,
	dttr_hook_cleanup_title_resources_original
)
void __cdecl dttr_hook_cleanup_title_resources_callback();

// Points the game at the resolved PCDogs data path.
uint32_t __cdecl dttr_hook_resolve_pcdogs_path_callback();

// This declaration installs file, path, and title-resource cleanup hooks.
bool dttr_game_hooks_init(const DTTR_Mods_Context *ctx);

// This declaration releases the game hook group and saved callback pointers.
void dttr_game_hooks_cleanup(const DTTR_Mods_Context *ctx);

#endif // DTTR_GAME_HOOKS_PRIVATE_H
