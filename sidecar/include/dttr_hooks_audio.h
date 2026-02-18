#ifndef DTTR_HOOKS_AUDIO_H
#define DTTR_HOOKS_AUDIO_H

#include <dttr_interop.h>
#include <stdint.h>
#include <windows.h>

#include "dttr_interop_pcdogs.h"

#include <SDL3/SDL.h>

DTTR_INTEROP_WRAP_CACHED_CC_SIG(
	pcdogs_audio_shutdown_system,
	__cdecl,
	"\xA1????\x85\xC0\x74?\x53\x8B\x1D",
	"x????xxx?xxx",
	match,
	void,
	(void),
	()
)

/// Skip MSS init when no SDL audio devices are available
void __cdecl dttr_hook_audio_init_system_callback(void);

DTTR_INTEROP_HOOK_FUNC_TRAMPOLINE_SIG(
	dttr_hook_audio_init_system,
	6,
	"\x81\xEC\x90???\x55\x56\x57\xFF\x15",
	"xxx???xxxxx",
	match,
	dttr_hook_audio_init_system_callback
)

/// Null driver guard for Audio_StopAllSounds
void __cdecl dttr_hook_audio_stop_all_sounds_callback(void);

DTTR_INTEROP_HOOK_FUNC_TRAMPOLINE_SIG(
	dttr_hook_audio_stop_all_sounds,
	5,
	"\xA1????\x6A?\x50\xFF\x15",
	"x????x?xxx",
	match,
	dttr_hook_audio_stop_all_sounds_callback
)

/// Null driver guard for Audio_InitializeLevelAudio
void __cdecl dttr_hook_audio_init_level_audio_callback(void);

DTTR_INTEROP_HOOK_FUNC_TRAMPOLINE_SIG(
	dttr_hook_audio_init_level_audio,
	5,
	"\xA1????\x6A\x7F\x50\xFF\x15",
	"x????xxxxx",
	match,
	dttr_hook_audio_init_level_audio_callback
)

/// Null driver guard for Audio_StopAllSamples
void __cdecl dttr_hook_audio_stop_all_samples_callback(void);

DTTR_INTEROP_HOOK_FUNC_TRAMPOLINE_SIG(
	dttr_hook_audio_stop_all_samples,
	8,
	"\x56\x57\x8B\x3D????\xBE",
	"xxxx????x",
	match,
	dttr_hook_audio_stop_all_samples_callback
)

/// Installs audio crash prevention hooks
void dttr_audio_init(HMODULE mod);

/// Removes audio hooks and frees trampoline memory
void dttr_audio_cleanup(void);

/// Handles SDL audio device added/removed events for hot-plug
void dttr_audio_handle_device_event(const SDL_Event *event);

#endif /* DTTR_HOOKS_AUDIO_H */
