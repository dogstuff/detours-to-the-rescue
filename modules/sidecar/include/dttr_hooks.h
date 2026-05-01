#ifndef DTTR_HOOKS_H
#define DTTR_HOOKS_H

#include <dttr_components.h>

#include "dttr_hooks_audio.h"
#include "dttr_hooks_game.h"
#include "dttr_hooks_graphics.h"
#include "dttr_hooks_inputs.h"
#include "dttr_hooks_movies.h"

/// Replaces the game's WinMain entry point with sidecar bootstrapping logic.
int32_t _stdcall dttr_hook_win_main_callback(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	int32_t nCmdShow
);

DTTR_HOOK(dttr_hook_win_main)

#endif
