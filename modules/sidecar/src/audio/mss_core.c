#include "mss_private.h"

#include <dttr_log.h>

#include <dttr_config.h>

#include <string.h>

static MIX_Mixer *mixer;
static bool mix_initialized;
static int driver_open_count;
static float master_gain = DTTR_MSS_DEFAULT_MASTER_GAIN;
static int preferences[DTTR_MSS_PREFERENCES_CAPACITY];
static SDL_AudioSpec desired_spec;
static SDL_AudioSpec mixer_spec;
static bool has_desired_spec;
static bool preferences_initialized;

static int *preference_slot(unsigned int preference) {
	if (preference >= SDL_arraysize(preferences)) {
		return NULL;
	}

	return &preferences[preference];
}

static void clear_desired_spec() {
	memset(&desired_spec, 0, sizeof(desired_spec));
	has_desired_spec = false;
}

static float clamp_float(float value, float min_value, float max_value) {
	if (value < min_value) {
		return min_value;
	}

	if (value > max_value) {
		return max_value;
	}

	return value;
}

bool dttr_mss_core_has_driver() { return driver_open_count > 0 && mixer; }

void dttr_mss_core_reset_preferences() {
	dttr_mss_reset_preferences(preferences, SDL_arraysize(preferences));
	preferences_initialized = true;
}

void dttr_mss_core_ensure_preferences() {
	if (preferences_initialized) {
		return;
	}

	dttr_mss_core_reset_preferences();
}

int dttr_mss_core_get_preference(unsigned int preference) {
	dttr_mss_core_ensure_preferences();
	const int *slot = preference_slot(preference);
	if (!slot) {
		return 0;
	}

	return *slot;
}

int dttr_mss_core_set_preference(unsigned int preference, int value) {
	dttr_mss_core_ensure_preferences();
	int *slot = preference_slot(preference);
	if (!slot) {
		return 0;
	}

	int previous = *slot;
	*slot = value;
	return previous;
}

float dttr_mss_core_sample_headroom_gain() {
	return clamp_float(dttr_config.mss_sample_gain, 0.0f, 2.0f);
}

float dttr_mss_core_sample_preemphasis() {
	return clamp_float(dttr_config.mss_sample_preemphasis, -1.0f, 2.0f);
}

bool dttr_mss_core_ensure_mix_initialized() {
	if (mix_initialized) {
		return true;
	}

	if (!MIX_Init()) {
		DTTR_LOG_ERROR("MIX_Init failed: %s", SDL_GetError());
		return false;
	}

	mix_initialized = true;
	return true;
}

bool dttr_mss_core_ensure_mixer() {
	if (mixer) {
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

	const SDL_AudioSpec *desired = has_desired_spec ? &desired_spec : &fallback;
	mixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, desired);
	if (!mixer) {
		DTTR_LOG_ERROR("MIX_CreateMixerDevice failed: %s", SDL_GetError());
		return false;
	}

	SDL_AudioSpec actual = {0};
	if (!MIX_GetMixerFormat(mixer, &actual)) {
		mixer_spec = *desired;
		return true;
	}

	mixer_spec = actual;
	DTTR_LOG_INFO(
		"MSS mixer opened: format=0x%04X channels=%d freq=%d",
		(unsigned)actual.format,
		actual.channels,
		actual.freq
	);
	return true;
}

void dttr_mss_core_destroy_mixer() {
	if (mixer) {
		MIX_DestroyMixer(mixer);
		mixer = NULL;
	}

	if (mix_initialized) {
		MIX_Quit();
		mix_initialized = false;
	}

	memset(&mixer_spec, 0, sizeof(mixer_spec));
	clear_desired_spec();
}

MIX_Mixer *dttr_mss_core_mixer() { return mixer; }

SDL_AudioSpec dttr_mss_core_mixer_spec() { return mixer_spec; }

void dttr_mss_core_set_desired_spec(const SDL_AudioSpec *spec) {
	if (!spec) {
		clear_desired_spec();
		return;
	}

	desired_spec = *spec;
	has_desired_spec = true;
}

int dttr_mss_core_driver_open_count() { return driver_open_count; }

void dttr_mss_core_increment_driver_open_count() { driver_open_count++; }

void dttr_mss_core_decrement_driver_open_count() {
	if (driver_open_count <= 0) {
		return;
	}

	driver_open_count--;
}

void dttr_mss_core_reset_driver_open_count() { driver_open_count = 0; }

float dttr_mss_core_master_gain() { return master_gain; }

void dttr_mss_core_set_master_gain(float gain) { master_gain = gain; }
