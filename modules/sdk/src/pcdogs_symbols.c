#include <dttr_pcdogs.h>

#include "core_internal.h"

static void patch_report_init(DTTR_PCDOGS_T_Patch_Report *report) {
	if (!report) {
		return;
	}

	report->attempted = 0;
	report->installed = 0;
	report->skipped_optional = 0;
	report->failed_index = (size_t)-1;
	report->status = DTTR_OK;
	report->message = "ok";
}

static void patch_report_fail(
	DTTR_PCDOGS_T_Patch_Report *report,
	size_t index,
	DTTR_Result result
) {
	if (!report) {
		return;
	}

	report->failed_index = index;
	report->status = result.status;
	report->message = result.message;
}

static uintptr_t function_address_at(uint32_t index) {
	const DTTR_PCDOGS_T_Symbol_Function *fn = DTTR_PCDOGS_SymbolFunctionAt(index);
	return fn ? fn->address : 0;
}

static uintptr_t global_address_at(uint32_t index) {
	const DTTR_PCDOGS_T_Symbol_Data *global = DTTR_PCDOGS_SymbolDataAt(index);
	return global ? global->address : 0;
}

static DTTR_Result resolve_symbol(
	const DTTR_Core_Context *ctx,
	uint32_t id,
	uint32_t count,
	uintptr_t (*address_at)(uint32_t index),
	uintptr_t *out_addr,
	const char *invalid_message,
	const char *not_found_message
) {
	if (!ctx || !out_addr || id >= count) {
		return dttr_core_result(DTTR_ERR_INVALID_ARGUMENT, invalid_message);
	}

	*out_addr = 0;
	DTTR_PCDOGS_ResolveAll(ctx);
	const uintptr_t addr = address_at(id);
	if (!addr) {
		return dttr_core_result(DTTR_ERR_NOT_FOUND, not_found_message);
	}

	*out_addr = addr;
	return dttr_core_result(DTTR_OK, "ok");
}

DTTR_Result DTTR_PCDOGS_SymbolFunctionResolve(
	const DTTR_Core_Context *ctx,
	DTTR_PCDOGS_T_Symbol_Function_ID id,
	uintptr_t *out_addr
) {
	return resolve_symbol(
		ctx,
		(uint32_t)id,
		DTTR_PCDOGS_SYMBOL_FUNCTION_COUNT,
		function_address_at,
		out_addr,
		"invalid PCDOGS symbol function resolve arguments",
		"PCDOGS symbol function was not resolved"
	);
}

DTTR_Result DTTR_PCDOGS_FunctionResolve(
	const DTTR_Core_Context *ctx,
	DTTR_PCDOGS_T_Function_ID id,
	uintptr_t *out_addr
) {
	DTTR_PCDOGS_T_Symbol_Function_ID symbol_id;
	if (!DTTR_PCDOGS_FunctionSymbolID(id, &symbol_id)) {
		return dttr_core_result(
			DTTR_ERR_INVALID_ARGUMENT,
			"invalid PCDOGS function resolve arguments"
		);
	}

	return DTTR_PCDOGS_SymbolFunctionResolve(ctx, symbol_id, out_addr);
}

DTTR_Result DTTR_PCDOGS_SymbolDataResolve(
	const DTTR_Core_Context *ctx,
	DTTR_PCDOGS_T_Symbol_Data_ID id,
	uintptr_t *out_addr
) {
	return resolve_symbol(
		ctx,
		(uint32_t)id,
		DTTR_PCDOGS_SYMBOL_DATA_COUNT,
		global_address_at,
		out_addr,
		"invalid PCDOGS symbol global resolve arguments",
		"PCDOGS symbol global was not resolved"
	);
}

DTTR_Result DTTR_PCDOGS_DataResolve(
	const DTTR_Core_Context *ctx,
	DTTR_PCDOGS_T_Data_ID id,
	uintptr_t *out_addr
) {
	DTTR_PCDOGS_T_Symbol_Data_ID symbol_id;
	if (!DTTR_PCDOGS_DataSymbolID(id, &symbol_id)) {
		return dttr_core_result(
			DTTR_ERR_INVALID_ARGUMENT,
			"invalid PCDOGS global resolve arguments"
		);
	}

	return DTTR_PCDOGS_SymbolDataResolve(ctx, symbol_id, out_addr);
}

