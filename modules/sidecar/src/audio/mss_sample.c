#include "mss_private.h"

#include <dttr_log.h>

#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>

#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define AIL_STATUS_DONE DTTR_MSS_STATUS_DONE
#define AIL_STATUS_PLAYING DTTR_MSS_STATUS_PLAYING
#define AIL_STATUS_STOPPED DTTR_MSS_STATUS_STOPPED
#define MAX_SAMPLES DTTR_MSS_DEFAULT_MIXER_CHANNELS
#define PREF_DIG_DEFAULT_VOLUME DTTR_MSS_PREF_DIG_DEFAULT_VOLUME

typedef DTTR_MssWaveInfo wave_info;

typedef struct {
	uint32_t magic;
	MIX_Audio *audio;
	MIX_Track *track;
	float *pcm_frames;
	size_t pcm_frame_count;
	wave_info wave;
	int base_rate;
	int current_rate;
	int rendered_rate;
	int volume;
	int pan;
	int loops;
	int status;
	bool rate_overridden;
	bool paused_by_rate;
	bool allocated;
} mss_sample;

static const uint32_t SAMPLE_MAGIC = 0x4453414d;

static mss_sample samples[MAX_SAMPLES];

static void apply_rate(mss_sample *sample) {
	if (!sample || !sample->track) {
		return;
	}

	const int reference_rate = sample->rendered_rate > 0 ? sample->rendered_rate
														 : sample->base_rate;
	const float ratio = dttr_mss_track_frequency_ratio(
		sample->current_rate,
		reference_rate
	);
	if (ratio == 0.0f) {
		return;
	}

	if (!MIX_SetTrackFrequencyRatio(sample->track, ratio)) {
		DTTR_LOG_ERROR("MIX_SetTrackFrequencyRatio failed: %s", SDL_GetError());
	}
}

static void reset_sample_defaults(mss_sample *sample) {
	if (!sample) {
		return;
	}

	sample->base_rate = DTTR_MSS_DEFAULT_RATE;
	sample->current_rate = DTTR_MSS_DEFAULT_RATE;
	dttr_mss_core_ensure_preferences();
	const int default_volume = dttr_mss_core_get_preference(PREF_DIG_DEFAULT_VOLUME);
	sample->volume = default_volume > 0 ? default_volume : DTTR_MSS_DEFAULT_VOLUME;
	sample->pan = DTTR_MSS_DEFAULT_PAN;
	sample->loops = DTTR_MSS_DEFAULT_LOOP_COUNT;
	sample->status = AIL_STATUS_DONE;
	sample->rate_overridden = false;
	sample->paused_by_rate = false;
}

static bool is_sample(const void *ptr) {
	const mss_sample *sample = ptr;
	return sample && sample >= samples && sample < samples + MAX_SAMPLES
		   && sample->magic == SAMPLE_MAGIC && sample->allocated;
}

static int sample_slot(const mss_sample *sample) {
	return sample ? (int)(sample - samples) : -1;
}

static mss_sample *require_sample(void *sample_ptr) {
	if (is_sample(sample_ptr)) {
		return sample_ptr;
	}

	return NULL;
}

static void apply_sample_gain(mss_sample *sample) {
	if (!sample || !sample->track) {
		return;
	}

	MIX_SetTrackGain(
		sample->track,
		dttr_mss_track_gain(
			sample->volume,
			dttr_mss_core_master_gain(),
			dttr_mss_core_sample_headroom_gain()
		)
	);
}

static void destroy_sample_audio_object(mss_sample *sample) {
	if (!sample) {
		return;
	}

	if (sample->track) {
		MIX_SetTrackAudio(sample->track, NULL);
	}

	if (sample->audio) {
		MIX_DestroyAudio(sample->audio);
		sample->audio = NULL;
	}

	sample->rendered_rate = 0;
	sample->paused_by_rate = false;
}

static void clear_sample_wave(mss_sample *sample) {
	if (!sample) {
		return;
	}

	dttr_mss_wave_free(sample->pcm_frames);
	sample->pcm_frames = NULL;
	sample->pcm_frame_count = 0;
	memset(&sample->wave, 0, sizeof(sample->wave));
}

