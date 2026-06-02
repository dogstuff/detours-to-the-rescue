#include <dttr_pcdogs.h>

#include "dttr_sidecar.h"
#include "hooks_private.h"
#include "sidecar_private.h"
#include <dttr_log.h>
#include <dttr_path.h>
#include <sds.h>

#include <SDL3/SDL.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libavutil/error.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

static DTTR_PCDOGS_F_Video_PlayMovieFile_proto movie_play_file_original;

// Replaces the game's blocking movie call with sidecar-managed FFmpeg playback.
static BOOL __cdecl movie_play_file_detour(
	const char *movie_path,
	char use_alt_video_rect
) {
	return dttr_movies_hook_movie_play_file_callback(movie_path, use_alt_video_rect);
}

#define AUDIO_CHANNELS 2
#define AUDIO_FORMAT AV_SAMPLE_FMT_S16
#define AUDIO_QUEUE_LIMIT_MS 500
#define AUDIO_DRAIN_LIMIT_MS 750

typedef struct {
	AVFormatContext *format;
	AVCodecContext *video_codec;
	AVCodecContext *audio_codec;
	struct SwsContext *sws;
	SwrContext *swr;
	SDL_AudioStream *audio_stream;
	uint8_t *buffer;
	int video_stream;
	int audio_stream_index;
	int buf_w;
	int buf_h;
	int buf_stride;
	bool video_frame_ready;
	bool hit_eof;
	bool video_flushed;
	bool audio_flushed;
	bool has_timing_origin;
	uint64_t start_ticks;
	uint64_t audio_drain_deadline_ticks;
	double pts_origin;
	double next_video_time;
	double last_video_pts;
	double frame_duration;
	DTTR_MovieResult result;
} movie_state;

static movie_state movie = {
	.video_stream = -1,
	.audio_stream_index = -1,
	.frame_duration = 1.0 / 15.0,
	.result = DTTR_MOVIE_ENDED,
};

static AVFrame *video_frame = NULL;
static AVFrame *audio_frame = NULL;
static AVPacket *packet = NULL;

// Restores the movie decoder state to its idle defaults while preserving the final result.
static void reset_movie_state(DTTR_MovieResult result) {
	movie = (movie_state){
		.video_stream = -1,
		.audio_stream_index = -1,
		.frame_duration = 1.0 / 15.0,
		.result = result,
	};
}

// Formats the latest FFmpeg error into a reusable buffer for sidecar log messages.
static const char *av_error(const int err) {
	static char buf[AV_ERROR_MAX_STRING_SIZE];
	av_strerror(err, buf, sizeof(buf));
	return buf;
}

// Frees the converted BGRA frame buffer and marks the presentation frame as empty.
static void reset_video_buffer() {
	free(movie.buffer);
	movie.buffer = NULL;
	movie.buf_w = 0;
	movie.buf_h = 0;
	movie.buf_stride = 0;
	movie.video_frame_ready = false;
}

// Releases the SDL audio stream and resampler used by decoded movie audio.
static void close_audio() {
	if (movie.audio_stream) {
		SDL_DestroyAudioStream(movie.audio_stream);
		movie.audio_stream = NULL;
	}

	swr_free(&movie.swr);
}

// Releases all FFmpeg and SDL movie resources while keeping the playback result intact.
static void close_movie() {
	const DTTR_MovieResult result = movie.result;
	close_audio();
	sws_freeContext(movie.sws);
	avcodec_free_context(&movie.video_codec);
	avcodec_free_context(&movie.audio_codec);
	avformat_close_input(&movie.format);
	reset_video_buffer();
	reset_movie_state(result);
}

