#ifndef DTTR_SIDECAR_H
#define DTTR_SIDECAR_H

#include <stdbool.h>
#include <stdint.h>
#include <windows.h>

#include <SDL3/SDL.h>
#include <dttr_components.h>
#include <dttr_config.h>
#include <dttr_interop_pcdogs.h>

#include "game_api_internal.h"

// Handle to the injected sidecar DLL itself
extern HINSTANCE g_dttr_sidecar_module;

// Directory containing the loader DLL, with a trailing backslash.
extern char g_dttr_loader_dir[MAX_PATH];

// 16-character lowercase hex XXH3_64 hash of the game executable.
extern char g_dttr_exe_hash[17];

/// Initializes the SDL graphics backend and returns the game window handle
HWND dttr_graphics_init(void);

/// Releases all graphics resources and shuts down the SDL graphics backend
void dttr_graphics_cleanup(void);

/// Returns the main game window
SDL_Window *dttr_graphics_get_window(void);

/// Returns the GPU device
SDL_GPUDevice *dttr_graphics_get_device(void);

/// Applies runtime window resize to rendering policy
void dttr_graphics_handle_window_resize(int width, int height);

/// Uploads and presents one BGRA video frame directly to the swapchain
bool dttr_graphics_present_video_frame_bgra(
	const uint8_t *pixels,
	int width,
	int height,
	int stride
);

extern SDL_Gamepad *g_dttr_gamepad;

/// Initializes SDL gamepad subsystem
void dttr_inputs_init(void);

/// Installs input hooks
void dttr_inputs_hook_init(const DTTR_ComponentContext *ctx);

/// Handles gamepad connection and disconnection events
void dttr_inputs_handle_device_event(const SDL_Event *event);

/// Sets the joystick-available flag after game systems are initialized
void dttr_inputs_late_init(void);

/// Removes input hooks
void dttr_inputs_hook_cleanup(const DTTR_ComponentContext *ctx);

/// Releases input subsystem resources
void dttr_inputs_cleanup(void);

typedef enum {
	DTTR_MOVIE_PLAYING = 0,
	DTTR_MOVIE_ENDED = 1,
	DTTR_MOVIE_ESCAPE = 2,
	DTTR_MOVIE_QUIT = 3,
} DTTR_MovieResult;

/// Initializes the video playback subsystem (creates mpv instance).
void dttr_movies_init(void);

/// Installs video playback hooks.
void dttr_movies_hook_init(const DTTR_ComponentContext *ctx);

/// Removes video playback hooks.
void dttr_movies_hook_cleanup(const DTTR_ComponentContext *ctx);

/// Releases video playback subsystem resources.
void dttr_movies_cleanup(void);

/// Begins async playback of the movie at the given path.
void dttr_movies_start(const char *path);

/// Renders one frame (or EOF).
void dttr_movies_tick(void);

/// Handles input events for movie playback.
/// Returns whether an event was consumed.
bool dttr_movies_handle_event(const SDL_Event *event);

/// Stops playback, drains mpv events, and returns DTTR_MovieResult.
DTTR_MovieResult dttr_movies_stop(void);

/// Returns true if a movie is currently playing.
bool dttr_movies_movie_is_playing(void);

#endif
