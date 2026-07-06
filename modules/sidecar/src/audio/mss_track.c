#include "mss_private.h"

#include <SDL3/SDL.h>

float dttr_mss_track_gain(int volume, float master_gain, float headroom) {
	const int clamped_volume = SDL_clamp(volume, 0, DTTR_MSS_DEFAULT_VOLUME);
	if (clamped_volume <= 0) {
		return 0.0f;
	}

	const float gain = master_gain * headroom;
	if (clamped_volume >= DTTR_MSS_DEFAULT_VOLUME) {
		return gain;
	}

	return gain * ((float)clamped_volume / DTTR_MSS_MAX_VOLUME);
}

void dttr_mss_track_play(MIX_Track *track, int sdl_loops) {
	if (!track) {
		return;
	}

	if (sdl_loops == 0) {
		MIX_PlayTrack(track, 0);
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
