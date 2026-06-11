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

/// Host context passed to DTTR_Mod_Init. The pointer is valid until DTTR_Mod_Cleanup
/// returns, so mods may retain it for logging and runtime cleanup. Contained
/// window/graphics resources have shorter host lifetimes and may also outlive a
/// hot-reloaded mod: use destroying callbacks for host teardown/device loss, and
/// cleanup for resources owned by one mod instance.
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

typedef enum {
	/// Host-native timing.
	DTTR_MODS_TIMING_NATIVE = 0,

	/// Fixed-step accumulator loop.
	DTTR_MODS_TIMING_FIXED_SIM_VARIABLE_RENDER = 1,
} DTTR_Mods_TimingMode;

/// Rational num/den Hz. den must be nonzero.
typedef struct {
	uint32_t num;
	uint32_t den;
} DTTR_Mods_RatioU32;

typedef struct {
	uint32_t struct_size;
	uint32_t abi_version;
	uint32_t flags;
	DTTR_Mods_TimingMode mode;

	/// Slowest acceptable simulation rate.
	DTTR_Mods_RatioU32 min_sim_hz;

	/// Fastest acceptable simulation rate.
	DTTR_Mods_RatioU32 max_sim_hz;

	/// Target simulation rate; required for fixed timing.
	DTTR_Mods_RatioU32 preferred_sim_hz;

	/// Values above 1 cost one full render+present per step.
	uint32_t max_sim_steps_per_host_frame;

	/// Largest host frame delta before clamping.
	uint64_t max_host_delta_ns;

	/// Largest accumulated debt before clamping.
	uint64_t max_accumulator_debt_ns;

	void *reserved[4];
} DTTR_Mods_TimingPolicyRequest;

/// Identifies which timing callback a DTTR_Mods_TimingFrameState describes.
typedef enum {
	DTTR_MODS_TIMING_PHASE_HOST_FRAME_BEGIN = 1,
	DTTR_MODS_TIMING_PHASE_BEFORE_SIMULATION_STEP = 2,
	DTTR_MODS_TIMING_PHASE_AFTER_SIMULATION_STEP = 3,
	DTTR_MODS_TIMING_PHASE_BEFORE_RENDER_FRAME = 4,
	DTTR_MODS_TIMING_PHASE_AFTER_RENDER_FRAME = 5,
	DTTR_MODS_TIMING_PHASE_BEFORE_PRESENT_FRAME = 6,
	DTTR_MODS_TIMING_PHASE_AFTER_PRESENT_FRAME = 7,
	DTTR_MODS_TIMING_PHASE_HOST_FRAME_END = 8,
	DTTR_MODS_TIMING_PHASE_SIMULATION_STEP_DEFERRED = 9,
} DTTR_Mods_TimingPhase;

/// Per-phase snapshot passed to timing callbacks; values are only valid for the
/// duration of the callback.
typedef struct {
	uint32_t struct_size;
	uint32_t abi_version;
	uint32_t flags;
	DTTR_Mods_TimingPhase phase;
	uint64_t host_frame_index;
	uint64_t render_frame_index;
	uint64_t simulation_tick_index;
	uint64_t monotonic_time_ns;
	uint64_t host_delta_ns;

	/// Fixed-step duration; 0 under native timing.
	uint64_t sim_step_ns;

	/// Unconsumed simulation debt.
	uint64_t accumulator_ns;

	/// [0,1) render position between sim steps.
	float interpolation_alpha;

	uint32_t sim_steps_due;
	uint32_t sim_steps_ran_this_host_frame;
	uint32_t sim_steps_deferred_this_host_frame;

	/// True when the host frame ran no simulation step.
	bool render_reuses_previous_sim_state;
} DTTR_Mods_TimingFrameState;

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

typedef bool (*DTTR_Mods_QueryTimingPolicyFn)(DTTR_Mods_TimingPolicyRequest *out_request);
typedef void (*DTTR_Mods_TimingHostFrameBeginFn)(const DTTR_Mods_TimingFrameState *ctx);
typedef bool (*DTTR_Mods_TimingShouldRunSimulationStepFn)(
	const DTTR_Mods_TimingFrameState *ctx
);
typedef void (*DTTR_Mods_TimingBeforeSimulationStepFn)(
	const DTTR_Mods_TimingFrameState *ctx
);
typedef void (*DTTR_Mods_TimingAfterSimulationStepFn)(
	const DTTR_Mods_TimingFrameState *ctx
);
typedef void (*DTTR_Mods_TimingSimulationStepDeferredFn)(
	const DTTR_Mods_TimingFrameState *ctx
);
typedef void (*DTTR_Mods_TimingBeforeRenderFrameFn)(const DTTR_Mods_TimingFrameState *ctx);
typedef void (*DTTR_Mods_TimingAfterRenderFrameFn)(const DTTR_Mods_TimingFrameState *ctx);
typedef void (*DTTR_Mods_TimingBeforePresentFrameFn)(
	const DTTR_Mods_TimingFrameState *ctx
);
typedef void (*DTTR_Mods_TimingAfterPresentFrameFn)(const DTTR_Mods_TimingFrameState *ctx);
typedef void (*DTTR_Mods_TimingHostFrameEndFn)(const DTTR_Mods_TimingFrameState *ctx);

