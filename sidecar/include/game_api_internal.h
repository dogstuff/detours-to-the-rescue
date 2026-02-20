#ifndef DTTR_GAME_API_INTERNAL_H
#define DTTR_GAME_API_INTERNAL_H

#include <windows.h>

#include <dttr_components.h>

/// Initializes the shared game API context used by the sidecar and components.
/// Must be called once after DllMain, before any interop macros are used.
void dttr_game_api_init(HMODULE game_module, HMODULE sidecar_module);

/// Returns the shared component context for use with interop macros.
const DTTR_ComponentContext *dttr_game_api_get_ctx(void);

/// Detaches all remaining hooks in reverse order and frees the sigscan cache.
/// Call at the end of the cleanup sequence.
void dttr_game_api_cleanup(void);

#endif /* DTTR_GAME_API_INTERNAL_H */