// Allocates and opens an FFmpeg decoder for one selected movie stream.
static AVCodecContext *open_codec(const AVStream *stream) {
	const AVCodec *codec = avcodec_find_decoder(stream->codecpar->codec_id);
	if (!codec) {
		DTTR_LOG_ERROR(
			"Missing movie decoder for codec id %d",
			stream->codecpar->codec_id
		);
		return NULL;
	}

	AVCodecContext *ctx = avcodec_alloc_context3(codec);
	if (!ctx) {
		DTTR_LOG_ERROR("Failed to allocate movie decoder context");
		return NULL;
	}

	int err = avcodec_parameters_to_context(ctx, stream->codecpar);
	if (err < 0) {
		DTTR_LOG_ERROR("Failed to configure movie decoder: %s", av_error(err));
		avcodec_free_context(&ctx);
		return NULL;
	}

	err = avcodec_open2(ctx, codec, NULL);
	if (err < 0) {
		DTTR_LOG_ERROR("Failed to open movie decoder %s: %s", codec->name, av_error(err));
		avcodec_free_context(&ctx);
		return NULL;
	}

	return ctx;
}

// Builds the resampler and SDL playback stream for movies that include audio.
static bool prepare_audio() {
	if (!movie.audio_codec) {
		return true;
	}

	AVChannelLayout out_layout;
	av_channel_layout_default(&out_layout, AUDIO_CHANNELS);

	const int err = swr_alloc_set_opts2(
		&movie.swr,
		&out_layout,
		AUDIO_FORMAT,
		movie.audio_codec->sample_rate,
		&movie.audio_codec->ch_layout,
		movie.audio_codec->sample_fmt,
		movie.audio_codec->sample_rate,
		0,
		NULL
	);
	av_channel_layout_uninit(&out_layout);

	if (err < 0 || !movie.swr) {
		DTTR_LOG_ERROR("Failed to allocate movie audio resampler: %s", av_error(err));
		return false;
	}

	const int init_err = swr_init(movie.swr);
	if (init_err < 0) {
		DTTR_LOG_ERROR(
			"Failed to initialize movie audio resampler: %s",
			av_error(init_err)
		);
		return false;
	}

	const SDL_AudioSpec spec = {
		.format = SDL_AUDIO_S16LE,
		.channels = AUDIO_CHANNELS,
		.freq = movie.audio_codec->sample_rate,
	};

	movie.audio_stream = SDL_OpenAudioDeviceStream(
		SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
		&spec,
		NULL,
		NULL
	);
	if (!movie.audio_stream) {
		DTTR_LOG_ERROR("Failed to open movie audio stream: %s", SDL_GetError());
		return false;
	}

	SDL_ResumeAudioStreamDevice(movie.audio_stream);
	return true;
}

// Opens the movie container, selects streams, and prepares decoders for sidecar playback.
static bool open_movie(const char *path) {
	int err = avformat_open_input(&movie.format, path, NULL, NULL);
	if (err < 0) {
		DTTR_LOG_ERROR("Failed to open movie %s: %s", path, av_error(err));
		return false;
	}

	err = avformat_find_stream_info(movie.format, NULL);
	if (err < 0) {
		DTTR_LOG_ERROR("Failed to read movie stream info: %s", av_error(err));
		return false;
	}

	movie.video_stream = av_find_best_stream(
		movie.format,
		AVMEDIA_TYPE_VIDEO,
		-1,
		-1,
		NULL,
		0
	);
	if (movie.video_stream < 0) {
		DTTR_LOG_ERROR(
			"Movie has no playable video stream: %s",
			av_error(movie.video_stream)
		);
		return false;
	}

	AVStream *const video_stream = movie.format->streams[movie.video_stream];
	movie.video_codec = open_codec(video_stream);
	if (!movie.video_codec) {
		return false;
	}

	movie.audio_stream_index = av_find_best_stream(
		movie.format,
		AVMEDIA_TYPE_AUDIO,
		-1,
		movie.video_stream,
		NULL,
		0
	);
	if (movie.audio_stream_index >= 0) {
		AVStream *const audio_stream = movie.format->streams[movie.audio_stream_index];
		movie.audio_codec = open_codec(audio_stream);
		if (!movie.audio_codec || !prepare_audio()) {
			return false;
		}
	}

	AVRational rate = av_guess_frame_rate(movie.format, video_stream, NULL);
	if (rate.num > 0 && rate.den > 0) {
		movie.frame_duration = av_q2d((AVRational){rate.den, rate.num});
	}

	return true;
}

