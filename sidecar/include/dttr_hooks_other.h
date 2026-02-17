#ifndef DTTR_HOOKS_OTHER_H
#define DTTR_HOOKS_OTHER_H

#include <dttr_interop.h>
#include <stdint.h>
#include <windows.h>

#include "dttr_interop_pcdogs.h"

DTTR_INTEROP_WRAP_CACHED_SIG(
	dttr_crt_open_file_with_mode,
	"\xE8????\x85\xC0\x75?\xC3",
	"x????xxx?x",
	match,
	void *,
	(const char *path, char *mode, int flags),
	(path, mode, flags)
)

/// Hooks the game's internal OpenFile to gracefully handle failures
void *__cdecl dttr_crt_hook_open_file_callback(const char *path, char *mode);

DTTR_INTEROP_HOOK_FUNC_SIG(
	dttr_crt_hook_open_file,
	"\x6A\x40\xFF\x74\x24\x0C\xFF\x74\x24\x0C\xE8",
	"xxxxxxxxxxx",
	match,
	dttr_crt_hook_open_file_callback
)

/// Fixes an issue in later releases of the game that causes the
/// game directory to be incorrectly resolved if the 'p' character
/// is present in the path before 'pcdogs.exe'.
///
/// This issue would cause crashes on boot if the user did not have
/// write access to the incorrectly resolved directory.
uint32_t __cdecl dttr_hook_resolve_pcdogs_path_callback(void);

DTTR_INTEROP_HOOK_FUNC_OPTIONAL_SIG(
	dttr_hook_resolve_pcdogs_path,
	"\x51\x8D\x44\x24?\x57",
	"xxxx?x",
	match,
	dttr_hook_resolve_pcdogs_path_callback
)

/// Fixes a game bug where the CleanupLevelAssets frees a few global
/// resource pointers without setting them to 0, causing a crash
/// when it subsequently tries to free any "allocated" resources.
///
/// This was causing crashes on the main European of the game when
/// trying to press "new game" when on the level select screen.
void __cdecl dttr_hook_cleanup_level_assets_callback(void);

DTTR_INTEROP_HOOK_FUNC_TRAMPOLINE_SIG(
	dttr_hook_cleanup_level_assets,
	7,
	DTTR_CLEANUP_LEVEL_ASSETS_SIG,
	DTTR_CLEANUP_LEVEL_ASSETS_MASK,
	match,
	dttr_hook_cleanup_level_assets_callback
)

/// Initializes miscellaneous hooks
void dttr_other_hook_init(HMODULE mod);

/// Removes miscellaneous hooks and frees trampoline memory
void dttr_other_hook_cleanup(void);

#endif
