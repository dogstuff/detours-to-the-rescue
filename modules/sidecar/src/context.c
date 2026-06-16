#include "context_private.h"

#include <windows.h>

#include <dttr_config.h>
#include <dttr_core.h>
#include <dttr_input_binding.h>
#include <dttr_log.h>
#include <dttr_mods.h>
#include <dttr_runtime.h>

#include "crash_private.h"
#include "sidecar_private.h"

static DTTR_Mods_Context sidecar_ctx;

// Each scalar accessor forwards to its DTTR_Config_GetMod* getter against the global
// config, mirroring DTTR_DEFINE_MOD_SCALAR in common/config/mods.c.
#define DTTR_SIDECAR_CONFIG_GET_SCALAR(name, Getter, CType)                              \
	static DTTR_Result dttr_sidecar_config_get_##name(                                   \
		const char *mod_id,                                                              \
		const char *field_id,                                                            \
		CType *out_value                                                                 \
	) {                                                                                  \
		return Getter(&dttr_config, mod_id, field_id, out_value);                        \
	}

DTTR_SIDECAR_CONFIG_GET_SCALAR(bool, DTTR_Config_GetModBool, bool)
DTTR_SIDECAR_CONFIG_GET_SCALAR(int, DTTR_Config_GetModInt, int)
DTTR_SIDECAR_CONFIG_GET_SCALAR(float, DTTR_Config_GetModFloat, float)

#undef DTTR_SIDECAR_CONFIG_GET_SCALAR

static DTTR_Result dttr_sidecar_config_get_string(
	const char *mod_id,
	const char *field_id,
	char *out_value,
	size_t out_size
) {
	return DTTR_Config_GetModString(&dttr_config, mod_id, field_id, out_value, out_size);
}

static DTTR_Result dttr_sidecar_config_get_input_binding(
	const char *mod_id,
	const char *field_id,
	DTTR_Mods_ConfigInputBinding *out_value
) {
	if (!out_value) {
		return (DTTR_Result){
			.status = DTTR_ERR_INVALID_ARGUMENT,
			.message = "Input binding output is required.",
		};
	}

	char token[DTTR_CONFIG_MOD_STRING_MAX] = {0};
	DTTR_Result result = DTTR_Config_GetModString(
		&dttr_config,
		mod_id,
		field_id,
		token,
		sizeof(token)
	);
	if (result.status != DTTR_OK) {
		return result;
	}

	return DTTR_InputBinding_Parse(token, out_value);
}

static const DTTR_Mods_API MOD_API = {
	.log = DTTR_Log,
	.log_is_enabled = DTTR_Log_IsEnabled,
	.log_unchecked = DTTR_Log_Unchecked,
	.struct_size = sizeof(DTTR_Mods_API),
	.abi_version = DTTR_SDK_ABI_VERSION,
	.write_exception_report = dttr_sidecar_write_exception_report,
	.config_get_bool = dttr_sidecar_config_get_bool,
	.config_get_int = dttr_sidecar_config_get_int,
	.config_get_float = dttr_sidecar_config_get_float,
	.config_get_string = dttr_sidecar_config_get_string,
	.config_get_input_binding = dttr_sidecar_config_get_input_binding,
};

static const DTTR_Core_API RUNTIME_API = {
	.sigscan = DTTR_Core_HookCachedSigscan,
	.sigscan_all = DTTR_Core_HookSigscanAll,
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
