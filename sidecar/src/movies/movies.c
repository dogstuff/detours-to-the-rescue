#include "dttr_hooks_movies.h"

#include "dttr_sidecar.h"
#include "log.h"
#include <sds.h>

#include <SDL3/SDL.h>
#include <mpv/client.h>
#include <mpv/render.h>

#include <stdbool.h>
#include <stdlib.h>

static mpv_handle *s_mpv = NULL;
static mpv_render_context *s_mpv_render = NULL;

static uint8_t *s_buffer = NULL;
static int s_buf_w = 0;
static int s_buf_h = 0;
static DTTR_MovieResult s_result = DTTR_MOVIE_ENDED;

static void s_reset_video_buffer(void) {
	free(s_buffer);
	s_buffer = NULL;
	s_buf_w = 0;
	s_buf_h = 0;
}

void dttr_movies_init(void) {
	s_mpv = mpv_create();
	if (!s_mpv) {
		log_error("Failed to create mpv instance");
		return;
	}

	mpv_set_option_string(s_mpv, "vo", "libmpv");
	mpv_set_option_string(s_mpv, "hwdec", "no");
	mpv_set_option_string(s_mpv, "keep-open", "no");

	int err = mpv_initialize(s_mpv);
	if (err < 0) {
		log_error("Failed to initialize mpv: %s", mpv_error_string(err));
		mpv_destroy(s_mpv);
		s_mpv = NULL;
		return;
	}

	mpv_render_param params[] = {{MPV_RENDER_PARAM_API_TYPE, MPV_RENDER_API_TYPE_SW}, {0}};

	err = mpv_render_context_create(&s_mpv_render, s_mpv, params);
	if (err < 0) {
		log_error("Failed to create mpv render context: %s", mpv_error_string(err));
		mpv_terminate_destroy(s_mpv);
		s_mpv = NULL;
		return;
	}

	log_info("Successfully initialized mpv instance");
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
	if (s_mpv_render) {
		mpv_render_context_free(s_mpv_render);
		s_mpv_render = NULL;
	}

	if (s_mpv) {
		mpv_terminate_destroy(s_mpv);
		s_mpv = NULL;
		log_info("Released mpv instance");
	}
}

void dttr_movies_start(const char *path) {
	if (!s_mpv || !s_mpv_render) {
		log_error("Missing mpv instance");
		s_result = DTTR_MOVIE_ENDED;
		return;
	}

	sds abs_path = sdscatprintf(sdsempty(), "%s\\%s", g_pcdogs_directory_ptr(), path);
	log_info("Playing movie %s", abs_path);

	const char *cmd[] = {"loadfile", abs_path, NULL};
	const int err = mpv_command(s_mpv, cmd);
	if (err < 0) {
		log_error("Failed to load file %s", mpv_error_string(err));
		sdsfree(abs_path);
		s_result = DTTR_MOVIE_ENDED;
		return;
	}

	sdsfree(abs_path);

	s_result = DTTR_MOVIE_PLAYING;
	s_reset_video_buffer();
}

void dttr_movies_tick(void) {
	if (s_result != DTTR_MOVIE_PLAYING) {
		return;
	}

	if (!(mpv_render_context_update(s_mpv_render) & MPV_RENDER_UPDATE_FRAME)) {
		SDL_Delay(1);
		return;
	}

	int64_t w = 0, h = 0;
	mpv_get_property(s_mpv, "dwidth", MPV_FORMAT_INT64, &w);
	mpv_get_property(s_mpv, "dheight", MPV_FORMAT_INT64, &h);

	if (w <= 0 || h <= 0) {
		return;
	}

	if ((int)w != s_buf_w || (int)h != s_buf_h) {
		free(s_buffer);
		s_buf_w = (int)w;
		s_buf_h = (int)h;
		s_buffer = calloc((size_t)s_buf_w * s_buf_h * 4, 1);
		log_debug("Video Format: %dx%d", s_buf_w, s_buf_h);
	}

	int size[] = {s_buf_w, s_buf_h};
	int stride = s_buf_w * 4;
	mpv_render_param rp[] = {
		{MPV_RENDER_PARAM_SW_SIZE, size},
		{MPV_RENDER_PARAM_SW_FORMAT, "bgr0"},
		{MPV_RENDER_PARAM_SW_STRIDE, &stride},
		{MPV_RENDER_PARAM_SW_POINTER, s_buffer},
		{0}
	};
	mpv_render_context_render(s_mpv_render, rp);
	dttr_graphics_present_video_frame_bgra(s_buffer, s_buf_w, s_buf_h, stride);

	const mpv_event *const ev = mpv_wait_event(s_mpv, 0);
	if (ev->event_id != MPV_EVENT_END_FILE) {
		return;
	}

	const mpv_event_end_file *eof = ev->data;
	if (eof->reason == MPV_END_FILE_REASON_EOF) {
		s_result = DTTR_MOVIE_ENDED;
	}
}

bool dttr_movies_handle_event(const SDL_Event *event) {
	if (s_result != DTTR_MOVIE_PLAYING) {
		return false;
	}

	if (event->type == SDL_EVENT_KEY_DOWN && !event->key.repeat) {
		if (event->key.scancode == SDL_SCANCODE_ESCAPE) {
			s_result = DTTR_MOVIE_ESCAPE;
			return true;
		}

		if (event->key.scancode == SDL_SCANCODE_RETURN) {
			s_result = DTTR_MOVIE_ENDED;
			return true;
		}

		if (event->key.scancode == SDL_SCANCODE_F4 && (event->key.mod & SDL_KMOD_ALT)) {
			s_result = DTTR_MOVIE_QUIT;
			return true;
		}
	}

	if (event->type == SDL_EVENT_GAMEPAD_BUTTON_DOWN) {
		s_result = DTTR_MOVIE_ENDED;
		return true;
	}

	if (event->type == SDL_EVENT_QUIT) {
		s_result = DTTR_MOVIE_QUIT;
		return true;
	}

	return false;
}

DTTR_MovieResult dttr_movies_stop(void) {
	if (s_mpv) {
		mpv_command_string(s_mpv, "stop");

		// Wait for mpv to fully stop
		while (mpv_wait_event(s_mpv, 0)->event_id != MPV_EVENT_NONE)
			;
	}

	s_reset_video_buffer();

	log_info("Stopped movie with result %d", s_result);
	return s_result;
}

bool dttr_movies_movie_is_playing(void) { return s_result == DTTR_MOVIE_PLAYING; }

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
