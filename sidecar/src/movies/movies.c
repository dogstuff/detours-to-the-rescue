#include "dttr_hooks_movies.h"

#include "dttr_sidecar.h"
#include "log.h"
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

#define S_AUDIO_CHANNELS 2
#define S_AUDIO_FORMAT AV_SAMPLE_FMT_S16
#define S_AUDIO_QUEUE_LIMIT_MS 500
#define S_AUDIO_DRAIN_LIMIT_MS 750

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
} S_MovieState;

static S_MovieState s_movie = {
	.video_stream = -1,
	.audio_stream_index = -1,
	.frame_duration = 1.0 / 15.0,
	.result = DTTR_MOVIE_ENDED,
};
static AVFrame *s_video_frame = NULL;
static AVFrame *s_audio_frame = NULL;
static AVPacket *s_packet = NULL;

static void s_reset_movie_state(DTTR_MovieResult result) {
	s_movie = (S_MovieState){
		.video_stream = -1,
		.audio_stream_index = -1,
		.frame_duration = 1.0 / 15.0,
		.result = result,
	};
}

static const char *s_av_error(const int err) {
	static char buf[AV_ERROR_MAX_STRING_SIZE];
	av_strerror(err, buf, sizeof(buf));
	return buf;
}

static void s_reset_video_buffer(void) {
	free(s_movie.buffer);
	s_movie.buffer = NULL;
	s_movie.buf_w = 0;
	s_movie.buf_h = 0;
	s_movie.buf_stride = 0;
	s_movie.video_frame_ready = false;
}

static void s_close_audio(void) {
	if (s_movie.audio_stream) {
		SDL_DestroyAudioStream(s_movie.audio_stream);
		s_movie.audio_stream = NULL;
	}

	swr_free(&s_movie.swr);
}

static void s_close_movie(void) {
	const DTTR_MovieResult result = s_movie.result;
	s_close_audio();
	sws_freeContext(s_movie.sws);
	avcodec_free_context(&s_movie.video_codec);
	avcodec_free_context(&s_movie.audio_codec);
	avformat_close_input(&s_movie.format);
	s_reset_video_buffer();
	s_reset_movie_state(result);
}

static AVCodecContext *s_open_codec(const AVStream *stream) {
	const AVCodec *codec = avcodec_find_decoder(stream->codecpar->codec_id);
	if (!codec) {
		log_error("Missing movie decoder for codec id %d", stream->codecpar->codec_id);
		return NULL;
	}

	AVCodecContext *ctx = avcodec_alloc_context3(codec);
	if (!ctx) {
		log_error("Failed to allocate movie decoder context");
		return NULL;
	}

	int err = avcodec_parameters_to_context(ctx, stream->codecpar);
	if (err < 0) {
		log_error("Failed to configure movie decoder: %s", s_av_error(err));
		avcodec_free_context(&ctx);
		return NULL;
	}

	err = avcodec_open2(ctx, codec, NULL);
	if (err < 0) {
		log_error("Failed to open movie decoder %s: %s", codec->name, s_av_error(err));
		avcodec_free_context(&ctx);
		return NULL;
	}

	return ctx;
}

