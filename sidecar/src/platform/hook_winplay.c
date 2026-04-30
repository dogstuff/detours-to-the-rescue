#include "imports_internal.h"

DTTR_IMPORT_HOOK_WARN_DECL(
	dttr_import_winplay,
	player_shutdownsound,
	"winplay.dll!Player_ShutDownSound"
)
DTTR_IMPORT_HOOK_WARN_DECL(
	dttr_import_winplay,
	player_shutdownvideosystem,
	"winplay.dll!Player_ShutDownVideoSystem"
)
DTTR_IMPORT_HOOK_WARN_DECL(
	dttr_import_winplay,
	player_shutdownsoundsystem,
	"winplay.dll!Player_ShutDownSoundSystem"
)
DTTR_IMPORT_HOOK_WARN_DECL(
	dttr_import_winplay,
	player_playframe,
	"winplay.dll!Player_PlayFrame"
)
DTTR_IMPORT_HOOK_WARN_DECL(
	dttr_import_winplay,
	player_initmovie,
	"winplay.dll!Player_InitMovie"
)
DTTR_IMPORT_HOOK_WARN_DECL(
	dttr_import_winplay,
	player_initvideo,
	"winplay.dll!Player_InitVideo"
)
DTTR_IMPORT_HOOK_WARN_DECL(
	dttr_import_winplay,
	player_initplaybackmode,
	"winplay.dll!Player_InitPlaybackMode"
)
DTTR_IMPORT_HOOK_WARN_DECL(
	dttr_import_winplay,
	player_initsound,
	"winplay.dll!Player_InitSound"
)
DTTR_IMPORT_HOOK_WARN_DECL(
	dttr_import_winplay,
	player_initmovieplayback,
	"winplay.dll!Player_InitMoviePlayback"
)
DTTR_IMPORT_HOOK_WARN_DECL(
	dttr_import_winplay,
	player_mapvideo,
	"winplay.dll!Player_MapVideo"
)
DTTR_IMPORT_HOOK_WARN_DECL(
	dttr_import_winplay,
	player_starttimer,
	"winplay.dll!Player_StartTimer"
)
DTTR_IMPORT_HOOK_WARN_DECL(
	dttr_import_winplay,
	player_stoptimer,
	"winplay.dll!Player_StopTimer"
)
DTTR_IMPORT_HOOK_WARN_DECL(
	dttr_import_winplay,
	player_returnplaybackmode,
	"winplay.dll!Player_ReturnPlaybackMode"
)
DTTR_IMPORT_HOOK_WARN_DECL(
	dttr_import_winplay,
	player_shutdownvideo,
	"winplay.dll!Player_ShutDownVideo"
)
DTTR_IMPORT_HOOK_WARN_DECL(
	dttr_import_winplay,
	player_shutdownmovie,
	"winplay.dll!Player_ShutDownMovie"
)
DTTR_IMPORT_HOOK_WARN_DECL(
	dttr_import_winplay,
	player_initvideosystem,
	"winplay.dll!Player_InitVideoSystem"
)
DTTR_IMPORT_HOOK_WARN_DECL(
	dttr_import_winplay,
	player_initsoundsystem,
	"winplay.dll!Player_InitSoundSystem"
)

static const DTTR_ImportHookSpec s_winplay_hooks[] = {
	DTTR_IMPORT_HOOK_SPEC(
		dttr_import_winplay,
		player_shutdownsound,
		"Player_ShutDownSound"
	),
	DTTR_IMPORT_HOOK_SPEC(
		dttr_import_winplay,
		player_shutdownvideosystem,
		"Player_ShutDownVideoSystem"
	),
	DTTR_IMPORT_HOOK_SPEC(
		dttr_import_winplay,
		player_shutdownsoundsystem,
		"Player_ShutDownSoundSystem"
	),
	DTTR_IMPORT_HOOK_SPEC(dttr_import_winplay, player_playframe, "Player_PlayFrame"),
	DTTR_IMPORT_HOOK_SPEC(dttr_import_winplay, player_initmovie, "Player_InitMovie"),
	DTTR_IMPORT_HOOK_SPEC(dttr_import_winplay, player_initvideo, "Player_InitVideo"),
	DTTR_IMPORT_HOOK_SPEC(
		dttr_import_winplay,
		player_initplaybackmode,
		"Player_InitPlaybackMode"
	),
	DTTR_IMPORT_HOOK_SPEC(dttr_import_winplay, player_initsound, "Player_InitSound"),
	DTTR_IMPORT_HOOK_SPEC(
		dttr_import_winplay,
		player_initmovieplayback,
		"Player_InitMoviePlayback"
	),
	DTTR_IMPORT_HOOK_SPEC(dttr_import_winplay, player_mapvideo, "Player_MapVideo"),
	DTTR_IMPORT_HOOK_SPEC(dttr_import_winplay, player_starttimer, "Player_StartTimer"),
	DTTR_IMPORT_HOOK_SPEC(dttr_import_winplay, player_stoptimer, "Player_StopTimer"),
	DTTR_IMPORT_HOOK_SPEC(
		dttr_import_winplay,
		player_returnplaybackmode,
		"Player_ReturnPlaybackMode"
	),
	DTTR_IMPORT_HOOK_SPEC(
		dttr_import_winplay,
		player_shutdownvideo,
		"Player_ShutDownVideo"
	),
	DTTR_IMPORT_HOOK_SPEC(
		dttr_import_winplay,
		player_shutdownmovie,
		"Player_ShutDownMovie"
	),
	DTTR_IMPORT_HOOK_SPEC(
		dttr_import_winplay,
		player_initvideosystem,
		"Player_InitVideoSystem"
	),
	DTTR_IMPORT_HOOK_SPEC(
		dttr_import_winplay,
		player_initsoundsystem,
		"Player_InitSoundSystem"
	)
};

void dttr_platform_hooks_winplay_init(const DTTR_ComponentContext *ctx) {
	dttr_platform_hooks_install_module(
		ctx,
		"winplay.dll",
		s_winplay_hooks,
		sizeof(s_winplay_hooks) / sizeof(s_winplay_hooks[0])
	);
}

void dttr_platform_hooks_winplay_cleanup(const DTTR_ComponentContext *ctx) {
	dttr_platform_hooks_cleanup_module(
		ctx,
		s_winplay_hooks,
		sizeof(s_winplay_hooks) / sizeof(s_winplay_hooks[0])
	);
}
