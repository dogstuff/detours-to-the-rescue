#ifndef DTTR_HOOKS_MOVIES_H
#define DTTR_HOOKS_MOVIES_H

#include <dttr_interop.h>
#include <stdint.h>
#include <windows.h>

#include "dttr_interop_pcdogs.h"

/// Hooks Movie_PlayFile to use mpv instead of MCI
int32_t __cdecl dttr_movies_hook_movie_play_file_callback(
	const char *path,
	int32_t use_alt_rect
);

// clang-format off
DTTR_INTEROP_HOOK_FUNC_SIG(
	dttr_movies_hook_movie_play_file,
	"\x8B\x44\x24\x08\x8B\x0D????\x8B\x54\x24\x04\x56\x50",
	"xxxxxx????xxxxxx",
	match,
	dttr_movies_hook_movie_play_file_callback
)
// clang-format on

#endif /* DTTR_HOOKS_MOVIES_H */