static bool s_prepare_audio(void) {
	if (!s_movie.audio_codec) {
		return true;
	}

	AVChannelLayout out_layout;
	av_channel_layout_default(&out_layout, S_AUDIO_CHANNELS);

	const int err = swr_alloc_set_opts2(
		&s_movie.swr,
		&out_layout,
		S_AUDIO_FORMAT,
		s_movie.audio_codec->sample_rate,
		&s_movie.audio_codec->ch_layout,
		s_movie.audio_codec->sample_fmt,
		s_movie.audio_codec->sample_rate,
		0,
		NULL
	);
	av_channel_layout_uninit(&out_layout);

	if (err < 0 || !s_movie.swr) {
		log_error("Failed to allocate movie audio resampler: %s", s_av_error(err));
		return false;
	}

	const int init_err = swr_init(s_movie.swr);
	if (init_err < 0) {
		log_error("Failed to initialize movie audio resampler: %s", s_av_error(init_err));
		return false;
	}

	const SDL_AudioSpec spec = {
		.format = SDL_AUDIO_S16LE,
		.channels = S_AUDIO_CHANNELS,
		.freq = s_movie.audio_codec->sample_rate,
	};
	s_movie.audio_stream = SDL_OpenAudioDeviceStream(
		SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
		&spec,
		NULL,
		NULL
	);
	if (!s_movie.audio_stream) {
		log_error("Failed to open movie audio stream: %s", SDL_GetError());
		return false;
	}

	SDL_ResumeAudioStreamDevice(s_movie.audio_stream);
	return true;
}

static bool s_open_movie(const char *path) {
	int err = avformat_open_input(&s_movie.format, path, NULL, NULL);
	if (err < 0) {
		log_error("Failed to open movie %s: %s", path, s_av_error(err));
		return false;
	}

	err = avformat_find_stream_info(s_movie.format, NULL);
	if (err < 0) {
		log_error("Failed to read movie stream info: %s", s_av_error(err));
		return false;
	}

	s_movie.video_stream = av_find_best_stream(
		s_movie.format,
		AVMEDIA_TYPE_VIDEO,
		-1,
		-1,
		NULL,
		0
	);
	if (s_movie.video_stream < 0) {
		log_error(
			"Movie has no playable video stream: %s",
			s_av_error(s_movie.video_stream)
		);
		return false;
	}

	s_movie.video_codec = s_open_codec(s_movie.format->streams[s_movie.video_stream]);
	if (!s_movie.video_codec) {
		return false;
	}

	s_movie.audio_stream_index = av_find_best_stream(
		s_movie.format,
		AVMEDIA_TYPE_AUDIO,
		-1,
		s_movie.video_stream,
		NULL,
		0
	);
	if (s_movie.audio_stream_index >= 0) {
		s_movie.audio_codec = s_open_codec(
			s_movie.format->streams[s_movie.audio_stream_index]
		);
		if (!s_movie.audio_codec || !s_prepare_audio()) {
			return false;
		}
	}

	AVRational rate = av_guess_frame_rate(
		s_movie.format,
		s_movie.format->streams[s_movie.video_stream],
		NULL
	);
	if (rate.num > 0 && rate.den > 0) {
		s_movie.frame_duration = av_q2d((AVRational){rate.den, rate.num});
	}

	return true;
}

static double s_video_pts_seconds(const int64_t pts) {
	if (pts == AV_NOPTS_VALUE) {
		return s_movie.has_timing_origin ? s_movie.last_video_pts + s_movie.frame_duration
									 : 0.0;
	}

	const AVStream *stream = s_movie.format->streams[s_movie.video_stream];
	return (double)pts * av_q2d(stream->time_base);
}

static void s_set_next_video_time(const double pts) {
	if (!s_movie.has_timing_origin) {
		s_movie.start_ticks = SDL_GetTicks();
		s_movie.pts_origin = pts;
		s_movie.has_timing_origin = true;
	}

	s_movie.next_video_time = pts - s_movie.pts_origin;
	if (s_movie.next_video_time < 0.0) {
		s_movie.next_video_time = 0.0;
	}
	s_movie.last_video_pts = pts;
}

static double s_frame_pts_seconds(const AVFrame *frame) {
	return s_video_pts_seconds(frame->best_effort_timestamp);
}

static double s_packet_pts_seconds(const AVPacket *packet) {
	return s_video_pts_seconds(packet->pts != AV_NOPTS_VALUE ? packet->pts : packet->dts);
}