DTTR_Result DTTR_PCDOGS_Hook_DataPointer(
	const DTTR_Core_Context *ctx,
	DTTR_PCDOGS_T_Data_ID id,
	void *new_value,
	void **out_original,
	DTTR_Core_Hook **out_hook
) {
	if (out_hook) {
		*out_hook = NULL;
	}

	if (!ctx || !out_hook) {
		return dttr_core_result(
			DTTR_ERR_INVALID_ARGUMENT,
			"invalid PCDOGS global pointer hook arguments"
		);
	}

	uintptr_t address = 0;
	DTTR_Result resolved = DTTR_PCDOGS_DataResolve(ctx, id, &address);
	if (!DTTR_ResultOK(resolved)) {
		return resolved;
	}

	return DTTR_Core_HookPointer(ctx, address, new_value, out_original, out_hook);
}

static DTTR_Result patch_group_hook_symbol_function(
	DTTR_Core_PatchGroup *group,
	DTTR_PCDOGS_T_Symbol_Function_ID id,
	void *detour,
	void **out_original
) {
	if (!group || !detour) {
		return dttr_core_result(
			DTTR_ERR_INVALID_ARGUMENT,
			"invalid PCDOGS patch group symbol function hook arguments"
		);
	}

	const DTTR_PCDOGS_T_Symbol_Function *fn = DTTR_PCDOGS_SymbolFunctionAt((uint32_t)id);
	if (!fn || fn->hook_kind != DTTR_PCDOGS_HOOK_REL32 || !fn->callable) {
		return dttr_core_result(
			DTTR_ERR_UNSUPPORTED,
			"PCDOGS symbol function is not hookable"
		);
	}

	const DTTR_Core_Context *ctx = dttr_core_patch_group_context(group);
	uintptr_t address = 0;
	DTTR_Result resolved = DTTR_PCDOGS_SymbolFunctionResolve(ctx, id, &address);
	if (!DTTR_ResultOK(resolved)) {
		return resolved;
	}

	return DTTR_Core_PatchGroupHookFunction(
		group,
		address,
		(int)fn->patch_size,
		detour,
		out_original,
		NULL
	);
}

DTTR_Result DTTR_PCDOGS_PatchGroup_HookFunction(
	DTTR_Core_PatchGroup *group,
	DTTR_PCDOGS_T_Function_ID id,
	void *detour,
	void **out_original
) {
	DTTR_PCDOGS_T_Symbol_Function_ID symbol_id;
	if (!DTTR_PCDOGS_FunctionSymbolID(id, &symbol_id)) {
		return dttr_core_result(
			DTTR_ERR_INVALID_ARGUMENT,
			"invalid PCDOGS patch group function hook arguments"
		);
	}

	return patch_group_hook_symbol_function(group, symbol_id, detour, out_original);
}

DTTR_Result DTTR_PCDOGS_PatchGroup_HookDataPointer(
	DTTR_Core_PatchGroup *group,
	DTTR_PCDOGS_T_Data_ID id,
	void *new_value,
	void **out_original
) {
	if (!group) {
		return dttr_core_result(
			DTTR_ERR_INVALID_ARGUMENT,
			"invalid PCDOGS patch group global pointer hook arguments"
		);
	}

	const DTTR_Core_Context *ctx = dttr_core_patch_group_context(group);
	uintptr_t address = 0;
	DTTR_Result resolved = DTTR_PCDOGS_DataResolve(ctx, id, &address);
	if (!DTTR_ResultOK(resolved)) {
		return resolved;
	}

	return DTTR_Core_PatchGroupHookPointer(group, address, new_value, out_original, NULL);
}

static DTTR_Result patch_spec_target(
	const DTTR_PCDOGS_T_Patch_Spec *spec,
	DTTR_Core_TargetSpec *out_target
) {
	if (!spec || !out_target) {
		return dttr_core_result(
			DTTR_ERR_INVALID_ARGUMENT,
			"invalid PCDOGS patch spec target"
		);
	}

	DTTR_Core_TargetSpec target = spec->target;
	target.required = spec->required || target.required;
	switch (spec->kind) {
	case DTTR_PCDOGS_PATCH_TARGET:
		break;
	case DTTR_PCDOGS_PATCH_ADDRESS_BYTES:
		target.kind = DTTR_TARGET_ADDRESS_PATCH;
		target.address = spec->address;
		target.patch_bytes = spec->patch_bytes;
		target.patch_size = spec->patch_size;
		break;
	case DTTR_PCDOGS_PATCH_AOB_BYTES:
		target.kind = DTTR_TARGET_AOB_PATCH;
		target.aob = spec->aob;
		target.offset = spec->offset;
		target.patch_bytes = spec->patch_bytes;
		target.patch_size = spec->patch_size;
		break;
	case DTTR_PCDOGS_PATCH_AOB_REL32_JMP:
		target.kind = DTTR_TARGET_AOB_REL32_JMP;
		target.aob = spec->aob;
		target.offset = spec->offset;
		target.detour = spec->detour;
		break;
	default:
		return dttr_core_result(
			DTTR_ERR_UNSUPPORTED,
			"unsupported PCDOGS patch spec kind"
		);
	}

	*out_target = target;
	return dttr_core_result(DTTR_OK, "ok");
}