// Converts a decoded video timestamp into seconds using the stream time base.
static double video_pts_seconds(const int64_t pts) {
	if (pts == AV_NOPTS_VALUE) {
		return movie.has_timing_origin ? movie.last_video_pts + movie.frame_duration
									   : 0.0;
	}

	const AVStream *stream = movie.format->streams[movie.video_stream];
	return (double)pts * av_q2d(stream->time_base);
}

// Establishes the movie clock origin and next presentation timestamp for video pacing.
static void set_next_video_time(const double pts) {
	if (!movie.has_timing_origin) {
		movie.start_ticks = SDL_GetTicks();
		movie.pts_origin = pts;
		movie.has_timing_origin = true;
	}

	movie.next_video_time = pts - movie.pts_origin;
	if (movie.next_video_time < 0.0) {
		movie.next_video_time = 0.0;
	}

	movie.last_video_pts = pts;
}

// Chooses the best available frame timestamp before scheduling presentation.
static double frame_pts_seconds(const AVFrame *frame) {
	return video_pts_seconds(frame->best_effort_timestamp);
}

// Converts a packet timestamp into seconds for fallback movie pacing.
static double packet_pts_seconds(const AVPacket *packet) {
	return video_pts_seconds(packet->pts != AV_NOPTS_VALUE ? packet->pts : packet->dts);
}

// Converts a decoded frame to BGRA and records when the renderer should present it.
static bool queue_video_frame(const AVFrame *frame) {
	const int w = frame->width;
	const int h = frame->height;
	if (w <= 0 || h <= 0 || w > INT_MAX / 4) {
		return false;
	}

	const int stride = w * 4;
	if ((size_t)h > SIZE_MAX / (size_t)stride) {
		return false;
	}

	const size_t size = (size_t)stride * (size_t)h;
	if (w != movie.buf_w || h != movie.buf_h || stride != movie.buf_stride) {
		uint8_t *new_buffer = realloc(movie.buffer, size);
		if (!new_buffer) {
			DTTR_LOG_ERROR("Failed to allocate %zu bytes for movie frame", size);
			return false;
		}

		movie.buffer = new_buffer;
		movie.buf_w = w;
		movie.buf_h = h;
		movie.buf_stride = stride;
		DTTR_LOG_DEBUG("Video Format: %dx%d", movie.buf_w, movie.buf_h);
	}

	movie.sws = sws_getCachedContext(
		movie.sws,
		w,
		h,
		(enum AVPixelFormat)frame->format,
		w,
		h,
		AV_PIX_FMT_BGRA,
		SWS_BILINEAR,
		NULL,
		NULL,
		NULL
	);
	if (!movie.sws) {
		DTTR_LOG_ERROR("Failed to create movie video converter");
		return false;
	}

	uint8_t *dst_data[] = {movie.buffer, NULL, NULL, NULL};
	int dst_linesize[] = {movie.buf_stride, 0, 0, 0};
	sws_scale(
		movie.sws,
		(const uint8_t *const *)frame->data,
		frame->linesize,
		0,
		h,
		dst_data,
		dst_linesize
	);

	set_next_video_time(frame_pts_seconds(frame));
	movie.video_frame_ready = true;
	return true;
}