typedef void (*DTTR_Mods_GameFrameAdvancedFn)();

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

/// Check SDK ABI compatibility and delegate to the mod body.
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

/// Called at the start of a host frame before game-frame advancement and presentation.
/// The context pointer is callback-local; copy values you need after return.
#define DTTR_MODS_FRAME_BEGIN                                                            \
	DTTR_EXPORT void DTTR_Mod_FrameBegin(const DTTR_Mods_FrameContext *ctx)

/// Called at the end of a host frame after game-frame and presentation callbacks.
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

/// Return true to consume the event.
#define DTTR_MODS_EVENT DTTR_EXPORT bool DTTR_Mod_Event(const SDL_Event *event)

/// Render at game resolution, letterboxed and scaled with the game image.
#define DTTR_MODS_RENDER_GAME                                                            \
	DTTR_EXPORT void DTTR_Mod_RenderGame(const DTTR_Mods_RenderGameContext *ctx)

/// Render at full window resolution, above letterbox bars.
#define DTTR_MODS_RENDER                                                                 \
	DTTR_EXPORT void DTTR_Mod_Render(const DTTR_Mods_RenderContext *ctx)

/// Called once at the first host frame. Return true and fill out_request to opt into a
/// timing policy; return false to stay on native timing.
#define DTTR_MODS_QUERY_TIMING_POLICY                                                    \
	DTTR_EXPORT bool DTTR_Mod_QueryTimingPolicy(                                         \
		DTTR_Mods_TimingPolicyRequest *out_request                                       \
	)

/// Called at the start of each host frame, before any simulation step.
#define DTTR_MODS_TIMING_HOST_FRAME_BEGIN                                                \
	DTTR_EXPORT void DTTR_Mod_TimingHostFrameBegin(const DTTR_Mods_TimingFrameState *ctx)

/// Called before each due simulation step. Return false to defer the step.
#define DTTR_MODS_TIMING_SHOULD_RUN_SIMULATION_STEP                                      \
	DTTR_EXPORT bool DTTR_Mod_TimingShouldRunSimulationStep(                             \
		const DTTR_Mods_TimingFrameState *ctx                                            \
	)

/// Called immediately before a simulation step runs.
#define DTTR_MODS_TIMING_BEFORE_SIMULATION_STEP                                          \
	DTTR_EXPORT void DTTR_Mod_TimingBeforeSimulationStep(                                \
		const DTTR_Mods_TimingFrameState *ctx                                            \
	)

/// Called immediately after a simulation step runs.
#define DTTR_MODS_TIMING_AFTER_SIMULATION_STEP                                           \
	DTTR_EXPORT void DTTR_Mod_TimingAfterSimulationStep(                                 \
		const DTTR_Mods_TimingFrameState *ctx                                            \
	)

/// Called when a due simulation step did not run this host frame.
#define DTTR_MODS_TIMING_SIMULATION_STEP_DEFERRED                                        \
	DTTR_EXPORT void DTTR_Mod_TimingSimulationStepDeferred(                              \
		const DTTR_Mods_TimingFrameState *ctx                                            \
	)

/// Called immediately before the host renders the frame.
#define DTTR_MODS_TIMING_BEFORE_RENDER_FRAME                                             \
	DTTR_EXPORT void DTTR_Mod_TimingBeforeRenderFrame(                                   \
		const DTTR_Mods_TimingFrameState *ctx                                            \
	)

/// Called immediately after the host renders the frame.
#define DTTR_MODS_TIMING_AFTER_RENDER_FRAME                                              \
	DTTR_EXPORT void DTTR_Mod_TimingAfterRenderFrame(                                    \
		const DTTR_Mods_TimingFrameState *ctx                                            \
	)

/// Called immediately before the rendered frame is presented.
#define DTTR_MODS_TIMING_BEFORE_PRESENT_FRAME                                            \
	DTTR_EXPORT void DTTR_Mod_TimingBeforePresentFrame(                                  \
		const DTTR_Mods_TimingFrameState *ctx                                            \
	)

/// Called immediately after the rendered frame is presented.
#define DTTR_MODS_TIMING_AFTER_PRESENT_FRAME                                             \
	DTTR_EXPORT void DTTR_Mod_TimingAfterPresentFrame(                                   \
		const DTTR_Mods_TimingFrameState *ctx                                            \
	)

/// Called at the end of each host frame, after render and present.
#define DTTR_MODS_TIMING_HOST_FRAME_END                                                  \
	DTTR_EXPORT void DTTR_Mod_TimingHostFrameEnd(const DTTR_Mods_TimingFrameState *ctx)

/// Called after a game frame was advanced because all mods allowed it.
#define DTTR_MODS_GAME_FRAME_ADVANCED DTTR_EXPORT void DTTR_Mod_GameFrameAdvanced()

#endif // DTTR_MODS_H
