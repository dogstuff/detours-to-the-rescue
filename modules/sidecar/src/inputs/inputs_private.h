#ifndef DTTR_INPUTS_PRIVATE_H
#define DTTR_INPUTS_PRIVATE_H

#include <SDL3/SDL.h>
#include <stdbool.h>
#include <stdint.h>

#define DTTR_DINPUT_AXIS_SCALE 32

extern SDL_Gamepad *dttr_inputs_gamepad;

SDL_Joystick *dttr_inputs_raw_joystick();
int32_t dttr_inputs_read_raw_axis(int axis_idx);

void dttr_inputs_init();
void dttr_inputs_handle_device_event(const SDL_Event *event);
void dttr_inputs_controls_menu_handle_event(const SDL_Event *event);
bool dttr_inputs_late_init();
void dttr_inputs_cleanup();

#endif // DTTR_INPUTS_PRIVATE_H
