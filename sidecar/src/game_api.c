#include "game_api_internal.h"

#include "dttr_sidecar.h"
#include "hook_registry_internal.h"
#include <dttr_components.h>
#include <log.h>

static const DTTR_ComponentAPI s_api = {
	.m_log = log_log,
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
	s_ctx.m_api_version = DTTR_COMPONENT_API_VERSION;
	s_ctx.m_game_module = game_module;
	s_ctx.m_sidecar_module = sidecar_module;
	s_ctx.m_window = NULL;
	s_ctx.m_gpu_device = NULL;
	s_ctx.m_loader_dir = g_dttr_loader_dir;
	s_ctx.m_exe_hash = g_dttr_exe_hash;
	s_ctx.m_config = &g_dttr_config;
	s_ctx.m_api = &s_api;
	s_ctx.m_game_api = &s_game_api;
}

void dttr_game_api_cleanup(void) { dttr_hook_cleanup_all(); }

const DTTR_ComponentContext *dttr_game_api_get_ctx(void) { return &s_ctx; }
