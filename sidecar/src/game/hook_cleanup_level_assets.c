#include "dttr_hooks_game.h"

void __cdecl dttr_hook_cleanup_level_assets_callback(void) {
	((void(__cdecl *)(void))dttr_hook_cleanup_level_assets_trampoline)();

	g_pcdogs_level_asset_0_set(NULL);
	g_pcdogs_level_asset_1_set(NULL);
	g_pcdogs_level_asset_2_set(NULL);
	g_pcdogs_level_asset_3_set(NULL);
	g_pcdogs_level_asset_4_set(NULL);
}
