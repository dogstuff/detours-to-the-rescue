#include "dttr_interop_pcdogs.h"
#include "game/game_data_source_private.h"
#include "mss_private.h"

#include <dttr_log.h>

#include <sds.h>

#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#define S_AIL_STATUS_DONE DTTR_MSS_STATUS_DONE
#define S_AIL_STATUS_PLAYING DTTR_MSS_STATUS_PLAYING

typedef struct S_MssStream {
	MIX_Audio *audio;
	MIX_Track *track;
	struct S_MssStream *prev;
	struct S_MssStream *next;
	int volume;
	int loops;
	int status;
} S_MssStream;

static S_MssStream *s_streams;

static S_MssStream *s_find_stream(const void *ptr) {
	for (S_MssStream *stream = s_streams; stream; stream = stream->next) {
		if (stream == ptr) {
			return stream;
		}
	}

	return NULL;
}

static int s_stream_slot(const S_MssStream *needle) {
	int index = 0;
	for (S_MssStream *stream = s_streams; stream; stream = stream->next, index++) {
		if (stream == needle) {
			return index;
		}
	}

	return -1;
}

static void s_reset_stream_defaults(S_MssStream *stream) {
	if (!stream) {
		return;
	}

	stream->volume = DTTR_MSS_DEFAULT_VOLUME;
	stream->loops = DTTR_MSS_DEFAULT_LOOP_COUNT;
	stream->status = S_AIL_STATUS_DONE;
}

