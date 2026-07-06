#include "mss_private.h"

#include <dttr_config.h>
#include <dttr_log.h>

#include <SDL3/SDL.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define DTTR_MSS_DIRECTSOUND_SFX_LATENCY_BASE_RATE 22050u
#define DTTR_MSS_DIRECTSOUND_SFX_LATENCY_BASE_FRAMES 1845u
#define DTTR_MSS_SFX_DELAY_MAX_CHANNELS 8u
#define DTTR_MSS_SFX_DELAY_MAX_FRAMES 8192u
#define DTTR_MSS_SFX_DELAY_VALUES                                                        \
	(DTTR_MSS_SFX_DELAY_MAX_FRAMES * DTTR_MSS_SFX_DELAY_MAX_CHANNELS)

typedef struct {
	uint32_t magic;
	float *pcm_frames;
	size_t pcm_frame_count;
	mss_wave_info wave;
	int base_rate;
	int current_rate;
	int volume;
	int pan;
	int loops;
	int status;
	int mixer_loops_remaining;
	uint64_t playback_position_fp;
	bool rate_overridden;
	bool paused_by_rate;
	bool allocated;
} mss_sample;

static const uint32_t SAMPLE_MAGIC = 0x4453414d;

static mss_sample samples[DTTR_MSS_DEFAULT_MIXER_CHANNELS];
static SDL_Mutex *sample_mutex;
static float sfx_delay_ring[DTTR_MSS_SFX_DELAY_VALUES];
static size_t sfx_delay_cursor;
static size_t sfx_delay_frames;
static int sfx_delay_channels;
static int sfx_delay_rate;

static bool ensure_sample_mutex() {
	if (sample_mutex) {
		return true;
	}

	sample_mutex = SDL_CreateMutex();
	if (!sample_mutex) {
		DTTR_LOG_ERROR(
			"%s sample mutex create failed: %s",
			DTTR_MSS_LOG_TAG,
			SDL_GetError()
		);
		return false;
	}

	return true;
}

static void lock_sample_state() {
	if (ensure_sample_mutex()) {
		SDL_LockMutex(sample_mutex);
	}
}

static void unlock_sample_state() {
	if (sample_mutex) {
		SDL_UnlockMutex(sample_mutex);
	}
}

static void reset_sfx_delay_line() {
	memset(sfx_delay_ring, 0, sizeof(sfx_delay_ring));
	sfx_delay_cursor = 0;
	sfx_delay_frames = 0;
	sfx_delay_channels = 0;
	sfx_delay_rate = 0;
}

static float clamp_mix(float value) {
	return SDL_clamp(value, -1.0f, 1.0f);
}

static float directsound_pcm16_f32(float value) {
	const float scaled = clamp_mix(value) * 32768.0f;
	int sample = (int)(scaled + (scaled >= 0.0f ? 0.5f : -0.5f));
	if (sample < -32768) {
		sample = -32768;
	} else if (sample > 32767) {
		sample = 32767;
	}

	return (float)sample / 32768.0f;
}

static size_t directsound_sfx_latency_frames(int output_rate) {
	if (output_rate <= 0) {
		return 0;
	}

	return ((size_t)output_rate * (size_t)DTTR_MSS_DIRECTSOUND_SFX_LATENCY_BASE_FRAMES
			+ (size_t)DTTR_MSS_DIRECTSOUND_SFX_LATENCY_BASE_RATE / 2u)
		   / (size_t)DTTR_MSS_DIRECTSOUND_SFX_LATENCY_BASE_RATE;
}

static float sample_pan_gain(int pan, bool right) {
	const int clamped = SDL_clamp(pan, 0, 127);
	const int side = right ? clamped : 127 - clamped;
	return (float)(side >= 63 ? 128 : side * 2) / 128.0f;
}

