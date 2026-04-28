#ifndef DTTR_HOOKS_MOVIES_H
#define DTTR_HOOKS_MOVIES_H

#include <dttr_components.h>

/// Hooks Movie_PlayFile to use mpv instead of MCI.
int32_t __cdecl dttr_movies_hook_movie_play_file_callback(
	const char *path,
	int32_t use_alt_rect
);

DTTR_HOOK(dttr_movies_hook_movie_play_file)

#endif /* DTTR_HOOKS_MOVIES_H */
