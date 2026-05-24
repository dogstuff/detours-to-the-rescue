#include "hooks_private.h"
#include "mss_private.h"
#include "sidecar_private.h"
#include <dttr_log.h>
#include <dttr_pcdogs.h>

#include <SDL3/SDL.h>

static DTTR_PCDOGS_F_AudioInitializeSystem_proto audio_init_system_original;
static DTTR_PCDOGS_F_AudioStopAllSounds_proto audio_stop_all_sounds_original;
static DTTR_PCDOGS_F_AudioInitializeLevelAudio_proto audio_init_level_audio_original;
static DTTR_PCDOGS_F_AudioStopAllSamples_proto audio_stop_all_samples_original;

static int32_t audio_init_system();
static int32_t __cdecl audio_init_system_detour();
static int32_t __cdecl audio_stop_all_sounds_detour();
static int32_t __cdecl audio_init_level_audio_detour();
static int32_t __cdecl audio_stop_all_samples_detour();

// Detects whether SDL can see a playback endpoint before MSS initializes.
static bool has_playback_devices() {
	int count = 0;
	SDL_AudioDeviceID *devices = SDL_GetAudioPlaybackDevices(&count);
	SDL_free(devices);
	return count > 0;
}

// Treats either the original MSS driver or SDL shim as an active audio backend.
static bool has_audio_driver() {
	void *audio_driver = NULL;
	DTTR_PCDOGS_D_AudioDigitalDriver->Read(&audio_driver);
	return audio_driver || dttr_mss_sdl_has_driver();
}

// Stops SDL-backed samples before delegating only while an audio driver is alive.
static int32_t run_guarded_audio_hook(int32_t(__cdecl *original)(), bool stop_all_samples) {
	if (stop_all_samples) {
		dttr_mss_sdl_stop_all_samples();
	}

	if (!original || !has_audio_driver()) {
		return 0;
	}

	return original();
}

// Shuts down game audio after the last playback device disappears.
static void handle_audio_device_removed() {
	if (!has_audio_driver() || has_playback_devices()) {
		return;
	}

	DTTR_LOG_ERROR("Audio device removed, shutting down audio subsystem");
	DTTR_PCDOGS_F_AudioShutdownSystem->Call(dttr_sidecar_runtime_context(), 0);
}

// Reinitializes game audio when a playback device returns.
static void handle_audio_device_added() {
	if (has_audio_driver()) {
		return;
	}

	DTTR_LOG_INFO("Audio device connected, reinitializing audio");
	audio_init_system();
	if (has_audio_driver()) {
		DTTR_LOG_INFO("Audio reinitialized successfully");
		return;
	}

	DTTR_LOG_ERROR("Audio reinitialization failed");
}

// Starts the MSS audio system only when SDL still has a playback device available.
static int32_t audio_init_system() {
	if (!has_playback_devices()) {
		DTTR_LOG_ERROR("No audio playback devices found, skipping audio init");
		return 0;
	}

	return audio_init_system_original ? audio_init_system_original() : 0;
}

// Funnels game audio initialization through the SDL device guard.
static int32_t __cdecl audio_init_system_detour() { return audio_init_system(); }

// Mirrors stop-all-sounds into the SDL sample backend before delegating.
static int32_t __cdecl audio_stop_all_sounds_detour() {
	return run_guarded_audio_hook(audio_stop_all_sounds_original, true);
}

// Skips level-audio initialization after the audio backend is gone.
static int32_t __cdecl audio_init_level_audio_detour() {
	return run_guarded_audio_hook(audio_init_level_audio_original, false);
}

// Clears SDL sample playback before the original stop-samples routine runs.
static int32_t __cdecl audio_stop_all_samples_detour() {
	return run_guarded_audio_hook(audio_stop_all_samples_original, true);
}

// Reports failed audio hook installation without adding success noise to normal startup.
static bool log_hook_result(const DTTR_Mods_Context *ctx, const char *name, bool ok) {
	if (!ok) {
		DTTR_MODS_LOG_ERROR(ctx, "%s: hook failed", name);
		return false;
	}

	return true;
}

// Keeps the game MSS driver matched to SDL audio device availability.
void dttr_audio_handle_device_event(const SDL_Event *event) {
	switch (event->type) {
	case SDL_EVENT_AUDIO_DEVICE_REMOVED:
		handle_audio_device_removed();
		return;
	case SDL_EVENT_AUDIO_DEVICE_ADDED:
		handle_audio_device_added();
		return;
	default:
		return;
	}
}

// Installs MSS audio detours after SDL audio startup.
bool dttr_audio_init(const DTTR_Mods_Context *ctx) {
	if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
		DTTR_LOG_ERROR("SDL_InitSubSystem(SDL_INIT_AUDIO) failed: %s", SDL_GetError());
	}

	bool ok = dttr_mss_sdl_install_hooks(ctx);

	ok = log_hook_result(
			 ctx,
			 "audio_initializesystem",
			 DTTR_PCDOGS_F_AudioInitializeSystem->Hook(
				 &ctx->runtime,
				 audio_init_system_detour,
				 &audio_init_system_original
			 )
		 )
		 && ok;
	ok = log_hook_result(
			 ctx,
			 "audio_stopallsounds",
			 DTTR_PCDOGS_F_AudioStopAllSounds->Hook(
				 &ctx->runtime,
				 audio_stop_all_sounds_detour,
				 &audio_stop_all_sounds_original
			 )
		 )
		 && ok;
	ok = log_hook_result(
			 ctx,
			 "audio_initializelevelaudio",
			 DTTR_PCDOGS_F_AudioInitializeLevelAudio->Hook(
				 &ctx->runtime,
				 audio_init_level_audio_detour,
				 &audio_init_level_audio_original
			 )
		 )
		 && ok;
	ok = log_hook_result(
			 ctx,
			 "audio_stopallsamples",
			 DTTR_PCDOGS_F_AudioStopAllSamples->Hook(
				 &ctx->runtime,
				 audio_stop_all_samples_detour,
				 &audio_stop_all_samples_original
			 )
		 )
		 && ok;
	return ok;
}

// Removes audio detours before shutting down the SDL replacement backend.
void dttr_audio_cleanup(const DTTR_Mods_Context *ctx) {
	DTTR_PCDOGS_F_AudioStopAllSamples->Unhook(&ctx->runtime);
	DTTR_PCDOGS_F_AudioInitializeLevelAudio->Unhook(&ctx->runtime);
	DTTR_PCDOGS_F_AudioStopAllSounds->Unhook(&ctx->runtime);
	DTTR_PCDOGS_F_AudioInitializeSystem->Unhook(&ctx->runtime);
	audio_init_system_original = NULL;
	audio_stop_all_sounds_original = NULL;
	audio_init_level_audio_original = NULL;
	audio_stop_all_samples_original = NULL;
	dttr_mss_sdl_release_hooks();
	dttr_mss_sdl_shutdown();
}
