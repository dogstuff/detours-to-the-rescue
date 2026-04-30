#include "imports_internal.h"

#define S_GDI32_IMPORTS(X)                                                               \
	X(TRACE,                                                                             \
	  dttr_import_gdi32,                                                                 \
	  getstockobject,                                                                    \
	  "GetStockObject",                                                                  \
	  "GDI32.dll!GetStockObject")

S_GDI32_IMPORTS(DTTR_IMPORT_ENTRY_DECL)

static const DTTR_ImportHookSpec s_gdi32_hooks[] = {
	S_GDI32_IMPORTS(DTTR_IMPORT_ENTRY_SPEC)
};

void dttr_platform_hooks_gdi32_init(const DTTR_ComponentContext *ctx) {
	dttr_platform_hooks_install_module(
		ctx,
		"GDI32.dll",
		s_gdi32_hooks,
		DTTR_IMPORT_ARRAY_COUNT(s_gdi32_hooks)
	);
}

void dttr_platform_hooks_gdi32_cleanup(const DTTR_ComponentContext *ctx) {
	dttr_platform_hooks_cleanup_module(
		ctx,
		s_gdi32_hooks,
		DTTR_IMPORT_ARRAY_COUNT(s_gdi32_hooks)
	);
}

#undef S_GDI32_IMPORTS
