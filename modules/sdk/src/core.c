#include "core_internal.h"

#include <kvec.h>

#include <ctype.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

typedef struct install_entry {
	DTTR_Core_Hook *hook;
	void **out_original;
} install_entry;

typedef kvec_t(install_entry) install_entry_vec;

struct DTTR_Core_PatchGroup {
	DTTR_Core_Context ctx;
	install_entry_vec entries;
};

// Return stable status tokens for logs and tests that compare SDK failures by name.
const char *DTTR_StatusName(DTTR_Status status) {
	switch (status) {
	case DTTR_OK:
		return "DTTR_OK";
	case DTTR_ERR_INVALID_ARGUMENT:
		return "DTTR_ERR_INVALID_ARGUMENT";
	case DTTR_ERR_NOT_FOUND:
		return "DTTR_ERR_NOT_FOUND";
	case DTTR_ERR_UNSUPPORTED:
		return "DTTR_ERR_UNSUPPORTED";
	case DTTR_ERR_ALREADY_INSTALLED:
		return "DTTR_ERR_ALREADY_INSTALLED";
	case DTTR_ERR_NOT_INSTALLED:
		return "DTTR_ERR_NOT_INSTALLED";
	case DTTR_ERR_MEMORY_PROTECTION:
		return "DTTR_ERR_MEMORY_PROTECTION";
	case DTTR_ERR_RUNTIME_UNAVAILABLE:
		return "DTTR_ERR_RUNTIME_UNAVAILABLE";
	case DTTR_ERR_ABI_MISMATCH:
		return "DTTR_ERR_ABI_MISMATCH";
	case DTTR_ERR_OUT_OF_MEMORY:
		return "DTTR_ERR_OUT_OF_MEMORY";
	case DTTR_ERR_HOOK_CHAIN_UNSUPPORTED:
		return "DTTR_ERR_HOOK_CHAIN_UNSUPPORTED";
	case DTTR_ERR_MISSING_SYMBOL:
		return "DTTR_ERR_MISSING_SYMBOL";
	case DTTR_ERR_UNRESOLVED:
		return "DTTR_ERR_UNRESOLVED";
	case DTTR_ERR_NOT_CALLABLE:
		return "DTTR_ERR_NOT_CALLABLE";
	case DTTR_ERR_READ_FAILED:
		return "DTTR_ERR_READ_FAILED";
	case DTTR_ERR_WRITE_FAILED:
		return "DTTR_ERR_WRITE_FAILED";
	case DTTR_ERR_POLICY_MISMATCH:
		return "DTTR_ERR_POLICY_MISMATCH";
	case DTTR_ERR_UNSUPPORTED_LAYOUT:
		return "DTTR_ERR_UNSUPPORTED_LAYOUT";
	case DTTR_ERR_UNSUPPORTED_CONTRACT:
		return "DTTR_ERR_UNSUPPORTED_CONTRACT";
	case DTTR_ERR_PROVENANCE_UNSAFE:
		return "DTTR_ERR_PROVENANCE_UNSAFE";
	default:
		return "DTTR_ERR_UNKNOWN";
	}
}

bool DTTR_StatusOK(DTTR_Status status) {
	return status == DTTR_OK;
}

bool DTTR_StatusFailed(DTTR_Status status) {
	return !DTTR_StatusOK(status);
}

bool DTTR_ResultOK(DTTR_Result result) {
	return DTTR_StatusOK(result.status);
}

static bool runtime_context_valid(const DTTR_Core_Context *ctx) {
	return ctx && ctx->game_module && ctx->api;
}

static int hex_value(char ch) {
	if (ch >= '0' && ch <= '9') {
		return ch - '0';
	}

	if (ch >= 'a' && ch <= 'f') {
		return 10 + ch - 'a';
	}

	if (ch >= 'A' && ch <= 'F') {
		return 10 + ch - 'A';
	}

	return -1;
}

static DTTR_Result parse_aob_fail(
	char *sig,
	char *mask,
	DTTR_Status status,
	const char *message
) {
	free(sig);
	free(mask);
	return dttr_core_result(status, message);
}

