#include "game_api_private.h"

#include "dttr_sidecar.h"
#include "hook_registry_private.h"
#include <dttr_components.h>
#include <dttr_log.h>

static const DTTR_ComponentAPI s_api = {
	.m_log = dttr_log,
	.m_log_is_enabled = dttr_log_is_enabled,
	.m_log_unchecked = dttr_log_unchecked,
};

static const DTTR_ComponentGameAPI s_game_api = {
	.m_sigscan = dttr_hook_cached_sigscan,
	.m_hook_function = dttr_hook_attach_function,
	.m_hook_pointer = dttr_hook_attach_pointer,
	.m_patch_bytes = dttr_hook_patch_bytes,
	.m_unhook = dttr_hook_detach,
};

static DTTR_ComponentContext s_ctx;

void dttr_game_api_init(HMODULE game_module, HMODULE sidecar_module) {
	s_ctx = (DTTR_ComponentContext){
		.m_api_version = DTTR_COMPONENT_API_VERSION,
		.m_game_module = game_module,
		.m_sidecar_module = sidecar_module,
		.m_window = NULL,
		.m_loader_dir = g_dttr_loader_dir,
		.m_exe_hash = g_dttr_exe_hash,
		.m_config = &g_dttr_config,
		.m_api = &s_api,
		.m_game_api = &s_game_api,
	};
}

void dttr_game_api_cleanup(void) { dttr_hook_cleanup_all(); }

const DTTR_ComponentContext *dttr_game_api_get_ctx(void) { return &s_ctx; }
