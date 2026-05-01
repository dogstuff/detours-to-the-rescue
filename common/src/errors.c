#include <dttr_errors.h>
#include <dttr_sdl.h>

static DTTR_ErrorMessageHandler s_message_handler;

void dttr_errors_set_message_handler(DTTR_ErrorMessageHandler handler) {
	s_message_handler = handler;
}

void dttr_errors_show_message(const char *title, const char *message) {
	const DTTR_ErrorMessageHandler handler = s_message_handler;
	if (handler) {
		handler(title, message);
		return;
	}

	dttr_sdl_show_simple_message_box(SDL_MESSAGEBOX_ERROR, title, message, NULL);
}