static void apply_directsound_sfx_latency(
	const SDL_AudioSpec *spec,
	float *values,
	int value_count
) {
	if (!spec || !values || value_count <= 0 || spec->channels <= 0 || spec->freq <= 0) {
		return;
	}

	const int channels = spec->channels;
	if ((size_t)channels > DTTR_MSS_SFX_DELAY_MAX_CHANNELS) {
		reset_sfx_delay_line();
		return;
	}

	const size_t frames = (size_t)value_count / (size_t)channels;
	const size_t delay_frames = directsound_sfx_latency_frames(spec->freq);
	if (frames == 0 || delay_frames == 0
		|| delay_frames > DTTR_MSS_SFX_DELAY_MAX_FRAMES) {
		reset_sfx_delay_line();
		return;
	}

	if (sfx_delay_frames != delay_frames || sfx_delay_channels != channels
		|| sfx_delay_rate != spec->freq) {
		reset_sfx_delay_line();

		sfx_delay_frames = delay_frames;
		sfx_delay_channels = channels;
		sfx_delay_rate = spec->freq;
	}

	for (size_t frame = 0; frame < frames; frame++) {
		const size_t ring_frame = sfx_delay_cursor * (size_t)channels;
		const size_t bus_frame = frame * (size_t)channels;

		for (int channel = 0; channel < channels; channel++) {
			const size_t ring_index = ring_frame + (size_t)channel;
			const size_t bus_index = bus_frame + (size_t)channel;
			const float delayed = sfx_delay_ring[ring_index];

			sfx_delay_ring[ring_index] = values[bus_index];
			values[bus_index] = delayed;
		}

		sfx_delay_cursor = (sfx_delay_cursor + 1u) % sfx_delay_frames;
	}
}

static int sample_loops_to_remaining(int loops) {
	return loops == 0 ? 0 : (loops > 0 ? loops : 1);
}

static void reset_sample_playback_cursor(mss_sample *sample) {
	sample->playback_position_fp = 0;
	sample->mixer_loops_remaining = sample_loops_to_remaining(sample->loops);
}

static void reset_sample_defaults(mss_sample *sample) {
	sample->base_rate = DTTR_MSS_DEFAULT_RATE;
	sample->current_rate = DTTR_MSS_DEFAULT_RATE;
	dttr_mss_core_ensure_preferences();
	const int default_volume = dttr_mss_core_get_preference(
		DTTR_MSS_PREF_DIG_DEFAULT_VOLUME
	);
	sample->volume = default_volume > 0 ? default_volume : DTTR_MSS_DEFAULT_VOLUME;
	sample->pan = DTTR_MSS_DEFAULT_PAN;
	sample->loops = DTTR_MSS_DEFAULT_LOOP_COUNT;
	sample->status = DTTR_MSS_STATUS_DONE;
	reset_sample_playback_cursor(sample);
	sample->rate_overridden = false;
	sample->paused_by_rate = false;
}

static bool is_sample(const void *ptr) {
	const mss_sample *sample = ptr;
	return sample && sample >= samples
		   && sample < samples + DTTR_MSS_DEFAULT_MIXER_CHANNELS
		   && sample->magic == SAMPLE_MAGIC && sample->allocated;
}

static int sample_slot(const mss_sample *sample) {
	return sample ? (int)(sample - samples) : -1;
}

static mss_sample *require_sample(void *sample_ptr) {
	return is_sample(sample_ptr) ? sample_ptr : NULL;
}

static void clear_sample_wave(mss_sample *sample) {
	dttr_mss_wave_free(sample->pcm_frames);
	sample->pcm_frames = NULL;
	sample->pcm_frame_count = 0;
	memset(&sample->wave, 0, sizeof(sample->wave));
}

static void free_sample_audio(mss_sample *sample) {
	clear_sample_wave(sample);
	sample->paused_by_rate = false;
}

static void reset_sample_slot(mss_sample *sample) {
	memset(sample, 0, sizeof(*sample));
	sample->magic = SAMPLE_MAGIC;
	reset_sample_defaults(sample);
	sample->allocated = true;
}

static bool decode_sample_file(
	const void *file_image,
	size_t size,
	mss_wave_info *wave,
	float **frames_out,
	size_t *frame_count_out
) {
	if (!wave || !frames_out || !frame_count_out) {
		return false;
	}

	float *frames = NULL;
	if (!dttr_mss_wave_decode_f32(file_image, size, wave, &frames)) {
		return false;
	}

	if (wave->frame_count > (uint64_t)SIZE_MAX) {
		dttr_mss_wave_free(frames);
		return false;
	}

	*frames_out = frames;
	*frame_count_out = (size_t)wave->frame_count;
	return true;
}