// Convert user-facing AOB text into the signature and mask strings consumed by sigscan.
static DTTR_Result parse_aob(const char *aob, char **out_sig, char **out_mask) {
	if (!aob || !out_sig || !out_mask) {
		return dttr_core_result(DTTR_ERR_INVALID_ARGUMENT, "invalid AOB parser arguments");
	}

	*out_sig = NULL;
	*out_mask = NULL;

	size_t cap = strlen(aob) / 2u + 2u;
	char *sig = (char *)calloc(cap, 1u);
	char *mask = (char *)calloc(cap, 1u);

	if (!sig || !mask) {
		return parse_aob_fail(
			sig,
			mask,
			DTTR_ERR_OUT_OF_MEMORY,
			"failed to allocate AOB buffers"
		);
	}

	size_t count = 0;
	const char *p = aob;

	while (*p) {
		while (*p && isspace((unsigned char)*p)) {
			p++;
		}

		if (!*p) {
			break;
		}

		if (count + 1u >= cap) {
			return parse_aob_fail(
				sig,
				mask,
				DTTR_ERR_INVALID_ARGUMENT,
				"AOB pattern is malformed"
			);
		}

		if (*p == '?') {
			p++;

			if (*p == '?') {
				p++;
			}

			sig[count] = '?';
			mask[count] = '?';
			count++;
			continue;
		}

		int hi = hex_value(p[0]);
		int lo = p[1] ? hex_value(p[1]) : -1;

		if (hi < 0 || lo < 0) {
			return parse_aob_fail(
				sig,
				mask,
				DTTR_ERR_INVALID_ARGUMENT,
				"AOB pattern contains a non-hex byte"
			);
		}

		sig[count] = (char)((hi << 4) | lo);
		mask[count] = 'x';
		count++;
		p += 2;

		if (*p && !isspace((unsigned char)*p)) {
			return parse_aob_fail(
				sig,
				mask,
				DTTR_ERR_INVALID_ARGUMENT,
				"AOB bytes must be separated by spaces"
			);
		}
	}

	if (!count) {
		return parse_aob_fail(
			sig,
			mask,
			DTTR_ERR_INVALID_ARGUMENT,
			"AOB pattern is empty"
		);
	}

	sig[count] = '\0';
	mask[count] = '\0';
	*out_sig = sig;
	*out_mask = mask;
	return dttr_core_result(DTTR_OK, "ok");
}

static DTTR_Result aob_scan_with(
	HMODULE mod,
	DTTR_Core_SigscanFn sigscan,
	const char *aob,
	uintptr_t *out_addr
) {
	char *sig = NULL;
	char *mask = NULL;
	DTTR_Result parsed = parse_aob(aob, &sig, &mask);

	if (!DTTR_ResultOK(parsed)) {
		return parsed;
	}

	const uintptr_t addr = sigscan(mod, sig, mask);
	free(sig);
	free(mask);

	if (!addr) {
		return dttr_core_result(DTTR_ERR_NOT_FOUND, "AOB signature not found");
	}

	*out_addr = addr;
	return dttr_core_result(DTTR_OK, "ok");
}

DTTR_Result DTTR_Core_AOBFindInModule(HMODULE mod, const char *aob, uintptr_t *out_addr) {
	if (!out_addr) {
		return dttr_core_result(DTTR_ERR_INVALID_ARGUMENT, "out_addr is NULL");
	}

	*out_addr = 0;
	return aob_scan_with(mod, DTTR_Core_HookSigscan, aob, out_addr);
}

DTTR_Result DTTR_Core_AOBFind(
	const DTTR_Core_Context *ctx,
	const char *aob,
	uintptr_t *out_addr
) {
	if (!runtime_context_valid(ctx) || !out_addr) {
		return dttr_core_result(DTTR_ERR_INVALID_ARGUMENT, "invalid AOB find arguments");
	}

	*out_addr = 0;
	const DTTR_Core_API *runtime = ctx->api;

	if (!runtime->sigscan) {
		return dttr_core_result(
			DTTR_ERR_RUNTIME_UNAVAILABLE,
			"runtime sigscan is unavailable"
		);
	}

	return aob_scan_with(ctx->game_module, runtime->sigscan, aob, out_addr);
}

