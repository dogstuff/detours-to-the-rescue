#ifndef DTTR_SDL_H
#define DTTR_SDL_H

#include <stdbool.h>

#include <SDL3/SDL.h>

bool dttr_sdl_show_simple_message_box(
	SDL_MessageBoxFlags flags,
	const char *title,
	const char *message,
	SDL_Window *window
);
bool dttr_sdl_show_message_box(const SDL_MessageBoxData *messageboxdata, int *buttonid);
void dttr_sdl_show_open_folder_dialog(
	SDL_DialogFileCallback callback,
	void *userdata,
	SDL_Window *window,
	const char *default_location,
	bool allow_many
);
void dttr_sdl_show_open_file_dialog(
	SDL_DialogFileCallback callback,
	void *userdata,
	SDL_Window *window,
	const SDL_DialogFileFilter *filters,
	int nfilters,
	const char *default_location,
	bool allow_many
);
void dttr_sdl_pump_events(void);
void dttr_sdl_delay(Uint32 ms);

#endif /* DTTR_SDL_H */
