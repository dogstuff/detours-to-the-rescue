#ifndef DTTR_HOOKS_H
#define DTTR_HOOKS_H

#include <dttr_interop.h>
#include <stdint.h>
#include <windows.h>

#include "dttr_hooks_audio.h"
#include "dttr_hooks_graphics.h"
#include "dttr_hooks_inputs.h"
#include "dttr_hooks_movies.h"
#include "dttr_hooks_other.h"

/// Replaces the game's WinMain entry point with sidecar bootstrapping logic
int32_t _stdcall dttr_hook_win_main_callback(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	int32_t nCmdShow
);

// Bootstrap hook installed in DllMain before the module context exists.
// Uses the raw HMODULE variant that patches directly via VirtualProtect.
DTTR_INTEROP_HOOK_FUNC_RAW_SIG(
	dttr_hook_win_main,
	"\x83\xEC\x40\x53\x8B\x5C\x24",
	"xxxxxxx",
	match,
	dttr_hook_win_main_callback
);

#endif