static void commit_empty_sample_file_state(mss_sample *sample) {
	free_sample_audio(sample);

	sample->status = DTTR_MSS_STATUS_DONE;
	sample->paused_by_rate = false;

	reset_sample_playback_cursor(sample);
}

static void clear_sample_file_state_if_alive(void *sample_ptr) {
	lock_sample_state();
	mss_sample *sample = require_sample(sample_ptr);
	if (sample) {
		commit_empty_sample_file_state(sample);
	}
	unlock_sample_state();
}

static void commit_sample_file_state(
	mss_sample *sample,
	float *frames,
	size_t frame_count,
	const mss_wave_info *wave
) {
	const int requested_rate = sample->current_rate;
	const bool rate_overridden = sample->rate_overridden;

	commit_empty_sample_file_state(sample);
	sample->pcm_frames = frames;
	sample->pcm_frame_count = frame_count;
	sample->wave = *wave;
	sample->base_rate = dttr_mss_wave_rate(wave);
	sample->current_rate = rate_overridden ? requested_rate : sample->base_rate;
	sample->rate_overridden = rate_overridden;
	sample->status = DTTR_MSS_STATUS_DONE;
	reset_sample_playback_cursor(sample);
}

static void stop_sample(mss_sample *sample) {
	sample->status = DTTR_MSS_STATUS_STOPPED;
	sample->paused_by_rate = false;
}

static float sample_frame_channel(const mss_sample *sample, size_t frame, int channel) {
	const int channels = (int)sample->wave.channels;
	return sample->pcm_frames[frame * (size_t)channels + (size_t)channel];
}

static float sample_interp_channel(
	const mss_sample *sample,
	size_t frame0,
	size_t frame1,
	float fraction,
	int channel
) {
	const float a = sample_frame_channel(sample, frame0, channel);
	const float b = sample_frame_channel(sample, frame1, channel);
	return a + (b - a) * fraction;
}

static bool advance_sample_position(
	mss_sample *sample,
	uint64_t *pos_fp,
	uint64_t step_fp
) {
	*pos_fp += step_fp;
	const uint64_t end_fp = (uint64_t)sample->pcm_frame_count << 16;
	if (*pos_fp < end_fp) {
		return true;
	}

	uint64_t wrapped = *pos_fp / end_fp;
	if (sample->mixer_loops_remaining == 0) {
		*pos_fp -= wrapped * end_fp;
		if (*pos_fp >= end_fp) {
			*pos_fp = 0;
		}

		return true;
	}

	if (wrapped >= (uint64_t)sample->mixer_loops_remaining) {
		sample->status = DTTR_MSS_STATUS_DONE;
		sample->paused_by_rate = false;
		*pos_fp = end_fp;
		return false;
	}

	sample->mixer_loops_remaining -= (int)wrapped;
	*pos_fp -= wrapped * end_fp;
	if (*pos_fp >= end_fp) {
		*pos_fp = 0;
	}

	return true;
}

