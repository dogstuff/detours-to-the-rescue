#include <SDL3/SDL_events.h>
#include <dttr_components.h>
#include <dttr_interop.h>
#include <stdbool.h>

static const DTTR_ComponentContext *s_ctx;

static const DTTR_ComponentInfo s_info = {
	.m_name = "Example Component",
	.m_version = "1.0.0",
	.m_author = "DttR",
};

// Example: declare a cached function wrapper via signature scan
// clang-format off
// DTTR_INTEROP_WRAP_CACHED_CC_SIG(
// 	pcdogs_example_func, __stdcall,
// 	"\x55\x8B\xEC\x83\xEC\x10",
// 	"xxxxxx",
// 	match,
// 	int, (int arg1, int arg2), (arg1, arg2)
// )
// clang-format on

// Example: declare a variable accessor via signature scan
// clang-format off
// DTTR_INTEROP_VAR_SIG(
// 	pcdogs_example_var, int,
// 	"\xA1\x00\x00\x00\x00\x85\xC0",
// 	"x????xx",
// 	*(uintptr_t *)(match + 1)
// )
// clang-format on

// Example: declare a hook with trampoline for calling the original
// static int __stdcall s_my_hook(int arg1, int arg2);
//
// clang-format off
// DTTR_INTEROP_HOOK_FUNC_TRAMPOLINE_SIG(
// 	hook_example, 6,
// 	"\x55\x8B\xEC\x83\xEC\x10",
// 	"xxxxxx",
// 	match,
// 	s_my_hook
// )
// clang-format on
//
// static int __stdcall s_my_hook(int arg1, int arg2) {
// 	// Call original via trampoline
// 	typedef int (__stdcall *orig_fn_t)(int, int);
// 	return ((orig_fn_t)hook_example_trampoline)(arg1, arg2);
// }

__declspec(dllexport) const DTTR_ComponentInfo *dttr_component_info(void) {
	return &s_info;
}

__declspec(dllexport) bool dttr_component_init(const DTTR_ComponentContext *ctx) {
	if (ctx->m_api_version != DTTR_COMPONENT_API_VERSION) {
		return false;
	}

	s_ctx = ctx;

	DTTR_LOG_INFO(s_ctx, "Example component initialized");

	// Example: resolve cached function & variable, apply hook
	// if (!pcdogs_example_func_init(ctx)) {
	// 	DTTR_LOG_ERROR(ctx, "Failed to resolve example_func");
	// 	return false;
	// }
	// if (!pcdogs_example_var_init(ctx)) {
	// 	DTTR_LOG_ERROR(ctx, "Failed to resolve example_var");
	// 	return false;
	// }
	// if (!hook_example_install(ctx)) {
	// 	DTTR_LOG_ERROR(ctx, "Failed to apply example hook");
	// 	return false;
	// }

	return true;
}

__declspec(dllexport) void dttr_component_tick(void) {
	// Called every frame
}

__declspec(dllexport) bool dttr_component_event(const SDL_Event *event) {
	// Return true to consume the event
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
	// Example: remove hooks before unload
	// hook_example_unpatch(s_ctx);

	DTTR_LOG_INFO(s_ctx, "Example component cleaned up");
	s_ctx = NULL;
}
