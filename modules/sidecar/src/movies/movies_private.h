#ifndef DTTR_MOVIES_PRIVATE_H
#define DTTR_MOVIES_PRIVATE_H

#include <stdbool.h>
#include <stdint.h>

#include <SDL3/SDL.h>

typedef enum {
	DTTR_MOVIE_PLAYING = 0,
	DTTR_MOVIE_ENDED = 1,
	DTTR_MOVIE_ESCAPE = 2,
	DTTR_MOVIE_QUIT = 3,
} dttr_movie_result;

void dttr_movies_init();
void dttr_movies_cleanup();
void dttr_movies_start(const char *path);
void dttr_movies_tick();
bool dttr_movies_handle_event(const SDL_Event *event);
dttr_movie_result dttr_movies_stop();
bool dttr_movies_is_playing();

#endif // DTTR_MOVIES_PRIVATE_H
