#include "mss_private.h"

#include <SDL3/SDL.h>

#define STEREO_BYTE_MAX 128.0f
#define MIN_FREQUENCY_RATIO 0.01f
#define MAX_FREQUENCY_RATIO 100.0f

float dttr_mss_track_gain(int volume, float master_gain, float headroom) {
	if (volume <= 0) {
		return 0.0f;
	}

	const float gain = master_gain * headroom;
	if (volume >= DTTR_MSS_DEFAULT_VOLUME) {
		return gain;
	}

	return gain * ((float)volume / DTTR_MSS_MAX_VOLUME);
}

void dttr_mss_track_apply_pan(MIX_Track *track, int pan) {
	if (!track) {
		return;
	}

	int left = 128;
	int right = 128;
	dttr_mss_pan_to_stereo_bytes(pan, &left, &right);
	MIX_StereoGains gains = {
		(float)left / STEREO_BYTE_MAX,
		(float)right / STEREO_BYTE_MAX,
	};

	MIX_SetTrackStereo(track, &gains);
}

void dttr_mss_track_play(MIX_Track *track, int sdl_loops) {
	if (!track) {
		return;
	}

	SDL_PropertiesID props = SDL_CreateProperties();
	if (!props) {
		MIX_PlayTrack(track, 0);
		return;
	}

	SDL_SetNumberProperty(props, MIX_PROP_PLAY_LOOPS_NUMBER, sdl_loops);
	MIX_PlayTrack(track, props);
	SDL_DestroyProperties(props);
}

float dttr_mss_track_frequency_ratio(int rate, int reference_rate) {
	if (rate <= 0 || reference_rate <= 0) {
		return 0.0f;
	}

	float ratio = (float)rate / (float)reference_rate;
	if (ratio < MIN_FREQUENCY_RATIO) {
		return MIN_FREQUENCY_RATIO;
	}

	if (ratio > MAX_FREQUENCY_RATIO) {
		return MAX_FREQUENCY_RATIO;
	}

	return ratio;
}

int dttr_mss_track_status(MIX_Track *track, int previous_status) {
	const int stopped_status = previous_status == DTTR_MSS_STATUS_STOPPED
								   ? DTTR_MSS_STATUS_STOPPED
								   : DTTR_MSS_STATUS_DONE;
	if (!track) {
		return stopped_status;
	}

	if (MIX_TrackPlaying(track) || MIX_TrackPaused(track)) {
		return DTTR_MSS_STATUS_PLAYING;
	}

	return stopped_status;
}
