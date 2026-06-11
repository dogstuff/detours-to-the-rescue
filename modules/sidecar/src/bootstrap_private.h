#ifndef DTTR_SIDECAR_BOOTSTRAP_PRIVATE_H
#define DTTR_SIDECAR_BOOTSTRAP_PRIVATE_H

#include <stdbool.h>
#include <windows.h>

#include <dttr_mods.h>
#include <dttr_runtime.h>

typedef enum {
	DTTR_STARTUP_MOVIES_CONTINUE,
	DTTR_STARTUP_MOVIES_QUIT,
	DTTR_STARTUP_MOVIES_FAILED,
} dttr_startup_movies_result;

// Initializes subsystems that own required hooks. The order mirrors
// dttr_bootstrap_cleanup_runtime().
bool dttr_bootstrap_install_required_hooks(const DTTR_Mods_Context *ctx);
// Releases modding runtime hooks and mod state before graphics and audio shutdown.
void dttr_bootstrap_cleanup_runtime(const DTTR_Mods_Context *ctx);

// Runs required PCDOGS startup calls after the game window exists.
bool dttr_bootstrap_initialize_pcdogs_runtime(const DTTR_Core_Context *ctx, HWND hwnd);
// Moves the modding runtime into its started state after initialization succeeds.
bool dttr_bootstrap_start_pcdogs_runtime(const DTTR_Core_Context *ctx, HWND hwnd);

// Runs per-frame sidecar systems before yielding back to the original game loop.
bool dttr_bootstrap_tick_main_loop();
// Plays startup movies through the normal sidecar tick loop.
dttr_startup_movies_result dttr_bootstrap_attempt_play_startup_movies();

#endif // DTTR_SIDECAR_BOOTSTRAP_PRIVATE_H