DTTR_Result DTTR_Core_SignatureFind(
	const DTTR_Core_Context *ctx,
	const char *sig,
	const char *mask,
	uintptr_t *out_addr
) {
	if (!runtime_context_valid(ctx) || !sig || !mask || !out_addr) {
		return dttr_core_result(
			DTTR_ERR_INVALID_ARGUMENT,
			"invalid signature find arguments"
		);
	}

	*out_addr = 0;
	const DTTR_Core_API *runtime = ctx->api;

	if (!runtime->sigscan) {
		return dttr_core_result(
			DTTR_ERR_RUNTIME_UNAVAILABLE,
			"runtime sigscan is unavailable"
		);
	}

	uintptr_t addr = runtime->sigscan(ctx->game_module, sig, mask);

	if (!addr) {
		return dttr_core_result(DTTR_ERR_NOT_FOUND, "signature not found");
	}

	*out_addr = addr;
	return dttr_core_result(DTTR_OK, "ok");
}

DTTR_Result DTTR_Core_PatchBytes(
	const DTTR_Core_Context *ctx,
	uintptr_t address,
	const uint8_t *bytes,
	size_t size,
	DTTR_Core_Patch **out_patch
) {
	if (out_patch) {
		*out_patch = NULL;
	}

	if (!runtime_context_valid(ctx) || !address || !bytes || !size || !out_patch) {
		return dttr_core_result(DTTR_ERR_INVALID_ARGUMENT, "invalid patch arguments");
	}

	const DTTR_Core_API *runtime = ctx->api;

	if (!runtime->patch_bytes) {
		return dttr_core_result(
			DTTR_ERR_RUNTIME_UNAVAILABLE,
			"runtime byte patcher is unavailable"
		);
	}

	DTTR_Core_Hook *hook = runtime->patch_bytes(address, bytes, size);

	if (!hook) {
		return dttr_core_result(DTTR_ERR_MEMORY_PROTECTION, "failed to patch bytes");
	}

	*out_patch = hook;
	return dttr_core_result(DTTR_OK, "ok");
}

DTTR_Result DTTR_Core_HookFunction(
	const DTTR_Core_Context *ctx,
	uintptr_t address,
	int prologue_size,
	void *detour,
	void **out_original,
	DTTR_Core_Hook **out_hook
) {
	if (out_hook) {
		*out_hook = NULL;
	}

	if (!runtime_context_valid(ctx) || !address || !detour || !out_hook) {
		return dttr_core_result(
			DTTR_ERR_INVALID_ARGUMENT,
			"invalid function hook arguments"
		);
	}

	const DTTR_Core_API *runtime = ctx->api;

	if (!runtime->hook_function) {
		return dttr_core_result(
			DTTR_ERR_RUNTIME_UNAVAILABLE,
			"runtime function hooker is unavailable"
		);
	}

	DTTR_Core_Hook
		*hook = runtime->hook_function(address, prologue_size, detour, out_original);

	if (!hook) {
		DTTR_Result hook_error = dttr_core_hook_last_error();
		if (hook_error.status == DTTR_ERR_HOOK_CHAIN_UNSUPPORTED) {
			return hook_error;
		}

		return dttr_core_result(DTTR_ERR_MEMORY_PROTECTION, "failed to hook function");
	}

	*out_hook = hook;
	return dttr_core_result(DTTR_OK, "ok");
}

DTTR_Result DTTR_Core_HookAOB(
	const DTTR_Core_Context *ctx,
	const char *aob,
	intptr_t offset,
	int prologue_size,
	void *detour,
	void **out_original,
	DTTR_Core_Hook **out_hook
) {
	uintptr_t match = 0;
	DTTR_Result found = DTTR_Core_AOBFind(ctx, aob, &match);

	if (!DTTR_ResultOK(found)) {
		if (out_hook) {
			*out_hook = NULL;
		}

		return found;
	}

	return DTTR_Core_HookFunction(
		ctx,
		(uintptr_t)((intptr_t)match + offset),
		prologue_size,
		detour,
		out_original,
		out_hook
	);
}

static bool rel32_displacement(uintptr_t site, void *detour, int32_t *out_rel) {
	const int64_t rel = (int64_t)(intptr_t)detour - (int64_t)(site + 5u);

	if (rel < INT32_MIN || rel > INT32_MAX) {
		return false;
	}

	*out_rel = (int32_t)rel;
	return true;
}