static void s_apply_stream_gain(S_MssStream *stream) {
	if (!stream || !stream->track) {
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

static void s_apply_stream_track(S_MssStream *stream) {
	if (!stream || !stream->track || !stream->audio) {
		return;
	}

	MIX_SetTrackAudio(stream->track, stream->audio);
	s_apply_stream_gain(stream);
}

static void s_link_stream(S_MssStream *stream) {
	if (!stream) {
		return;
	}

	stream->prev = NULL;
	stream->next = s_streams;
	if (s_streams) {
		s_streams->prev = stream;
	}

	s_streams = stream;
}

static void s_unlink_stream(S_MssStream *stream) {
	if (!stream) {
		return;
	}

	if (stream->prev) {
		stream->prev->next = stream->next;
	} else if (s_streams == stream) {
		s_streams = stream->next;
	}

	if (stream->next) {
		stream->next->prev = stream->prev;
	}

	stream->prev = NULL;
	stream->next = NULL;
}

static void s_destroy_stream_objects(S_MssStream *stream) {
	if (!stream) {
		return;
	}

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

static void s_destroy_stream(S_MssStream *stream) {
	if (!stream) {
		return;
	}

	s_unlink_stream(stream);
	s_destroy_stream_objects(stream);
	memset(stream, 0, sizeof(*stream));
	free(stream);
}

void dttr_mss_stream_shutdown_all(void) {
	while (s_streams) {
		s_destroy_stream(s_streams);
	}
}

void dttr_mss_stream_apply_master_gain(void) {
	for (S_MssStream *stream = s_streams; stream; stream = stream->next) {
		s_apply_stream_gain(stream);
	}
}

static sds s_resolve_game_relative_stream_path(const char *relative) {
	sds requested = sdscatprintf(sdsempty(), "%s\\%s", g_pcdogs_directory_ptr(), relative);
	char case_resolved[MAX_PATH];
	const char *resolved = NULL;
	if (dttr_game_data_source_resolve_existing_read_path(
			requested,
			case_resolved,
			sizeof(case_resolved)
		)) {
		resolved = case_resolved;
	}

	char cached[MAX_PATH];
	if (!resolved
		&& dttr_game_data_source_resolve_read_path(relative, cached, sizeof(cached))) {
		resolved = cached;
	}

	if (!resolved) {
		return requested;
	}

	sds out = sdsnew(resolved);
	sdsfree(requested);
	return out;
}

static sds s_resolve_stream_path(const char *path) {
	if (!path) {
		return sdsempty();
	}

	const bool absolute = strlen(path) >= 3 && path[1] == ':'
						  && (path[2] == '\\' || path[2] == '/');
	const char *relative = path;
	bool found_game_relative = !absolute;
	if (absolute) {
		static const char *const data_roots[] = {
			"data\\",
			"DATA\\",
			"data/",
			"DATA/",
		};
		relative = path + 3;
		while (*relative == '\\' || *relative == '/') {
			relative++;
		}

		for (size_t i = 0; i < SDL_arraysize(data_roots); i++) {
			const char *data = strstr(relative, data_roots[i]);
			if (!data) {
				continue;
			}

			relative = data;
			found_game_relative = true;
			break;
		}
	}

	sds resolved = sdsnew(path);
	if (found_game_relative) {
		sdsfree(resolved);
		resolved = s_resolve_game_relative_stream_path(relative);
	}
	for (char *p = resolved; *p; p++) {
		if (*p == '/') {
			*p = '\\';
		}
	}
	return resolved;
}

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

	S_MssStream *stream = calloc(1, sizeof(S_MssStream));
	if (!stream) {
		return NULL;
	}

	s_reset_stream_defaults(stream);

	sds open_path = s_resolve_stream_path(path);
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
		s_destroy_stream_objects(stream);
		free(stream);
		return NULL;
	}

	s_apply_stream_track(stream);
	s_link_stream(stream);
	DTTR_LOG_TRACE(
		"MSS AIL_open_stream -> stream[%d]=%p track=%p audio=%p",
		s_stream_slot(stream),
		stream,
		stream->track,
		stream->audio
	);
	return stream;
}

void __stdcall dttr_mss_ail_close_stream(void *stream_ptr) {
	S_MssStream *stream = s_find_stream(stream_ptr);
	if (!stream) {
		return;
	}

	DTTR_LOG_TRACE("MSS AIL_close_stream(stream[%d]=%p)", s_stream_slot(stream), stream);
	s_destroy_stream(stream);
}

void __stdcall dttr_mss_ail_start_stream(void *stream_ptr) {
	S_MssStream *stream = s_find_stream(stream_ptr);
	if (!stream) {
		return;
	}

	DTTR_LOG_TRACE(
		"MSS AIL_start_stream(stream[%d]=%p status=%d loops=%d volume=%d)",
		s_stream_slot(stream),
		stream,
		stream->status,
		stream->loops,
		stream->volume
	);
	s_apply_stream_track(stream);
	stream->status = S_AIL_STATUS_PLAYING;
	const int sdl_loops = dttr_mss_loops_to_sdl(stream->loops);
	dttr_mss_track_play(stream->track, sdl_loops);
	DTTR_LOG_TRACE(
		"MSS AIL_start_stream stream[%d] played sdl_loops=%d",
		s_stream_slot(stream),
		sdl_loops
	);
}

int __stdcall dttr_mss_ail_stream_status(void *stream_ptr) {
	S_MssStream *stream = s_find_stream(stream_ptr);
	if (!stream) {
		return S_AIL_STATUS_DONE;
	}

	stream->status = dttr_mss_track_status(stream->track, stream->status);
	return stream->status;
}

void __stdcall dttr_mss_ail_pause_stream(void *stream_ptr, int pause) {
	S_MssStream *stream = s_find_stream(stream_ptr);
	if (!stream) {
		return;
	}

	if (pause) {
		MIX_PauseTrack(stream->track);
	} else {
		MIX_ResumeTrack(stream->track);
	}

	stream->status = S_AIL_STATUS_PLAYING;
}

void __stdcall dttr_mss_ail_set_stream_volume(void *stream_ptr, int volume) {
	S_MssStream *stream = s_find_stream(stream_ptr);
	if (!stream) {
		return;
	}

	stream->volume = volume;
	s_apply_stream_gain(stream);
}

void __stdcall dttr_mss_ail_set_stream_loop_count(void *stream_ptr, int loops) {
	S_MssStream *stream = s_find_stream(stream_ptr);
	if (!stream) {
		return;
	}

	stream->loops = loops;
	MIX_SetTrackLoops(stream->track, dttr_mss_loops_to_sdl(loops));
}
