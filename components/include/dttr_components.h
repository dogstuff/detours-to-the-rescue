/// DttR component API.
///
/// Components are shared libraries (.dll) placed in the DttR `components/`
/// directory and loaded during sidecar startup.
///
/// Required exports:
///
///   bool dttr_component_init(const DTTR_ComponentContext *ctx)
///
///   void dttr_component_cleanup(void)
///
/// Optional exports:
///
///   const DTTR_ComponentInfo *dttr_component_info(void)
///
///   void dttr_component_tick(void)
///
///   bool dttr_component_event(const SDL_Event *event)
///
///   void dttr_component_render_game(const DTTR_RenderGameContext *ctx)
///
///   void dttr_component_render(const DTTR_RenderContext *ctx)

#ifndef DTTR_COMPONENTS_H
#define DTTR_COMPONENTS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#if defined(_WIN32) || defined(__CYGWIN__)
#include <windows.h>
#else
// Parse-only stand-ins for non-Windows hosts. Components still build with the
// Windows cross-compiler.
typedef void *HMODULE;
typedef void *HINSTANCE;
typedef int BOOL;
typedef unsigned long DWORD;
typedef void *LPVOID;
#define WINAPI
#define TRUE 1
#define FALSE 0
#endif

// Forward declarations for components that do not include SDL3/SDL.h.
#ifndef SDL_h_
typedef struct SDL_Window SDL_Window;
typedef union SDL_Event SDL_Event;
#endif

// Reject incompatible hosts by comparing ctx->m_api_version against this value.
#define DTTR_COMPONENT_API_VERSION 4

typedef void (*DTTR_LogFn)(int level, const char *file, int line, const char *fmt, ...);
typedef bool (*DTTR_LogIsEnabledFn)(int level);

