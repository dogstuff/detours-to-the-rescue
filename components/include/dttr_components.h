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
#define DTTR_COMPONENT_API_VERSION 2

typedef void (*DTTR_LogFn)(int level, const char *file, int line, const char *fmt, ...);

typedef struct {
	DTTR_LogFn m_log;
} DTTR_ComponentAPI;

typedef uintptr_t (*DTTR_SigscanFn)(HMODULE mod, const char *sig, const char *mask);

/// Opaque hook handle returned by hook/patch functions.
typedef struct DTTR_Hook DTTR_Hook;

typedef DTTR_Hook *(*DTTR_HookFunctionFn)(
	uintptr_t addr,
	// Minimum prologue bytes before instruction-boundary alignment. Pass 0 for auto.
	int prologue_size,
	void *handler,
	void **out_original
);

typedef DTTR_Hook
	*(*DTTR_HookPointerFn)(uintptr_t addr, void *new_value, void **out_original);

typedef DTTR_Hook *(*DTTR_PatchBytesFn)(uintptr_t addr, const uint8_t *bytes, size_t size);

typedef void (*DTTR_UnhookFn)(DTTR_Hook *hook);

typedef struct {
	DTTR_SigscanFn m_sigscan;
	DTTR_HookFunctionFn m_hook_function;
	DTTR_HookPointerFn m_hook_pointer;
	DTTR_PatchBytesFn m_patch_bytes;
	DTTR_UnhookFn m_unhook;
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

// Interop storage macros (gated on DTTR_INTEROP_IMPLEMENT)

#ifdef DTTR_INTEROP_IMPLEMENT
#define DTTR_STORAGE(type, name) type name = 0;
#else
#define DTTR_STORAGE(type, name) extern type name;
#endif

// Stores a hook site address and registry handle.
#define DTTR_HOOK(name)                                                                  \
	DTTR_STORAGE(uintptr_t, name##_site)                                                 \
	DTTR_STORAGE(DTTR_Hook *, name##_handle)

// Stores a hook site, handle, and trampoline pointer.
#define DTTR_TRAMPOLINE_HOOK(name)                                                       \
	DTTR_HOOK(name)                                                                      \
	DTTR_STORAGE(uint8_t *, name##_trampoline)

// Stores a function address with a typedef and inline call wrapper.
#define DTTR_FUNC(name, cc, ret, params, args)                                           \
	typedef ret(cc *name##_fn_t) params;                                                 \
	DTTR_STORAGE(uintptr_t, name##_addr)                                                 \
	static inline ret name params { return ((name##_fn_t)name##_addr)args; }

// Stores a variable address with typed ptr/get/set accessors.
#define DTTR_VAR(name, type)                                                             \
	DTTR_STORAGE(uintptr_t, name##_addr)                                                 \
	static inline type *name##_ptr(void) { return (type *)name##_addr; }                 \
	static inline type name##_get(void) { return *(type *)name##_addr; }                 \
	static inline void name##_set(type val) { *(type *)name##_addr = val; }

// Detaches a hook and clears its tracking variables.
#define DTTR_UNINSTALL(name, ctx)                                                        \
	do {                                                                                 \
		if (name##_handle) {                                                             \
			(ctx)->m_game_api->m_unhook(name##_handle);                                  \
			name##_handle = NULL;                                                        \
			name##_site = 0;                                                             \
		}                                                                                \
	} while (0)

// Detaches a trampoline hook and clears its tracking variables.
#define DTTR_TRAMPOLINE_UNINSTALL(name, ctx)                                             \
	do {                                                                                 \
		if (name##_handle) {                                                             \
			(ctx)->m_game_api->m_unhook(name##_handle);                                  \
			name##_handle = NULL;                                                        \
			name##_site = 0;                                                             \
			name##_trampoline = NULL;                                                    \
		}                                                                                \
	} while (0)

// Scans for a signature, installs an E9 JMP hook, and logs the result.
#define DTTR_INSTALL_JMP(name, ctx, sig, mask)                                            \
	do {                                                                                  \
		uintptr_t match_ = (ctx)->m_game_api->m_sigscan((ctx)->m_game_module, sig, mask); \
		if (match_) {                                                                     \
			name##_site = match_;                                                         \
			uint8_t jmp_[5] = {0xE9};                                                     \
			int32_t rel_ = (int32_t)((uintptr_t)(name##_callback) - (match_ + 5));        \
			memcpy(jmp_ + 1, &rel_, 4);                                                   \
			name##_handle = (ctx)->m_game_api->m_patch_bytes(match_, jmp_, 5);            \
			log_debug("Installed " #name " at 0x%08X", (unsigned)match_);                 \
		} else {                                                                          \
			log_error(#name ": signature not found");                                     \
		}                                                                                 \
	} while (0)

// Scans for a signature and installs an E9 JMP hook, silently skipping if not found.
#define DTTR_INSTALL_JMP_OPTIONAL(name, ctx, sig, mask)                                   \
	do {                                                                                  \
		uintptr_t match_ = (ctx)->m_game_api->m_sigscan((ctx)->m_game_module, sig, mask); \
		if (match_) {                                                                     \
			name##_site = match_;                                                         \
			uint8_t jmp_[5] = {0xE9};                                                     \
			int32_t rel_ = (int32_t)((uintptr_t)(name##_callback) - (match_ + 5));        \
			memcpy(jmp_ + 1, &rel_, 4);                                                   \
			name##_handle = (ctx)->m_game_api->m_patch_bytes(match_, jmp_, 5);            \
			log_debug("Installed " #name " at 0x%08X", (unsigned)match_);                 \
		}                                                                                 \
	} while (0)

// Scans for a signature, installs a trampoline hook, and stores the trampoline pointer.
#define DTTR_INSTALL_TRAMPOLINE(name, ctx, sig, mask, prologue)                           \
	do {                                                                                  \
		uintptr_t match_ = (ctx)->m_game_api->m_sigscan((ctx)->m_game_module, sig, mask); \
		if (match_) {                                                                     \
			name##_site = match_;                                                         \
			void *orig_ = NULL;                                                           \
			name##_handle = (ctx)->m_game_api->m_hook_function(                           \
				match_,                                                                   \
				prologue,                                                                 \
				(void *)(name##_callback),                                                \
				&orig_                                                                    \
			);                                                                            \
			if (name##_handle) {                                                          \
				name##_trampoline = (uint8_t *)orig_;                                     \
				log_debug("Installed " #name " at 0x%08X", (unsigned)match_);             \
			} else {                                                                      \
				log_error(#name ": hook_function failed");                                \
				name##_site = 0;                                                          \
			}                                                                             \
		} else {                                                                          \
			log_error(#name ": signature not found");                                     \
		}                                                                                 \
	} while (0)

// Scans for a signature and installs a trampoline hook with automatic prologue sizing.
// The hook backend decodes instructions and copies the minimum whole-instruction prologue
// needed to place the 5-byte JMP patch safely.
#define DTTR_INSTALL_TRAMPOLINE_AUTO(name, ctx, sig, mask)                               \
	DTTR_INSTALL_TRAMPOLINE(name, ctx, sig, mask, 0)

// Scans for a signature and patches bytes at the match plus an offset.
#define DTTR_INSTALL_BYTES(name, ctx, sig, mask, offset, bytes, size)                     \
	do {                                                                                  \
		uintptr_t match_ = (ctx)->m_game_api->m_sigscan((ctx)->m_game_module, sig, mask); \
		if (match_) {                                                                     \
			name##_site = match_ + (offset);                                              \
			name##_handle = (ctx)->m_game_api->m_patch_bytes(name##_site, bytes, size);   \
			log_debug("Applied " #name " at 0x%08X", (unsigned)name##_site);              \
		} else {                                                                          \
			log_error(#name ": signature not found");                                     \
		}                                                                                 \
	} while (0)

// Scans for a signature, computes the hook site from the match, and installs a pointer
// hook. The site_expr parameter is evaluated with match_ in scope.
#define DTTR_INSTALL_POINTER(name, ctx, sig, mask, site_expr)                             \
	do {                                                                                  \
		uintptr_t match_ = (ctx)->m_game_api->m_sigscan((ctx)->m_game_module, sig, mask); \
		if (match_) {                                                                     \
			name##_site = (uintptr_t)(site_expr);                                         \
			void *orig_ = NULL;                                                           \
			name##_handle = (ctx)->m_game_api->m_hook_pointer(                            \
				name##_site,                                                              \
				(void *)(name##_callback),                                                \
				&orig_                                                                    \
			);                                                                            \
			log_debug("Installed " #name " at 0x%08X", (unsigned)name##_site);            \
		} else {                                                                          \
			log_error(#name ": signature not found");                                     \
		}                                                                                 \
	} while (0)

// Scans for a signature and patches bytes at match plus an offset, silently skipping if
// not found.
#define DTTR_INSTALL_BYTES_OPTIONAL(name, ctx, sig, mask, offset, bytes, size)            \
	do {                                                                                  \
		uintptr_t match_ = (ctx)->m_game_api->m_sigscan((ctx)->m_game_module, sig, mask); \
		if (match_) {                                                                     \
			name##_site = match_ + (offset);                                              \
			name##_handle = (ctx)->m_game_api->m_patch_bytes(name##_site, bytes, size);   \
			log_debug("Applied " #name " at 0x%08X", (unsigned)name##_site);              \
		}                                                                                 \
	} while (0)

// Scans for a signature and resolves a DTTR_FUNC or DTTR_VAR address.
// The expr parameter is evaluated with match in scope.
#define DTTR_RESOLVE(name, ctx, sig, mask, expr)                                         \
	do {                                                                                 \
		uintptr_t match = (ctx)->m_game_api->m_sigscan((ctx)->m_game_module, sig, mask); \
		if (match) {                                                                     \
			name##_addr = (uintptr_t)(expr);                                             \
			log_debug("Resolved " #name " at 0x%08X", (unsigned)name##_addr);            \
		} else {                                                                         \
			log_error(#name ": resolved to NULL");                                       \
		}                                                                                \
	} while (0)

// Resolves an E8 relative call at address p to its absolute target.
#define DTTR_E8_TARGET(p) ((p) + 5 + *(int32_t *)((p) + 1))

// Reads the absolute jump target from an FF 25 import thunk at address p.
#define DTTR_FF25_ADDR(p) (*(uint32_t *)((p) + 2))

// Logging macros

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
