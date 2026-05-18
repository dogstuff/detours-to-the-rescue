#ifndef DTTR_MOVIES_HOOKS_PRIVATE_H
#define DTTR_MOVIES_HOOKS_PRIVATE_H

#include <stdint.h>

#include <dttr_mods.h>

// Allocates shared FFmpeg playback objects used by movie hooks.
bool dttr_movies_hooks_init(const DTTR_Mods_Context *ctx);
// Releases shared FFmpeg playback objects during sidecar shutdown.
void dttr_movies_hooks_cleanup(const DTTR_Mods_Context *ctx);

// Replacement movie-play callback that drives FFmpeg playback on the sidecar loop.
int32_t __cdecl dttr_movies_hook_movie_play_file_callback(
	const char *path,
	int32_t use_alt_rect
);

#endif // DTTR_MOVIES_HOOKS_PRIVATE_H