typedef struct {
	DTTR_LogFn m_log;
	DTTR_LogIsEnabledFn m_log_is_enabled;
	DTTR_LogFn m_log_unchecked;
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

/// Context passed to the render_game callback at game resolution.
typedef struct {
	uint32_t m_width;
	uint32_t m_height;
	float m_scale;
} DTTR_RenderGameContext;

/// Context passed to the render callback at window resolution.
typedef struct {
	uint32_t m_window_w;
	uint32_t m_window_h;
	uint32_t m_game_x;
	uint32_t m_game_y;
	uint32_t m_game_w;
	uint32_t m_game_h;
	float m_scale;
} DTTR_RenderContext;

typedef void (*DTTR_ComponentRenderGameFn)(const DTTR_RenderGameContext *ctx);

typedef void (*DTTR_ComponentRenderFn)(const DTTR_RenderContext *ctx);

// Interop storage macros (gated on DTTR_INTEROP_IMPLEMENT).

#ifdef DTTR_INTEROP_IMPLEMENT
#define DTTR_STORAGE(type, name) type name = 0;
#else
#define DTTR_STORAGE(type, name) extern type name;
#endif

// Track a hook site address and registry handle.
#define DTTR_HOOK(name)                                                                  \
	DTTR_STORAGE(uintptr_t, name##_site)                                                 \
	DTTR_STORAGE(DTTR_Hook *, name##_handle)

// Track an import-address-table hook site, handle, original pointer, and log label.
#define DTTR_IMPORT_HOOK_STATE(prefix, import_ident, import_log_name)                    \
	static uintptr_t prefix##_##import_ident##_site;                                     \
	static DTTR_Hook *prefix##_##import_ident##_handle;                                  \
	static void *prefix##_##import_ident##_original;                                     \
	static const char prefix##_##import_ident##_log_name[] = import_log_name;

#define DTTR_IMPORT_HOOK_INSTALLED(spec) ((spec)->m_handle && *(spec)->m_handle)
#define DTTR_IMPORT_INSTALL_SPEC(ctx, spec, site)                                        \
	dttr_import_hook_install_spec(ctx, spec, (uintptr_t)(site))
#define DTTR_IMPORT_UNINSTALL_SPEC(ctx, spec) dttr_import_hook_uninstall_spec(ctx, spec)

extern int g_dttr_import_hook_trace_enabled;

#define DTTR_IMPORT_HOOK_DECL(prefix, import_ident, import_log_name)                     \
	DTTR_IMPORT_HOOK_STATE(prefix, import_ident, import_log_name)                        \
	static __attribute__((naked)) void prefix##_##import_ident##_callback(void) {        \
		__asm__ __volatile__("pushfl\n\t"                                                \
							 "cmpl $0, _g_dttr_import_hook_trace_enabled\n\t"            \
							 "je 1f\n\t"                                                 \
							 "pushal\n\t"                                                \
							 "pushl %0\n\t"                                              \
							 "call _dttr_import_hook_trace\n\t"                          \
							 "addl $4, %%esp\n\t"                                        \
							 "popal\n\t"                                                 \
							 "popfl\n\t"                                                 \
							 "jmp *%1\n\t"                                               \
							 "1:\n\t"                                                    \
							 "popfl\n\t"                                                 \
							 "jmp *%1"                                                   \
							 :                                                           \
							 : "i"(prefix##_##import_ident##_log_name),                  \
							   "m"(prefix##_##import_ident##_original));                 \
	}

#define DTTR_IMPORT_HOOK_WARN_DECL(prefix, import_ident, import_log_name)                \
	DTTR_IMPORT_HOOK_STATE(prefix, import_ident, import_log_name)                        \
	static __attribute__((naked)) void prefix##_##import_ident##_callback(void) {        \
		__asm__ __volatile__("pushfl\n\t"                                                \
							 "pushal\n\t"                                                \
							 "pushl %0\n\t"                                              \
							 "call _dttr_import_hook_warn\n\t"                           \
							 "addl $4, %%esp\n\t"                                        \
							 "popal\n\t"                                                 \
							 "popfl\n\t"                                                 \
							 "jmp *%1"                                                   \
							 :                                                           \
							 : "i"(prefix##_##import_ident##_log_name),                  \
							   "m"(prefix##_##import_ident##_original));                 \
	}

#define DTTR_IMPORT_HOOK_SPEC(prefix, import_ident, import_name)                         \
	{                                                                                    \
		.m_import_name = import_name,                                                    \
		.m_callback = prefix##_##import_ident##_callback,                                \
		.m_original = &prefix##_##import_ident##_original,                               \
		.m_site = &prefix##_##import_ident##_site,                                       \
		.m_handle = &prefix##_##import_ident##_handle,                                   \
	}

#define DTTR_IMPORT_SHOULD_NOT_CALL_SPEC(import_name)                                    \
	{                                                                                    \
		.m_import_name = import_name,                                                    \
		.m_callback = NULL,                                                              \
		.m_original = NULL,                                                              \
		.m_site = NULL,                                                                  \
		.m_handle = NULL,                                                                \
	}

// Track a hook site, handle, and trampoline pointer.
#define DTTR_TRAMPOLINE_HOOK(name)                                                       \
	DTTR_HOOK(name)                                                                      \
	DTTR_STORAGE(uint8_t *, name##_trampoline)

// Track a function address with a typedef and inline call wrapper.
#define DTTR_FUNC(name, cc, ret, params, args)                                           \
	typedef ret(cc *name##_fn_t) params;                                                 \
	DTTR_STORAGE(uintptr_t, name##_addr)                                                 \
	static inline ret name params { return ((name##_fn_t)name##_addr)args; }

// Track a variable address with typed ptr/get/set accessors.
#define DTTR_VAR(name, type)                                                             \
	DTTR_STORAGE(uintptr_t, name##_addr)                                                 \
	static inline type *name##_ptr(void) { return (type *)name##_addr; }                 \
	static inline type name##_get(void) { return *(type *)name##_addr; }                 \
	static inline void name##_set(type val) { *(type *)name##_addr = val; }

// Unhook and clear tracked state.
#define DTTR_UNINSTALL(name, ctx)                                                        \
	do {                                                                                 \
		if (name##_handle) {                                                             \
			(ctx)->m_game_api->m_unhook(name##_handle);                                  \
			name##_handle = NULL;                                                        \
			name##_site = 0;                                                             \
		}                                                                                \
	} while (0)

// Unhook and clear tracked trampoline state.
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
			DTTR_COMPONENT_LOG_DEBUG(                                                     \
				ctx,                                                                      \
				"Installed " #name " at 0x%08X",                                          \
				(unsigned)match_                                                          \
			);                                                                            \
		} else {                                                                          \
			DTTR_COMPONENT_LOG_ERROR(ctx, #name ": signature not found");                 \
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
			DTTR_COMPONENT_LOG_DEBUG(                                                     \
				ctx,                                                                      \
				"Installed " #name " at 0x%08X",                                          \
				(unsigned)match_                                                          \
			);                                                                            \
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
				DTTR_COMPONENT_LOG_DEBUG(                                                 \
					ctx,                                                                  \
					"Installed " #name " at 0x%08X",                                      \
					(unsigned)match_                                                      \
				);                                                                        \
			} else {                                                                      \
				DTTR_COMPONENT_LOG_ERROR(ctx, #name ": hook_function failed");            \
				name##_site = 0;                                                          \
			}                                                                             \
		} else {                                                                          \
			DTTR_COMPONENT_LOG_ERROR(ctx, #name ": signature not found");                 \
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
			DTTR_COMPONENT_LOG_DEBUG(                                                     \
				ctx,                                                                      \
				"Applied " #name " at 0x%08X",                                            \
				(unsigned)name##_site                                                     \
			);                                                                            \
		} else {                                                                          \
			DTTR_COMPONENT_LOG_ERROR(ctx, #name ": signature not found");                 \
		}                                                                                 \
	} while (0)

// Installs a pointer hook at a known site.
#define DTTR_INSTALL_POINTER_AT(name, ctx, site, new_value)                              \
	do {                                                                                 \
		if (!name##_handle) {                                                            \
			name##_site = (uintptr_t)(site);                                             \
			void *unused_original_ = NULL;                                               \
			name##_handle = (ctx)->m_game_api->m_hook_pointer(                           \
				name##_site,                                                             \
				(void *)(new_value),                                                     \
				&unused_original_                                                        \
			);                                                                           \
			if (name##_handle) {                                                         \
				DTTR_COMPONENT_LOG_DEBUG(                                                \
					ctx,                                                                 \
					"Installed " #name " at 0x%08X",                                     \
					(unsigned)name##_site                                                \
				);                                                                       \
			} else {                                                                     \
				DTTR_COMPONENT_LOG_ERROR(ctx, #name ": hook_pointer failed");            \
				name##_site = 0;                                                         \
			}                                                                            \
		}                                                                                \
	} while (0)

// Scans for a signature, computes the hook site from the match, and installs a pointer
// hook. The site_expr parameter is evaluated with match_ in scope.
#define DTTR_INSTALL_POINTER(name, ctx, sig, mask, site_expr)                             \
	do {                                                                                  \
		uintptr_t match_ = (ctx)->m_game_api->m_sigscan((ctx)->m_game_module, sig, mask); \
		if (match_) {                                                                     \
			DTTR_INSTALL_POINTER_AT(name, ctx, site_expr, name##_callback);               \
		} else {                                                                          \
			DTTR_COMPONENT_LOG_ERROR(ctx, #name ": signature not found");                 \
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
			DTTR_COMPONENT_LOG_DEBUG(                                                     \
				ctx,                                                                      \
				"Applied " #name " at 0x%08X",                                            \
				(unsigned)name##_site                                                     \
			);                                                                            \
		}                                                                                 \
	} while (0)

// Scans for a signature and resolves a DTTR_FUNC or DTTR_VAR address.
// The expr parameter is evaluated with match in scope.
#define DTTR_RESOLVE(name, ctx, sig, mask, expr)                                         \
	do {                                                                                 \
		uintptr_t match = (ctx)->m_game_api->m_sigscan((ctx)->m_game_module, sig, mask); \
		if (match) {                                                                     \
			name##_addr = (uintptr_t)(expr);                                             \
			DTTR_COMPONENT_LOG_DEBUG(                                                    \
				ctx,                                                                     \
				"Resolved " #name " at 0x%08X",                                          \
				(unsigned)name##_addr                                                    \
			);                                                                           \
		} else {                                                                         \
			DTTR_COMPONENT_LOG_ERROR(ctx, #name ": resolved to NULL");                   \
		}                                                                                \
	} while (0)

// Resolves an E8 relative call at address p to its absolute target.
#define DTTR_E8_TARGET(p) ((p) + 5 + *(int32_t *)((p) + 1))

// Reads the absolute jump target from an FF 25 import thunk at address p.
#define DTTR_FF25_ADDR(p) (*(uint32_t *)((p) + 2))

// Logging macros

#define DTTR_COMPONENT_LOG_LVL_TRACE 0
#define DTTR_COMPONENT_LOG_LVL_DEBUG 1
#define DTTR_COMPONENT_LOG_LVL_INFO 2
#define DTTR_COMPONENT_LOG_LVL_WARN 3
#define DTTR_COMPONENT_LOG_LVL_ERROR 4
#define DTTR_COMPONENT_LOG_LVL_FATAL 5

#define DTTR_COMPONENT_LOG(ctx, level, ...)                                              \
	do {                                                                                 \
		if ((ctx)->m_api->m_log_is_enabled(level)) {                                     \
			(ctx)->m_api->m_log_unchecked(level, __FILE__, __LINE__, __VA_ARGS__);       \
		}                                                                                \
	} while (0)
#define DTTR_COMPONENT_LOG_TRACE(ctx, ...)                                               \
	DTTR_COMPONENT_LOG(ctx, DTTR_COMPONENT_LOG_LVL_TRACE, __VA_ARGS__)
#define DTTR_COMPONENT_LOG_DEBUG(ctx, ...)                                               \
	DTTR_COMPONENT_LOG(ctx, DTTR_COMPONENT_LOG_LVL_DEBUG, __VA_ARGS__)
#define DTTR_COMPONENT_LOG_INFO(ctx, ...)                                                \
	DTTR_COMPONENT_LOG(ctx, DTTR_COMPONENT_LOG_LVL_INFO, __VA_ARGS__)
#define DTTR_COMPONENT_LOG_WARN(ctx, ...)                                                \
	DTTR_COMPONENT_LOG(ctx, DTTR_COMPONENT_LOG_LVL_WARN, __VA_ARGS__)
#define DTTR_COMPONENT_LOG_ERROR(ctx, ...)                                               \
	DTTR_COMPONENT_LOG(ctx, DTTR_COMPONENT_LOG_LVL_ERROR, __VA_ARGS__)
#define DTTR_COMPONENT_LOG_FATAL(ctx, ...)                                               \
	DTTR_COMPONENT_LOG(ctx, DTTR_COMPONENT_LOG_LVL_FATAL, __VA_ARGS__)

// Component export macros.
//
//   DTTR_COMPONENT_INFO("My Plugin", "1.0.0", "Author")
//   DTTR_COMPONENT_INIT { return true; }
//   DTTR_COMPONENT_CLEANUP { }

#if defined(_WIN32) || defined(__CYGWIN__)
#define DTTR_EXPORT __declspec(dllexport)
#else
#define DTTR_EXPORT __attribute__((visibility("default")))
#endif

#define DTTR_COMPONENT_INFO(name, version, author)                                       \
	static const DTTR_ComponentInfo s_dttr_component_info_ = {                           \
		.m_name = name,                                                                  \
		.m_version = version,                                                            \
		.m_author = author,                                                              \
	};                                                                                   \
	DTTR_EXPORT const DTTR_ComponentInfo *dttr_component_info(void) {                    \
		return &s_dttr_component_info_;                                                  \
	}

// Checks API version and delegates to the component body.
#define DTTR_COMPONENT_INIT                                                              \
	static bool s_dttr_component_init_(const DTTR_ComponentContext *);                   \
	DTTR_EXPORT bool dttr_component_init(const DTTR_ComponentContext *ctx) {             \
		if (ctx->m_api_version < DTTR_COMPONENT_API_VERSION) {                           \
			return false;                                                                \
		}                                                                                \
		return s_dttr_component_init_(ctx);                                              \
	}                                                                                    \
	static bool s_dttr_component_init_(const DTTR_ComponentContext *ctx)

#define DTTR_COMPONENT_CLEANUP DTTR_EXPORT void dttr_component_cleanup(void)

#define DTTR_COMPONENT_TICK DTTR_EXPORT void dttr_component_tick(void)

// Return true to consume the event.
#define DTTR_COMPONENT_EVENT DTTR_EXPORT bool dttr_component_event(const SDL_Event *event)

// Renders at game resolution, letterboxed and scaled with the game image.
#define DTTR_COMPONENT_RENDER_GAME                                                       \
	DTTR_EXPORT void dttr_component_render_game(const DTTR_RenderGameContext *ctx)

// Renders at full window resolution, above letterbox bars.
#define DTTR_COMPONENT_RENDER                                                            \
	DTTR_EXPORT void dttr_component_render(const DTTR_RenderContext *ctx)

#endif /* DTTR_COMPONENTS_H */