static DTTR_Result patch_spec_install(
	DTTR_Core_PatchGroup *group,
	const DTTR_PCDOGS_T_Patch_Spec *spec
) {
	switch (spec->kind) {
	case DTTR_PCDOGS_PATCH_UNSUPPORTED:
		return dttr_core_result(
			DTTR_ERR_UNSUPPORTED,
			"unsupported PCDOGS patch spec kind"
		);
	case DTTR_PCDOGS_PATCH_FUNCTION_HOOK:
		return DTTR_PCDOGS_PatchGroup_HookFunction(
			group,
			spec->function,
			spec->detour,
			spec->out_original
		);
	case DTTR_PCDOGS_PATCH_DATA_POINTER_HOOK:
		return DTTR_PCDOGS_PatchGroup_HookDataPointer(
			group,
			spec->global,
			spec->new_value,
			spec->out_original
		);
	case DTTR_PCDOGS_PATCH_TARGET:
	case DTTR_PCDOGS_PATCH_ADDRESS_BYTES:
	case DTTR_PCDOGS_PATCH_AOB_BYTES:
	case DTTR_PCDOGS_PATCH_AOB_REL32_JMP: {
		DTTR_Core_TargetSpec target = {0};
		DTTR_Result result = patch_spec_target(spec, &target);
		if (!DTTR_ResultOK(result)) {
			return result;
		}

		DTTR_Core_TargetReport target_report = {0};
		result = DTTR_Core_PatchGroupInstallTargets(group, &target, 1u, &target_report);
		if (!DTTR_ResultOK(result)) {
			return result;
		}

		if (target_report.skipped_optional) {
			return dttr_core_result(
				DTTR_ERR_NOT_FOUND,
				"optional PCDOGS patch spec skipped"
			);
		}

		return dttr_core_result(DTTR_OK, "ok");
	}
	default:
		return dttr_core_result(
			DTTR_ERR_UNSUPPORTED,
			"unsupported PCDOGS patch spec kind"
		);
	}
}

DTTR_Result DTTR_PCDOGS_PatchGroup_Install(
	const DTTR_Core_Context *ctx,
	const DTTR_PCDOGS_T_Patch_Spec *specs,
	size_t spec_count,
	DTTR_Core_PatchGroup **out_group,
	DTTR_PCDOGS_T_Patch_Report *out_report
) {
	patch_report_init(out_report);
	if (!out_group || (!specs && spec_count)) {
		DTTR_Result result = dttr_core_result(
			DTTR_ERR_INVALID_ARGUMENT,
			"invalid PCDOGS patch spec install arguments"
		);
		patch_report_fail(out_report, 0, result);
		return result;
	}

	if (*out_group) {
		DTTR_Result result = dttr_core_result(
			DTTR_ERR_ALREADY_INSTALLED,
			"PCDOGS patch group output is already set"
		);
		patch_report_fail(out_report, 0, result);
		return result;
	}

	DTTR_Core_PatchGroup *group = NULL;
	DTTR_Result result = DTTR_Core_PatchGroupCreate(ctx, &group);
	if (!DTTR_ResultOK(result)) {
		patch_report_fail(out_report, 0, result);
		return result;
	}

	for (size_t i = 0; i < spec_count; i++) {
		if (out_report) {
			out_report->attempted++;
		}

		result = patch_spec_install(group, &specs[i]);
		if (!DTTR_ResultOK(result)) {
			if (!specs[i].required
				&& (result.status == DTTR_ERR_NOT_FOUND
					|| result.status == DTTR_ERR_UNSUPPORTED)) {
				if (out_report) {
					out_report->skipped_optional++;
				}

				continue;
			}

			DTTR_Result cleanup = DTTR_Core_PatchGroupRelease(&group);
			if (!DTTR_ResultOK(cleanup)) {
				*out_group = group;
				patch_report_fail(out_report, i, cleanup);
				return cleanup;
			}

			patch_report_fail(out_report, i, result);
			return result;
		}

		if (out_report) {
			out_report->installed++;
		}
	}

	*out_group = group;
	return dttr_core_result(DTTR_OK, "ok");
}
