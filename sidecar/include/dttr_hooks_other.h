#ifndef DTTR_HOOKS_OTHER_H
#define DTTR_HOOKS_OTHER_H

#include <dttr_components.h>

#include "dttr_interop_pcdogs.h"

DTTR_FUNC(
	dttr_crt_open_file_with_mode,
	/* default cc */,
	void *,
	(const char *path, char *mode, int flags),
	(path, mode, flags)
)

/// Hooks the game's internal OpenFile to gracefully handle failures
void *__cdecl dttr_crt_hook_open_file_callback(const char *path, char *mode);

DTTR_HOOK(dttr_crt_hook_open_file)

/// Fixes an issue in later releases of the game that causes the
/// game directory to be incorrectly resolved if the 'p' character
/// is present in the path before 'pcdogs.exe'.
///
/// This issue would cause crashes on boot if the user did not have
/// write access to the incorrectly resolved directory.
uint32_t __cdecl dttr_hook_resolve_pcdogs_path_callback(void);

DTTR_HOOK(dttr_hook_resolve_pcdogs_path)

/// Fixes a game bug where the CleanupLevelAssets frees a few global
/// resource pointers without setting them to 0, causing a crash
/// when it subsequently tries to free any "allocated" resources.
///
/// This was causing crashes on the main European of the game when
/// trying to press "new game" when on the level select screen.
void __cdecl dttr_hook_cleanup_level_assets_callback(void);

DTTR_TRAMPOLINE_HOOK(dttr_hook_cleanup_level_assets)

/// Initializes miscellaneous hooks
void dttr_other_hooks_init(const DTTR_ComponentContext *ctx);

/// Removes miscellaneous hooks and frees trampoline memory
void dttr_other_hooks_cleanup(const DTTR_ComponentContext *ctx);

#endif
