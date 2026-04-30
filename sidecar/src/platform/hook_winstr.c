#include "imports_internal.h"

DTTR_IMPORT_HOOK_WARN_DECL(dttr_import_winstr, movie_getxsize, "winstr.dll!Movie_GetXSize")
DTTR_IMPORT_HOOK_WARN_DECL(
	dttr_import_winstr,
	movie_gettotalframes,
	"winstr.dll!Movie_GetTotalFrames"
)
DTTR_IMPORT_HOOK_WARN_DECL(
	dttr_import_winstr,
	movie_getcurrentframe,
	"winstr.dll!Movie_GetCurrentFrame"
)
DTTR_IMPORT_HOOK_WARN_DECL(dttr_import_winstr, movie_getysize, "winstr.dll!Movie_GetYSize")
DTTR_IMPORT_HOOK_WARN_DECL(
	dttr_import_winstr,
	movie_getsoundchannels,
	"winstr.dll!Movie_GetSoundChannels"
)
DTTR_IMPORT_HOOK_WARN_DECL(
	dttr_import_winstr,
	movie_getsoundprecision,
	"winstr.dll!Movie_GetSoundPrecision"
)
DTTR_IMPORT_HOOK_WARN_DECL(
	dttr_import_winstr,
	movie_getsoundrate,
	"winstr.dll!Movie_GetSoundRate"
)
DTTR_IMPORT_HOOK_WARN_DECL(
	dttr_import_winstr,
	movie_setsyncadjust,
	"winstr.dll!Movie_SetSyncAdjust"
)

static const DTTR_ImportHookSpec s_winstr_hooks[] = {
	DTTR_IMPORT_HOOK_SPEC(dttr_import_winstr, movie_getxsize, "Movie_GetXSize"),
	DTTR_IMPORT_HOOK_SPEC(
		dttr_import_winstr,
		movie_gettotalframes,
		"Movie_GetTotalFrames"
	),
	DTTR_IMPORT_HOOK_SPEC(
		dttr_import_winstr,
		movie_getcurrentframe,
		"Movie_GetCurrentFrame"
	),
	DTTR_IMPORT_HOOK_SPEC(dttr_import_winstr, movie_getysize, "Movie_GetYSize"),
	DTTR_IMPORT_HOOK_SPEC(
		dttr_import_winstr,
		movie_getsoundchannels,
		"Movie_GetSoundChannels"
	),
	DTTR_IMPORT_HOOK_SPEC(
		dttr_import_winstr,
		movie_getsoundprecision,
		"Movie_GetSoundPrecision"
	),
	DTTR_IMPORT_HOOK_SPEC(dttr_import_winstr, movie_getsoundrate, "Movie_GetSoundRate"),
	DTTR_IMPORT_HOOK_SPEC(dttr_import_winstr, movie_setsyncadjust, "Movie_SetSyncAdjust")
};

void dttr_platform_hooks_winstr_init(const DTTR_ComponentContext *ctx) {
	dttr_platform_hooks_install_module(
		ctx,
		"winstr.dll",
		s_winstr_hooks,
		sizeof(s_winstr_hooks) / sizeof(s_winstr_hooks[0])
	);
}

void dttr_platform_hooks_winstr_cleanup(const DTTR_ComponentContext *ctx) {
	dttr_platform_hooks_cleanup_module(
		ctx,
		s_winstr_hooks,
		sizeof(s_winstr_hooks) / sizeof(s_winstr_hooks[0])
	);
}
