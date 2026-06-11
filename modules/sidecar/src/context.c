#include "context_private.h"

#include <windows.h>

#include <dttr_config.h>
#include <dttr_core.h>
#include <dttr_log.h>
#include <dttr_mods.h>
#include <dttr_runtime.h>

#include "crash_private.h"
#include "sidecar_private.h"

static DTTR_Mods_Context sidecar_ctx;

static const DTTR_Mods_API MOD_API = {
	.log = DTTR_Log,
	.log_is_enabled = DTTR_Log_IsEnabled,
	.log_unchecked = DTTR_Log_Unchecked,
	.struct_size = sizeof(DTTR_Mods_API),
	.abi_version = DTTR_SDK_ABI_VERSION,
	.write_exception_report = dttr_sidecar_write_exception_report,
};

static const DTTR_Core_API RUNTIME_API = {
	.sigscan = DTTR_Core_HookCachedSigscan,
	.hook_function = DTTR_Core_HookAttachFunction,
	.hook_pointer = DTTR_Core_HookAttachPointer,
	.patch_bytes = DTTR_Core_HookPatchBytes,
	.unhook = DTTR_Core_HookDetach,
	.hook_is_active = DTTR_Core_HookIsActive,
	.unhook_checked = DTTR_Core_HookDetachChecked,
	.struct_size = sizeof(DTTR_Core_API),
	.abi_version = DTTR_SDK_ABI_VERSION,
};

// Exposes the single sidecar context shared by hooks, mods, and runtime calls.
const DTTR_Mods_Context *dttr_sidecar_context() {
	return &sidecar_ctx;
}

const DTTR_Core_Context *dttr_sidecar_runtime_context() {
	return &sidecar_ctx.runtime;
}

// Captures module handles and APIs before callbacks expose sidecar state.
void dttr_sidecar_init_context(HMODULE game_module, HMODULE sidecar_module) {
	sidecar_ctx = (DTTR_Mods_Context){
		.abi_version = DTTR_SDK_ABI_VERSION,
		.runtime =
			(DTTR_Core_Context){
				.game_module = game_module,
				.api = &RUNTIME_API,
				.struct_size = sizeof(DTTR_Core_Context),
				.abi_version = DTTR_SDK_ABI_VERSION,
			},
		.sidecar_module = sidecar_module,
		.window = NULL,
		.loader_dir = dttr_loader_dir,
		.exe_hash = dttr_exe_hash,
		.config = &dttr_config,
		.api = &MOD_API,
		.struct_size = sizeof(DTTR_Mods_Context),
	};
}