void dttr_mss_sample_mix_into(
	const SDL_AudioSpec *spec,
	float *pcm,
	int values,
	float *sfx_bus
) {
	if (!spec || !pcm || values <= 0 || spec->channels <= 0 || spec->freq <= 0) {
		return;
	}

	const int out_channels = spec->channels;
	const int frames = values / out_channels;
	if (frames <= 0) {
		return;
	}
	if (sfx_bus) {
		memset(sfx_bus, 0, (size_t)values * sizeof(*sfx_bus));
	}

	lock_sample_state();

	const float master_gain = dttr_mss_core_master_gain();
	const float sample_headroom = dttr_mss_core_sample_headroom_gain();
	for (int slot = 0; slot < DTTR_MSS_DEFAULT_MIXER_CHANNELS; slot++) {
		mss_sample *sample = &samples[slot];
		if (!sample->allocated || sample->status != DTTR_MSS_STATUS_PLAYING
			|| sample->paused_by_rate || !sample->pcm_frames
			|| sample->pcm_frame_count == 0 || sample->wave.channels == 0
			|| sample->current_rate <= 0) {
			continue;
		}

		const uint64_t step_fp = ((uint64_t)sample->current_rate << 16)
								 / (uint64_t)spec->freq;
		if (step_fp == 0) {
			continue;
		}

		uint64_t pos_fp = sample->playback_position_fp;

		const float left_pan = sample_pan_gain(sample->pan, false);
		const float right_pan = sample_pan_gain(sample->pan, true);
		const float gain = dttr_mss_track_gain(
			sample->volume,
			master_gain,
			sample_headroom
		);
		float *const out = sfx_bus ? sfx_bus : pcm;

		for (int out_frame = 0; out_frame < frames; out_frame++) {
			if (sample->status != DTTR_MSS_STATUS_PLAYING) {
				break;
			}

			size_t frame0 = (size_t)(pos_fp >> 16);
			if (frame0 >= sample->pcm_frame_count) {
				frame0 = sample->pcm_frame_count - 1u;
			}

			const size_t frame1 = frame0 + 1u < sample->pcm_frame_count ? frame0 + 1u
																		: frame0;
			const uint32_t frac_fp = (uint32_t)(pos_fp & 0xffffu);
			const float fraction = (float)frac_fp / 65536.0f;
			float left = sample_interp_channel(sample, frame0, frame1, fraction, 0);
			float right = sample->wave.channels == 1
							  ? left
							  : sample_interp_channel(sample, frame0, frame1, fraction, 1);

			left *= left_pan * gain;
			right *= right_pan * gain;

			const size_t out_index = (size_t)out_frame * (size_t)out_channels;
			if (out_channels == 1) {
				out[out_index] += (left + right) * 0.5f;
			} else {
				out[out_index] += left;
				out[out_index + 1u] += right;
				const float mono = (left + right) * 0.5f;
				for (int channel = 2; channel < out_channels; channel++) {
					out[out_index + (size_t)channel] += mono;
				}
			}

			advance_sample_position(sample, &pos_fp, step_fp);
		}

		sample->playback_position_fp = pos_fp;
	}

	unlock_sample_state();

	if (sfx_bus) {
		for (int i = 0; i < values; i++) {
			sfx_bus[i] = directsound_pcm16_f32(sfx_bus[i]);
		}
		if (dttr_config.mss_simulate_directsound_delay) {
			apply_directsound_sfx_latency(spec, sfx_bus, values);
		}
		for (int i = 0; i < values; i++) {
			pcm[i] = clamp_mix(pcm[i] + sfx_bus[i]);
		}
		return;
	}

	for (int i = 0; i < values; i++) {
		pcm[i] = clamp_mix(pcm[i]);
	}
}

void dttr_mss_sample_shutdown_all() {
	lock_sample_state();
	for (int i = 0; i < DTTR_MSS_DEFAULT_MIXER_CHANNELS; i++) {
		free_sample_audio(&samples[i]);
		memset(&samples[i], 0, sizeof(samples[i]));
	}
	reset_sfx_delay_line();
	unlock_sample_state();
}

void dttr_mss_sample_destroy_sync() {
	if (!sample_mutex) {
		return;
	}

	SDL_DestroyMutex(sample_mutex);
	sample_mutex = NULL;
}

void dttr_mss_sample_stop_all() {
	lock_sample_state();
	for (int i = 0; i < DTTR_MSS_DEFAULT_MIXER_CHANNELS; i++) {
		if (samples[i].allocated) {
			stop_sample(&samples[i]);
		}
	}
	unlock_sample_state();
}

void dttr_mss_sample_set_master_gain(float gain) {
	lock_sample_state();
	dttr_mss_core_set_master_gain(gain);
	unlock_sample_state();
}

void *__stdcall dttr_mss_ail_allocate_sample_handle(void *driver) {
	DTTR_LOG_TRACE("MSS AIL_allocate_sample_handle(driver=%p)", driver);

	if (!dttr_mss_core_ensure_mixer()) {
		DTTR_LOG_TRACE("MSS AIL_allocate_sample_handle -> NULL (mixer unavailable)");
		return NULL;
	}

	lock_sample_state();
	for (int i = 0; i < DTTR_MSS_DEFAULT_MIXER_CHANNELS; i++) {
		if (samples[i].allocated) {
			continue;
		}

		mss_sample *sample = &samples[i];
		reset_sample_slot(sample);
		DTTR_LOG_TRACE("MSS AIL_allocate_sample_handle -> sample[%d]=%p", i, sample);
		unlock_sample_state();
		return sample;
	}
	unlock_sample_state();

	DTTR_LOG_TRACE("MSS AIL_allocate_sample_handle -> NULL (pool exhausted)");
	return NULL;
}

