/// Mod DLL API for modules loaded from `mods/`.
///
/// Mods must export `DTTR_Mod_Init` and `DTTR_Mod_Cleanup`;
/// optional callbacks can be exported to observe frame, window, graphics, input,
/// event, and unload lifecycle events.

#ifndef DTTR_MODS_H
#define DTTR_MODS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <windows.h>

#include <dttr_runtime.h>

typedef struct SDL_Window SDL_Window;
typedef union SDL_Event SDL_Event;

#define DTTR_MODS_EXCEPTION_REPORT_STACK_TRACE_CAPACITY 16384u

typedef struct {
	uint32_t struct_size;
	EXCEPTION_RECORD exception_record;
	CONTEXT context;
	DWORD thread_id;
	const char *tag;
} DTTR_Mods_ExceptionReportRequest;

typedef struct {
	uint32_t struct_size;
	bool dump_written;
	bool stack_trace_written;
	char dump_path[MAX_PATH];
	char stack_trace[DTTR_MODS_EXCEPTION_REPORT_STACK_TRACE_CAPACITY];
	DWORD win32_error;
} DTTR_Mods_ExceptionReport;

typedef bool (*DTTR_Mods_WriteExceptionReportFn)(
	const DTTR_Mods_ExceptionReportRequest *request,
	DTTR_Mods_ExceptionReport *report
);

typedef void (*DTTR_Mods_LogFn)(
	int level,
	const char *file,
	int line,
	const char *fmt,
	...
);
typedef bool (*DTTR_Mods_LogIsEnabledFn)(int level);
typedef struct {
	DTTR_Mods_LogFn log;
	DTTR_Mods_LogIsEnabledFn log_is_enabled;
	DTTR_Mods_LogFn log_unchecked;
	uint32_t struct_size;
	uint32_t abi_version;
	uint32_t flags;
	DTTR_Mods_WriteExceptionReportFn write_exception_report;
} DTTR_Mods_API;

static inline DTTR_Mods_WriteExceptionReportFn DTTR_Mods_GetWriteExceptionReportFn(
	const DTTR_Mods_API *api
) {
	if (!api || api->abi_version < DTTR_SDK_ABI_VERSION
		|| api->struct_size < offsetof(DTTR_Mods_API, write_exception_report)
								  + sizeof(api->write_exception_report)) {
		return NULL;
	}

	return api->write_exception_report;
}

// Host context passed to DTTR_Mod_Init. The pointer is valid until DTTR_Mod_Cleanup
// returns, so mods may retain it for logging and runtime cleanup. Contained
// window/graphics resources have shorter host lifetimes and may also outlive a
// hot-reloaded mod: use destroying callbacks for host teardown/device loss, and
// cleanup for resources owned by one mod instance.
typedef struct {
	uint32_t abi_version;
	DTTR_Core_Context runtime;
	HMODULE sidecar_module;
	SDL_Window *window;
	const char *loader_dir;
	const char *exe_hash;
	const void *config;
	const DTTR_Mods_API *api;
	uint32_t struct_size;
	uint32_t flags;
	const void *reserved[4];
} DTTR_Mods_Context;

typedef struct {
	const char *name;
	const char *version;
	const char *author;
} DTTR_Mods_Info;

typedef bool (*DTTR_Mods_InitFn)(const DTTR_Mods_Context *ctx);
typedef void (*DTTR_Mods_CleanupFn)();
typedef void (*DTTR_Mods_TickFn)();
typedef bool (*DTTR_Mods_EventFn)(const SDL_Event *event);
typedef const DTTR_Mods_Info *(*DTTR_Mods_InfoFn)();
typedef void (*DTTR_Mods_LateInitFn)();
typedef void (*DTTR_Mods_BeforeUnloadFn)();

typedef struct {
	uint64_t frame_index;
	uint32_t window_w;
	uint32_t window_h;
	uint32_t game_x;
	uint32_t game_y;
	uint32_t game_w;
	uint32_t game_h;
	float scale;
} DTTR_Mods_FrameContext;

typedef struct {
	uint64_t frame_index;
	uint32_t window_w;
	uint32_t window_h;
	uint32_t game_x;
	uint32_t game_y;
	uint32_t game_w;
	uint32_t game_h;
	float scale;
	bool imgui_frame_active;
	bool overlay_rendered;
} DTTR_Mods_PresentContext;

typedef struct {
	SDL_Window *window;
	HWND hwnd;
	uint32_t window_w;
	uint32_t window_h;
} DTTR_Mods_WindowContext;

