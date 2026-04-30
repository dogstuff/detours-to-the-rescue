#include "imports_internal.h"

static const DTTR_ImportHookSpec s_ddraw_hooks[] = {
	DTTR_IMPORT_SHOULD_NOT_CALL_SPEC("DirectDrawCreateEx"),
	DTTR_IMPORT_SHOULD_NOT_CALL_SPEC("DirectDrawEnumerateExA")
};

void dttr_platform_hooks_ddraw_init(const DTTR_ComponentContext *ctx) {
	dttr_platform_hooks_install_module(
		ctx,
		"DDRAW.dll",
		s_ddraw_hooks,
		sizeof(s_ddraw_hooks) / sizeof(s_ddraw_hooks[0])
	);
}

void dttr_platform_hooks_ddraw_cleanup(const DTTR_ComponentContext *ctx) {
	dttr_platform_hooks_cleanup_module(
		ctx,
		s_ddraw_hooks,
		sizeof(s_ddraw_hooks) / sizeof(s_ddraw_hooks[0])
	);
}
