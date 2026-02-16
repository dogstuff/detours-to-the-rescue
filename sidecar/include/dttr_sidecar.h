#ifndef DTTR_SIDECAR_H
#define DTTR_SIDECAR_H

#include <stdbool.h>
#include <stdint.h>
#include <windows.h>

#include <SDL3/SDL.h>
#include <dttr_config.h>
#include <dttr_interop_pcdogs.h>

// Holds the loaded game module handle and shared configuration state
extern DTTR_GameModule g_dttr_pc_dogs_module;

// Directory containing the loader DLL, with trailing backslash
extern char g_dttr_loader_dir[MAX_PATH];

// 16-char lowercase hex XXH3_64 hash of the game executable
extern char g_dttr_exe_hash[17];

/// Initializes the SDL graphics backend and returns the game window handle
HWND dttr_graphics_init(void);
/// Releases all graphics resources and shuts down the SDL graphics backend
void dttr_graphics_cleanup(void);

extern SDL_Gamepad *g_dttr_gamepad;

/// Installs input hooks and initializes input subsystem state
void dttr_inputs_init(HMODULE module);

/// Sets the joystick-available flag after game systems are initialized
void dttr_inputs_late_init(void);

/// Removes input hooks and releases input subsystem resources
void dttr_inputs_cleanup(void);

#endif
