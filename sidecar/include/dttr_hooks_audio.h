#ifndef DTTR_HOOKS_AUDIO_H
#define DTTR_HOOKS_AUDIO_H

#include <dttr_components.h>

#include <SDL3/SDL.h>

/// Skip MSS init when no SDL audio devices are available
void __cdecl dttr_hook_audio_init_system_callback(void);

DTTR_TRAMPOLINE_HOOK(dttr_hook_audio_init_system)

/// Null driver guard for Audio_StopAllSounds
void __cdecl dttr_hook_audio_stop_all_sounds_callback(void);

DTTR_TRAMPOLINE_HOOK(dttr_hook_audio_stop_all_sounds)

/// Null driver guard for Audio_InitializeLevelAudio
void __cdecl dttr_hook_audio_init_level_audio_callback(void);

DTTR_TRAMPOLINE_HOOK(dttr_hook_audio_init_level_audio)

/// Null driver guard for Audio_StopAllSamples
void __cdecl dttr_hook_audio_stop_all_samples_callback(void);

DTTR_TRAMPOLINE_HOOK(dttr_hook_audio_stop_all_samples)

/// Installs audio crash prevention hooks
void dttr_audio_init(const DTTR_ComponentContext *ctx);

/// Removes audio hooks and frees trampoline memory
void dttr_audio_cleanup(const DTTR_ComponentContext *ctx);

/// Handles SDL audio device added/removed events for hot-plug
void dttr_audio_handle_device_event(const SDL_Event *event);

#endif /* DTTR_HOOKS_AUDIO_H */
