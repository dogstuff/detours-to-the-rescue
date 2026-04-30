#include "imports_internal.h"

#define S_WINSTR_IMPORTS(X)                                                              \
	X(WARN,                                                                              \
	  dttr_import_winstr,                                                                \
	  movie_getxsize,                                                                    \
	  "Movie_GetXSize",                                                                  \
	  "winstr.dll!Movie_GetXSize")                                                       \
	X(WARN,                                                                              \
	  dttr_import_winstr,                                                                \
	  movie_gettotalframes,                                                              \
	  "Movie_GetTotalFrames",                                                            \
	  "winstr.dll!Movie_GetTotalFrames")                                                 \
	X(WARN,                                                                              \
	  dttr_import_winstr,                                                                \
	  movie_getcurrentframe,                                                             \
	  "Movie_GetCurrentFrame",                                                           \
	  "winstr.dll!Movie_GetCurrentFrame")                                                \
	X(WARN,                                                                              \
	  dttr_import_winstr,                                                                \
	  movie_getysize,                                                                    \
	  "Movie_GetYSize",                                                                  \
	  "winstr.dll!Movie_GetYSize")                                                       \
	X(WARN,                                                                              \
	  dttr_import_winstr,                                                                \
	  movie_getsoundchannels,                                                            \
	  "Movie_GetSoundChannels",                                                          \
	  "winstr.dll!Movie_GetSoundChannels")                                               \
	X(WARN,                                                                              \
	  dttr_import_winstr,                                                                \
	  movie_getsoundprecision,                                                           \
	  "Movie_GetSoundPrecision",                                                         \
	  "winstr.dll!Movie_GetSoundPrecision")                                              \
	X(WARN,                                                                              \
	  dttr_import_winstr,                                                                \
	  movie_getsoundrate,                                                                \
	  "Movie_GetSoundRate",                                                              \
	  "winstr.dll!Movie_GetSoundRate")                                                   \
	X(WARN,                                                                              \
	  dttr_import_winstr,                                                                \
	  movie_setsyncadjust,                                                               \
	  "Movie_SetSyncAdjust",                                                             \
	  "winstr.dll!Movie_SetSyncAdjust")

S_WINSTR_IMPORTS(DTTR_IMPORT_ENTRY_DECL)

static const DTTR_ImportHookSpec s_winstr_hooks[] = {
	S_WINSTR_IMPORTS(DTTR_IMPORT_ENTRY_SPEC)
};

void dttr_platform_hooks_winstr_init(const DTTR_ComponentContext *ctx) {
	dttr_platform_hooks_install_module(
		ctx,
		"winstr.dll",
		s_winstr_hooks,
		DTTR_IMPORT_ARRAY_COUNT(s_winstr_hooks)
	);
}

void dttr_platform_hooks_winstr_cleanup(const DTTR_ComponentContext *ctx) {
	dttr_platform_hooks_cleanup_module(
		ctx,
		s_winstr_hooks,
		DTTR_IMPORT_ARRAY_COUNT(s_winstr_hooks)
	);
}

#undef S_WINSTR_IMPORTS