void __stdcall dttr_mss_ail_release_sample_handle(void *sample_ptr) {
	lock_sample_state();
	mss_sample *sample = require_sample(sample_ptr);
	if (!sample) {
		unlock_sample_state();
		return;
	}

	DTTR_LOG_TRACE(
		"MSS AIL_release_sample_handle(sample[%d]=%p)",
		sample_slot(sample),
		sample
	);
	free_sample_audio(sample);
	memset(sample, 0, sizeof(*sample));
	unlock_sample_state();
}

void __stdcall dttr_mss_ail_init_sample(void *sample_ptr) {
	lock_sample_state();
	mss_sample *sample = require_sample(sample_ptr);
	if (!sample) {
		unlock_sample_state();
		return;
	}

	DTTR_LOG_TRACE("MSS AIL_init_sample(sample[%d]=%p)", sample_slot(sample), sample);
	free_sample_audio(sample);
	reset_sample_defaults(sample);
	unlock_sample_state();
}

int __stdcall dttr_mss_ail_set_sample_file(
	void *sample_ptr,
	const void *file_image,
	int block
) {
	if (!file_image || !dttr_mss_core_ensure_mixer()) {
		DTTR_LOG_TRACE(
			"MSS AIL_set_sample_file(sample=%p, file=%p, block=%d) -> 0 (invalid)",
			sample_ptr,
			file_image,
			block
		);
		return 0;
	}

	lock_sample_state();
	mss_sample *sample = require_sample(sample_ptr);
	const int slot = sample_slot(sample);
	unlock_sample_state();
	if (!sample) {
		DTTR_LOG_TRACE(
			"MSS AIL_set_sample_file(sample=%p, file=%p, block=%d) -> 0 (invalid)",
			sample_ptr,
			file_image,
			block
		);
		return 0;
	}

	DTTR_LOG_TRACE(
		"MSS AIL_set_sample_file(sample[%d]=%p, file=%p, block=%d)",
		slot,
		sample_ptr,
		file_image,
		block
	);

	const size_t size = dttr_mss_wave_riff_size(file_image);
	if (block > 0 && size > (size_t)block) {
		DTTR_LOG_ERROR("AIL_set_sample_file received truncated WAVE data");
		clear_sample_file_state_if_alive(sample_ptr);
		return 0;
	}

	mss_wave_info wave = {0};
	if (!size || !dttr_mss_wave_parse(file_image, &wave)) {
		DTTR_LOG_ERROR("AIL_set_sample_file received non-WAVE data");
		clear_sample_file_state_if_alive(sample_ptr);
		return 0;
	}

	DTTR_LOG_TRACE(
		"MSS AIL_set_sample_file sample[%d] RIFF size=%zu format=%u channels=%u "
		"rate=%u bits=%u data=%zu",
		slot,
		size,
		wave.format_tag,
		wave.channels,
		wave.sample_rate,
		wave.bits_per_sample,
		wave.data_size
	);

	mss_wave_info decoded_wave = wave;
	float *decoded_frames = NULL;
	size_t decoded_frame_count = 0;
	if (!decode_sample_file(
			file_image,
			size,
			&decoded_wave,
			&decoded_frames,
			&decoded_frame_count
		)) {
		DTTR_LOG_ERROR("%s sample[%d] decode failed", DTTR_MSS_LOG_TAG, slot);
		clear_sample_file_state_if_alive(sample_ptr);
		return 0;
	}

	lock_sample_state();
	sample = require_sample(sample_ptr);
	if (!sample) {
		unlock_sample_state();
		dttr_mss_wave_free(decoded_frames);
		return 0;
	}

	commit_sample_file_state(sample, decoded_frames, decoded_frame_count, &decoded_wave);
	DTTR_LOG_TRACE(
		"MSS AIL_set_sample_file sample[%d] -> 1 pcm_frames=%zu base_rate=%d "
		"current_rate=%d",
		sample_slot(sample),
		sample->pcm_frame_count,
		sample->base_rate,
		sample->current_rate
	);
	unlock_sample_state();
	return 1;
}

