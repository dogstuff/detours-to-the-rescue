#ifndef DTTR_AUDIO_HOOKS_PRIVATE_H
#define DTTR_AUDIO_HOOKS_PRIVATE_H

#include <dttr_mods.h>

// This declaration installs audio hooks that keep MSS initialized.
bool dttr_audio_init(const DTTR_Mods_Context *ctx);

// This declaration releases audio hooks and the SDL-backed MSS shim.
void dttr_audio_cleanup(const DTTR_Mods_Context *ctx);

#endif // DTTR_AUDIO_HOOKS_PRIVATE_H
