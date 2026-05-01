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
	char m_label[64];
	char m_path[260];
} DTTR_LoaderUIDiscCandidate;

DTTR_LoaderUIChoice dttr_loader_ui_choice_from_id(int choice_id);
bool dttr_loader_ui_choice_is_browse(DTTR_LoaderUIChoice choice);
DTTR_LoaderUIChoice dttr_loader_ui_disc_choice_for_index(size_t index);
bool dttr_loader_ui_choice_is_disc(DTTR_LoaderUIChoice choice, size_t *out_index);
DTTR_LoaderUIChoice dttr_loader_ui_choose_game_source(
	const DTTR_LoaderUIDiscCandidate *disc_candidates,
	size_t disc_candidate_count
);
void dttr_loader_ui_show_error(const char *title, const char *message);

#ifdef __cplusplus
}
#endif

#endif /* DTTR_LOADER_UI_H */
