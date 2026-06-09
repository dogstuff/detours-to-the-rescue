#include "mss_private.h"
#include "sidecar_private.h"

#include <dttr_log.h>
#include <dttr_path.h>

#include <sds.h>

#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

typedef struct mss_stream {
	MIX_Audio *audio;
	MIX_Track *track;
	struct mss_stream *prev;
	struct mss_stream *next;
	int volume;
	int loops;
	int status;
} mss_stream;

static mss_stream *streams;

static mss_stream *find_stream(const void *ptr) {
	for (mss_stream *stream = streams; stream; stream = stream->next) {
		if (stream == ptr) {
			return stream;
		}
	}

	return NULL;
}

// Returns the zero-based slot for an active SDL_mixer stream handle.
static int stream_slot(const mss_stream *target_stream) {
	int index = 0;

	for (mss_stream *stream = streams; stream; stream = stream->next, index++) {
		if (stream == target_stream) {
			return index;
		}
	}

	return -1;
}

static void reset_stream_defaults(mss_stream *stream) {
	stream->volume = DTTR_MSS_DEFAULT_VOLUME;
	stream->loops = DTTR_MSS_DEFAULT_LOOP_COUNT;
	stream->status = DTTR_MSS_STATUS_DONE;
}

// Combines per-stream volume with the master gain before updating the SDL_mixer track.
static void apply_stream_gain(mss_stream *stream) {
	if (!stream->track) {
		return;
	}

	MIX_SetTrackGain(
		stream->track,
		dttr_mss_track_gain(
			stream->volume,
			dttr_mss_core_master_gain(),
			DTTR_MSS_STREAM_HEADROOM_GAIN
		)
	);
}

// Binds decoded audio to its track and reapplies gain after stream setup changes.
static void apply_stream_track(mss_stream *stream) {
	if (!stream->track || !stream->audio) {
		return;
	}

	MIX_SetTrackAudio(stream->track, stream->audio);
	apply_stream_gain(stream);
}

// Adds a newly opened stream to the intrusive list used for handle validation.
static void link_stream(mss_stream *stream) {
	stream->prev = NULL;
	stream->next = streams;

	if (streams) {
		streams->prev = stream;
	}

	streams = stream;
}

// Removes a stream from the active list before its handle memory is cleared.
static void unlink_stream(mss_stream *stream) {
	if (stream->prev) {
		stream->prev->next = stream->next;
	} else if (streams == stream) {
		streams = stream->next;
	}

	if (stream->next) {
		stream->next->prev = stream->prev;
	}

	stream->prev = NULL;
	stream->next = NULL;
}

// Releases SDL_mixer track and audio objects owned by one MSS stream handle.
static void destroy_stream_objects(mss_stream *stream) {
	if (stream->track) {
		MIX_SetTrackAudio(stream->track, NULL);
		MIX_DestroyTrack(stream->track);
		stream->track = NULL;
	}

	if (stream->audio) {
		MIX_DestroyAudio(stream->audio);
		stream->audio = NULL;
	}
}

// Unlinks, releases, clears, and frees one stream handle returned to the game.
static void destroy_stream(mss_stream *stream) {
	unlink_stream(stream);
	destroy_stream_objects(stream);
	memset(stream, 0, sizeof(*stream));
	free(stream);
}

// Releases all active SDL_mixer stream handles.
void dttr_mss_stream_shutdown_all() {
	while (streams) {
		destroy_stream(streams);
	}
}

// Reapplies master gain to every active SDL_mixer stream.
void dttr_mss_stream_apply_master_gain() {
	for (mss_stream *stream = streams; stream; stream = stream->next) {
		apply_stream_gain(stream);
	}
}

// Normalizes absolute and relative stream paths into the file path SDL_mixer should load.
static sds resolve_stream_path(const char *path) {
	if (!path) {
		return sdsempty();
	}

	const bool absolute = DTTR_Path_IsWindowsAbsolute(path);
	const char *relative = absolute ? dttr_game_data_find_data_segment(path + 3) : path;
	sds resolved = relative ? dttr_game_data_resolve_media_path(relative) : sdsnew(path);

	if (!resolved) {
		return sdsempty();
	}

	sdsmapchars(resolved, "/", "\\", 1);
	return resolved;
}