static void free_sample_audio(mss_sample *sample) {
	if (!sample) {
		return;
	}

	destroy_sample_audio_object(sample);
	if (sample->track) {
		MIX_DestroyTrack(sample->track);
		sample->track = NULL;
	}

	clear_sample_wave(sample);
}

static void reset_sample_slot(mss_sample *sample) {
	if (!sample) {
		return;
	}

	memset(sample, 0, sizeof(*sample));

	sample->magic = SAMPLE_MAGIC;

	reset_sample_defaults(sample);

	sample->allocated = true;
}

static bool load_sample_frames(
	mss_sample *sample,
	const void *file_image,
	size_t size,
	const wave_info *wave
) {
	wave_info decoded = *wave;
	float *frames = NULL;
	if (!dttr_mss_wave_decode_f32(file_image, size, &decoded, &frames)) {
		return false;
	}

	if (decoded.frame_count > SIZE_MAX) {
		dttr_mss_wave_free(frames);
		return false;
	}

	sample->pcm_frames = frames;
	sample->pcm_frame_count = (size_t)decoded.frame_count;
	sample->wave = decoded;
	return true;
}

static bool load_sample_audio_from_memory(
	mss_sample *sample,
	const void *file_image,
	size_t size
) {
	SDL_IOStream *io = SDL_IOFromConstMem(file_image, size);
	if (!io) {
		DTTR_LOG_ERROR("SDL_IOFromConstMem failed: %s", SDL_GetError());
		return false;
	}

	sample->audio = MIX_LoadAudio_IO(dttr_mss_core_mixer(), io, true, true);
	if (!sample->audio) {
		DTTR_LOG_ERROR("MIX_LoadAudio_IO sample failed: %s", SDL_GetError());
		return false;
	}

	sample->rendered_rate = sample->base_rate;
	return true;
}

static void apply_sample_track(mss_sample *sample) {
	if (!sample || !sample->track || !sample->audio) {
		return;
	}

	MIX_SetTrackAudio(sample->track, sample->audio);
	apply_sample_gain(sample);
	dttr_mss_track_apply_pan(sample->track, sample->pan);
	apply_rate(sample);
}

static void stop_sample(mss_sample *sample) {
	if (!sample) {
		return;
	}

	if (sample->track) {
		MIX_StopTrack(sample->track, 0);
	}

	sample->status = AIL_STATUS_STOPPED;
	sample->paused_by_rate = false;
}

