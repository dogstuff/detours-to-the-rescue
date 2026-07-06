#ifndef DTTR_MOVIES_HOOKS_PRIVATE_H
#define DTTR_MOVIES_HOOKS_PRIVATE_H

#include <stdint.h>

#include <dttr_mods.h>

// Routes movie playback through the sidecar movie player.
int32_t __cdecl dttr_movies_hook_movie_play_file_callback(
	const char *path,
	int32_t use_alt_rect
);

// This declaration installs the movie hook group.
bool dttr_movies_hooks_init(const DTTR_Mods_Context *ctx);

// This declaration releases the movie hook group.
void dttr_movies_hooks_cleanup(const DTTR_Mods_Context *ctx);

#endif // DTTR_MOVIES_HOOKS_PRIVATE_H