static bool s_queue_video_frame(const AVFrame *frame) {
	const int w = frame->width;
	const int h = frame->height;
	if (w <= 0 || h <= 0) {
		return false;
	}

	const int stride = w * 4;
	const size_t size = (size_t)stride * (size_t)h;
	if (w != s_movie.buf_w || h != s_movie.buf_h || stride != s_movie.buf_stride) {
		uint8_t *new_buffer = realloc(s_movie.buffer, size);
		if (!new_buffer) {
			log_error("Failed to allocate %zu bytes for movie frame", size);
			return false;
		}

		s_movie.buffer = new_buffer;
		s_movie.buf_w = w;
		s_movie.buf_h = h;
		s_movie.buf_stride = stride;
		log_debug("Video Format: %dx%d", s_movie.buf_w, s_movie.buf_h);
	}

	s_movie.sws = sws_getCachedContext(
		s_movie.sws,
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
	if (!s_movie.sws) {
		log_error("Failed to create movie video converter");
		return false;
	}

	uint8_t *dst_data[] = {s_movie.buffer, NULL, NULL, NULL};
	int dst_linesize[] = {s_movie.buf_stride, 0, 0, 0};
	sws_scale(
		s_movie.sws,
		(const uint8_t *const *)frame->data,
		frame->linesize,
		0,
		h,
		dst_data,
		dst_linesize
	);

	s_set_next_video_time(s_frame_pts_seconds(frame));
	s_movie.video_frame_ready = true;
	return true;
}

static bool s_queue_audio_frame(const AVFrame *frame) {
	if (!s_movie.audio_stream || !s_movie.swr) {
		return true;
	}

	const int out_samples = (int)swr_get_delay(
								s_movie.swr,
								s_movie.audio_codec->sample_rate
							)
							+ frame->nb_samples;
	const int out_size = av_samples_get_buffer_size(
		NULL,
		S_AUDIO_CHANNELS,
		out_samples,
		S_AUDIO_FORMAT,
		1
	);
	if (out_size <= 0) {
		return false;
	}

	uint8_t *out = av_malloc((size_t)out_size);
	if (!out) {
		log_error("Failed to allocate movie audio buffer");
		return false;
	}

	uint8_t *out_planes[] = {out, NULL};
	const int converted = swr_convert(
		s_movie.swr,
		out_planes,
		out_samples,
		(const uint8_t **)frame->extended_data,
		frame->nb_samples
	);
	if (converted < 0) {
		log_error("Failed to convert movie audio: %s", s_av_error(converted));
		av_free(out);
		return false;
	}

	const int converted_size = av_samples_get_buffer_size(
		NULL,
		S_AUDIO_CHANNELS,
		converted,
		S_AUDIO_FORMAT,
		1
	);
	if (converted_size > 0) {
		SDL_PutAudioStreamData(s_movie.audio_stream, out, converted_size);
	}
	av_free(out);

	const int queue_limit = (s_movie.audio_codec->sample_rate * S_AUDIO_CHANNELS
							 * av_get_bytes_per_sample(S_AUDIO_FORMAT)
							 * S_AUDIO_QUEUE_LIMIT_MS)
							/ 1000;
	while (SDL_GetAudioStreamQueued(s_movie.audio_stream) > queue_limit) {
		SDL_Delay(1);
	}

	return true;
}

static bool s_receive_video_frame(void) {
	while (!s_movie.video_frame_ready) {
		const int err = avcodec_receive_frame(s_movie.video_codec, s_video_frame);
		if (err == AVERROR(EAGAIN) || err == AVERROR_EOF) {
			return false;
		}
		if (err < 0) {
			log_error("Failed to decode movie video: %s", s_av_error(err));
			s_movie.result = DTTR_MOVIE_ENDED;
			return false;
		}

		if (!s_queue_video_frame(s_video_frame)) {
			s_movie.result = DTTR_MOVIE_ENDED;
		}
		av_frame_unref(s_video_frame);
	}

	return true;
}

static void s_receive_audio_frames(void) {
	if (!s_movie.audio_codec) {
		return;
	}

	for (;;) {
		const int err = avcodec_receive_frame(s_movie.audio_codec, s_audio_frame);
		if (err == AVERROR(EAGAIN) || err == AVERROR_EOF) {
			return;
		}
		if (err < 0) {
			log_warn("Failed to decode movie audio: %s", s_av_error(err));
			return;
		}

		s_queue_audio_frame(s_audio_frame);
		av_frame_unref(s_audio_frame);
	}
}

static bool s_drain_eof(void) {
	if (!s_movie.video_flushed) {
		avcodec_send_packet(s_movie.video_codec, NULL);
		s_movie.video_flushed = true;
		return true;
	}

	if (s_movie.audio_codec && !s_movie.audio_flushed) {
		avcodec_send_packet(s_movie.audio_codec, NULL);
		s_movie.audio_flushed = true;
		s_receive_audio_frames();
		if (s_movie.audio_stream) {
			SDL_FlushAudioStream(s_movie.audio_stream);
			s_movie.audio_drain_deadline_ticks = SDL_GetTicks() + S_AUDIO_DRAIN_LIMIT_MS;
		}
	}

	if (s_movie.audio_stream) {
		const int queued = SDL_GetAudioStreamQueued(s_movie.audio_stream);
		if (queued > 0 && SDL_GetTicks() < s_movie.audio_drain_deadline_ticks) {
			SDL_Delay(1);
			return false;
		}

		if (queued > 0) {
			log_warn("Movie audio drain timed out with %d bytes queued", queued);
		}
	}

	s_movie.result = DTTR_MOVIE_ENDED;
	return false;
}

static void s_send_packet(void) {
	if (s_packet->stream_index == s_movie.video_stream) {
		if (s_packet->size <= 0) {
			if (s_movie.buffer) {
				s_set_next_video_time(s_packet_pts_seconds(s_packet));
				s_movie.video_frame_ready = true;
			}
			return;
		}

		const int err = avcodec_send_packet(s_movie.video_codec, s_packet);
		if (err < 0 && err != AVERROR(EAGAIN)) {
			log_error("Failed to submit movie video packet: %s", s_av_error(err));
			s_movie.result = DTTR_MOVIE_ENDED;
		}
		return;
	}

	if (s_packet->stream_index != s_movie.audio_stream_index || !s_movie.audio_codec) {
		return;
	}

	const int err = avcodec_send_packet(s_movie.audio_codec, s_packet);
	if (err < 0 && err != AVERROR(EAGAIN)) {
		log_warn("Failed to submit movie audio packet: %s", s_av_error(err));
	}
}

static bool s_decode_until_video_frame(void) {
	while (s_movie.result == DTTR_MOVIE_PLAYING && !s_movie.video_frame_ready) {
		if (s_receive_video_frame()) {
			return true;
		}
		s_receive_audio_frames();

		if (s_movie.hit_eof) {
			if (s_drain_eof()) {
				continue;
			}

			return false;
		}

		const int err = av_read_frame(s_movie.format, s_packet);
		if (err == AVERROR_EOF) {
			s_movie.hit_eof = true;
			continue;
		}
		if (err < 0) {
			log_error("Failed to read movie packet: %s", s_av_error(err));
			s_movie.result = DTTR_MOVIE_ENDED;
			return false;
		}

		s_send_packet();
		av_packet_unref(s_packet);
	}

	return s_movie.video_frame_ready;
}

void dttr_movies_init(void) {
	s_video_frame = av_frame_alloc();
	s_audio_frame = av_frame_alloc();
	s_packet = av_packet_alloc();
	if (!s_video_frame || !s_audio_frame || !s_packet) {
		log_error("Failed to allocate movie playback state");
	}
}

void dttr_movies_hooks_init(const DTTR_ComponentContext *ctx) {
	DTTR_INSTALL_JMP(
		dttr_movies_hook_movie_play_file,
		ctx,
		"\x8B\x44\x24\x08\x8B\x0D????\x8B\x54\x24\x04\x56\x50",
		"xxxxxx????xxxxxx"
	);
}

void dttr_movies_hooks_cleanup(const DTTR_ComponentContext *ctx) {
	DTTR_UNINSTALL(dttr_movies_hook_movie_play_file, ctx);
}

void dttr_movies_cleanup(void) {
	s_close_movie();
	av_packet_free(&s_packet);
	av_frame_free(&s_audio_frame);
	av_frame_free(&s_video_frame);
	log_info("Released movie playback state");
}

void dttr_movies_start(const char *path) {
	if (!s_video_frame || !s_audio_frame || !s_packet) {
		log_error("Missing movie playback state");
		s_movie.result = DTTR_MOVIE_ENDED;
		return;
	}

	s_close_movie();

	sds abs_path = sdscatprintf(sdsempty(), "%s\\%s", g_pcdogs_directory_ptr(), path);
	log_info("Playing movie %s", abs_path);

	if (!s_open_movie(abs_path)) {
		sdsfree(abs_path);
		s_close_movie();
		s_movie.result = DTTR_MOVIE_ENDED;
		return;
	}

	sdsfree(abs_path);
	s_movie.result = DTTR_MOVIE_PLAYING;
}

void dttr_movies_tick(void) {
	if (s_movie.result != DTTR_MOVIE_PLAYING) {
		return;
	}

	if (!s_movie.video_frame_ready && !s_decode_until_video_frame()) {
		return;
	}

	if (!s_movie.video_frame_ready) {
		return;
	}

	const double elapsed = (double)(SDL_GetTicks() - s_movie.start_ticks) / 1000.0;
	if (elapsed + 0.001 < s_movie.next_video_time) {
		SDL_Delay(1);
		return;
	}

	dttr_graphics_present_video_frame_bgra(
		s_movie.buffer,
		s_movie.buf_w,
		s_movie.buf_h,
		s_movie.buf_stride
	);
	s_movie.video_frame_ready = false;
}

bool dttr_movies_handle_event(const SDL_Event *event) {
	if (s_movie.result != DTTR_MOVIE_PLAYING) {
		return false;
	}

	if (event->type == SDL_EVENT_KEY_DOWN && !event->key.repeat) {
		if (event->key.scancode == SDL_SCANCODE_ESCAPE) {
			s_movie.result = DTTR_MOVIE_ESCAPE;
			return true;
		}

		if (event->key.scancode == SDL_SCANCODE_RETURN) {
			s_movie.result = DTTR_MOVIE_ENDED;
			return true;
		}

		if (event->key.scancode == SDL_SCANCODE_F4 && (event->key.mod & SDL_KMOD_ALT)) {
			s_movie.result = DTTR_MOVIE_QUIT;
			return true;
		}
	}

	if (event->type == SDL_EVENT_GAMEPAD_BUTTON_DOWN) {
		s_movie.result = DTTR_MOVIE_ENDED;
		return true;
	}

	if (event->type == SDL_EVENT_QUIT) {
		s_movie.result = DTTR_MOVIE_QUIT;
		return true;
	}

	return false;
}

DTTR_MovieResult dttr_movies_stop(void) {
	const DTTR_MovieResult result = s_movie.result;
	s_close_movie();
	log_info("Stopped movie with result %d", result);
	return result;
}

bool dttr_movies_movie_is_playing(void) { return s_movie.result == DTTR_MOVIE_PLAYING; }

// This likely is not called outside the overridden WinMain, but keep the hook safe.
int32_t __cdecl dttr_movies_hook_movie_play_file_callback(
	const char *path,
	const int32_t use_alt_rect
) {
	dttr_movies_start(path);

	while (dttr_movies_movie_is_playing()) {
		SDL_Event event;
		while (SDL_PollEvent(&event))
			dttr_movies_handle_event(&event);
		dttr_movies_tick();
	}

	return dttr_movies_stop();
}