static bool render_sample_audio(mss_sample *sample) {
	if (!sample || !dttr_mss_core_mixer() || !sample->pcm_frames
		|| sample->pcm_frame_count == 0 || sample->current_rate <= 0
		|| sample->wave.channels == 0) {
		return false;
	}

	const SDL_AudioSpec mixer_spec = dttr_mss_core_mixer_spec();
	const int out_rate = mixer_spec.freq > 0 ? mixer_spec.freq : DTTR_MSS_MIXER_RATE;
	const int channels = (int)sample->wave.channels;
	const size_t in_frames_size = sample->pcm_frame_count;

	if (in_frames_size > INT_MAX) {
		return false;
	}

	const int in_frames = (int)in_frames_size;

	int64_t out_frames64 = ((int64_t)in_frames * out_rate + sample->current_rate - 1)
						   / sample->current_rate;
	if (out_frames64 <= 0 || out_frames64 > INT_MAX) {
		return false;
	}

	const int out_frames = (int)out_frames64;
	float previous_values[8] = {0};

	if (channels > (int)SDL_arraysize(previous_values)) {
		return false;
	}

	if ((size_t)out_frames > SIZE_MAX / (size_t)channels) {
		return false;
	}

	const size_t out_values = (size_t)out_frames * (size_t)channels;
	if (out_values > SIZE_MAX / sizeof(float)) {
		return false;
	}

	float *converted = calloc(out_values, sizeof(float));
	if (!converted) {
		return false;
	}

	const float preemphasis = dttr_mss_core_sample_preemphasis();

	uint64_t source_pos = 0;
	const uint64_t source_step = ((uint64_t)sample->current_rate << 32) / out_rate;

	for (int frame = 0; frame < out_frames; frame++) {
		size_t source_frame = (size_t)(source_pos >> 32);

		if (source_frame >= in_frames_size) {
			source_frame = in_frames_size - 1;
		}

		for (int channel = 0; channel < channels; channel++) {
			const size_t source_index = source_frame * (size_t)channels + (size_t)channel;
			const float value = sample->pcm_frames[source_index];

			converted[(size_t)frame * (size_t)channels + (size_t)channel]
				= value + preemphasis * (value - previous_values[channel]);
			previous_values[channel] = value;
		}

		source_pos += source_step;
	}

	const SDL_AudioSpec spec = {
		.format = SDL_AUDIO_F32,
		.channels = channels,
		.freq = out_rate,
	};

	MIX_Audio *audio = MIX_LoadRawAudio(
		dttr_mss_core_mixer(),
		converted,
		out_values * sizeof(float),
		&spec
	);
	free(converted);
	if (!audio) {
		DTTR_LOG_ERROR("MSS sample render load failed: %s", SDL_GetError());
		return false;
	}

	destroy_sample_audio_object(sample);

	sample->audio = audio;
	sample->rendered_rate = sample->current_rate;

	if (!sample->track) {
		return true;
	}

	MIX_SetTrackAudio(sample->track, sample->audio);
	return true;
}

void dttr_mss_sample_shutdown_all() {
	for (int i = 0; i < MAX_SAMPLES; i++) {
		free_sample_audio(&samples[i]);
		memset(&samples[i], 0, sizeof(samples[i]));
	}
}

void dttr_mss_sample_stop_all() {
	for (int i = 0; i < MAX_SAMPLES; i++) {
		if (!samples[i].allocated) {
			continue;
		}

		stop_sample(&samples[i]);
	}
}

void dttr_mss_sample_apply_master_gain() {
	for (int i = 0; i < MAX_SAMPLES; i++) {
		if (!samples[i].allocated) {
			continue;
		}

		apply_sample_gain(&samples[i]);
	}
}

void dttr_mss_sdl_stop_all_samples() { dttr_mss_sample_stop_all(); }

void *__stdcall dttr_mss_ail_allocate_sample_handle(void *driver) {
	DTTR_LOG_TRACE("MSS AIL_allocate_sample_handle(driver=%p)", driver);

	if (!dttr_mss_core_ensure_mixer()) {
		DTTR_LOG_TRACE("MSS AIL_allocate_sample_handle -> NULL (mixer unavailable)");
		return NULL;
	}

	for (int i = 0; i < MAX_SAMPLES; i++) {
		if (samples[i].allocated) {
			continue;
		}

		mss_sample *sample = &samples[i];
		reset_sample_slot(sample);
		DTTR_LOG_TRACE("MSS AIL_allocate_sample_handle -> sample[%d]=%p", i, sample);
		return sample;
	}

	DTTR_LOG_TRACE("MSS AIL_allocate_sample_handle -> NULL (pool exhausted)");

	return NULL;
}

void __stdcall dttr_mss_ail_release_sample_handle(void *sample_ptr) {
	mss_sample *sample = require_sample(sample_ptr);
	if (!sample) {
		return;
	}

	DTTR_LOG_TRACE(
		"MSS AIL_release_sample_handle(sample[%d]=%p)",
		sample_slot(sample),
		sample
	);

	free_sample_audio(sample);
	memset(sample, 0, sizeof(*sample));
}

void __stdcall dttr_mss_ail_init_sample(void *sample_ptr) {
	mss_sample *sample = require_sample(sample_ptr);
	if (!sample) {
		return;
	}

	DTTR_LOG_TRACE("MSS AIL_init_sample(sample[%d]=%p)", sample_slot(sample), sample);

	free_sample_audio(sample);
	reset_sample_defaults(sample);
}

