#ifndef DTTR_SDL_H
#define DTTR_SDL_H

#include <stdbool.h>

#include <SDL3/SDL.h>

bool DTTR_SDL_ShowSimpleMessageBox(
	SDL_MessageBoxFlags flags,
	const char *title,
	const char *message,
	SDL_Window *window
);
bool DTTR_SDL_ShowMessageBox(const SDL_MessageBoxData *messageboxdata, int *buttonid);
void DTTR_SDL_ShowOpenFolderDialog(
	SDL_DialogFileCallback callback,
	void *userdata,
	SDL_Window *window,
	const char *default_location,
	bool allow_many
);
void DTTR_SDL_ShowOpenFileDialog(
	SDL_DialogFileCallback callback,
	void *userdata,
	SDL_Window *window,
	const SDL_DialogFileFilter *filters,
	int nfilters,
	const char *default_location,
	bool allow_many
);
void DTTR_SDL_PumpEvents();
void DTTR_SDL_Delay(Uint32 ms);

#endif // DTTR_SDL_H
