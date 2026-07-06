#include "mss_private.h"

#include <dttr_log.h>

#include <dttr_config.h>

#include <string.h>

static MIX_Mixer *mixer;
static bool mix_initialized;
static bool silent_mixer;
static SDL_AudioSpec active_spec;
static uint64_t silent_mixer_last_ticks;
static uint64_t silent_mixer_frame_remainder;
static int driver_open_count;
static float master_gain = DTTR_MSS_DEFAULT_MASTER_GAIN;
static int preferences[DTTR_MSS_PREFERENCES_CAPACITY];
static SDL_AudioSpec desired_spec;
static bool has_desired_spec;
static bool preferences_initialized;

#define DTTR_MSS_POSTMIX_SFX_BUS_VALUES 65536u
#define DTTR_MSS_SILENT_PUMP_MAX_FRAMES 4096u
#define DTTR_MSS_SILENT_PUMP_VALUES                                                      \
	(DTTR_MSS_SILENT_PUMP_MAX_FRAMES * DTTR_MSS_MIXER_CHANNELS)

static float postmix_sfx_bus[DTTR_MSS_POSTMIX_SFX_BUS_VALUES];

static int *preference_slot(unsigned int preference) {
	if (preference >= SDL_arraysize(preferences)) {
		return NULL;
	}

	return &preferences[preference];
}

static void clear_mixer_state() {
	mixer = NULL;
	silent_mixer = false;

	memset(&active_spec, 0, sizeof(active_spec));

	silent_mixer_last_ticks = 0;
	silent_mixer_frame_remainder = 0;
}

static void clear_desired_spec() {
	memset(&desired_spec, 0, sizeof(desired_spec));
	has_desired_spec = false;
}

static void SDLCALL mixer_postmix_callback(
	void *userdata,
	MIX_Mixer *callback_mixer,
	const SDL_AudioSpec *spec,
	float *pcm,
	int values
) {
	if (values <= 0) {
		return;
	}

	const int channels = spec && spec->channels > 0 ? spec->channels : 0;
	if (channels <= 0) {
		return;
	}

	int chunk_values = (int)(DTTR_MSS_POSTMIX_SFX_BUS_VALUES
							 - (DTTR_MSS_POSTMIX_SFX_BUS_VALUES % (size_t)channels));
	if (chunk_values <= 0) {
		return;
	}

	const int complete_values = values - (values % channels);
	for (int offset = 0; offset < complete_values; offset += chunk_values) {
		int count = complete_values - offset;
		if (count > chunk_values) {
			count = chunk_values;
		}

		dttr_mss_sample_mix_into(spec, pcm + offset, count, postmix_sfx_bus);
	}
}

bool dttr_mss_core_has_driver() {
	return driver_open_count > 0 && mixer;
}

void dttr_mss_core_pump_silent_mixer() {
	if (!mixer || !silent_mixer || active_spec.channels <= 0 || active_spec.freq <= 0) {
		return;
	}

	const uint64_t now = SDL_GetTicks();
	if (now <= silent_mixer_last_ticks) {
		return;
	}

	const uint64_t elapsed_ms = now - silent_mixer_last_ticks;
	silent_mixer_last_ticks = now;

	const uint64_t frame_units = elapsed_ms * (uint64_t)active_spec.freq
								 + silent_mixer_frame_remainder;
	uint64_t frames = frame_units / 1000u;
	silent_mixer_frame_remainder = frame_units % 1000u;

	float buffer[DTTR_MSS_SILENT_PUMP_VALUES];
	const uint64_t max_frames = SDL_arraysize(buffer) / (uint64_t)active_spec.channels;

	if (max_frames == 0) {
		return;
	}

	while (frames > 0) {
		const uint64_t chunk_frames = frames > max_frames ? max_frames : frames;

		const int bytes = (int)(chunk_frames * (uint64_t)active_spec.channels
								* (uint64_t)sizeof(buffer[0]));

		if (MIX_Generate(mixer, buffer, bytes) < 0) {
			DTTR_LOG_ERROR(
				"%s silent MIX_Generate failed: %s",
				DTTR_MSS_LOG_TAG,
				SDL_GetError()
			);
			return;
		}

		frames -= chunk_frames;
	}
}

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
	return SDL_clamp(dttr_config.mss_sample_gain, 0.0f, 2.0f);
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
		DTTR_LOG_WARN(
			"%s MIX_CreateMixerDevice failed; using silent memory mixer: %s",
			DTTR_MSS_LOG_TAG,
			SDL_GetError()
		);
		mixer = MIX_CreateMixer(desired);
		if (!mixer) {
			DTTR_LOG_ERROR("MIX_CreateMixer failed: %s", SDL_GetError());
			return false;
		}

		silent_mixer = true;
		silent_mixer_last_ticks = SDL_GetTicks();
		silent_mixer_frame_remainder = 0;
	}

	if (!MIX_SetPostMixCallback(mixer, mixer_postmix_callback, NULL)) {
		DTTR_LOG_ERROR(
			"%s MIX_SetPostMixCallback failed: %s",
			DTTR_MSS_LOG_TAG,
			SDL_GetError()
		);

		MIX_DestroyMixer(mixer);
		clear_mixer_state();

		return false;
	}

	SDL_AudioSpec actual = {0};
	if (!MIX_GetMixerFormat(mixer, &actual)) {
		DTTR_LOG_WARN(
			"%s MIX_GetMixerFormat failed; using desired format=0x%04X channels=%d "
			"freq=%d error=%s",
			DTTR_MSS_LOG_TAG,
			(unsigned)desired->format,
			desired->channels,
			desired->freq,
			SDL_GetError()
		);

		active_spec = *desired;

		return true;
	}

	active_spec = actual;
	DTTR_LOG_INFO(
		"%s mixer opened%s: desired=0x%04X/%d/%d actual=0x%04X/%d/%d "
		"sample_gain=%.3f",
		DTTR_MSS_LOG_TAG,
		silent_mixer ? " (silent)" : "",
		(unsigned)desired->format,
		desired->channels,
		desired->freq,
		(unsigned)actual.format,
		actual.channels,
		actual.freq,
		dttr_config.mss_sample_gain
	);
	return true;
}

void dttr_mss_core_destroy_mixer() {
	if (mixer) {
		MIX_SetPostMixCallback(mixer, NULL, NULL);
		MIX_DestroyMixer(mixer);
	}
	clear_mixer_state();

	if (mix_initialized) {
		MIX_Quit();
		mix_initialized = false;
	}

	clear_desired_spec();
}

MIX_Mixer *dttr_mss_core_mixer() {
	return mixer;
}

void dttr_mss_core_set_desired_spec(const SDL_AudioSpec *spec) {
	if (!spec) {
		clear_desired_spec();
		return;
	}

	desired_spec = *spec;
	has_desired_spec = true;
}

int dttr_mss_core_driver_open_count() {
	return driver_open_count;
}

void dttr_mss_core_increment_driver_open_count() {
	driver_open_count++;
}

void dttr_mss_core_decrement_driver_open_count() {
	if (driver_open_count <= 0) {
		return;
	}

	driver_open_count--;
}

void dttr_mss_core_reset_driver_open_count() {
	driver_open_count = 0;
}

float dttr_mss_core_master_gain() {
	return master_gain;
}

void dttr_mss_core_set_master_gain(float gain) {
	master_gain = gain;
}