int __stdcall dttr_mss_ail_set_sample_file(
	void *sample_ptr,
	const void *file_image,
	int block
) {
	if (!is_sample(sample_ptr) || !file_image || !dttr_mss_core_ensure_mixer()) {
		DTTR_LOG_TRACE(
			"MSS AIL_set_sample_file(sample=%p, file=%p, block=%d) -> 0 (invalid)",
			sample_ptr,
			file_image,
			block
		);
		return 0;
	}

	mss_sample *sample = sample_ptr;

	DTTR_LOG_TRACE(
		"MSS AIL_set_sample_file(sample[%d]=%p, file=%p, block=%d)",
		sample_slot(sample),
		sample,
		file_image,
		block
	);

	free_sample_audio(sample);

	const size_t size = dttr_mss_wave_riff_size(file_image);
	if (block > 0 && size > (size_t)block) {
		DTTR_LOG_ERROR("AIL_set_sample_file received truncated WAVE data");
		return 0;
	}

	wave_info wave = {0};
	if (!size || !dttr_mss_wave_parse(file_image, &wave)) {
		DTTR_LOG_ERROR("AIL_set_sample_file received non-WAVE data");
		return 0;
	}

	DTTR_LOG_TRACE(
		"MSS AIL_set_sample_file sample[%d] RIFF size=%zu format=%u channels=%u rate=%u "
		"bits=%u data=%zu",
		sample_slot(sample),
		size,
		wave.format_tag,
		wave.channels,
		wave.sample_rate,
		wave.bits_per_sample,
		wave.data_size
	);

	sample->track = MIX_CreateTrack(dttr_mss_core_mixer());
	if (!sample->track) {
		DTTR_LOG_ERROR("MIX_CreateTrack sample failed: %s", SDL_GetError());
		free_sample_audio(sample);
		return 0;
	}

	const int requested_rate = sample->current_rate;
	sample->base_rate = dttr_mss_wave_rate(&wave);
	sample->current_rate = sample->rate_overridden && requested_rate > 0
							   ? requested_rate
							   : sample->base_rate;

	const bool loaded = load_sample_frames(sample, file_image, size, &wave)
						|| load_sample_audio_from_memory(sample, file_image, size);
	if (!loaded) {
		free_sample_audio(sample);
		return 0;
	}

	if (!sample->audio && !sample->pcm_frames) {
		DTTR_LOG_ERROR("MSS sample load failed: %s", SDL_GetError());
		free_sample_audio(sample);
		return 0;
	}

	apply_sample_track(sample);
	DTTR_LOG_TRACE(
		"MSS AIL_set_sample_file sample[%d] -> 1 track=%p audio=%p pcm_frames=%zu "
		"base_rate=%d current_rate=%d",
		sample_slot(sample),
		sample->track,
		sample->audio,
		sample->pcm_frame_count,
		sample->base_rate,
		sample->current_rate
	);
	return 1;
}

void __stdcall dttr_mss_ail_start_sample(void *sample_ptr) {
	mss_sample *sample = require_sample(sample_ptr);
	if (!sample) {
		return;
	}

	DTTR_LOG_TRACE(
		"MSS AIL_start_sample(sample[%d]=%p status=%d loops=%d volume=%d pan=%d rate=%d "
		"track=%p audio=%p)",
		sample_slot(sample),
		sample,
		sample->status,
		sample->loops,
		sample->volume,
		sample->pan,
		sample->current_rate,
		sample->track,
		sample->audio
	);

	if (sample->pcm_frames && sample->track && !sample->audio
		&& !render_sample_audio(sample)) {
		DTTR_LOG_ERROR("MSS sample render failed before start");
		return;
	}

	if (!sample->track || !sample->audio) {
		DTTR_LOG_TRACE(
			"MSS AIL_start_sample sample[%d] skipped missing track/audio",
			sample_slot(sample)
		);
		return;
	}

	apply_sample_track(sample);
	sample->paused_by_rate = false;
	sample->status = AIL_STATUS_PLAYING;
	const int sdl_loops = dttr_mss_loops_to_sdl(sample->loops);
	dttr_mss_track_play(sample->track, sdl_loops);
	DTTR_LOG_TRACE(
		"MSS AIL_start_sample sample[%d] played sdl_loops=%d",
		sample_slot(sample),
		sdl_loops
	);
}

