#ifndef DTTR_SIDECAR_EVENTS_PRIVATE_H
#define DTTR_SIDECAR_EVENTS_PRIVATE_H

#include <SDL3/SDL.h>

// Routes SDL events through sidecar handlers before game input observes them.
void dttr_sidecar_handle_sdl_event(const SDL_Event *event);
// Drains SDL events through the sidecar event bridge.
void dttr_sidecar_poll_sdl_events();

#endif // DTTR_SIDECAR_EVENTS_PRIVATE_H