DTTR_Result DTTR_Core_PatchRel32Jump(
	const DTTR_Core_Context *ctx,
	uintptr_t address,
	void *detour,
	DTTR_Core_Patch **out_patch
) {
	if (out_patch) {
		*out_patch = NULL;
	}

	if (!runtime_context_valid(ctx) || !address || !detour || !out_patch) {
		return dttr_core_result(DTTR_ERR_INVALID_ARGUMENT, "invalid rel32 jump arguments");
	}

	uint8_t jmp[5] = {0xE9};
	int32_t rel = 0;

	if (!rel32_displacement(address, detour, &rel)) {
		return dttr_core_result(DTTR_ERR_UNSUPPORTED, "rel32 jump target is out of range");
	}

	memcpy(jmp + 1, &rel, sizeof(rel));
	return DTTR_Core_PatchBytes(ctx, address, jmp, sizeof(jmp), out_patch);
}

DTTR_Result DTTR_Core_PatchAOBRel32Jump(
	const DTTR_Core_Context *ctx,
	const char *aob,
	intptr_t offset,
	void *detour,
	DTTR_Core_Patch **out_patch
) {
	if (out_patch) {
		*out_patch = NULL;
	}

	uintptr_t match = 0;
	DTTR_Result found = DTTR_Core_AOBFind(ctx, aob, &match);

	if (!DTTR_ResultOK(found)) {
		return found;
	}

	return DTTR_Core_PatchRel32Jump(
		ctx,
		(uintptr_t)((intptr_t)match + offset),
		detour,
		out_patch
	);
}

DTTR_Result DTTR_Core_HookPointer(
	const DTTR_Core_Context *ctx,
	uintptr_t address,
	void *new_value,
	void **out_original,
	DTTR_Core_Hook **out_hook
) {
	if (out_hook) {
		*out_hook = NULL;
	}

	if (!runtime_context_valid(ctx) || !address || !out_hook) {
		return dttr_core_result(
			DTTR_ERR_INVALID_ARGUMENT,
			"invalid pointer hook arguments"
		);
	}

	const DTTR_Core_API *runtime = ctx->api;

	if (!runtime->hook_pointer) {
		return dttr_core_result(
			DTTR_ERR_RUNTIME_UNAVAILABLE,
			"runtime pointer hooker is unavailable"
		);
	}

	DTTR_Core_Hook *hook = runtime->hook_pointer(address, new_value, out_original);

	if (!hook) {
		return dttr_core_result(DTTR_ERR_MEMORY_PROTECTION, "failed to hook pointer");
	}

	*out_hook = hook;
	return dttr_core_result(DTTR_OK, "ok");
}

DTTR_Result DTTR_Core_Unpatch(DTTR_Core_Patch *patch) {
	if (!patch || DTTR_Core_HookDetachChecked(patch)) {
		return dttr_core_result(DTTR_OK, "ok");
	}

	return dttr_core_result(DTTR_ERR_MEMORY_PROTECTION, "failed to detach patch");
}

DTTR_Result DTTR_Core_Unhook(DTTR_Core_Hook *hook) {
	if (!hook || DTTR_Core_HookDetachChecked(hook)) {
		return dttr_core_result(DTTR_OK, "ok");
	}

	return dttr_core_result(DTTR_ERR_MEMORY_PROTECTION, "failed to detach hook");
}

static DTTR_Result install_one_target(
	const DTTR_Core_Context *ctx,
	const DTTR_Core_TargetSpec *target,
	DTTR_Core_Hook **out_handle
);

// Grow patch-group storage before installing another owned hook or patch.
static DTTR_Result patch_group_reserve(DTTR_Core_PatchGroup *group) {
	if (kv_size(group->entries) < kv_max(group->entries)) {
		return dttr_core_result(DTTR_OK, "ok");
	}

	const size_t new_cap = kv_max(group->entries) ? kv_max(group->entries) * 2u : 4u;

	if (new_cap < kv_max(group->entries)
		|| new_cap > ((size_t)-1) / sizeof(install_entry)) {
		return dttr_core_result(DTTR_ERR_OUT_OF_MEMORY, "patch group is too large");
	}

	install_entry *entries = (install_entry *)
		realloc(group->entries.a, new_cap * sizeof(*entries));

	if (!entries) {
		return dttr_core_result(DTTR_ERR_OUT_OF_MEMORY, "failed to grow patch group");
	}

	group->entries.a = entries;
	group->entries.m = new_cap;
	return dttr_core_result(DTTR_OK, "ok");
}

