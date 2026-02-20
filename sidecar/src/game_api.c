#include "game_api_internal.h"

#include "dttr_sidecar.h"
#include <dttr_interop.h>
#include <log.h>

#include <string.h>

static uintptr_t s_sigscan(HMODULE mod, const char *sig, const char *mask) {
	return dttr_interop_sigscan(mod, sig, mask);
}

static bool s_patch_write(
	uintptr_t addr,
	const uint8_t *bytes,
	size_t size,
	uint8_t *out_original
) {
	DWORD old_protect;
	if (!VirtualProtect((void *)addr, size, PAGE_EXECUTE_READWRITE, &old_protect)) {
		return false;
	}

	if (out_original) {
		memcpy(out_original, (const void *)addr, size);
	}
	memcpy((void *)addr, bytes, size);

	VirtualProtect((void *)addr, size, old_protect, &old_protect);
	return true;
}

static bool s_patch_restore(uintptr_t addr, const uint8_t *original, size_t size) {
	DWORD old_protect;
	if (!VirtualProtect((void *)addr, size, PAGE_EXECUTE_READWRITE, &old_protect)) {
		return false;
	}

	memcpy((void *)addr, original, size);

	VirtualProtect((void *)addr, size, old_protect, &old_protect);
	return true;
}

static const DTTR_ComponentAPI s_api = {
	.m_log = log_log,
};

static const DTTR_ComponentGameAPI s_game_api = {
	.m_sigscan = s_sigscan,
	.m_patch_write = s_patch_write,
	.m_patch_restore = s_patch_restore,
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

const DTTR_ComponentContext *dttr_game_api_get_ctx(void) { return &s_ctx; }

void dttr_game_api_set_window(SDL_Window *window) { s_ctx.m_window = window; }

void dttr_game_api_set_device(SDL_GPUDevice *device) { s_ctx.m_gpu_device = device; }
