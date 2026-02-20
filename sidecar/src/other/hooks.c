#include "dttr_hooks_other.h"
#include "dttr_interop_pcdogs.h"
#include "log.h"

void dttr_other_hook_init(const DTTR_ComponentContext *ctx) {
	dttr_crt_open_file_with_mode_init(ctx);

	DTTR_INTEROP_HOOK_FUNC_LOG(dttr_crt_hook_open_file, ctx);
	DTTR_INTEROP_HOOK_FUNC_OPTIONAL_LOG(dttr_hook_resolve_pcdogs_path, ctx);
	DTTR_INTEROP_HOOK_FUNC_TRAMPOLINE_LOG(dttr_hook_cleanup_level_assets, ctx);
}

void dttr_other_hook_cleanup(const DTTR_ComponentContext *ctx) {
	DTTR_INTEROP_UNHOOK_LOG(dttr_hook_cleanup_level_assets, ctx);
	DTTR_INTEROP_UNHOOK_OPTIONAL_LOG(dttr_hook_resolve_pcdogs_path, ctx);
	DTTR_INTEROP_UNHOOK_LOG(dttr_crt_hook_open_file, ctx);
}
