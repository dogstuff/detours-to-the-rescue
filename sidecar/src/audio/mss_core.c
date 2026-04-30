#include "mss_internal.h"

#include <dttr_log.h>

#include <dttr_config.h>

#include <string.h>

static MIX_Mixer *s_mixer;
static bool s_mix_initialized;
static int s_driver_open_count;
static float s_master_gain = DTTR_MSS_DEFAULT_MASTER_GAIN;
static int s_preferences[DTTR_MSS_PREFERENCES_CAPACITY];
static SDL_AudioSpec s_desired_spec;
static SDL_AudioSpec s_mixer_spec;
static bool s_has_desired_spec;
static bool s_preferences_initialized;

static int *s_preference_slot(unsigned int preference) {
	if (preference >= SDL_arraysize(s_preferences)) {
		return NULL;
	}

	return &s_preferences[preference];
}

static void s_clear_desired_spec(void) {
	memset(&s_desired_spec, 0, sizeof(s_desired_spec));
	s_has_desired_spec = false;
}

static float s_clamp_float(float value, float min_value, float max_value) {
	if (value < min_value) {
		return min_value;
	}

	if (value > max_value) {
		return max_value;
	}

	return value;
}

bool dttr_mss_core_original_mode_enabled(void) {
	return !g_dttr_config.m_mss_sdl_enabled;
}

bool dttr_mss_core_has_driver(void) { return s_driver_open_count > 0 && s_mixer; }

void dttr_mss_core_reset_preferences(void) {
	dttr_mss_reset_preferences(s_preferences, SDL_arraysize(s_preferences));
	s_preferences_initialized = true;
}

void dttr_mss_core_ensure_preferences(void) {
	if (s_preferences_initialized) {
		return;
	}

	dttr_mss_core_reset_preferences();
}

int dttr_mss_core_get_preference(unsigned int preference) {
	dttr_mss_core_ensure_preferences();
	const int *slot = s_preference_slot(preference);
	if (!slot) {
		return 0;
	}

	return *slot;
}

int dttr_mss_core_set_preference(unsigned int preference, int value) {
	dttr_mss_core_ensure_preferences();
	int *slot = s_preference_slot(preference);
	if (!slot) {
		return 0;
	}

	int previous = *slot;
	*slot = value;
	return previous;
}

float dttr_mss_core_sample_headroom_gain(void) {
	return s_clamp_float(g_dttr_config.m_mss_sample_gain, 0.0f, 2.0f);
}

float dttr_mss_core_sample_preemphasis(void) {
	return s_clamp_float(g_dttr_config.m_mss_sample_preemphasis, -1.0f, 2.0f);
}

bool dttr_mss_core_ensure_mix_initialized(void) {
	if (s_mix_initialized) {
		return true;
	}

	if (!MIX_Init()) {
		DTTR_LOG_ERROR("MIX_Init failed: %s", SDL_GetError());
		return false;
	}

	s_mix_initialized = true;
	return true;
}

bool dttr_mss_core_ensure_mixer(void) {
	if (s_mixer) {
		return true;
	}

	if (!dttr_mss_core_ensure_mix_initialized()) {
		return false;
	}

	const SDL_AudioSpec fallback = {
		.format = DTTR_MSS_MIXER_FORMAT,
		.channels = DTTR_MSS_MIXER_CHANNELS,
		.freq = DTTR_MSS_MIXER_RATE,
	};
	const SDL_AudioSpec *desired = s_has_desired_spec ? &s_desired_spec : &fallback;
	s_mixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, desired);
	if (!s_mixer) {
		DTTR_LOG_ERROR("MIX_CreateMixerDevice failed: %s", SDL_GetError());
		return false;
	}

	SDL_AudioSpec actual = {0};
	if (!MIX_GetMixerFormat(s_mixer, &actual)) {
		s_mixer_spec = *desired;
		return true;
	}

	s_mixer_spec = actual;
	DTTR_LOG_INFO(
		"MSS mixer opened: format=0x%04X channels=%d freq=%d",
		(unsigned)actual.format,
		actual.channels,
		actual.freq
	);
	return true;
}

void dttr_mss_core_destroy_mixer(void) {
	if (s_mixer) {
		MIX_DestroyMixer(s_mixer);
		s_mixer = NULL;
	}

	if (s_mix_initialized) {
		MIX_Quit();
		s_mix_initialized = false;
	}

	memset(&s_mixer_spec, 0, sizeof(s_mixer_spec));
	s_clear_desired_spec();
}

MIX_Mixer *dttr_mss_core_mixer(void) { return s_mixer; }

SDL_AudioSpec dttr_mss_core_mixer_spec(void) { return s_mixer_spec; }

void dttr_mss_core_set_desired_spec(const SDL_AudioSpec *spec) {
	if (!spec) {
		s_clear_desired_spec();
		return;
	}

	s_desired_spec = *spec;
	s_has_desired_spec = true;
}

int dttr_mss_core_driver_open_count(void) { return s_driver_open_count; }

void dttr_mss_core_increment_driver_open_count(void) { s_driver_open_count++; }

void dttr_mss_core_decrement_driver_open_count(void) {
	if (s_driver_open_count <= 0) {
		return;
	}

	s_driver_open_count--;
}

void dttr_mss_core_reset_driver_open_count(void) { s_driver_open_count = 0; }

float dttr_mss_core_master_gain(void) { return s_master_gain; }

void dttr_mss_core_set_master_gain(float gain) { s_master_gain = gain; }
