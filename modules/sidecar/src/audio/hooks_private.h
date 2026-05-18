#ifndef DTTR_AUDIO_HOOKS_PRIVATE_H
#define DTTR_AUDIO_HOOKS_PRIVATE_H

#include <SDL3/SDL.h>
#include <dttr_mods.h>

// Integrates audio init with recovery from SDL device changes.
bool dttr_audio_init(const DTTR_Mods_Context *ctx);
// Integrates audio cleanup with recovery from SDL device changes.
void dttr_audio_cleanup(const DTTR_Mods_Context *ctx);
// Integrates SDL audio device events with MSS driver recovery.
void dttr_audio_handle_device_event(const SDL_Event *event);

#endif // DTTR_AUDIO_HOOKS_PRIVATE_H