// Opens an SDL_mixer stream for the Miles AIL stream API.
void *__stdcall dttr_mss_ail_open_stream(void *driver, const char *path, int stream_mem) {
	DTTR_LOG_TRACE(
		"MSS AIL_open_stream(driver=%p, path=\"%s\", stream_mem=%d)",
		driver,
		path ? path : "(null)",
		stream_mem
	);

	if (!path || !dttr_mss_core_ensure_mixer()) {
		DTTR_LOG_TRACE("MSS AIL_open_stream -> NULL (invalid path or mixer unavailable)");
		return NULL;
	}

	mss_stream *stream = calloc(1, sizeof(mss_stream));

	if (!stream) {
		return NULL;
	}

	reset_stream_defaults(stream);

	sds open_path = resolve_stream_path(path);
	DTTR_LOG_TRACE("MSS AIL_open_stream resolved path=\"%s\"", open_path);

	stream->audio = MIX_LoadAudio(dttr_mss_core_mixer(), open_path, false);

	if (!stream->audio) {
		DTTR_LOG_ERROR(
			"MIX_LoadAudio stream failed for %s resolved as %s: %s",
			path,
			open_path,
			SDL_GetError()
		);

		sdsfree(open_path);
		free(stream);
		return NULL;
	}

	sdsfree(open_path);

	stream->track = MIX_CreateTrack(dttr_mss_core_mixer());

	if (!stream->track) {
		DTTR_LOG_ERROR("MIX_CreateTrack stream failed: %s", SDL_GetError());
		destroy_stream_objects(stream);
		free(stream);
		return NULL;
	}

	apply_stream_track(stream);
	link_stream(stream);
	DTTR_LOG_TRACE(
		"MSS AIL_open_stream -> stream[%d]=%p track=%p audio=%p",
		stream_slot(stream),
		stream,
		stream->track,
		stream->audio
	);
	return stream;
}

// Closes an SDL_mixer stream returned to the Miles AIL stream API.
void __stdcall dttr_mss_ail_close_stream(void *stream_ptr) {
	mss_stream *stream = find_stream(stream_ptr);

	if (!stream) {
		return;
	}

	DTTR_LOG_TRACE("MSS AIL_close_stream(stream[%d]=%p)", stream_slot(stream), stream);
	destroy_stream(stream);
}

// Starts playback for an SDL_mixer stream handle.
void __stdcall dttr_mss_ail_start_stream(void *stream_ptr) {
	mss_stream *stream = find_stream(stream_ptr);

	if (!stream) {
		return;
	}

	DTTR_LOG_TRACE(
		"MSS AIL_start_stream(stream[%d]=%p status=%d loops=%d volume=%d)",
		stream_slot(stream),
		stream,
		stream->status,
		stream->loops,
		stream->volume
	);
	apply_stream_track(stream);
	stream->status = DTTR_MSS_STATUS_PLAYING;
	const int sdl_loops = dttr_mss_loops_to_sdl(stream->loops);
	dttr_mss_track_play(stream->track, sdl_loops);
	DTTR_LOG_TRACE(
		"MSS AIL_start_stream stream[%d] played sdl_loops=%d",
		stream_slot(stream),
		sdl_loops
	);
}

// Reports the Miles-compatible status for an SDL_mixer stream handle.
int __stdcall dttr_mss_ail_stream_status(void *stream_ptr) {
	mss_stream *stream = find_stream(stream_ptr);

	if (!stream) {
		return DTTR_MSS_STATUS_DONE;
	}

	stream->status = dttr_mss_track_status(stream->track, stream->status);
	return stream->status;
}

// Pauses or resumes an SDL_mixer stream handle.
void __stdcall dttr_mss_ail_pause_stream(void *stream_ptr, int pause) {
	mss_stream *stream = find_stream(stream_ptr);

	if (!stream) {
		return;
	}

	if (pause) {
		MIX_PauseTrack(stream->track);
	} else {
		MIX_ResumeTrack(stream->track);
	}

	stream->status = DTTR_MSS_STATUS_PLAYING;
}

// Applies Miles stream volume to an SDL_mixer stream handle.
void __stdcall dttr_mss_ail_set_stream_volume(void *stream_ptr, int volume) {
	mss_stream *stream = find_stream(stream_ptr);

	if (!stream) {
		return;
	}

	stream->volume = volume;
	apply_stream_gain(stream);
}

// Applies Miles loop count to an SDL_mixer stream handle.
void __stdcall dttr_mss_ail_set_stream_loop_count(void *stream_ptr, int loops) {
	mss_stream *stream = find_stream(stream_ptr);

	if (!stream) {
		return;
	}

	stream->loops = loops;
	MIX_SetTrackLoops(stream->track, dttr_mss_loops_to_sdl(loops));
}
