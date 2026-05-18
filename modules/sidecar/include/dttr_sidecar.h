#ifndef DTTR_SIDECAR_H
#define DTTR_SIDECAR_H

#include <stdbool.h>
#include <stdint.h>
#include <windows.h>

#include <SDL3/SDL.h>
#include <dttr_config.h>
#include <dttr_mods.h>

#define DTTR_EXE_HASH_LENGTH 16

// Handle to the injected sidecar DLL.
extern HINSTANCE dttr_sidecar_module;

// Directory containing the loader DLL, with a trailing backslash.
extern char dttr_loader_dir[MAX_PATH];

// 16-character lowercase hex XXH3_64 hash of the game executable.
extern char dttr_exe_hash[DTTR_EXE_HASH_LENGTH + 1];

HWND DTTR_Graphics_Init();

void DTTR_Graphics_Cleanup();

SDL_Window *DTTR_Graphics_GetWindow();

SDL_GPUDevice *DTTR_Graphics_GetDevice();

void DTTR_Graphics_HandleWindowResize(int width, int height);

/// Uploads one BGRA movie frame directly to the active swapchain.
bool DTTR_Graphics_PresentVideoFrameBGRA(
	const uint8_t *pixels,
	int width,
	int height,
	int stride
);

extern SDL_Gamepad *dttr_gamepad;

void DTTR_Inputs_Init();

void DTTR_Inputs_HandleDeviceEvent(const SDL_Event *event);

void DTTR_Inputs_LateInit();

void DTTR_Inputs_Cleanup();

typedef enum {
	DTTR_MOVIE_PLAYING = 0,
	DTTR_MOVIE_ENDED = 1,
	DTTR_MOVIE_ESCAPE = 2,
	DTTR_MOVIE_QUIT = 3,
} DTTR_MovieResult;

void DTTR_Movies_Init();

void DTTR_Movies_Cleanup();

void DTTR_Movies_Start(const char *path);

void DTTR_Movies_Tick();

bool DTTR_Movies_HandleEvent(const SDL_Event *event);

/// Stops active movie playback and returns the result expected by the game.
DTTR_MovieResult DTTR_Movies_Stop();

bool DTTR_Movies_MovieIsPlaying();

#endif // DTTR_SIDECAR_H
