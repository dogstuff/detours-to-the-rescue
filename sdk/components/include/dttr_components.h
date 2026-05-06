/// DttR component DLL API.
/// Components live in `components/`.
///
/// Required exports:
///   bool dttr_component_init(const DTTR_ComponentContext *ctx)
///   void dttr_component_cleanup(void)
///
/// Optional exports use the DTTR_COMPONENT_* macros below.

#ifndef DTTR_COMPONENTS_H
#define DTTR_COMPONENTS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <windows.h>

// Forward declarations for components that do not include SDL3/SDL.h.
#ifndef SDL_h_
typedef struct SDL_Window SDL_Window;
typedef union SDL_Event SDL_Event;
#endif

// Reject incompatible hosts by comparing ctx->m_api_version against this value.
#define DTTR_COMPONENT_API_VERSION 7

typedef void (*DTTR_LogFn)(int level, const char *file, int line, const char *fmt, ...);
typedef bool (*DTTR_LogIsEnabledFn)(int level);
typedef struct {
	DTTR_LogFn m_log;
	DTTR_LogIsEnabledFn m_log_is_enabled;
	DTTR_LogFn m_log_unchecked;
} DTTR_ComponentAPI;

typedef uintptr_t (*DTTR_SigscanFn)(HMODULE mod, const char *sig, const char *mask);

/// Opaque hook handle returned by hook and patch helpers.
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
typedef void (*DTTR_ComponentLateInitFn)(void);
typedef void (*DTTR_ComponentBeforeUnloadFn)(void);

typedef struct {
	uint64_t m_frame_index;
	uint32_t m_window_w;
	uint32_t m_window_h;
	uint32_t m_game_x;
	uint32_t m_game_y;
	uint32_t m_game_w;
	uint32_t m_game_h;
	float m_scale;
} DTTR_FrameContext;

typedef struct {
	uint64_t m_frame_index;
	uint32_t m_window_w;
	uint32_t m_window_h;
	uint32_t m_game_x;
	uint32_t m_game_y;
	uint32_t m_game_w;
	uint32_t m_game_h;
	float m_scale;
	bool m_imgui_frame_active;
	bool m_overlay_rendered;
} DTTR_PresentContext;

typedef struct {
	SDL_Window *m_window;
	HWND m_hwnd;
	uint32_t m_window_w;
	uint32_t m_window_h;
} DTTR_WindowContext;

typedef enum {
	DTTR_GRAPHICS_BACKEND_UNKNOWN = 0,
	DTTR_GRAPHICS_BACKEND_SDL_GPU = 1,
	DTTR_GRAPHICS_BACKEND_OPENGL = 2,
} DTTR_GraphicsBackend;

typedef struct {
	SDL_Window *m_window;
	HWND m_hwnd;
	DTTR_GraphicsBackend m_backend;
	const char *m_driver_name;
	uint32_t m_render_w;
	uint32_t m_render_h;
} DTTR_GraphicsContext;

typedef struct {
	bool m_overlay_visible;
	bool m_game_input_enabled;
} DTTR_InputContext;

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

typedef void (*DTTR_ComponentFrameBeginFn)(const DTTR_FrameContext *ctx);
typedef void (*DTTR_ComponentBeforeGameFrameFn)(const DTTR_FrameContext *ctx);
typedef void (*DTTR_ComponentAfterGameFrameFn)(const DTTR_FrameContext *ctx);
typedef void (*DTTR_ComponentBeforePresentFn)(const DTTR_PresentContext *ctx);
typedef void (*DTTR_ComponentAfterPresentFn)(const DTTR_PresentContext *ctx);
typedef void (*DTTR_ComponentFrameEndFn)(const DTTR_FrameContext *ctx);
typedef void (*DTTR_ComponentImguiBeginFn)(const DTTR_RenderContext *ctx);
typedef void (*DTTR_ComponentImguiEndFn)(const DTTR_RenderContext *ctx);
typedef void (*DTTR_ComponentOverlayVisibleChangedFn)(bool visible);
typedef void (*DTTR_ComponentWindowCreatedFn)(const DTTR_WindowContext *ctx);
typedef void (*DTTR_ComponentWindowResizedFn)(const DTTR_WindowContext *ctx);
typedef void (*DTTR_ComponentWindowDestroyingFn)(const DTTR_WindowContext *ctx);
typedef void (*DTTR_ComponentGraphicsDeviceCreatedFn)(const DTTR_GraphicsContext *ctx);
typedef void (*DTTR_ComponentGraphicsDeviceLostFn)(const DTTR_GraphicsContext *ctx);
typedef void (*DTTR_ComponentGraphicsDeviceRestoredFn)(const DTTR_GraphicsContext *ctx);
typedef void (*DTTR_ComponentGraphicsDeviceDestroyingFn)(const DTTR_GraphicsContext *ctx);
typedef bool (*DTTR_ComponentBeforeEventFn)(const SDL_Event *event);
typedef void (*DTTR_ComponentAfterEventFn)(const SDL_Event *event, bool consumed);
typedef void (*DTTR_ComponentInputModeChangedFn)(const DTTR_InputContext *ctx);

