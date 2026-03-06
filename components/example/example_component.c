#include <SDL3/SDL_events.h>
#include <dttr_components.h>
#include <stdbool.h>

#define DTTR_INTEROP_IMPLEMENT
#define log_debug(...) DTTR_LOG_DEBUG(s_ctx, __VA_ARGS__)
#define log_error(...) DTTR_LOG_ERROR(s_ctx, __VA_ARGS__)

static const DTTR_ComponentContext *s_ctx;

static const DTTR_ComponentInfo s_info = {
	.m_name = "Example Component",
	.m_version = "1.0.0",
	.m_author = "DttR",
};

// Example: declare a callable function wrapper resolved by sigscan.
// DTTR_FUNC(pcdogs_example_func, __stdcall, int, (int a, int b), (a, b))

// Example: declare a variable accessor resolved by sigscan.
// DTTR_VAR(pcdogs_example_var, int)

// Example: declare a trampoline hook that calls through to the original.
// DTTR_TRAMPOLINE_HOOK(hook_example)
//
// static int __stdcall hook_example_callback(int a, int b) {
// 	typedef int (__stdcall *orig_t)(int, int);
// 	return ((orig_t)hook_example_trampoline)(a, b);
// }

__declspec(dllexport) const DTTR_ComponentInfo *dttr_component_info(void) {
	return &s_info;
}

__declspec(dllexport) bool dttr_component_init(const DTTR_ComponentContext *ctx) {
	if (ctx->m_api_version < DTTR_COMPONENT_API_VERSION) {
		return false;
	}

	s_ctx = ctx;

	DTTR_LOG_INFO(s_ctx, "Example component initialized");

	// Example: resolve a function address.
	// DTTR_RESOLVE(pcdogs_example_func, ctx, "\x55\x8B\xEC", "xxx", match)

	// Example: resolve a variable address from an embedded pointer.
	// DTTR_RESOLVE(pcdogs_example_var, ctx, "\xA1????\x85\xC0", "x????xx",
	//     *(uint32_t *)(match + 1))

	// Example: install a trampoline hook with automatic prologue sizing.
	// DTTR_INSTALL_TRAMPOLINE_AUTO(hook_example, ctx, "\x55\x8B\xEC", "xxx")

	return true;
}

__declspec(dllexport) void dttr_component_tick(void) {
	// Called every frame.
}

__declspec(dllexport) bool dttr_component_event(const SDL_Event *event) {
	// Return true to consume the event.
	return false;
}

__declspec(dllexport) void dttr_component_render(
	SDL_GPUCommandBuffer *cmd,
	SDL_GPUTexture *render_target,
	uint32_t width,
	uint32_t height
) {
	// Called after game draws, before swapchain blit.
	// Create render passes against render_target with SDL_GPU_LOADOP_LOAD.
}

__declspec(dllexport) void dttr_component_cleanup(void) {
	// Example: remove a trampoline hook before unload.
	// DTTR_TRAMPOLINE_UNINSTALL(hook_example, s_ctx);

	DTTR_LOG_INFO(s_ctx, "Example component cleaned up");
	s_ctx = NULL;
}
