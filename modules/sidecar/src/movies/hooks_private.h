#ifndef DTTR_MOVIES_HOOKS_PRIVATE_H
#define DTTR_MOVIES_HOOKS_PRIVATE_H

#include <stdint.h>

#include <dttr_mods.h>

bool dttr_movies_hooks_init(const DTTR_Mods_Context *ctx);
void dttr_movies_hooks_cleanup(const DTTR_Mods_Context *ctx);

int32_t __cdecl dttr_movies_hook_movie_play_file_callback(
	const char *path,
	int32_t use_alt_rect
);

#endif // DTTR_MOVIES_HOOKS_PRIVATE_H
