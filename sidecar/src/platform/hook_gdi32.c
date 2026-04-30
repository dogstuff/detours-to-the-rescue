#include "imports_internal.h"

DTTR_IMPORT_HOOK_DECL(dttr_import_gdi32, getstockobject, "GDI32.dll!GetStockObject")

static const DTTR_ImportHookSpec s_gdi32_hooks[] = {
	DTTR_IMPORT_HOOK_SPEC(dttr_import_gdi32, getstockobject, "GetStockObject")
};

void dttr_platform_hooks_gdi32_init(const DTTR_ComponentContext *ctx) {
	dttr_platform_hooks_install_module(
		ctx,
		"GDI32.dll",
		s_gdi32_hooks,
		sizeof(s_gdi32_hooks) / sizeof(s_gdi32_hooks[0])
	);
}

void dttr_platform_hooks_gdi32_cleanup(const DTTR_ComponentContext *ctx) {
	dttr_platform_hooks_cleanup_module(
		ctx,
		s_gdi32_hooks,
		sizeof(s_gdi32_hooks) / sizeof(s_gdi32_hooks[0])
	);
}