void __stdcall dttr_mss_ail_start_sample(void *sample_ptr) {
	lock_sample_state();
	mss_sample *sample = require_sample(sample_ptr);
	if (!sample) {
		unlock_sample_state();
		return;
	}

	DTTR_LOG_TRACE(
		"MSS AIL_start_sample(sample[%d]=%p status=%d loops=%d volume=%d pan=%d rate=%d)",
		sample_slot(sample),
		sample,
		sample->status,
		sample->loops,
		sample->volume,
		sample->pan,
		sample->current_rate
	);

	if (!sample->pcm_frames || sample->pcm_frame_count == 0
		|| sample->wave.channels == 0) {
		DTTR_LOG_TRACE(
			"MSS AIL_start_sample sample[%d] skipped missing decoded PCM",
			sample_slot(sample)
		);
		unlock_sample_state();
		return;
	}

	reset_sample_playback_cursor(sample);
	sample->paused_by_rate = sample->current_rate <= 0;
	sample->status = DTTR_MSS_STATUS_PLAYING;
	unlock_sample_state();
}

void __stdcall dttr_mss_ail_stop_sample(void *sample_ptr) {
	lock_sample_state();
	mss_sample *sample = require_sample(sample_ptr);
	if (!sample) {
		unlock_sample_state();
		return;
	}

	DTTR_LOG_TRACE(
		"MSS AIL_stop_sample(sample[%d]=%p status=%d)",
		sample_slot(sample),
		sample,
		sample->status
	);
	stop_sample(sample);
	unlock_sample_state();
}

void __stdcall dttr_mss_ail_end_sample(void *sample_ptr) {
	dttr_mss_ail_stop_sample(sample_ptr);
}

int __stdcall dttr_mss_ail_sample_status(void *sample_ptr) {
	dttr_mss_core_pump_silent_mixer();

	lock_sample_state();

	mss_sample *sample = require_sample(sample_ptr);
	if (!sample) {
		unlock_sample_state();
		return DTTR_MSS_STATUS_DONE;
	}

	const int status = sample->status;

	unlock_sample_state();
	return status;
}

void __stdcall dttr_mss_ail_set_sample_loop_count(void *sample_ptr, int loops) {
	lock_sample_state();
	mss_sample *sample = require_sample(sample_ptr);
	if (!sample) {
		unlock_sample_state();
		return;
	}

	sample->loops = loops;
	if (sample->status == DTTR_MSS_STATUS_PLAYING) {
		sample->mixer_loops_remaining = sample_loops_to_remaining(loops);
	}
	unlock_sample_state();
}

void __stdcall dttr_mss_ail_set_sample_volume(void *sample_ptr, int volume) {
	lock_sample_state();
	mss_sample *sample = require_sample(sample_ptr);
	if (sample) {
		sample->volume = volume;
	}
	unlock_sample_state();
}

void __stdcall dttr_mss_ail_set_sample_pan(void *sample_ptr, int pan) {
	lock_sample_state();
	mss_sample *sample = require_sample(sample_ptr);
	if (sample) {
		sample->pan = pan;
	}
	unlock_sample_state();
}

void __stdcall dttr_mss_ail_set_sample_playback_rate(void *sample_ptr, int rate) {
	lock_sample_state();
	mss_sample *sample = require_sample(sample_ptr);
	if (!sample) {
		unlock_sample_state();
		return;
	}

	sample->current_rate = rate;
	sample->rate_overridden = true;
	sample->paused_by_rate = rate <= 0 && sample->status == DTTR_MSS_STATUS_PLAYING;
	unlock_sample_state();
}

int __stdcall dttr_mss_ail_sample_playback_rate(void *sample_ptr) {
	lock_sample_state();
	mss_sample *sample = require_sample(sample_ptr);
	if (!sample) {
		unlock_sample_state();
		return DTTR_MSS_DEFAULT_RATE;
	}

	const int rate = sample->current_rate > 0 ? sample->current_rate
											  : DTTR_MSS_DEFAULT_RATE;
	unlock_sample_state();
	return rate;
}
