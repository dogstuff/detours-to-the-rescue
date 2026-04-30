#include "imports_internal.h"

DTTR_IMPORT_HOOK_DECL(dttr_import_winmm, timegettime, "WINMM.dll!timeGetTime")
DTTR_IMPORT_HOOK_WARN_DECL(dttr_import_winmm, mcisendstringa, "WINMM.dll!mciSendStringA")

static const DTTR_ImportHookSpec s_winmm_hooks[] = {
	DTTR_IMPORT_HOOK_SPEC(dttr_import_winmm, timegettime, "timeGetTime"),
	DTTR_IMPORT_HOOK_SPEC(dttr_import_winmm, mcisendstringa, "mciSendStringA")
};

void dttr_platform_hooks_winmm_init(const DTTR_ComponentContext *ctx) {
	dttr_platform_hooks_install_module(
		ctx,
		"WINMM.dll",
		s_winmm_hooks,
		sizeof(s_winmm_hooks) / sizeof(s_winmm_hooks[0])
	);
}

void dttr_platform_hooks_winmm_cleanup(const DTTR_ComponentContext *ctx) {
	dttr_platform_hooks_cleanup_module(
		ctx,
		s_winmm_hooks,
		sizeof(s_winmm_hooks) / sizeof(s_winmm_hooks[0])
	);
}
