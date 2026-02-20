#include "dttr_hooks_audio.h"
#include "dttr_interop_pcdogs.h"
#include "log.h"

#include <SDL3/SDL.h>

static bool s_has_playback_devices(void) {
	int count = 0;
	SDL_AudioDeviceID *devices = SDL_GetAudioPlaybackDevices(&count);
	SDL_free(devices);
	return count > 0;
}

void __cdecl dttr_hook_audio_init_system_callback(void) {
	if (!s_has_playback_devices()) {
		log_warn("No audio playback devices found, skipping audio init");
		return;
	}

	((void(__cdecl *)(void))dttr_hook_audio_init_system_trampoline)();
}

void __cdecl dttr_hook_audio_stop_all_sounds_callback(void) {
	if (g_pcdogs_audio_digital_driver_get() == NULL) {
		return;
	}

	((void(__cdecl *)(void))dttr_hook_audio_stop_all_sounds_trampoline)();
}

void __cdecl dttr_hook_audio_init_level_audio_callback(void) {
	if (g_pcdogs_audio_digital_driver_get() == NULL) {
		return;
	}

	((void(__cdecl *)(void))dttr_hook_audio_init_level_audio_trampoline)();
}

void __cdecl dttr_hook_audio_stop_all_samples_callback(void) {
	if (g_pcdogs_audio_digital_driver_get() == NULL) {
		return;
	}

	((void(__cdecl *)(void))dttr_hook_audio_stop_all_samples_trampoline)();
}

void dttr_audio_handle_device_event(const SDL_Event *event) {
	if (event->type == SDL_EVENT_AUDIO_DEVICE_REMOVED) {
		if (g_pcdogs_audio_digital_driver_get() == NULL) {
			return;
		}

		if (s_has_playback_devices()) {
			return;
		}

		log_warn("Audio device removed, shutting down audio subsystem");
		pcdogs_audio_shutdown_system();
	}

	if (event->type == SDL_EVENT_AUDIO_DEVICE_ADDED) {
		if (g_pcdogs_audio_digital_driver_get() != NULL) {
			return;
		}

		log_info("Audio device connected, reinitializing audio");
		dttr_hook_audio_init_system_callback();

		if (g_pcdogs_audio_digital_driver_get() != NULL) {
			log_info("Audio reinitialized successfully");
		} else {
			log_warn("Audio reinitialization failed");
		}
	}
}

void dttr_audio_init(const DTTR_ComponentContext *ctx) {
	if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
		log_error("SDL_InitSubSystem(SDL_INIT_AUDIO) failed: %s", SDL_GetError());
	}

	pcdogs_audio_shutdown_system_init(ctx);

	DTTR_INTEROP_HOOK_FUNC_TRAMPOLINE_LOG(dttr_hook_audio_init_system, ctx);
	DTTR_INTEROP_HOOK_FUNC_TRAMPOLINE_LOG(dttr_hook_audio_stop_all_sounds, ctx);
	DTTR_INTEROP_HOOK_FUNC_TRAMPOLINE_LOG(dttr_hook_audio_init_level_audio, ctx);
	DTTR_INTEROP_HOOK_FUNC_TRAMPOLINE_LOG(dttr_hook_audio_stop_all_samples, ctx);
}

void dttr_audio_cleanup(const DTTR_ComponentContext *ctx) {
	DTTR_INTEROP_UNHOOK_LOG(dttr_hook_audio_stop_all_samples, ctx);
	DTTR_INTEROP_UNHOOK_LOG(dttr_hook_audio_init_level_audio, ctx);
	DTTR_INTEROP_UNHOOK_LOG(dttr_hook_audio_stop_all_sounds, ctx);
	DTTR_INTEROP_UNHOOK_LOG(dttr_hook_audio_init_system, ctx);
}