// Validate a patch group and reserve storage for one more owned hook or patch.
static DTTR_Result patch_group_prepare_install(DTTR_Core_PatchGroup *group) {
	if (!group) {
		return dttr_core_result(DTTR_ERR_INVALID_ARGUMENT, "patch group is NULL");
	}

	return patch_group_reserve(group);
}

// Detach through the same runtime API that created the group-owned handle.
static bool patch_group_detach_checked(
	const DTTR_Core_PatchGroup *group,
	DTTR_Core_Hook *hook
) {
	const DTTR_Core_API *runtime = group ? group->ctx.api : NULL;

	if (runtime && runtime->unhook_checked) {
		return runtime->unhook_checked(hook);
	}

	if (runtime && runtime->unhook) {
		runtime->unhook(hook);
		return true;
	}

	return DTTR_Core_HookDetachChecked(hook);
}

// Roll back patch-group entries after the requested keep count in reverse install order.
static DTTR_Result patch_group_uninstall_from(
	DTTR_Core_PatchGroup *group,
	size_t keep_count
) {
	while (kv_size(group->entries) > keep_count) {
		install_entry entry = kv_A(group->entries, kv_size(group->entries) - 1u);
		if (!patch_group_detach_checked(group, entry.hook)) {
			return dttr_core_result(
				DTTR_ERR_MEMORY_PROTECTION,
				"failed to restore one or more patch group entries"
			);
		}

		kv_pop(group->entries);
		if (entry.out_original) {
			*entry.out_original = NULL;
		}
	}

	return dttr_core_result(DTTR_OK, "ok");
}

DTTR_Result DTTR_Core_PatchGroupCreate(
	const DTTR_Core_Context *ctx,
	DTTR_Core_PatchGroup **out_group
) {
	if (out_group) {
		*out_group = NULL;
	}

	if (!runtime_context_valid(ctx) || !out_group) {
		return dttr_core_result(
			DTTR_ERR_INVALID_ARGUMENT,
			"invalid patch group arguments"
		);
	}

	DTTR_Core_PatchGroup *group = (DTTR_Core_PatchGroup *)calloc(1u, sizeof(*group));

	if (!group) {
		return dttr_core_result(DTTR_ERR_OUT_OF_MEMORY, "failed to allocate patch group");
	}

	group->ctx = *ctx;
	kv_init(group->entries);
	*out_group = group;
	return dttr_core_result(DTTR_OK, "ok");
}

DTTR_Result DTTR_Core_PatchGroupUninstall(DTTR_Core_PatchGroup *group) {
	if (!group) {
		return dttr_core_result(DTTR_OK, "ok");
	}

	return patch_group_uninstall_from(group, 0u);
}

DTTR_Result DTTR_Core_PatchGroupDestroy(DTTR_Core_PatchGroup *group) {
	if (!group) {
		return dttr_core_result(DTTR_OK, "ok");
	}

	DTTR_Result result = DTTR_Core_PatchGroupUninstall(group);
	if (!DTTR_ResultOK(result)) {
		return result;
	}

	kv_destroy(group->entries);
	free(group);
	return result;
}

DTTR_Result DTTR_Core_PatchGroupRelease(DTTR_Core_PatchGroup **group) {
	if (!group || !*group) {
		return dttr_core_result(DTTR_OK, "ok");
	}

	DTTR_Result result = DTTR_Core_PatchGroupDestroy(*group);
	if (!DTTR_ResultOK(result)) {
		return result;
	}

	*group = NULL;
	return result;
}

// Return the runtime context that backs a patch group for generated helper calls.
const DTTR_Core_Context *dttr_core_patch_group_context(const DTTR_Core_PatchGroup *group) {
	return group ? &group->ctx : NULL;
}

// Adopt a freshly installed handle into a patch group, or propagate the install failure.
static DTTR_Result patch_group_finish_install(
	DTTR_Core_PatchGroup *group,
	DTTR_Result result,
	DTTR_Core_Hook *hook,
	void **out_original,
	DTTR_Core_Hook **out_hook
) {
	if (!DTTR_ResultOK(result)) {
		return result;
	}

	group->entries.a[group->entries.n++] = (install_entry){hook, out_original};

	if (out_hook) {
		*out_hook = hook;
	}

	return result;
}