// Resamples decoded audio into the SDL stream while capping queued latency.
static bool queue_audio_frame(const AVFrame *frame) {
	if (!movie.audio_stream || !movie.swr) {
		return true;
	}

	const int out_samples = (int)swr_get_delay(movie.swr, movie.audio_codec->sample_rate)
							+ frame->nb_samples;
	const int out_size = av_samples_get_buffer_size(
		NULL,
		AUDIO_CHANNELS,
		out_samples,
		AUDIO_FORMAT,
		1
	);
	if (out_size <= 0) {
		return false;
	}

	uint8_t *out = av_malloc((size_t)out_size);
	if (!out) {
		DTTR_LOG_ERROR("Failed to allocate movie audio buffer");
		return false;
	}

	uint8_t *out_planes[] = {out, NULL};
	const int converted = swr_convert(
		movie.swr,
		out_planes,
		out_samples,
		(const uint8_t **)frame->extended_data,
		frame->nb_samples
	);
	if (converted < 0) {
		DTTR_LOG_ERROR("Failed to convert movie audio: %s", av_error(converted));
		av_free(out);
		return false;
	}

	const int converted_size = av_samples_get_buffer_size(
		NULL,
		AUDIO_CHANNELS,
		converted,
		AUDIO_FORMAT,
		1
	);
	if (converted_size > 0) {
		SDL_PutAudioStreamData(movie.audio_stream, out, converted_size);
	}

	av_free(out);

	const int queue_limit = (movie.audio_codec->sample_rate * AUDIO_CHANNELS
							 * av_get_bytes_per_sample(AUDIO_FORMAT)
							 * AUDIO_QUEUE_LIMIT_MS)
							/ 1000;
	while (SDL_GetAudioStreamQueued(movie.audio_stream) > queue_limit) {
		SDL_Delay(1);
	}

	return true;
}

// Pulls decoded video frames until one is ready for presentation or the decoder drains.
static bool receive_video_frame() {
	while (!movie.video_frame_ready) {
		const int err = avcodec_receive_frame(movie.video_codec, video_frame);
		if (err == AVERROR(EAGAIN) || err == AVERROR_EOF) {
			return false;
		}

		if (err < 0) {
			DTTR_LOG_ERROR("Failed to decode movie video: %s", av_error(err));
			movie.result = DTTR_MOVIE_ENDED;
			return false;
		}

		if (!queue_video_frame(video_frame)) {
			movie.result = DTTR_MOVIE_ENDED;
		}

		av_frame_unref(video_frame);
	}

	return true;
}

// Drains available audio frames so playback stays ahead of the video clock.
static void receive_audio_frames() {
	if (!movie.audio_codec) {
		return;
	}

	for (;;) {
		const int err = avcodec_receive_frame(movie.audio_codec, audio_frame);
		if (err == AVERROR(EAGAIN) || err == AVERROR_EOF) {
			return;
		}

		if (err < 0) {
			DTTR_LOG_WARN("Failed to decode movie audio: %s", av_error(err));
			return;
		}

		if (!queue_audio_frame(audio_frame)) {
			DTTR_LOG_WARN("Failed to queue movie audio frame");
			av_frame_unref(audio_frame);
			return;
		}

		av_frame_unref(audio_frame);
	}
}

// Waits for queued audio to finish after video reaches end-of-stream.
static bool drain_eof() {
	if (!movie.video_flushed) {
		avcodec_send_packet(movie.video_codec, NULL);
		movie.video_flushed = true;
		return true;
	}

	if (movie.audio_codec && !movie.audio_flushed) {
		avcodec_send_packet(movie.audio_codec, NULL);
		movie.audio_flushed = true;
		receive_audio_frames();
		if (movie.audio_stream) {
			SDL_FlushAudioStream(movie.audio_stream);
			movie.audio_drain_deadline_ticks = SDL_GetTicks() + AUDIO_DRAIN_LIMIT_MS;
		}
	}

	if (movie.audio_stream) {
		const int queued = SDL_GetAudioStreamQueued(movie.audio_stream);
		if (queued > 0 && SDL_GetTicks() < movie.audio_drain_deadline_ticks) {
			SDL_Delay(1);
			return false;
		}

		if (queued > 0) {
			DTTR_LOG_WARN("Movie audio drain timed out with %d bytes queued", queued);
		}
	}

	movie.result = DTTR_MOVIE_ENDED;
	return false;
}

