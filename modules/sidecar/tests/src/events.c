#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <SDL3/SDL.h>

#include "events_private.h"
#include "graphics/graphics_private.h"

#define EVENT_CAPACITY 8

DTTR_BackendState dttr_backend;

static SDL_Event queued_events[EVENT_CAPACITY];
static size_t queued_event_count;
static size_t next_event;
static SDL_Window *host_window;
static SDL_WindowID host_window_id;
static bool poll_drained;
static bool movies_consume_events;
static int movie_handler_calls;
static int controls_handler_calls;
static int resize_calls;
static int resize_width;
static int resize_height;
static bool resize_after_drain;

bool SDLCALL __wrap_SDL_PollEvent(SDL_Event *event) {
	if (next_event == queued_event_count) {
		poll_drained = true;
		return false;
	}

	*event = queued_events[next_event++];
	return true;
}

SDL_WindowID SDLCALL __wrap_SDL_GetWindowID(SDL_Window *window) {
	return window == host_window ? host_window_id : 0;
}

SDL_Window *dttr_graphics_get_window() {
	return host_window;
}

void dttr_graphics_handle_window_resize(int width, int height) {
	resize_calls++;
	resize_width = width;
	resize_height = height;
	resize_after_drain = poll_drained;
}

bool dttr_movies_handle_event(const SDL_Event *) {
	movie_handler_calls++;
	return movies_consume_events;
}

void dttr_inputs_controls_menu_handle_event(const SDL_Event *) {
	controls_handler_calls++;
}

void dttr_inputs_handle_device_event(const SDL_Event *) {}

static void queue_window_event(Uint32 type, SDL_WindowID window_id, int width, int height) {
	assert_true(queued_event_count < EVENT_CAPACITY);
	queued_events[queued_event_count++] = (SDL_Event){
		.window = {
			.type = type,
			.windowID = window_id,
			.data1 = width,
			.data2 = height,
		},
	};
}

static int reset_event_loop(void **) {
	queued_event_count = 0;
	next_event = 0;
	host_window = (SDL_Window *)(uintptr_t)1;
	host_window_id = 42;
	poll_drained = false;
	movies_consume_events = false;
	movie_handler_calls = 0;
	controls_handler_calls = 0;
	resize_calls = 0;
	resize_width = 0;
	resize_height = 0;
	resize_after_drain = false;
	return 0;
}

static void non_pixel_and_foreign_window_events_do_not_resize(void **) {
	queue_window_event(SDL_EVENT_WINDOW_RESIZED, host_window_id, 1024, 768);
	queue_window_event(SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED, 99, 1600, 900);

	dttr_sidecar_poll_sdl_events();

	assert_int_equal(movie_handler_calls, 2);
	assert_int_equal(controls_handler_calls, 2);
	assert_int_equal(resize_calls, 0);
}

static void host_pixel_events_coalesce_after_queue_drain_when_consumed(void **) {
	movies_consume_events = true;
	queue_window_event(SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED, host_window_id, 800, 600);
	queue_window_event(SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED, host_window_id, 1280, 720);

	dttr_sidecar_poll_sdl_events();

	assert_int_equal(movie_handler_calls, 2);
	assert_int_equal(controls_handler_calls, 0);
	assert_int_equal(resize_calls, 1);
	assert_int_equal(resize_width, 1280);
	assert_int_equal(resize_height, 720);
	assert_true(resize_after_drain);
}

static void missing_host_window_ignores_pixel_resize(void **) {
	host_window = NULL;
	queue_window_event(SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED, host_window_id, 1280, 720);

	dttr_sidecar_poll_sdl_events();

	assert_int_equal(movie_handler_calls, 1);
	assert_int_equal(controls_handler_calls, 1);
	assert_int_equal(resize_calls, 0);
}

static void unidentified_host_window_ignores_pixel_resize(void **) {
	host_window_id = 0;
	queue_window_event(SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED, 0, 1280, 720);

	dttr_sidecar_poll_sdl_events();

	assert_int_equal(movie_handler_calls, 1);
	assert_int_equal(controls_handler_calls, 1);
	assert_int_equal(resize_calls, 0);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test_setup(
			non_pixel_and_foreign_window_events_do_not_resize,
			reset_event_loop
		),
		cmocka_unit_test_setup(
			host_pixel_events_coalesce_after_queue_drain_when_consumed,
			reset_event_loop
		),
		cmocka_unit_test_setup(missing_host_window_ignores_pixel_resize, reset_event_loop),
		cmocka_unit_test_setup(
			unidentified_host_window_ignores_pixel_resize,
			reset_event_loop
		),
	};

	return cmocka_run_group_tests(tests, NULL, NULL);
}
