#include <dttr_errors.h>
#include <dttr_sdl.h>

static DTTR_ErrorMessageHandler message_handler;

void DTTR_Errors_SetMessageHandler(DTTR_ErrorMessageHandler handler) {
	message_handler = handler;
}

void DTTR_Errors_ShowMessage(const char *title, const char *message) {
	if (!message_handler) {
		DTTR_SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, title, message, NULL);
		return;
	}

	message_handler(title, message);
}