// Sends the current packet to the matching decoder and handles end-of-stream flushing.
static void send_packet() {
	if (packet->stream_index == movie.video_stream) {
		if (packet->size <= 0) {
			if (movie.buffer) {
				set_next_video_time(packet_pts_seconds(packet));
				movie.video_frame_ready = true;
			}

			return;
		}

		const int err = avcodec_send_packet(movie.video_codec, packet);
		if (err < 0 && err != AVERROR(EAGAIN)) {
			DTTR_LOG_ERROR("Failed to submit movie video packet: %s", av_error(err));
			movie.result = DTTR_MOVIE_ENDED;
		}

		return;
	}

	if (packet->stream_index != movie.audio_stream_index || !movie.audio_codec) {
		return;
	}

	const int err = avcodec_send_packet(movie.audio_codec, packet);
	if (err < 0 && err != AVERROR(EAGAIN)) {
		DTTR_LOG_WARN("Failed to submit movie audio packet: %s", av_error(err));
	}
}

// Advances packet decoding until a video frame is ready or playback reaches the end.
static bool decode_until_video_frame() {
	while (movie.result == DTTR_MOVIE_PLAYING && !movie.video_frame_ready) {
		if (receive_video_frame()) {
			return true;
		}

		receive_audio_frames();

		if (movie.hit_eof) {
			if (drain_eof()) {
				continue;
			}

			return false;
		}

		const int err = av_read_frame(movie.format, packet);
		if (err == AVERROR_EOF) {
			movie.hit_eof = true;
			continue;
		}

		if (err < 0) {
			DTTR_LOG_ERROR("Failed to read movie packet: %s", av_error(err));
			movie.result = DTTR_MOVIE_ENDED;
			return false;
		}

		send_packet();
		av_packet_unref(packet);
	}

	return movie.video_frame_ready;
}

// Allocates reusable FFmpeg frames and packet storage for movie playback.
void DTTR_Movies_Init() {
	video_frame = av_frame_alloc();
	audio_frame = av_frame_alloc();
	packet = av_packet_alloc();
	if (!video_frame || !audio_frame || !packet) {
		DTTR_LOG_ERROR("Failed to allocate movie playback state");
	}
}

// Installs the game movie hook so sidecar playback replaces the original routine.
bool dttr_movies_hooks_init(const DTTR_Mods_Context *ctx) {
	if (!DTTR_PCDOGS_F_Video_PlayMovieFile
			 ->Hook(&ctx->runtime, movie_play_file_detour, &movie_play_file_original)) {
		DTTR_MODS_LOG_ERROR(ctx, "movie_playfile: hook failed");
		return false;
	}

	return true;
}

// Removes the game movie hook and clears the saved original function pointer.
void dttr_movies_hooks_cleanup(const DTTR_Mods_Context *ctx) {
	DTTR_PCDOGS_F_Video_PlayMovieFile->Unhook(&ctx->runtime);
	movie_play_file_original = NULL;
}

// Releases persistent FFmpeg frame and packet storage during sidecar shutdown.
void DTTR_Movies_Cleanup() {
	close_movie();
	av_packet_free(&packet);
	av_frame_free(&audio_frame);
	av_frame_free(&video_frame);
	DTTR_LOG_INFO("Released movie playback state");
}

// Maps the game movie filename to an override path or bundled data-file path.
static sds resolve_movie_path(const char *path) {
	char (*base_path)[DTTR_PCDOGS_D_PKG_BASE_PATH_COUNT] = DTTR_PCDOGS_D_PKG_BasePath
															   ->Ptr();
	sds requested_path = sdsnew(base_path ? *base_path : NULL);
	if (!requested_path || !DTTR_Path_AppendSegment(&requested_path, path, '\\')) {
		sdsfree(requested_path);
		return sdsempty();
	}

	char resolved[MAX_PATH];
	const char *movie_path = NULL;

	if (dttr_game_data_resolve_existing_read_path(
			requested_path,
			resolved,
			sizeof(resolved)
		)) {
		movie_path = resolved;
	}

	char cached[MAX_PATH];
	if (!movie_path) {
		const bool got_cached = dttr_game_data_resolve_read_path(
			path,
			cached,
			sizeof(cached)
		);
		if (got_cached) {
			movie_path = cached;
		}
	}

	if (!movie_path) {
		return requested_path;
	}

	sds out = sdsnew(movie_path);
	sdsfree(requested_path);
	return out;
}

