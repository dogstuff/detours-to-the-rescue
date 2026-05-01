#include <dttr_loader_ui.h>

bool dttr_loader_ui_choice_is_browse(DTTR_LoaderUIChoice choice) {
	return choice == DTTR_LOADER_UI_CHOICE_BROWSE_FOLDER
		   || choice == DTTR_LOADER_UI_CHOICE_BROWSE_ISO;
}

bool dttr_loader_ui_choice_is_disc(DTTR_LoaderUIChoice choice, size_t *out_index) {
	const int index = (int)choice - DTTR_LOADER_UI_CHOICE_DISC_BASE;
	if (index < 0 || index >= DTTR_LOADER_UI_MAX_DISC_CANDIDATES) {
		return false;
	}

	if (out_index) {
		*out_index = (size_t)index;
	}
	return true;
}

DTTR_LoaderUIChoice dttr_loader_ui_choice_from_id(int choice_id) {
	const DTTR_LoaderUIChoice choice = (DTTR_LoaderUIChoice)choice_id;
	if (dttr_loader_ui_choice_is_browse(choice)) {
		return choice;
	}

	if (dttr_loader_ui_choice_is_disc(choice, NULL)) {
		return choice;
	}

	return DTTR_LOADER_UI_CHOICE_EXIT;
}

DTTR_LoaderUIChoice dttr_loader_ui_disc_choice_for_index(size_t index) {
	if (index >= DTTR_LOADER_UI_MAX_DISC_CANDIDATES) {
		return DTTR_LOADER_UI_CHOICE_EXIT;
	}

	return (DTTR_LoaderUIChoice)(DTTR_LOADER_UI_CHOICE_DISC_BASE + (int)index);
}