typedef enum {
	DTTR_MODS_GRAPHICS_BACKEND_UNKNOWN = 0,
	DTTR_MODS_GRAPHICS_BACKEND_SDL_GPU = 1,
	DTTR_MODS_GRAPHICS_BACKEND_OPENGL = 2,
} DTTR_Mods_GraphicsBackend;

typedef struct {
	SDL_Window *window;
	HWND hwnd;
	DTTR_Mods_GraphicsBackend backend;
	const char *driver_name;
	uint32_t render_w;
	uint32_t render_h;
} DTTR_Mods_GraphicsContext;

typedef struct {
	bool overlay_visible;
	bool game_input_enabled;
} DTTR_Mods_InputContext;

typedef struct {
	uint32_t width;
	uint32_t height;
	float scale;
} DTTR_Mods_RenderGameContext;

typedef struct {
	uint32_t window_w;
	uint32_t window_h;
	uint32_t game_x;
	uint32_t game_y;
	uint32_t game_w;
	uint32_t game_h;
	float scale;
} DTTR_Mods_RenderContext;

typedef void (*DTTR_Mods_RenderGameFn)(const DTTR_Mods_RenderGameContext *ctx);
typedef void (*DTTR_Mods_RenderFn)(const DTTR_Mods_RenderContext *ctx);

typedef void (*DTTR_Mods_FrameBeginFn)(const DTTR_Mods_FrameContext *ctx);
typedef void (*DTTR_Mods_BeforeGameFrameFn)(const DTTR_Mods_FrameContext *ctx);
typedef void (*DTTR_Mods_AfterGameFrameFn)(const DTTR_Mods_FrameContext *ctx);
typedef void (*DTTR_Mods_BeforePresentFn)(const DTTR_Mods_PresentContext *ctx);
typedef void (*DTTR_Mods_AfterPresentFn)(const DTTR_Mods_PresentContext *ctx);
typedef void (*DTTR_Mods_FrameEndFn)(const DTTR_Mods_FrameContext *ctx);
typedef void (*DTTR_Mods_ImGuiBeginFn)(const DTTR_Mods_RenderContext *ctx);
typedef void (*DTTR_Mods_ImGuiEndFn)(const DTTR_Mods_RenderContext *ctx);
typedef void (*DTTR_Mods_OverlayVisibleChangedFn)(bool visible);
typedef void (*DTTR_Mods_WindowCreatedFn)(const DTTR_Mods_WindowContext *ctx);
typedef void (*DTTR_Mods_WindowResizedFn)(const DTTR_Mods_WindowContext *ctx);
typedef void (*DTTR_Mods_WindowDestroyingFn)(const DTTR_Mods_WindowContext *ctx);
typedef void (*DTTR_Mods_GraphicsDeviceCreatedFn)(const DTTR_Mods_GraphicsContext *ctx);
typedef void (*DTTR_Mods_GraphicsDeviceLostFn)(const DTTR_Mods_GraphicsContext *ctx);
typedef void (*DTTR_Mods_GraphicsDeviceRestoredFn)(const DTTR_Mods_GraphicsContext *ctx);
typedef void (*DTTR_Mods_GraphicsDeviceDestroyingFn)(const DTTR_Mods_GraphicsContext *ctx);
typedef bool (*DTTR_Mods_BeforeEventFn)(const SDL_Event *event);
typedef void (*DTTR_Mods_AfterEventFn)(const SDL_Event *event, bool consumed);
typedef void (*DTTR_Mods_InputModeChangedFn)(const DTTR_Mods_InputContext *ctx);

typedef bool (*DTTR_Mods_ShouldAdvanceGameFrameFn)();
typedef void (*DTTR_Mods_GameFrameAdvancedFn)();
typedef void (*DTTR_Mods_GameFrameBlockedFn)();

// Logging macros.

#define DTTR_MODS_LOG_LVL_TRACE 0
#define DTTR_MODS_LOG_LVL_DEBUG 1
#define DTTR_MODS_LOG_LVL_INFO 2
#define DTTR_MODS_LOG_LVL_WARN 3
#define DTTR_MODS_LOG_LVL_ERROR 4
#define DTTR_MODS_LOG_LVL_FATAL 5

#define DTTR_MODS_LOG(ctx, level, ...)                                                   \
	do {                                                                                 \
		if ((ctx)->api->log_is_enabled(level)) {                                         \
			(ctx)->api->log_unchecked(level, __FILE__, __LINE__, __VA_ARGS__);           \
		}                                                                                \
	} while (0)
#define DTTR_MODS_LOG_TRACE(ctx, ...)                                                    \
	DTTR_MODS_LOG(ctx, DTTR_MODS_LOG_LVL_TRACE, __VA_ARGS__)
