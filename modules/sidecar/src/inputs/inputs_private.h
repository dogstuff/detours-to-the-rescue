#ifndef DTTR_INPUTS_PRIVATE_H
#define DTTR_INPUTS_PRIVATE_H

#include <SDL3/SDL.h>
#include <stdbool.h>

extern SDL_Gamepad *dttr_inputs_gamepad;

void dttr_inputs_init();
void dttr_inputs_handle_device_event(const SDL_Event *event);
bool dttr_inputs_late_init();
void dttr_inputs_cleanup();

#endif // DTTR_INPUTS_PRIVATE_H
