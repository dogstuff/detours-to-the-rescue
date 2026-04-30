#include "imports_internal.h"

#define S_DDRAW_IMPORTS(X)                                                               \
	X(SKIP,                                                                              \
	  dttr_import_ddraw,                                                                 \
	  directdrawcreateex,                                                                \
	  "DirectDrawCreateEx",                                                              \
	  "DirectDrawCreateEx")                                                              \
	X(SKIP,                                                                              \
	  dttr_import_ddraw,                                                                 \
	  directdrawenumerateexa,                                                            \
	  "DirectDrawEnumerateExA",                                                          \
	  "DirectDrawEnumerateExA")

S_DDRAW_IMPORTS(DTTR_IMPORT_ENTRY_DECL)

static const DTTR_ImportHookSpec s_ddraw_hooks[] = {
	S_DDRAW_IMPORTS(DTTR_IMPORT_ENTRY_SPEC)
};

void dttr_platform_hooks_ddraw_init(const DTTR_ComponentContext *ctx) {
	dttr_platform_hooks_install_module(
		ctx,
		"DDRAW.dll",
		s_ddraw_hooks,
		DTTR_IMPORT_ARRAY_COUNT(s_ddraw_hooks)
	);
}

void dttr_platform_hooks_ddraw_cleanup(const DTTR_ComponentContext *ctx) {
	dttr_platform_hooks_cleanup_module(
		ctx,
		s_ddraw_hooks,
		DTTR_IMPORT_ARRAY_COUNT(s_ddraw_hooks)
	);
}

#undef S_DDRAW_IMPORTS
