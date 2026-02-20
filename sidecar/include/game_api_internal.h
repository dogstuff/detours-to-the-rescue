#ifndef DTTR_GAME_API_INTERNAL_H
#define DTTR_GAME_API_INTERNAL_H

#include <windows.h>

#include <SDL3/SDL.h>
#include <dttr_components.h>

/// Initializes the shared game API context used by the sidecar and components.
/// Must be called once after DllMain, before any interop macros are used.
void dttr_game_api_init(HMODULE game_module, HMODULE sidecar_module);

/// Returns the shared component context for use with interop macros.
const DTTR_ComponentContext *dttr_game_api_get_ctx(void);

/// Sets the SDL window in the shared context (called after graphics init).
void dttr_game_api_set_window(SDL_Window *window);

/// Sets the GPU device in the shared context (called after graphics init).
void dttr_game_api_set_device(SDL_GPUDevice *device);

#endif /* DTTR_GAME_API_INTERNAL_H */
