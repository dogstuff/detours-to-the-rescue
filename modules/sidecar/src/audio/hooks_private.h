#ifndef DTTR_AUDIO_HOOKS_PRIVATE_H
#define DTTR_AUDIO_HOOKS_PRIVATE_H

#include <dttr_mods.h>

// Installs audio hooks that keep MSS initialized even without playback hardware.
bool dttr_audio_init(const DTTR_Mods_Context *ctx);
// Releases audio hooks and the SDL-backed MSS shim.
void dttr_audio_cleanup(const DTTR_Mods_Context *ctx);

#endif // DTTR_AUDIO_HOOKS_PRIVATE_H
