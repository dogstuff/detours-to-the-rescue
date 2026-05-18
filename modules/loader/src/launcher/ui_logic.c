#include <dttr_loader_ui.h>

bool DTTR_LoaderUI_ChoiceIsBrowse(DTTR_LoaderUIChoice choice) {
	return choice == DTTR_LOADER_UI_CHOICE_BROWSE_FOLDER
		   || choice == DTTR_LOADER_UI_CHOICE_BROWSE_ISO;
}

bool DTTR_LoaderUI_ChoiceIsDisc(DTTR_LoaderUIChoice choice, size_t *out_index) {
	const int index = (int)choice - DTTR_LOADER_UI_CHOICE_DISC_BASE;
	if (index < 0 || index >= DTTR_LOADER_UI_MAX_DISC_CANDIDATES) {
		return false;
	}

	if (out_index) {
		*out_index = (size_t)index;
	}
	return true;
}

DTTR_LoaderUIChoice DTTR_LoaderUI_ChoiceFromID(int choice_id) {
	const DTTR_LoaderUIChoice choice = (DTTR_LoaderUIChoice)choice_id;
	if (DTTR_LoaderUI_ChoiceIsBrowse(choice)) {
		return choice;
	}

	if (DTTR_LoaderUI_ChoiceIsDisc(choice, NULL)) {
		return choice;
	}

	return DTTR_LOADER_UI_CHOICE_EXIT;
}

DTTR_LoaderUIChoice DTTR_LoaderUI_DiscChoiceForIndex(size_t index) {
	if (index >= DTTR_LOADER_UI_MAX_DISC_CANDIDATES) {
		return DTTR_LOADER_UI_CHOICE_EXIT;
	}

	return (DTTR_LoaderUIChoice)(DTTR_LOADER_UI_CHOICE_DISC_BASE + (int)index);
}