DTTR_Result DTTR_Core_PatchGroupPatchBytes(
	DTTR_Core_PatchGroup *group,
	uintptr_t address,
	const uint8_t *bytes,
	size_t size,
	DTTR_Core_Patch **out_patch
) {
	if (out_patch) {
		*out_patch = NULL;
	}

	DTTR_Result reserved = patch_group_prepare_install(group);

	if (!DTTR_ResultOK(reserved)) {
		return reserved;
	}

	DTTR_Core_Patch *patch = NULL;
	DTTR_Result result = DTTR_Core_PatchBytes(&group->ctx, address, bytes, size, &patch);

	return patch_group_finish_install(
		group,
		result,
		patch,
		NULL,
		(DTTR_Core_Hook **)out_patch
	);
}

DTTR_Result DTTR_Core_PatchGroupHookFunction(
	DTTR_Core_PatchGroup *group,
	uintptr_t address,
	int prologue_size,
	void *detour,
	void **out_original,
	DTTR_Core_Hook **out_hook
) {
	if (out_hook) {
		*out_hook = NULL;
	}

	DTTR_Result reserved = patch_group_prepare_install(group);

	if (!DTTR_ResultOK(reserved)) {
		return reserved;
	}

	DTTR_Core_Hook *hook = NULL;
	DTTR_Result result = DTTR_Core_HookFunction(
		&group->ctx,
		address,
		prologue_size,
		detour,
		out_original,
		&hook
	);

	return patch_group_finish_install(group, result, hook, out_original, out_hook);
}

DTTR_Result DTTR_Core_PatchGroupHookPointer(
	DTTR_Core_PatchGroup *group,
	uintptr_t address,
	void *new_value,
	void **out_original,
	DTTR_Core_Hook **out_hook
) {
	if (out_hook) {
		*out_hook = NULL;
	}

	DTTR_Result reserved = patch_group_prepare_install(group);

	if (!DTTR_ResultOK(reserved)) {
		return reserved;
	}

	DTTR_Core_Hook *hook = NULL;
	DTTR_Result result = DTTR_Core_HookPointer(
		&group->ctx,
		address,
		new_value,
		out_original,
		&hook
	);

	return patch_group_finish_install(group, result, hook, out_original, out_hook);
}

DTTR_Result DTTR_Core_PatchGroupPatchRel32Jump(
	DTTR_Core_PatchGroup *group,
	uintptr_t address,
	void *detour,
	DTTR_Core_Patch **out_patch
) {
	if (out_patch) {
		*out_patch = NULL;
	}

	DTTR_Result reserved = patch_group_prepare_install(group);

	if (!DTTR_ResultOK(reserved)) {
		return reserved;
	}

	DTTR_Core_Patch *patch = NULL;
	DTTR_Result result = DTTR_Core_PatchRel32Jump(&group->ctx, address, detour, &patch);

	return patch_group_finish_install(
		group,
		result,
		patch,
		NULL,
		(DTTR_Core_Hook **)out_patch
	);
}

static void **target_original_slot(const DTTR_Core_TargetSpec *target) {
	switch (target->kind) {
	case DTTR_TARGET_ADDRESS_HOOK:
	case DTTR_TARGET_AOB_HOOK:
	case DTTR_TARGET_POINTER_HOOK:
		return target->out_original;
	default:
		return NULL;
	}
}

