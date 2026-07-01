#ifndef DTTR_INPUT_NAMES_H
#define DTTR_INPUT_NAMES_H

#include <stdbool.h>
#include <stddef.h>

#include <SDL3/SDL.h>

bool DTTR_InputNames_GamepadButton(int button, char *out, size_t out_size);
bool DTTR_InputNames_ControlCode(int code, char *out, size_t out_size);

#endif // DTTR_INPUT_NAMES_H