#define DTTR_MODS_LOG_DEBUG(ctx, ...)                                                    \
	DTTR_MODS_LOG(ctx, DTTR_MODS_LOG_LVL_DEBUG, __VA_ARGS__)
#define DTTR_MODS_LOG_INFO(ctx, ...)                                                     \
	DTTR_MODS_LOG(ctx, DTTR_MODS_LOG_LVL_INFO, __VA_ARGS__)
#define DTTR_MODS_LOG_WARN(ctx, ...)                                                     \
	DTTR_MODS_LOG(ctx, DTTR_MODS_LOG_LVL_WARN, __VA_ARGS__)
#define DTTR_MODS_LOG_ERROR(ctx, ...)                                                    \
	DTTR_MODS_LOG(ctx, DTTR_MODS_LOG_LVL_ERROR, __VA_ARGS__)
#define DTTR_MODS_LOG_FATAL(ctx, ...)                                                    \
	DTTR_MODS_LOG(ctx, DTTR_MODS_LOG_LVL_FATAL, __VA_ARGS__)

// Mod export macros.

#ifdef __cplusplus
#define DTTR_EXPORT extern "C" __declspec(dllexport)
#else
#define DTTR_EXPORT __declspec(dllexport)
#endif

#define DTTR_MODS_INFO(mod_name, mod_version, mod_author)                                \
	static const DTTR_Mods_Info dttr_mod_info = {                                        \
		mod_name,                                                                        \
		mod_version,                                                                     \
		mod_author,                                                                      \
	};                                                                                   \
	DTTR_EXPORT const DTTR_Mods_Info *DTTR_Mod_Info() {                                  \
		return &dttr_mod_info;                                                           \
	}

static inline bool DTTR_Mods_ContextIsCompatible(const DTTR_Mods_Context *ctx) {
	return ctx && ctx->abi_version >= DTTR_SDK_ABI_VERSION
		   && ctx->struct_size >= sizeof(DTTR_Mods_Context);
}

// Check SDK ABI compatibility and delegate to the mod body.
#define DTTR_MODS_INIT                                                                   \
	static bool dttr_mod_init(const DTTR_Mods_Context *);                                \
	DTTR_EXPORT bool DTTR_Mod_Init(const DTTR_Mods_Context *ctx) {                       \
		if (!DTTR_Mods_ContextIsCompatible(ctx)) {                                       \
			return false;                                                                \
		}                                                                                \
		return dttr_mod_init(ctx);                                                       \
	}                                                                                    \
	static bool dttr_mod_init(const DTTR_Mods_Context *ctx)

#define DTTR_MODS_CLEANUP DTTR_EXPORT void DTTR_Mod_Cleanup()
#define DTTR_MODS_TICK DTTR_EXPORT void DTTR_Mod_Tick()
#define DTTR_MODS_LATE_INIT DTTR_EXPORT void DTTR_Mod_LateInit()
#define DTTR_MODS_BEFORE_UNLOAD DTTR_EXPORT void DTTR_Mod_BeforeUnload()

// Called at the start of a host frame before game-frame advancement and presentation.
// The context pointer is callback-local; copy values you need after return.
#define DTTR_MODS_FRAME_BEGIN                                                            \
	DTTR_EXPORT void DTTR_Mod_FrameBegin(const DTTR_Mods_FrameContext *ctx)

// Called from the render backend immediately before game image submission for a host
// frame. This currently runs even when DTTR_MODS_SHOULD_ADVANCE_GAME_FRAME blocked
// simulation; use DTTR_MODS_GAME_FRAME_ADVANCED for strictly advanced-frame work.
#define DTTR_MODS_BEFORE_GAME_FRAME                                                      \
	DTTR_EXPORT void DTTR_Mod_BeforeGameFrame(const DTTR_Mods_FrameContext *ctx)

// Called from the render backend after game image submission for a host frame. This
// also runs on blocked presentation frames; use DTTR_MODS_GAME_FRAME_ADVANCED for
// strictly advanced-frame work.
#define DTTR_MODS_AFTER_GAME_FRAME                                                       \
	DTTR_EXPORT void DTTR_Mod_AfterGameFrame(const DTTR_Mods_FrameContext *ctx)

// Called after the backend has queued game/overlay draw or blit work and
// immediately before command-buffer submit or buffer swap. This can run even when
// DTTR_MODS_SHOULD_ADVANCE_GAME_FRAME blocked simulation.
#define DTTR_MODS_BEFORE_PRESENT                                                         \
	DTTR_EXPORT void DTTR_Mod_BeforePresent(const DTTR_Mods_PresentContext *ctx)

