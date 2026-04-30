#ifndef DTTR_HOOKS_GAME_H
#define DTTR_HOOKS_GAME_H

#include <dttr_components.h>

#include "dttr_interop_pcdogs.h"

DTTR_FUNC(
	dttr_crt_open_file_with_mode,
	/* default cc */,
	void *,
	(const char *path, char *mode, int flags),
	(path, mode, flags)
)

/// Hooks the game's internal OpenFile to handle failures gracefully.
void *__cdecl dttr_crt_hook_open_file_callback(const char *path, char *mode);

DTTR_HOOK(dttr_crt_hook_open_file)

/// Fixes later releases that can misresolve the game directory when
/// 'p' appears earlier in the path before "pcdogs.exe".
uint32_t __cdecl dttr_hook_resolve_pcdogs_path_callback(void);

DTTR_HOOK(dttr_hook_resolve_pcdogs_path)

/// Clears stale CleanupLevelAssets pointers after the original routine runs.
void __cdecl dttr_hook_cleanup_level_assets_callback(void);

DTTR_TRAMPOLINE_HOOK(dttr_hook_cleanup_level_assets)

/// Initializes miscellaneous hooks.
void dttr_game_hooks_init(const DTTR_ComponentContext *ctx);

/// Removes miscellaneous hooks and frees trampoline memory.
void dttr_game_hooks_cleanup(const DTTR_ComponentContext *ctx);

#endif
