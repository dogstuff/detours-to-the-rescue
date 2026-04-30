#include "imports_internal.h"

DTTR_IMPORT_HOOK_DECL(
	dttr_import_dinput,
	directinputcreatea,
	"DINPUT.dll!DirectInputCreateA"
)

static const DTTR_ImportHookSpec s_dinput_hooks[] = {
	DTTR_IMPORT_HOOK_SPEC(dttr_import_dinput, directinputcreatea, "DirectInputCreateA")
};

void dttr_platform_hooks_dinput_init(const DTTR_ComponentContext *ctx) {
	dttr_platform_hooks_install_module(
		ctx,
		"DINPUT.dll",
		s_dinput_hooks,
		sizeof(s_dinput_hooks) / sizeof(s_dinput_hooks[0])
	);
}

void dttr_platform_hooks_dinput_cleanup(const DTTR_ComponentContext *ctx) {
	dttr_platform_hooks_cleanup_module(
		ctx,
		s_dinput_hooks,
		sizeof(s_dinput_hooks) / sizeof(s_dinput_hooks[0])
	);
}
