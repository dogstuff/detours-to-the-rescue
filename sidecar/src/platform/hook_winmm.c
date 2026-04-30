#include "imports_internal.h"

#define S_WINMM_IMPORTS(X)                                                               \
	X(TRACE, dttr_import_winmm, timegettime, "timeGetTime", "WINMM.dll!timeGetTime")     \
	X(WARN,                                                                              \
	  dttr_import_winmm,                                                                 \
	  mcisendstringa,                                                                    \
	  "mciSendStringA",                                                                  \
	  "WINMM.dll!mciSendStringA")

S_WINMM_IMPORTS(DTTR_IMPORT_ENTRY_DECL)

static const DTTR_ImportHookSpec s_winmm_hooks[] = {
	S_WINMM_IMPORTS(DTTR_IMPORT_ENTRY_SPEC)
};

void dttr_platform_hooks_winmm_init(const DTTR_ComponentContext *ctx) {
	dttr_platform_hooks_install_module(
		ctx,
		"WINMM.dll",
		s_winmm_hooks,
		DTTR_IMPORT_ARRAY_COUNT(s_winmm_hooks)
	);
}

void dttr_platform_hooks_winmm_cleanup(const DTTR_ComponentContext *ctx) {
	dttr_platform_hooks_cleanup_module(
		ctx,
		s_winmm_hooks,
		DTTR_IMPORT_ARRAY_COUNT(s_winmm_hooks)
	);
}

#undef S_WINMM_IMPORTS