void __stdcall dttr_mss_ail_stop_sample(void *sample_ptr) {
	mss_sample *sample = require_sample(sample_ptr);
	if (!sample) {
		return;
	}

	DTTR_LOG_TRACE(
		"MSS AIL_stop_sample(sample[%d]=%p status=%d track=%p)",
		sample_slot(sample),
		sample,
		sample->status,
		sample->track
	);
	stop_sample(sample);
}

void __stdcall dttr_mss_ail_end_sample(void *sample_ptr) {
	dttr_mss_ail_stop_sample(sample_ptr);
}

int __stdcall dttr_mss_ail_sample_status(void *sample_ptr) {
	if (!is_sample(sample_ptr)) {
		return AIL_STATUS_DONE;
	}

	mss_sample *sample = sample_ptr;
	sample->status = dttr_mss_track_status(sample->track, sample->status);
	return sample->status;
}

void __stdcall dttr_mss_ail_set_sample_loop_count(void *sample_ptr, int loops) {
	mss_sample *sample = require_sample(sample_ptr);
	if (!sample) {
		return;
	}

	sample->loops = loops;
	if (sample->track) {
		MIX_SetTrackLoops(sample->track, dttr_mss_loops_to_sdl(loops));
	}
}

void __stdcall dttr_mss_ail_set_sample_volume(void *sample_ptr, int volume) {
	mss_sample *sample = require_sample(sample_ptr);
	if (!sample) {
		return;
	}

	sample->volume = volume;
	apply_sample_gain(sample);
}

void __stdcall dttr_mss_ail_set_sample_pan(void *sample_ptr, int pan) {
	mss_sample *sample = require_sample(sample_ptr);
	if (!sample) {
		return;
	}

	sample->pan = pan;
	dttr_mss_track_apply_pan(sample->track, pan);
}

void __stdcall dttr_mss_ail_set_sample_playback_rate(void *sample_ptr, int rate) {
	mss_sample *sample = require_sample(sample_ptr);
	if (!sample) {
		return;
	}

	const int previous = sample->current_rate;
	const bool pause_for_rate = dttr_mss_sample_rate_pauses_playback(rate);

	if (pause_for_rate) {
		if (sample->track) {
			MIX_PauseTrack(sample->track);
		}

		sample->paused_by_rate = true;
		sample->status = AIL_STATUS_PLAYING;
		DTTR_LOG_TRACE(
			"MSS AIL_set_sample_playback_rate(sample[%d]=%p, rate=%d previous=%d) "
			"paused sample",
			sample_slot(sample),
			sample,
			rate,
			previous
		);
		return;
	}

	sample->current_rate = rate;
	sample->rate_overridden = true;
	if (sample->pcm_frames && sample->track && !MIX_TrackPlaying(sample->track)
		&& sample->rendered_rate != sample->current_rate) {
		render_sample_audio(sample);
	}

	apply_rate(sample);
	if (sample->paused_by_rate && sample->track) {
		MIX_ResumeTrack(sample->track);
	}

	sample->paused_by_rate = false;
	DTTR_LOG_TRACE(
		"MSS AIL_set_sample_playback_rate(sample[%d]=%p, rate=%d previous=%d current=%d "
		"base=%d resumed=%d)",
		sample_slot(sample),
		sample,
		rate,
		previous,
		sample->current_rate,
		sample->base_rate,
		sample->track ? !MIX_TrackPaused(sample->track) : 0
	);
}

int __stdcall dttr_mss_ail_sample_playback_rate(void *sample_ptr) {
	if (!is_sample(sample_ptr)) {
		return DTTR_MSS_DEFAULT_RATE;
	}

	mss_sample *sample = sample_ptr;
	return sample->current_rate > 0 ? sample->current_rate : DTTR_MSS_DEFAULT_RATE;
}