// Resolves and opens a movie file, then marks playback active when decoders are ready.
void DTTR_Movies_Start(const char *path) {
	if (!video_frame || !audio_frame || !packet) {
		DTTR_LOG_ERROR("Missing movie playback state");
		movie.result = DTTR_MOVIE_ENDED;
		return;
	}

	close_movie();

	sds abs_path = resolve_movie_path(path);
	DTTR_LOG_INFO("Playing movie %s", abs_path);

	if (!open_movie(abs_path)) {
		sdsfree(abs_path);
		close_movie();
		movie.result = DTTR_MOVIE_ENDED;
		return;
	}

	sdsfree(abs_path);
	movie.result = DTTR_MOVIE_PLAYING;
}

// Decodes and presents movie frames on the sidecar loop according to the video clock.
void DTTR_Movies_Tick() {
	if (movie.result != DTTR_MOVIE_PLAYING) {
		return;
	}

	if (!movie.video_frame_ready && !decode_until_video_frame()) {
		return;
	}

	if (!movie.video_frame_ready) {
		return;
	}

	const double elapsed = (double)(SDL_GetTicks() - movie.start_ticks) / 1000.0;
	if (elapsed + 0.001 < movie.next_video_time) {
		SDL_Delay(1);
		return;
	}

	if (!DTTR_Graphics_PresentVideoFrameBGRA(
			movie.buffer,
			movie.buf_w,
			movie.buf_h,
			movie.buf_stride
		)) {
		DTTR_LOG_WARN(
			"Failed to present movie frame (%dx%d stride=%d)",
			movie.buf_w,
			movie.buf_h,
			movie.buf_stride
		);
	}

	movie.video_frame_ready = false;
}

// Handles skip, quit, and gamepad input while a sidecar movie is playing.
bool DTTR_Movies_HandleEvent(const SDL_Event *event) {
	if (movie.result != DTTR_MOVIE_PLAYING) {
		return false;
	}

	if (event->type == SDL_EVENT_KEY_DOWN && !event->key.repeat) {
		if (event->key.scancode == SDL_SCANCODE_ESCAPE) {
			movie.result = DTTR_MOVIE_ESCAPE;
			return true;
		}

		if (event->key.scancode == SDL_SCANCODE_RETURN) {
			movie.result = DTTR_MOVIE_ENDED;
			return true;
		}

		if (event->key.scancode == SDL_SCANCODE_F4 && (event->key.mod & SDL_KMOD_ALT)) {
			movie.result = DTTR_MOVIE_QUIT;
			return true;
		}
	}

	if (event->type == SDL_EVENT_GAMEPAD_BUTTON_DOWN) {
		movie.result = DTTR_MOVIE_ENDED;
		return true;
	}

	if (event->type == SDL_EVENT_QUIT) {
		movie.result = DTTR_MOVIE_QUIT;
		return true;
	}

	return false;
}

// Stops playback, releases per-movie resources, and returns the final movie result.
DTTR_MovieResult DTTR_Movies_Stop() {
	const DTTR_MovieResult result = movie.result;
	close_movie();
	DTTR_LOG_INFO("Stopped movie with result %d", result);
	return result;
}

// Reports whether the blocking movie hook should keep pumping events and decoding frames.
bool DTTR_Movies_MovieIsPlaying() { return movie.result == DTTR_MOVIE_PLAYING; }

// Runs replacement movie playback to completion while pumping sidecar events.
int32_t __cdecl dttr_movies_hook_movie_play_file_callback(
	const char *path,
	const int32_t use_alt_rect
) {
	DTTR_Movies_Start(path);

	while (DTTR_Movies_MovieIsPlaying()) {
		dttr_sidecar_poll_sdl_events();
		DTTR_Movies_Tick();
	}

	return DTTR_Movies_Stop();
}