typedef bool (*DTTR_ComponentShouldAdvanceGameFrameFn)(void);
typedef void (*DTTR_ComponentGameFrameAdvancedFn)(void);
typedef void (*DTTR_ComponentGameFrameBlockedFn)(void);

// Interop storage macros.

#ifdef DTTR_INTEROP_IMPLEMENT
#define DTTR_STORAGE(type, name) type name = 0;
#else
#define DTTR_STORAGE(type, name) extern type name;
#endif

// Track a hook site and registry handle.
#define DTTR_HOOK(name)                                                                  \
	DTTR_STORAGE(uintptr_t, name##_site)                                                 \
	DTTR_STORAGE(DTTR_Hook *, name##_handle)

// Track a hook site, handle, and trampoline pointer.
#define DTTR_TRAMPOLINE_HOOK(name)                                                       \
	DTTR_HOOK(name)                                                                      \
	DTTR_STORAGE(uint8_t *, name##_trampoline)

// Track a function address with a typed inline wrapper.
#define DTTR_FUNC(name, cc, ret, params, args)                                           \
	typedef ret(cc *name##_fn_t) params;                                                 \
	DTTR_STORAGE(uintptr_t, name##_addr)                                                 \
	static inline ret name params { return ((name##_fn_t)name##_addr)args; }

// Track a variable address with typed accessors.
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

// Scan for a signature, install an E9 JMP hook, and log the result.
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

// Scan for a signature and install an optional E9 JMP hook.
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

// Scan for a signature, install a trampoline hook, and store the trampoline pointer.
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

// Scan for a signature and install a trampoline hook with automatic prologue sizing.
#define DTTR_INSTALL_TRAMPOLINE_AUTO(name, ctx, sig, mask)                               \
	DTTR_INSTALL_TRAMPOLINE(name, ctx, sig, mask, 0)

// Scan for a signature and patch bytes at match plus an offset.
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

// Scan for a signature and optionally patch bytes at match plus an offset.
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

// Install an IAT hook at a known site.
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

// Scan for a signature, compute the hook site, and install an IAT hook.
// site_expr is evaluated with match_ in scope.
#define DTTR_INSTALL_POINTER(name, ctx, sig, mask, site_expr)                             \
	do {                                                                                  \
		uintptr_t match_ = (ctx)->m_game_api->m_sigscan((ctx)->m_game_module, sig, mask); \
		if (match_) {                                                                     \
			DTTR_INSTALL_POINTER_AT(name, ctx, site_expr, name##_callback);               \
		} else {                                                                          \
			DTTR_COMPONENT_LOG_ERROR(ctx, #name ": signature not found");                 \
		}                                                                                 \
	} while (0)

// Scan for a signature and resolve a DTTR_FUNC or DTTR_VAR address.
// expr is evaluated with match in scope.
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

// Resolve an E8 relative call at address p to its absolute target.
#define DTTR_E8_TARGET(p) ((p) + 5 + *(int32_t *)((p) + 1))

// Read the absolute jump target from an FF 25 import thunk at address p.
#define DTTR_FF25_ADDR(p) (*(uint32_t *)((p) + 2))

// Logging macros.

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

#define DTTR_EXPORT __declspec(dllexport)

#define DTTR_COMPONENT_INFO(name, version, author)                                       \
	static const DTTR_ComponentInfo s_dttr_component_info_ = {                           \
		.m_name = name,                                                                  \
		.m_version = version,                                                            \
		.m_author = author,                                                              \
	};                                                                                   \
	DTTR_EXPORT const DTTR_ComponentInfo *dttr_component_info(void) {                    \
		return &s_dttr_component_info_;                                                  \
	}

// Check API version and delegate to the component body.
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
#define DTTR_COMPONENT_LATE_INIT DTTR_EXPORT void dttr_component_late_init(void)
#define DTTR_COMPONENT_BEFORE_UNLOAD DTTR_EXPORT void dttr_component_before_unload(void)

#define DTTR_COMPONENT_FRAME_BEGIN                                                       \
	DTTR_EXPORT void dttr_component_frame_begin(const DTTR_FrameContext *ctx)

#define DTTR_COMPONENT_BEFORE_GAME_FRAME                                                 \
	DTTR_EXPORT void dttr_component_before_game_frame(const DTTR_FrameContext *ctx)

#define DTTR_COMPONENT_AFTER_GAME_FRAME                                                  \
	DTTR_EXPORT void dttr_component_after_game_frame(const DTTR_FrameContext *ctx)

#define DTTR_COMPONENT_BEFORE_PRESENT                                                    \
	DTTR_EXPORT void dttr_component_before_present(const DTTR_PresentContext *ctx)

#define DTTR_COMPONENT_AFTER_PRESENT                                                     \
	DTTR_EXPORT void dttr_component_after_present(const DTTR_PresentContext *ctx)

#define DTTR_COMPONENT_FRAME_END                                                         \
	DTTR_EXPORT void dttr_component_frame_end(const DTTR_FrameContext *ctx)

#define DTTR_COMPONENT_IMGUI_BEGIN                                                       \
	DTTR_EXPORT void dttr_component_imgui_begin(const DTTR_RenderContext *ctx)

#define DTTR_COMPONENT_IMGUI_END                                                         \
	DTTR_EXPORT void dttr_component_imgui_end(const DTTR_RenderContext *ctx)

#define DTTR_COMPONENT_OVERLAY_VISIBLE_CHANGED                                           \
	DTTR_EXPORT void dttr_component_overlay_visible_changed(bool visible)

#define DTTR_COMPONENT_WINDOW_CREATED                                                    \
	DTTR_EXPORT void dttr_component_window_created(const DTTR_WindowContext *ctx)

#define DTTR_COMPONENT_WINDOW_RESIZED                                                    \
	DTTR_EXPORT void dttr_component_window_resized(const DTTR_WindowContext *ctx)

#define DTTR_COMPONENT_WINDOW_DESTROYING                                                 \
	DTTR_EXPORT void dttr_component_window_destroying(const DTTR_WindowContext *ctx)

#define DTTR_COMPONENT_GRAPHICS_DEVICE_CREATED                                           \
	DTTR_EXPORT void dttr_component_graphics_device_created(                             \
		const DTTR_GraphicsContext *ctx                                                  \
	)

#define DTTR_COMPONENT_GRAPHICS_DEVICE_LOST                                              \
	DTTR_EXPORT void dttr_component_graphics_device_lost(const DTTR_GraphicsContext *ctx)

#define DTTR_COMPONENT_GRAPHICS_DEVICE_RESTORED                                          \
	DTTR_EXPORT void dttr_component_graphics_device_restored(                            \
		const DTTR_GraphicsContext *ctx                                                  \
	)

#define DTTR_COMPONENT_GRAPHICS_DEVICE_DESTROYING                                        \
	DTTR_EXPORT void dttr_component_graphics_device_destroying(                          \
		const DTTR_GraphicsContext *ctx                                                  \
	)

#define DTTR_COMPONENT_BEFORE_EVENT                                                      \
	DTTR_EXPORT bool dttr_component_before_event(const SDL_Event *event)

#define DTTR_COMPONENT_AFTER_EVENT                                                       \
	DTTR_EXPORT void dttr_component_after_event(const SDL_Event *event, bool consumed)

#define DTTR_COMPONENT_INPUT_MODE_CHANGED                                                \
	DTTR_EXPORT void dttr_component_input_mode_changed(const DTTR_InputContext *ctx)

// Return true to consume the event.
#define DTTR_COMPONENT_EVENT DTTR_EXPORT bool dttr_component_event(const SDL_Event *event)

// Render at game resolution, letterboxed and scaled with the game image.
#define DTTR_COMPONENT_RENDER_GAME                                                       \
	DTTR_EXPORT void dttr_component_render_game(const DTTR_RenderGameContext *ctx)

// Render at full window resolution, above letterbox bars.
#define DTTR_COMPONENT_RENDER                                                            \
	DTTR_EXPORT void dttr_component_render(const DTTR_RenderContext *ctx)

// Return false to skip this host-loop game frame while still presenting overlays.
#define DTTR_COMPONENT_SHOULD_ADVANCE_GAME_FRAME                                         \
	DTTR_EXPORT bool dttr_component_should_advance_game_frame(void)

// Called after a game frame was advanced because all components allowed it.
#define DTTR_COMPONENT_GAME_FRAME_ADVANCED                                               \
	DTTR_EXPORT void dttr_component_game_frame_advanced(void)

// Called after a host frame presented overlays without advancing the game.
#define DTTR_COMPONENT_GAME_FRAME_BLOCKED                                                \
	DTTR_EXPORT void dttr_component_game_frame_blocked(void)

#endif /* DTTR_COMPONENTS_H */
