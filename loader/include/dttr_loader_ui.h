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

typedef enum DTTR_LoaderUiChoice {
	DTTR_LOADER_UI_CHOICE_EXIT = 0,
	DTTR_LOADER_UI_CHOICE_BROWSE_FOLDER = 1,
	DTTR_LOADER_UI_CHOICE_BROWSE_ISO = 2,
} DTTR_LoaderUiChoice;

typedef struct DTTR_LoaderUiDiscCandidate {
	char m_label[64];
	char m_path[260];
} DTTR_LoaderUiDiscCandidate;

DTTR_LoaderUiChoice dttr_loader_ui_choice_from_id(int choice_id);
DTTR_LoaderUiChoice dttr_loader_ui_disc_choice_for_index(size_t index);
bool dttr_loader_ui_choice_is_disc(DTTR_LoaderUiChoice choice, size_t *out_index);
DTTR_LoaderUiChoice dttr_loader_ui_choose_game_source(
	const DTTR_LoaderUiDiscCandidate *disc_candidates,
	size_t disc_candidate_count
);
void dttr_loader_ui_show_error(const char *title, const char *message);

#ifdef __cplusplus
}
#endif

#endif /* DTTR_LOADER_UI_H */
