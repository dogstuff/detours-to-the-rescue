#include "imports_internal.h"

#define S_WINPLAY_IMPORTS(X)                                                             \
	X(WARN,                                                                              \
	  dttr_import_winplay,                                                               \
	  player_shutdownsound,                                                              \
	  "Player_ShutDownSound",                                                            \
	  "winplay.dll!Player_ShutDownSound")                                                \
	X(WARN,                                                                              \
	  dttr_import_winplay,                                                               \
	  player_shutdownvideosystem,                                                        \
	  "Player_ShutDownVideoSystem",                                                      \
	  "winplay.dll!Player_ShutDownVideoSystem")                                          \
	X(WARN,                                                                              \
	  dttr_import_winplay,                                                               \
	  player_shutdownsoundsystem,                                                        \
	  "Player_ShutDownSoundSystem",                                                      \
	  "winplay.dll!Player_ShutDownSoundSystem")                                          \
	X(WARN,                                                                              \
	  dttr_import_winplay,                                                               \
	  player_playframe,                                                                  \
	  "Player_PlayFrame",                                                                \
	  "winplay.dll!Player_PlayFrame")                                                    \
	X(WARN,                                                                              \
	  dttr_import_winplay,                                                               \
	  player_initmovie,                                                                  \
	  "Player_InitMovie",                                                                \
	  "winplay.dll!Player_InitMovie")                                                    \
	X(WARN,                                                                              \
	  dttr_import_winplay,                                                               \
	  player_initvideo,                                                                  \
	  "Player_InitVideo",                                                                \
	  "winplay.dll!Player_InitVideo")                                                    \
	X(WARN,                                                                              \
	  dttr_import_winplay,                                                               \
	  player_initplaybackmode,                                                           \
	  "Player_InitPlaybackMode",                                                         \
	  "winplay.dll!Player_InitPlaybackMode")                                             \
	X(WARN,                                                                              \
	  dttr_import_winplay,                                                               \
	  player_initsound,                                                                  \
	  "Player_InitSound",                                                                \
	  "winplay.dll!Player_InitSound")                                                    \
	X(WARN,                                                                              \
	  dttr_import_winplay,                                                               \
	  player_initmovieplayback,                                                          \
	  "Player_InitMoviePlayback",                                                        \
	  "winplay.dll!Player_InitMoviePlayback")                                            \
	X(WARN,                                                                              \
	  dttr_import_winplay,                                                               \
	  player_mapvideo,                                                                   \
	  "Player_MapVideo",                                                                 \
	  "winplay.dll!Player_MapVideo")                                                     \
	X(WARN,                                                                              \
	  dttr_import_winplay,                                                               \
	  player_starttimer,                                                                 \
	  "Player_StartTimer",                                                               \
	  "winplay.dll!Player_StartTimer")                                                   \
	X(WARN,                                                                              \
	  dttr_import_winplay,                                                               \
	  player_stoptimer,                                                                  \
	  "Player_StopTimer",                                                                \
	  "winplay.dll!Player_StopTimer")                                                    \
	X(WARN,                                                                              \
	  dttr_import_winplay,                                                               \
	  player_returnplaybackmode,                                                         \
	  "Player_ReturnPlaybackMode",                                                       \
	  "winplay.dll!Player_ReturnPlaybackMode")                                           \
	X(WARN,                                                                              \
	  dttr_import_winplay,                                                               \
	  player_shutdownvideo,                                                              \
	  "Player_ShutDownVideo",                                                            \
	  "winplay.dll!Player_ShutDownVideo")                                                \
	X(WARN,                                                                              \
	  dttr_import_winplay,                                                               \
	  player_shutdownmovie,                                                              \
	  "Player_ShutDownMovie",                                                            \
	  "winplay.dll!Player_ShutDownMovie")                                                \
	X(WARN,                                                                              \
	  dttr_import_winplay,                                                               \
	  player_initvideosystem,                                                            \
	  "Player_InitVideoSystem",                                                          \
	  "winplay.dll!Player_InitVideoSystem")                                              \
	X(WARN,                                                                              \
	  dttr_import_winplay,                                                               \
	  player_initsoundsystem,                                                            \
	  "Player_InitSoundSystem",                                                          \
	  "winplay.dll!Player_InitSoundSystem")

S_WINPLAY_IMPORTS(DTTR_IMPORT_ENTRY_DECL)

static const DTTR_ImportHookSpec s_winplay_hooks[] = {
	S_WINPLAY_IMPORTS(DTTR_IMPORT_ENTRY_SPEC)
};

void dttr_platform_hooks_winplay_init(const DTTR_ComponentContext *ctx) {
	dttr_platform_hooks_install_module(
		ctx,
		"winplay.dll",
		s_winplay_hooks,
		DTTR_IMPORT_ARRAY_COUNT(s_winplay_hooks)
	);
}

void dttr_platform_hooks_winplay_cleanup(const DTTR_ComponentContext *ctx) {
	dttr_platform_hooks_cleanup_module(
		ctx,
		s_winplay_hooks,
		DTTR_IMPORT_ARRAY_COUNT(s_winplay_hooks)
	);
}

#undef S_WINPLAY_IMPORTS
