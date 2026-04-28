#include "dttr_hooks_audio.h"
#include "dttr_interop_pcdogs.h"
#include "log.h"

#include <SDL3/SDL.h>

typedef void(__cdecl *DTTR_AudioTrampoline)(void);

static bool s_has_playback_devices(void) {
	int count = 0;
	SDL_AudioDeviceID *devices = SDL_GetAudioPlaybackDevices(&count);
	SDL_free(devices);
	return count > 0;
}

static bool s_has_audio_driver(void) {
	return g_pcdogs_audio_digital_driver_get() != NULL;
}

static void s_run_audio_trampoline(void *trampoline) {
	((DTTR_AudioTrampoline)trampoline)();
}

void __cdecl dttr_hook_audio_init_system_callback(void) {
	if (!s_has_playback_devices()) {
		log_warn("No audio playback devices found, skipping audio init");
		return;
	}

	s_run_audio_trampoline(dttr_hook_audio_init_system_trampoline);
}

void __cdecl dttr_hook_audio_stop_all_sounds_callback(void) {
	if (!s_has_audio_driver()) {
		return;
	}

	s_run_audio_trampoline(dttr_hook_audio_stop_all_sounds_trampoline);
}

void __cdecl dttr_hook_audio_init_level_audio_callback(void) {
	if (!s_has_audio_driver()) {
		return;
	}

	s_run_audio_trampoline(dttr_hook_audio_init_level_audio_trampoline);
}

void __cdecl dttr_hook_audio_stop_all_samples_callback(void) {
	if (!s_has_audio_driver()) {
		return;
	}

	s_run_audio_trampoline(dttr_hook_audio_stop_all_samples_trampoline);
}

void dttr_audio_handle_device_event(const SDL_Event *event) {
	if (event->type == SDL_EVENT_AUDIO_DEVICE_REMOVED) {
		if (!s_has_audio_driver()) {
			return;
		}

		if (s_has_playback_devices()) {
			return;
		}

		log_warn("Audio device removed, shutting down audio subsystem");
		pcdogs_audio_shutdown_system();
		return;
	}

	if (event->type != SDL_EVENT_AUDIO_DEVICE_ADDED || s_has_audio_driver()) {
		return;
	}

	log_info("Audio device connected, reinitializing audio");
	dttr_hook_audio_init_system_callback();

	if (s_has_audio_driver()) {
		log_info("Audio reinitialized successfully");
	} else {
		log_warn("Audio reinitialization failed");
	}
}

void dttr_audio_init(const DTTR_ComponentContext *ctx) {
	if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
		log_error("SDL_InitSubSystem(SDL_INIT_AUDIO) failed: %s", SDL_GetError());
	}

	DTTR_INSTALL_TRAMPOLINE_AUTO(
		dttr_hook_audio_init_system,
		ctx,
		"\x81\xEC\x90???\x55\x56\x57\xFF\x15",
		"xxx???xxxxx"
	);

	DTTR_INSTALL_TRAMPOLINE_AUTO(
		dttr_hook_audio_stop_all_sounds,
		ctx,
		"\xA1????\x6A?\x50\xFF\x15",
		"x????x?xxx"
	);

	DTTR_INSTALL_TRAMPOLINE_AUTO(
		dttr_hook_audio_init_level_audio,
		ctx,
		"\xA1????\x6A\x7F\x50\xFF\x15",
		"x????xxxxx"
	);

	DTTR_INSTALL_TRAMPOLINE_AUTO(
		dttr_hook_audio_stop_all_samples,
		ctx,
		"\x56\x57\x8B\x3D????\xBE",
		"xxxx????x"
	);
}

void dttr_audio_cleanup(const DTTR_ComponentContext *ctx) {
	DTTR_TRAMPOLINE_UNINSTALL(dttr_hook_audio_stop_all_samples, ctx);
	DTTR_TRAMPOLINE_UNINSTALL(dttr_hook_audio_init_level_audio, ctx);
	DTTR_TRAMPOLINE_UNINSTALL(dttr_hook_audio_stop_all_sounds, ctx);
	DTTR_TRAMPOLINE_UNINSTALL(dttr_hook_audio_init_system, ctx);
}
