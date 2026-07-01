#include "events_private.h"

#include <SDL3/SDL.h>

#include <dttr_log.h>
#include <dttr_pcdogs.h>

#include "audio/hooks_private.h"
#include "graphics/graphics_private.h"
#include "inputs/inputs_private.h"
#include "movies/movies_private.h"
#include "sidecar_private.h"

#ifdef DTTR_MODS_ENABLED
#include "graphics/imgui_overlay_private.h"
#include "mods/mods_private.h"
#endif

// Applies the runtime fullscreen toggle to the active graphics window.
static void toggle_fullscreen() {
	SDL_Window *window = dttr_backend.window;
	const bool is_fullscreen = (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN) != 0;
	if (!SDL_SetWindowFullscreen(window, !is_fullscreen)) {
		DTTR_LOG_WARN("SDL_SetWindowFullscreen failed: %s", SDL_GetError());
	}
}

#ifdef DTTR_MODS_ENABLED
// Sends post-dispatch SDL events to mods after the sidecar and game handlers run.
static void after_sdl_event(const SDL_Event *event, bool consumed) {
	dttr_mods_after_event(event, consumed);
}
#else
#define after_sdl_event(event, consumed)                                                 \
	do {                                                                                 \
	} while (0)
#endif

// Routes SDL events through sidecar handlers before game input observes them.
void dttr_sidecar_handle_sdl_event(const SDL_Event *event) {
#ifdef DTTR_MODS_ENABLED
	if (dttr_mods_before_event(event)) {
		after_sdl_event(event, true);
		return;
	}

	if (dttr_imgui_process_event(event)) {
		after_sdl_event(event, true);
		return;
	}

	if (dttr_mods_handle_event(event)) {
		after_sdl_event(event, true);
		return;
	}

#endif

	if (dttr_movies_handle_event(event)) {
		after_sdl_event(event, true);
		return;
	}

	dttr_inputs_controls_menu_handle_event(event);

	switch (event->type) {
	case SDL_EVENT_QUIT:
		REQUIRE_PCDOGS_CALL(
			DTTR_PCDOGS_D_Input_ProcessWindowMessages_ShouldQuit->Write(1)
		);
		after_sdl_event(event, true);
		return;

	case SDL_EVENT_GAMEPAD_ADDED:
	case SDL_EVENT_GAMEPAD_REMOVED:
	case SDL_EVENT_JOYSTICK_ADDED:
	case SDL_EVENT_JOYSTICK_REMOVED:
		dttr_inputs_handle_device_event(event);
		after_sdl_event(event, true);
		return;

	case SDL_EVENT_AUDIO_DEVICE_ADDED:
	case SDL_EVENT_AUDIO_DEVICE_REMOVED:
		dttr_audio_handle_device_event(event);
		after_sdl_event(event, true);
		return;

	case SDL_EVENT_KEY_DOWN:
		if (event->key.scancode == SDL_SCANCODE_F11) {
			toggle_fullscreen();
			after_sdl_event(event, true);
			return;
		}

		break;

	case SDL_EVENT_WINDOW_RESIZED:
	case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
		dttr_graphics_handle_window_resize(event->window.data1, event->window.data2);
		after_sdl_event(event, true);
		return;

	default:
		break;
	}

	after_sdl_event(event, false);
}

// Drains SDL events through the sidecar event bridge.
void dttr_sidecar_poll_sdl_events() {
	SDL_Event event;

	while (SDL_PollEvent(&event)) {
		dttr_sidecar_handle_sdl_event(&event);
	}
}
