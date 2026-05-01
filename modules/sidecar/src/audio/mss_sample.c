#include "mss_private.h"

#include <dttr_log.h>

#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>

#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define S_AIL_STATUS_DONE DTTR_MSS_STATUS_DONE
#define S_AIL_STATUS_PLAYING DTTR_MSS_STATUS_PLAYING
#define S_AIL_STATUS_STOPPED DTTR_MSS_STATUS_STOPPED
#define S_MAX_SAMPLES DTTR_MSS_DEFAULT_MIXER_CHANNELS
#define S_PREF_DIG_DEFAULT_VOLUME DTTR_MSS_PREF_DIG_DEFAULT_VOLUME

typedef DTTR_MssWaveInfo S_WaveInfo;

typedef struct {
	uint32_t magic;
	MIX_Audio *audio;
	MIX_Track *track;
	float *pcm_frames;
	size_t pcm_frame_count;
	S_WaveInfo wave;
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
} S_MssSample;

static const uint32_t S_SAMPLE_MAGIC = 0x4453414d;

static S_MssSample s_samples[S_MAX_SAMPLES];

static void s_apply_rate(S_MssSample *sample) {
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

static void s_reset_sample_defaults(S_MssSample *sample) {
	if (!sample) {
		return;
	}

	sample->base_rate = DTTR_MSS_DEFAULT_RATE;
	sample->current_rate = DTTR_MSS_DEFAULT_RATE;
	dttr_mss_core_ensure_preferences();
	const int default_volume = dttr_mss_core_get_preference(S_PREF_DIG_DEFAULT_VOLUME);
	sample->volume = default_volume > 0 ? default_volume : DTTR_MSS_DEFAULT_VOLUME;
	sample->pan = DTTR_MSS_DEFAULT_PAN;
	sample->loops = DTTR_MSS_DEFAULT_LOOP_COUNT;
	sample->status = S_AIL_STATUS_DONE;
	sample->rate_overridden = false;
	sample->paused_by_rate = false;
}

static bool s_is_sample(const void *ptr) {
	const S_MssSample *sample = ptr;
	return sample && sample >= s_samples && sample < s_samples + S_MAX_SAMPLES
		   && sample->magic == S_SAMPLE_MAGIC && sample->allocated;
}

static int s_sample_slot(const S_MssSample *sample) {
	return sample ? (int)(sample - s_samples) : -1;
}

static S_MssSample *s_require_sample(void *sample_ptr) {
	if (s_is_sample(sample_ptr)) {
		return sample_ptr;
	}

	return NULL;
}

static void s_apply_sample_gain(S_MssSample *sample) {
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

static void s_destroy_sample_audio_object(S_MssSample *sample) {
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

static void s_clear_sample_wave(S_MssSample *sample) {
	if (!sample) {
		return;
	}

	dttr_mss_wave_free(sample->pcm_frames);
	sample->pcm_frames = NULL;
	sample->pcm_frame_count = 0;
	memset(&sample->wave, 0, sizeof(sample->wave));
}

static void s_free_sample_audio(S_MssSample *sample) {
	if (!sample) {
		return;
	}

	s_destroy_sample_audio_object(sample);
	if (sample->track) {
		MIX_DestroyTrack(sample->track);
		sample->track = NULL;
	}

	s_clear_sample_wave(sample);
}

static void s_reset_sample_slot(S_MssSample *sample) {
	if (!sample) {
		return;
	}

	memset(sample, 0, sizeof(*sample));

	sample->magic = S_SAMPLE_MAGIC;

	s_reset_sample_defaults(sample);

	sample->allocated = true;
}

static bool s_load_sample_frames(
	S_MssSample *sample,
	const void *file_image,
	size_t size,
	const S_WaveInfo *wave
) {
	S_WaveInfo decoded = *wave;
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

static bool s_load_sample_audio_from_memory(
	S_MssSample *sample,
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

static void s_apply_sample_track(S_MssSample *sample) {
	if (!sample || !sample->track || !sample->audio) {
		return;
	}

	MIX_SetTrackAudio(sample->track, sample->audio);
	s_apply_sample_gain(sample);
	dttr_mss_track_apply_pan(sample->track, sample->pan);
	s_apply_rate(sample);
}

static void s_stop_sample(S_MssSample *sample) {
	if (!sample) {
		return;
	}

	if (sample->track) {
		MIX_StopTrack(sample->track, 0);
	}

	sample->status = S_AIL_STATUS_STOPPED;
	sample->paused_by_rate = false;
}

static bool s_render_sample_audio(S_MssSample *sample) {
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

	s_destroy_sample_audio_object(sample);

	sample->audio = audio;
	sample->rendered_rate = sample->current_rate;

	if (!sample->track) {
		return true;
	}

	MIX_SetTrackAudio(sample->track, sample->audio);
	return true;
}

void dttr_mss_sample_shutdown_all(void) {
	for (int i = 0; i < S_MAX_SAMPLES; i++) {
		s_free_sample_audio(&s_samples[i]);
		memset(&s_samples[i], 0, sizeof(s_samples[i]));
	}
}

void dttr_mss_sample_stop_all(void) {
	for (int i = 0; i < S_MAX_SAMPLES; i++) {
		if (!s_samples[i].allocated) {
			continue;
		}

		s_stop_sample(&s_samples[i]);
	}
}

void dttr_mss_sample_apply_master_gain(void) {
	for (int i = 0; i < S_MAX_SAMPLES; i++) {
		if (!s_samples[i].allocated) {
			continue;
		}

		s_apply_sample_gain(&s_samples[i]);
	}
}

void dttr_mss_sdl_stop_all_samples(void) { dttr_mss_sample_stop_all(); }

void *__stdcall dttr_mss_ail_allocate_sample_handle(void *driver) {
	DTTR_LOG_TRACE("MSS AIL_allocate_sample_handle(driver=%p)", driver);

	if (!dttr_mss_core_ensure_mixer()) {
		DTTR_LOG_TRACE("MSS AIL_allocate_sample_handle -> NULL (mixer unavailable)");
		return NULL;
	}

	for (int i = 0; i < S_MAX_SAMPLES; i++) {
		if (s_samples[i].allocated) {
			continue;
		}

		S_MssSample *sample = &s_samples[i];
		s_reset_sample_slot(sample);
		DTTR_LOG_TRACE("MSS AIL_allocate_sample_handle -> sample[%d]=%p", i, sample);
		return sample;
	}

	DTTR_LOG_TRACE("MSS AIL_allocate_sample_handle -> NULL (pool exhausted)");

	return NULL;
}

void __stdcall dttr_mss_ail_release_sample_handle(void *sample_ptr) {
	S_MssSample *sample = s_require_sample(sample_ptr);
	if (!sample) {
		return;
	}

	DTTR_LOG_TRACE(
		"MSS AIL_release_sample_handle(sample[%d]=%p)",
		s_sample_slot(sample),
		sample
	);

	s_free_sample_audio(sample);
	memset(sample, 0, sizeof(*sample));
}

void __stdcall dttr_mss_ail_init_sample(void *sample_ptr) {
	S_MssSample *sample = s_require_sample(sample_ptr);
	if (!sample) {
		return;
	}

	DTTR_LOG_TRACE("MSS AIL_init_sample(sample[%d]=%p)", s_sample_slot(sample), sample);

	s_free_sample_audio(sample);
	s_reset_sample_defaults(sample);
}

int __stdcall dttr_mss_ail_set_sample_file(
	void *sample_ptr,
	const void *file_image,
	int block
) {
	if (!s_is_sample(sample_ptr) || !file_image || !dttr_mss_core_ensure_mixer()) {
		DTTR_LOG_TRACE(
			"MSS AIL_set_sample_file(sample=%p, file=%p, block=%d) -> 0 (invalid)",
			sample_ptr,
			file_image,
			block
		);
		return 0;
	}

	S_MssSample *sample = sample_ptr;

	DTTR_LOG_TRACE(
		"MSS AIL_set_sample_file(sample[%d]=%p, file=%p, block=%d)",
		s_sample_slot(sample),
		sample,
		file_image,
		block
	);

	s_free_sample_audio(sample);

	const size_t size = dttr_mss_wave_riff_size(file_image);
	if (block > 0 && size > (size_t)block) {
		DTTR_LOG_ERROR("AIL_set_sample_file received truncated WAVE data");
		return 0;
	}

	S_WaveInfo wave = {0};
	if (!size || !dttr_mss_wave_parse(file_image, &wave)) {
		DTTR_LOG_ERROR("AIL_set_sample_file received non-WAVE data");
		return 0;
	}

	DTTR_LOG_TRACE(
		"MSS AIL_set_sample_file sample[%d] RIFF size=%zu format=%u channels=%u rate=%u "
		"bits=%u data=%zu",
		s_sample_slot(sample),
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
		s_free_sample_audio(sample);
		return 0;
	}

	const int requested_rate = sample->current_rate;
	sample->base_rate = dttr_mss_wave_rate(&wave);
	sample->current_rate = sample->rate_overridden && requested_rate > 0
							   ? requested_rate
							   : sample->base_rate;

	const bool loaded = s_load_sample_frames(sample, file_image, size, &wave)
						|| s_load_sample_audio_from_memory(sample, file_image, size);
	if (!loaded) {
		s_free_sample_audio(sample);
		return 0;
	}

	if (!sample->audio && !sample->pcm_frames) {
		DTTR_LOG_ERROR("MSS sample load failed: %s", SDL_GetError());
		s_free_sample_audio(sample);
		return 0;
	}

	s_apply_sample_track(sample);
	DTTR_LOG_TRACE(
		"MSS AIL_set_sample_file sample[%d] -> 1 track=%p audio=%p pcm_frames=%zu "
		"base_rate=%d current_rate=%d",
		s_sample_slot(sample),
		sample->track,
		sample->audio,
		sample->pcm_frame_count,
		sample->base_rate,
		sample->current_rate
	);
	return 1;
}

void __stdcall dttr_mss_ail_start_sample(void *sample_ptr) {
	S_MssSample *sample = s_require_sample(sample_ptr);
	if (!sample) {
		return;
	}

	DTTR_LOG_TRACE(
		"MSS AIL_start_sample(sample[%d]=%p status=%d loops=%d volume=%d pan=%d rate=%d "
		"track=%p audio=%p)",
		s_sample_slot(sample),
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
		&& !s_render_sample_audio(sample)) {
		DTTR_LOG_ERROR("MSS sample render failed before start");
		return;
	}

	if (!sample->track || !sample->audio) {
		DTTR_LOG_TRACE(
			"MSS AIL_start_sample sample[%d] skipped missing track/audio",
			s_sample_slot(sample)
		);
		return;
	}

	s_apply_sample_track(sample);
	sample->paused_by_rate = false;
	sample->status = S_AIL_STATUS_PLAYING;
	const int sdl_loops = dttr_mss_loops_to_sdl(sample->loops);
	dttr_mss_track_play(sample->track, sdl_loops);
	DTTR_LOG_TRACE(
		"MSS AIL_start_sample sample[%d] played sdl_loops=%d",
		s_sample_slot(sample),
		sdl_loops
	);
}

void __stdcall dttr_mss_ail_stop_sample(void *sample_ptr) {
	S_MssSample *sample = s_require_sample(sample_ptr);
	if (!sample) {
		return;
	}

	DTTR_LOG_TRACE(
		"MSS AIL_stop_sample(sample[%d]=%p status=%d track=%p)",
		s_sample_slot(sample),
		sample,
		sample->status,
		sample->track
	);
	s_stop_sample(sample);
}

void __stdcall dttr_mss_ail_end_sample(void *sample_ptr) {
	dttr_mss_ail_stop_sample(sample_ptr);
}

int __stdcall dttr_mss_ail_sample_status(void *sample_ptr) {
	if (!s_is_sample(sample_ptr)) {
		return S_AIL_STATUS_DONE;
	}

	S_MssSample *sample = sample_ptr;
	sample->status = dttr_mss_track_status(sample->track, sample->status);
	return sample->status;
}

void __stdcall dttr_mss_ail_set_sample_loop_count(void *sample_ptr, int loops) {
	S_MssSample *sample = s_require_sample(sample_ptr);
	if (!sample) {
		return;
	}

	sample->loops = loops;
	if (sample->track) {
		MIX_SetTrackLoops(sample->track, dttr_mss_loops_to_sdl(loops));
	}
}

void __stdcall dttr_mss_ail_set_sample_volume(void *sample_ptr, int volume) {
	S_MssSample *sample = s_require_sample(sample_ptr);
	if (!sample) {
		return;
	}

	sample->volume = volume;
	s_apply_sample_gain(sample);
}

void __stdcall dttr_mss_ail_set_sample_pan(void *sample_ptr, int pan) {
	S_MssSample *sample = s_require_sample(sample_ptr);
	if (!sample) {
		return;
	}

	sample->pan = pan;
	dttr_mss_track_apply_pan(sample->track, pan);
}

void __stdcall dttr_mss_ail_set_sample_playback_rate(void *sample_ptr, int rate) {
	S_MssSample *sample = s_require_sample(sample_ptr);
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
		sample->status = S_AIL_STATUS_PLAYING;
		DTTR_LOG_TRACE(
			"MSS AIL_set_sample_playback_rate(sample[%d]=%p, rate=%d previous=%d) "
			"paused sample",
			s_sample_slot(sample),
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
		s_render_sample_audio(sample);
	}

	s_apply_rate(sample);
	if (sample->paused_by_rate && sample->track) {
		MIX_ResumeTrack(sample->track);
	}

	sample->paused_by_rate = false;
	DTTR_LOG_TRACE(
		"MSS AIL_set_sample_playback_rate(sample[%d]=%p, rate=%d previous=%d current=%d "
		"base=%d resumed=%d)",
		s_sample_slot(sample),
		sample,
		rate,
		previous,
		sample->current_rate,
		sample->base_rate,
		sample->track ? !MIX_TrackPaused(sample->track) : 0
	);
}

int __stdcall dttr_mss_ail_sample_playback_rate(void *sample_ptr) {
	if (!s_is_sample(sample_ptr)) {
		return DTTR_MSS_DEFAULT_RATE;
	}

	S_MssSample *sample = sample_ptr;
	return sample->current_rate > 0 ? sample->current_rate : DTTR_MSS_DEFAULT_RATE;
}