// Called after presentation work for this host frame.
#define DTTR_MODS_AFTER_PRESENT                                                          \
	DTTR_EXPORT void DTTR_Mod_AfterPresent(const DTTR_Mods_PresentContext *ctx)

// Called at the end of a host frame after game-frame and presentation callbacks.
#define DTTR_MODS_FRAME_END                                                              \
	DTTR_EXPORT void DTTR_Mod_FrameEnd(const DTTR_Mods_FrameContext *ctx)

#define DTTR_MODS_IMGUI_BEGIN                                                            \
	DTTR_EXPORT void DTTR_Mod_ImGuiBegin(const DTTR_Mods_RenderContext *ctx)

#define DTTR_MODS_IMGUI_END                                                              \
	DTTR_EXPORT void DTTR_Mod_ImGuiEnd(const DTTR_Mods_RenderContext *ctx)

#define DTTR_MODS_OVERLAY_VISIBLE_CHANGED                                                \
	DTTR_EXPORT void DTTR_Mod_OverlayVisibleChanged(bool visible)

#define DTTR_MODS_WINDOW_CREATED                                                         \
	DTTR_EXPORT void DTTR_Mod_WindowCreated(const DTTR_Mods_WindowContext *ctx)

#define DTTR_MODS_WINDOW_RESIZED                                                         \
	DTTR_EXPORT void DTTR_Mod_WindowResized(const DTTR_Mods_WindowContext *ctx)

#define DTTR_MODS_WINDOW_DESTROYING                                                      \
	DTTR_EXPORT void DTTR_Mod_WindowDestroying(const DTTR_Mods_WindowContext *ctx)

#define DTTR_MODS_GRAPHICS_DEVICE_CREATED                                                \
	DTTR_EXPORT void DTTR_Mod_GraphicsDeviceCreated(const DTTR_Mods_GraphicsContext *ctx)

#define DTTR_MODS_GRAPHICS_DEVICE_LOST                                                   \
	DTTR_EXPORT void DTTR_Mod_GraphicsDeviceLost(const DTTR_Mods_GraphicsContext *ctx)

#define DTTR_MODS_GRAPHICS_DEVICE_RESTORED                                               \
	DTTR_EXPORT void DTTR_Mod_GraphicsDeviceRestored(const DTTR_Mods_GraphicsContext *ctx)

#define DTTR_MODS_GRAPHICS_DEVICE_DESTROYING                                             \
	DTTR_EXPORT void DTTR_Mod_GraphicsDeviceDestroying(                                  \
		const DTTR_Mods_GraphicsContext *ctx                                             \
	)

#define DTTR_MODS_BEFORE_EVENT                                                           \
	DTTR_EXPORT bool DTTR_Mod_BeforeEvent(const SDL_Event *event)

#define DTTR_MODS_AFTER_EVENT                                                            \
	DTTR_EXPORT void DTTR_Mod_AfterEvent(const SDL_Event *event, bool consumed)

#define DTTR_MODS_INPUT_MODE_CHANGED                                                     \
	DTTR_EXPORT void DTTR_Mod_InputModeChanged(const DTTR_Mods_InputContext *ctx)

// Return true to consume the event.
#define DTTR_MODS_EVENT DTTR_EXPORT bool DTTR_Mod_Event(const SDL_Event *event)

// Render at game resolution, letterboxed and scaled with the game image.
#define DTTR_MODS_RENDER_GAME                                                            \
	DTTR_EXPORT void DTTR_Mod_RenderGame(const DTTR_Mods_RenderGameContext *ctx)

// Render at full window resolution, above letterbox bars.
#define DTTR_MODS_RENDER                                                                 \
	DTTR_EXPORT void DTTR_Mod_Render(const DTTR_Mods_RenderContext *ctx)

// Return false to skip this host-loop game frame while still presenting overlays.
#define DTTR_MODS_SHOULD_ADVANCE_GAME_FRAME                                              \
	DTTR_EXPORT bool DTTR_Mod_ShouldAdvanceGameFrame()

// Called after a game frame was advanced because all mods allowed it.
#define DTTR_MODS_GAME_FRAME_ADVANCED DTTR_EXPORT void DTTR_Mod_GameFrameAdvanced()

// Called after a host frame presented overlays without advancing the game.
#define DTTR_MODS_GAME_FRAME_BLOCKED DTTR_EXPORT void DTTR_Mod_GameFrameBlocked()

#endif // DTTR_MODS_H
