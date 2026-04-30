#include "imports_internal.h"

#define S_DINPUT_IMPORTS(X)                                                              \
	X(TRACE,                                                                             \
	  dttr_import_dinput,                                                                \
	  directinputcreatea,                                                                \
	  "DirectInputCreateA",                                                              \
	  "DINPUT.dll!DirectInputCreateA")

S_DINPUT_IMPORTS(DTTR_IMPORT_ENTRY_DECL)

static const DTTR_ImportHookSpec s_dinput_hooks[] = {
	S_DINPUT_IMPORTS(DTTR_IMPORT_ENTRY_SPEC)
};

void dttr_platform_hooks_dinput_init(const DTTR_ComponentContext *ctx) {
	dttr_platform_hooks_install_module(
		ctx,
		"DINPUT.dll",
		s_dinput_hooks,
		DTTR_IMPORT_ARRAY_COUNT(s_dinput_hooks)
	);
}

void dttr_platform_hooks_dinput_cleanup(const DTTR_ComponentContext *ctx) {
	dttr_platform_hooks_cleanup_module(
		ctx,
		s_dinput_hooks,
		DTTR_IMPORT_ARRAY_COUNT(s_dinput_hooks)
	);
}

#undef S_DINPUT_IMPORTS