// Install a target list transactionally so required failures roll back new entries.
DTTR_Result DTTR_Core_PatchGroupInstallTargets(
	DTTR_Core_PatchGroup *group,
	const DTTR_Core_TargetSpec *targets,
	size_t target_count,
	DTTR_Core_TargetReport *out_report
) {
	dttr_core_report_init(out_report);

	if (!group || (!targets && target_count)) {
		DTTR_Result result = dttr_core_result(
			DTTR_ERR_INVALID_ARGUMENT,
			"invalid patch group target arguments"
		);
		dttr_core_report_fail(out_report, 0, result);
		return result;
	}

	const size_t keep_count = kv_size(group->entries);

	for (size_t i = 0; i < target_count; i++) {
		if (out_report) {
			out_report->attempted++;
		}

		DTTR_Result reserved = patch_group_reserve(group);

		if (!DTTR_ResultOK(reserved)) {
			DTTR_Result rollback = patch_group_uninstall_from(group, keep_count);
			if (!DTTR_ResultOK(rollback)) {
				dttr_core_report_fail(out_report, i, rollback);
				return rollback;
			}

			dttr_core_report_fail(out_report, i, reserved);
			return reserved;
		}

		DTTR_Core_Hook *handle = NULL;
		DTTR_Result result = install_one_target(&group->ctx, &targets[i], &handle);

		if (!DTTR_ResultOK(result)) {
			if (!targets[i].required && result.status == DTTR_ERR_NOT_FOUND) {
				if (out_report) {
					out_report->skipped_optional++;
				}

				continue;
			}

			DTTR_Result rollback = patch_group_uninstall_from(group, keep_count);
			if (!DTTR_ResultOK(rollback)) {
				dttr_core_report_fail(out_report, i, rollback);
				return rollback;
			}

			dttr_core_report_fail(out_report, i, result);
			return result;
		}

		group->entries.a[group->entries.n++] = (install_entry){
			handle,
			target_original_slot(&targets[i])
		};

		if (out_report) {
			out_report->installed++;
		}
	}

	return dttr_core_result(DTTR_OK, "ok");
}

void dttr_core_report_init(DTTR_Core_TargetReport *report) {
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

// Record the first failing target so callers can diagnose partial install attempts.
void dttr_core_report_fail(
	DTTR_Core_TargetReport *report,
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

// Resolve direct and AOB target specs into concrete addresses before installation.
static DTTR_Result target_address(
	const DTTR_Core_Context *ctx,
	const DTTR_Core_TargetSpec *target,
	uintptr_t *out_address
) {
	if (target->kind == DTTR_TARGET_ADDRESS_PATCH
		|| target->kind == DTTR_TARGET_ADDRESS_HOOK
		|| target->kind == DTTR_TARGET_POINTER_HOOK
		|| target->kind == DTTR_TARGET_ADDRESS_REL32_JMP) {
		if (!target->address) {
			return dttr_core_result(DTTR_ERR_INVALID_ARGUMENT, "target address is NULL");
		}

		*out_address = target->address;
		return dttr_core_result(DTTR_OK, "ok");
	}

	uintptr_t match = 0;
	DTTR_Result found = DTTR_Core_AOBFind(ctx, target->aob, &match);

	if (!DTTR_ResultOK(found)) {
		return found;
	}

	*out_address = (uintptr_t)((intptr_t)match + target->offset);
	return dttr_core_result(DTTR_OK, "ok");
}

// Dispatch one target spec to the matching patch, hook, or jump primitive.
static DTTR_Result install_one_target(
	const DTTR_Core_Context *ctx,
	const DTTR_Core_TargetSpec *target,
	DTTR_Core_Hook **out_handle
) {
	*out_handle = NULL;
	uintptr_t address = 0;
	DTTR_Result address_result = target_address(ctx, target, &address);

	if (!DTTR_ResultOK(address_result)) {
		return address_result;
	}

	switch (target->kind) {
	case DTTR_TARGET_ADDRESS_PATCH:
	case DTTR_TARGET_AOB_PATCH:
		return DTTR_Core_PatchBytes(
			ctx,
			address,
			target->patch_bytes,
			target->patch_size,
			(DTTR_Core_Patch **)out_handle
		);
	case DTTR_TARGET_ADDRESS_HOOK:
	case DTTR_TARGET_AOB_HOOK:
		return DTTR_Core_HookFunction(
			ctx,
			address,
			target->prologue_size,
			target->detour,
			target->out_original,
			out_handle
		);
	case DTTR_TARGET_POINTER_HOOK:
		return DTTR_Core_HookPointer(
			ctx,
			address,
			target->new_value,
			target->out_original,
			out_handle
		);
	case DTTR_TARGET_ADDRESS_REL32_JMP:
	case DTTR_TARGET_AOB_REL32_JMP:
		return DTTR_Core_PatchRel32Jump(
			ctx,
			address,
			target->detour,
			(DTTR_Core_Patch **)out_handle
		);
	default:
		return dttr_core_result(DTTR_ERR_UNSUPPORTED, "unsupported target kind");
	}
}
