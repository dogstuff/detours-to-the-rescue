#ifndef DTTR_LOADER_UI_H
#define DTTR_LOADER_UI_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
	DTTR_LOADER_UI_MAX_DISC_CANDIDATES = 8,
	DTTR_LOADER_UI_CHOICE_DISC_BASE = 100,
};

typedef enum DTTR_LoaderUIChoice {
	DTTR_LOADER_UI_CHOICE_EXIT = 0,
	DTTR_LOADER_UI_CHOICE_BROWSE_FOLDER = 1,
	DTTR_LOADER_UI_CHOICE_BROWSE_ISO = 2,
} DTTR_LoaderUIChoice;

typedef struct DTTR_LoaderUIDiscCandidate {
	char label[64];
	char path[260];
} DTTR_LoaderUIDiscCandidate;

DTTR_LoaderUIChoice DTTR_LoaderUI_ChoiceFromID(int choice_id);
bool DTTR_LoaderUI_ChoiceIsBrowse(DTTR_LoaderUIChoice choice);
DTTR_LoaderUIChoice DTTR_LoaderUI_DiscChoiceForIndex(size_t index);
bool DTTR_LoaderUI_ChoiceIsDisc(DTTR_LoaderUIChoice choice, size_t *out_index);
DTTR_LoaderUIChoice DTTR_LoaderUI_ChooseGameSource(
	const DTTR_LoaderUIDiscCandidate *disc_candidates,
	size_t disc_candidate_count
);
void DTTR_LoaderUI_ShowError(const char *title, const char *message);

#ifdef __cplusplus
}
#endif

#endif // DTTR_LOADER_UI_H
