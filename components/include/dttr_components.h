/// DttR component API.
///
/// A component is a shared library (.dll) placed in the DttR `components/` directory
/// that is discovered and loaded during the sidecar startup process.
///
/// Required exports:
///
///   bool  dttr_component_init(const DTTR_ComponentContext *ctx)
///
///   void  dttr_component_cleanup(void)
///
/// Optional exports:
///
///   const DTTR_ComponentInfo *dttr_component_info(void)
///
///   void  dttr_component_tick(void)
///
///   bool  dttr_component_event(const SDL_Event *event)
///
///   void  dttr_component_render(
///		SDL_GPUCommandBuffer *cmd,
///     SDL_GPUTexture *render_target,
///     uint32_t width,
///     uint32_t height
///   )
///
/// See example_component.c for a minimal template.

#ifndef DTTR_COMPONENTS_H
#define DTTR_COMPONENTS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <windows.h>

#include <SDL3/SDL.h>

/// Check this against ctx->m_api_version in dttr_component_init to reject incompatible
/// hosts.
#define DTTR_COMPONENT_API_VERSION 1

typedef void (*DTTR_LogFn)(int level, const char *file, int line, const char *fmt, ...);

typedef struct {
	DTTR_LogFn m_log;
} DTTR_ComponentAPI;

typedef uintptr_t (*DTTR_SigscanFn)(HMODULE mod, const char *sig, const char *mask);

typedef bool (*DTTR_PatchWriteFn)(
	uintptr_t addr,
	const uint8_t *bytes,
	size_t size,
	uint8_t *out_original
);

typedef bool (*DTTR_PatchRestoreFn)(uintptr_t addr, const uint8_t *original, size_t size);

typedef struct {
	DTTR_SigscanFn m_sigscan;
	DTTR_PatchWriteFn m_patch_write;
	DTTR_PatchRestoreFn m_patch_restore;
} DTTR_ComponentGameAPI;

typedef struct {
	uint32_t m_api_version;
	HMODULE m_game_module;
	HMODULE m_sidecar_module;
	SDL_Window *m_window;
	SDL_GPUDevice *m_gpu_device;
	const char *m_loader_dir;
	const char *m_exe_hash;
	const void *m_config;
	const DTTR_ComponentAPI *m_api;
	const DTTR_ComponentGameAPI *m_game_api;
} DTTR_ComponentContext;

typedef struct {
	const char *m_name;
	const char *m_version;
	const char *m_author;
} DTTR_ComponentInfo;

typedef bool (*DTTR_ComponentInitFn)(const DTTR_ComponentContext *ctx);

typedef void (*DTTR_ComponentCleanupFn)(void);

typedef void (*DTTR_ComponentTickFn)(void);

typedef bool (*DTTR_ComponentEventFn)(const SDL_Event *event);

typedef const DTTR_ComponentInfo *(*DTTR_ComponentInfoFn)(void);

typedef void (*DTTR_ComponentRenderFn)(
	SDL_GPUCommandBuffer *cmd,
	SDL_GPUTexture *render_target,
	uint32_t width,
	uint32_t height
);

#define DTTR_LOG_LVL_TRACE 0
#define DTTR_LOG_LVL_DEBUG 1
#define DTTR_LOG_LVL_INFO 2
#define DTTR_LOG_LVL_WARN 3
#define DTTR_LOG_LVL_ERROR 4
#define DTTR_LOG_LVL_FATAL 5

#define DTTR_LOG(ctx, level, ...)                                                        \
	(ctx)->m_api->m_log(level, __FILE__, __LINE__, __VA_ARGS__)
#define DTTR_LOG_TRACE(ctx, ...) DTTR_LOG(ctx, DTTR_LOG_LVL_TRACE, __VA_ARGS__)
#define DTTR_LOG_DEBUG(ctx, ...) DTTR_LOG(ctx, DTTR_LOG_LVL_DEBUG, __VA_ARGS__)
#define DTTR_LOG_INFO(ctx, ...) DTTR_LOG(ctx, DTTR_LOG_LVL_INFO, __VA_ARGS__)
#define DTTR_LOG_WARN(ctx, ...) DTTR_LOG(ctx, DTTR_LOG_LVL_WARN, __VA_ARGS__)
#define DTTR_LOG_ERROR(ctx, ...) DTTR_LOG(ctx, DTTR_LOG_LVL_ERROR, __VA_ARGS__)
#define DTTR_LOG_FATAL(ctx, ...) DTTR_LOG(ctx, DTTR_LOG_LVL_FATAL, __VA_ARGS__)

#endif /* DTTR_COMPONENTS_H */
