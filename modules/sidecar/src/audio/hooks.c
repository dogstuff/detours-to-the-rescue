#include "context_private.h"
#include "hooks_private.h"
#include "mss_private.h"
#include "sidecar_private.h"
#include <dttr_log.h>
#include <dttr_pcdogs.h>

#include <SDL3/SDL.h>

static DTTR_PCDOGS_F_Audio_StopAllSounds_proto audio_stop_all_sounds_original;
static DTTR_PCDOGS_F_Audio_InitializeLevelAudio_proto audio_init_level_audio_original;
static DTTR_PCDOGS_F_Audio_StopAllSamples_proto audio_stop_all_samples_original;
static DTTR_Core_PatchGroup *audio_patch_group;
static bool audio_shim_installed;

static int32_t __cdecl audio_stop_all_sounds_detour();
static int32_t __cdecl audio_init_level_audio_detour();
static int32_t __cdecl audio_stop_all_samples_detour();

// Treats either the original MSS driver or SDL shim as an active audio backend.
static bool has_audio_driver() {
	void *audio_driver = NULL;
	if (!REQUIRE_PCDOGS_CALL(
			DTTR_PCDOGS_D_Audio_InitializeSystem_DigitalDriver->Read(&audio_driver)
		)) {
		return dttr_mss_core_has_driver();
	}

	return audio_driver || dttr_mss_core_has_driver();
}

// Stops SDL-backed samples before delegating only while an audio driver is alive.
static int32_t run_guarded_audio_hook(int32_t(__cdecl *original)(), bool stop_all_samples) {
	if (stop_all_samples && audio_shim_installed) {
		dttr_mss_sample_stop_all();
	}

	if (!original || !has_audio_driver()) {
		return 0;
	}

	return original();
}


// Mirrors stop-all-sounds into the SDL sample backend before delegating.
static int32_t __cdecl audio_stop_all_sounds_detour() {
	return run_guarded_audio_hook(audio_stop_all_sounds_original, true);
}

// Skips level-audio initialization only if neither MSS nor the shim is active.
static int32_t __cdecl audio_init_level_audio_detour() {
	return run_guarded_audio_hook(audio_init_level_audio_original, false);
}

// Clears SDL sample playback before the original stop-samples routine runs.
static int32_t __cdecl audio_stop_all_samples_detour() {
	return run_guarded_audio_hook(audio_stop_all_samples_original, true);
}

// Installs the MSS audio patch group after SDL audio startup.
bool dttr_audio_init(const DTTR_Mods_Context *ctx) {
	audio_shim_installed = false;

	if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
		DTTR_LOG_ERROR("SDL_InitSubSystem(SDL_INIT_AUDIO) failed: %s", SDL_GetError());
	}

	const bool imports_ok = dttr_mss_sdl_install_hooks(ctx);
	const DTTR_PCDOGS_T_Patch_Spec audio_patches[] = {
		DTTR_PCDOGS_F_Audio_StopAllSounds->PatchSpec(
			true,
			audio_stop_all_sounds_detour,
			&audio_stop_all_sounds_original
		),
		DTTR_PCDOGS_F_Audio_InitializeLevelAudio->PatchSpec(
			true,
			audio_init_level_audio_detour,
			&audio_init_level_audio_original
		),
		DTTR_PCDOGS_F_Audio_StopAllSamples->PatchSpec(
			true,
			audio_stop_all_samples_detour,
			&audio_stop_all_samples_original
		),
	};

	const bool patches_ok = dttr_sidecar_install_pcdogs_patch_group(
		ctx,
		"sidecar/audio",
		audio_patches,
		DTTR_ARRAY_COUNT(audio_patches),
		&audio_patch_group
	);
	audio_shim_installed = imports_ok && patches_ok;
	return imports_ok && patches_ok;
}

// Releases audio patches before shutting down the SDL replacement backend.
void dttr_audio_cleanup(const DTTR_Mods_Context *) {
	DTTR_Core_PatchGroupRelease(&audio_patch_group);
	audio_stop_all_sounds_original = NULL;
	audio_init_level_audio_original = NULL;
	audio_stop_all_samples_original = NULL;
	dttr_mss_sdl_release_hooks();

	if (audio_shim_installed) {
		dttr_mss_sdl_shutdown();
	}

	audio_shim_installed = false;
}
