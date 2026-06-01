#define DTTR_SDK_ENABLE_UNSTABLE

#include <stdint.h>
#include <string.h>
#include <windows.h>

#include <dttr_core.h>
#include <dttr_pcdogs.h>
#include <dttr_pcdogs_unstable.h>
#include <dttr_test_support.h>
#include <dttr_util.h>
#include <dttr_util_unstable.h>

static uint8_t sig_target[] = {0x55, 0x8B, 0xEC, 0x90, 0x90, 0xC3, 0x33, 0xC0};
static uint8_t patch_target[4];
static uint8_t rel32_target[5];
static uint8_t group_patch_target[2];
static void *pointer_target;

// Provide a stable detour address for rel32 patch assertions.
static void rel32_detour() {}

// Verify patched bytes encode a relative JMP to the expected detour.
static void assert_rel32_jump(uint8_t *site, void *detour) {
	assert_int_equal(site[0], 0xE9);
	int32_t rel = 0;
	memcpy(&rel, site + 1, sizeof(rel));
	const uintptr_t base = (uintptr_t)site + 5u;
	const int64_t displacement = rel;
	const uintptr_t target = displacement < 0 ? base - (uintptr_t)(-displacement)
											  : base + (uintptr_t)displacement;
	assert_int_equal(target, (uintptr_t)detour);
}

// Scan an in-memory byte buffer with the same mask semantics used by runtime tests.
static uintptr_t sigscan_bytes(
	const uint8_t *bytes,
	size_t size,
	const char *sig,
	const char *mask
) {
	const size_t len = strlen(mask);
	if (!len || len > size) {
		return 0;
	}

	for (size_t offset = 0; offset + len <= size; offset++) {
		bool matched = true;
		for (size_t i = 0; i < len; i++) {
			if (mask[i] == 'x' && (uint8_t)sig[i] != bytes[offset + i]) {
				matched = false;
				break;
			}
		}

		if (matched) {
			return (uintptr_t)&bytes[offset];
		}
	}

	return 0;
}

// Resolve test signatures against the static buffers used by SDK core tests.
static uintptr_t sigscan(HMODULE mod, const char *sig, const char *mask) {
	uintptr_t match = sigscan_bytes(sig_target, sizeof(sig_target), sig, mask);
	if (match) {
		return match;
	}

	return sigscan_bytes(rel32_target, sizeof(rel32_target), sig, mask);
}

static const DTTR_Core_API RUNTIME = {
	.sigscan = sigscan,
	.hook_function = DTTR_Core_HookAttachFunction,
	.hook_pointer = DTTR_Core_HookAttachPointer,
	.patch_bytes = DTTR_Core_HookPatchBytes,
	.unhook = DTTR_Core_HookDetach,
	.hook_is_active = DTTR_Core_HookIsActive,
	.unhook_checked = DTTR_Core_HookDetachChecked,
};

static int fail_next_unhook_count;

static bool fail_next_unhook_checked(DTTR_Core_Hook *hook) {
	if (fail_next_unhook_count > 0) {
		fail_next_unhook_count--;
		return false;
	}

	return DTTR_Core_HookDetachChecked(hook);
}

static const DTTR_Core_API RUNTIME_FAIL_NEXT_UNHOOK = {
	.sigscan = sigscan,
	.hook_function = DTTR_Core_HookAttachFunction,
	.hook_pointer = DTTR_Core_HookAttachPointer,
	.patch_bytes = DTTR_Core_HookPatchBytes,
	.unhook = DTTR_Core_HookDetach,
	.hook_is_active = DTTR_Core_HookIsActive,
	.unhook_checked = fail_next_unhook_checked,
};

static const char *symbol_sigscan_sig;
static const char *symbol_sigscan_mask;
static uintptr_t symbol_sigscan_match;

// Resolve exactly one generated symbol to a test executable address.
static uintptr_t symbol_sigscan(HMODULE mod, const char *sig, const char *mask) {
	if (symbol_sigscan_sig && symbol_sigscan_mask && strcmp(sig, symbol_sigscan_sig) == 0
		&& strcmp(mask, symbol_sigscan_mask) == 0) {
		return symbol_sigscan_match;
	}

	return 0;
}

static const DTTR_Core_API SYMBOL_RUNTIME = {
	.sigscan = symbol_sigscan,
	.hook_function = DTTR_Core_HookAttachFunction,
	.hook_pointer = DTTR_Core_HookAttachPointer,
	.patch_bytes = DTTR_Core_HookPatchBytes,
	.unhook = DTTR_Core_HookDetach,
	.hook_is_active = DTTR_Core_HookIsActive,
	.unhook_checked = DTTR_Core_HookDetachChecked,
};

static DTTR_Core_Context symbol_runtime_context() {
	DTTR_Core_Context ctx = {
		.game_module = (HMODULE)1,
		.api = &SYMBOL_RUNTIME,
	};

	return ctx;
}

static const DTTR_PCDOGS_T_Symbol_Function *prepare_symbol_lookup(
	DTTR_PCDOGS_T_Symbol_Function_Id id,
	uintptr_t addr
) {
	const DTTR_PCDOGS_T_Symbol_Function *fn = DTTR_PCDOGS_SymbolFunctionAt((uint32_t)id);
	if (!fn) {
		return NULL;
	}

	symbol_sigscan_sig = fn->sig;
	symbol_sigscan_mask = fn->mask;
	symbol_sigscan_match = addr - (uintptr_t)fn->match_offset;
	DTTR_PCDOGS_Reset();
	return fn;
}

static bool resolve_symbol_to_address(DTTR_PCDOGS_T_Symbol_Function_Id id, uintptr_t addr) {
	const DTTR_PCDOGS_T_Symbol_Function *fn = prepare_symbol_lookup(id, addr);
	if (!fn) {
		return false;
	}

	DTTR_PCDOGS_T_Symbol_Function *mutable_fn = (DTTR_PCDOGS_T_Symbol_Function *)fn;
	mutable_fn->address = addr;
	mutable_fn->resolved = true;
	return true;
}

static bool resolve_typed_function_to_address(
	const DTTR_Core_Context *ctx,
	DTTR_PCDOGS_T_Symbol_Function_Id id,
	uintptr_t addr
) {
	if (!prepare_symbol_lookup(id, addr)) {
		return false;
	}

	DTTR_PCDOGS_ResolveAll(ctx);
	return true;
}

static DTTR_PCDOGS_T_Actor_State *active_actor_result;

static DTTR_PCDOGS_T_Actor_State *__cdecl active_actor_stub() {
	return active_actor_result;
}

static BOOL __cdecl movie_playfile_detour(char const *moviePath, char useAltVideoRect) {
	return 0;
}

static int32_t __cdecl level_normalize_index_detour(int32_t level_id) { return level_id; }
typedef int32_t(__cdecl *pcdogs_chain_target_fn)();
static pcdogs_chain_target_fn pcdogs_chain_original_a = NULL;
static pcdogs_chain_target_fn pcdogs_chain_original_b = NULL;

__attribute__((noinline)) static int32_t __cdecl pcdogs_chain_target() {
	volatile int32_t value = 7;
	return value;
}

static int32_t __cdecl pcdogs_chain_detour_a() {
	assert_non_null(pcdogs_chain_original_a);
	return pcdogs_chain_original_a() + 10;
}

static int32_t __cdecl pcdogs_chain_detour_b() {
	assert_non_null(pcdogs_chain_original_b);
	return pcdogs_chain_original_b() + 100;
}

// Build a minimal runtime context backed by the test runtime API.
static DTTR_Core_Context runtime_context() {
	DTTR_Core_Context ctx = {
		.game_module = (HMODULE)1,
		.api = &RUNTIME,
	};

	return ctx;
}

static DTTR_Core_Context runtime_context_with_unhook_failure() {
	DTTR_Core_Context ctx = {
		.game_module = (HMODULE)1,
		.api = &RUNTIME_FAIL_NEXT_UNHOOK,
	};

	return ctx;
}

// Covers status naming and invalid runtime-context handling.
static void test_core_validation(void **state) {
	DTTR_Core_Context ctx = runtime_context();
	DTTR_Core_PatchGroup *group = NULL;
	uintptr_t addr = 0;

	DTTR_Result result = DTTR_Core_PatchGroupCreate(&ctx, &group);
	assert_true(DTTR_ResultOK(result));
	assert_string_equal(DTTR_StatusName(result.status), "DTTR_OK");
	DTTR_Core_PatchGroupDestroy(group);

	result = DTTR_Core_PatchGroupDestroy(NULL);
	assert_true(DTTR_ResultOK(result));
	result = DTTR_Core_PatchGroupRelease(NULL);
	assert_true(DTTR_ResultOK(result));
	group = NULL;
	result = DTTR_Core_PatchGroupRelease(&group);
	assert_true(DTTR_ResultOK(result));
	assert_null(group);

	result = DTTR_Core_AOBFind(NULL, "55 8B", &addr);
	assert_int_equal(result.status, DTTR_ERR_INVALID_ARGUMENT);
}

// Covers AOB parsing and raw signature lookup through the runtime scanner.
static void test_signature_helpers_resolve_aob_patterns(void **state) {
	DTTR_Core_Context ctx = runtime_context();
	uintptr_t addr = 0;

	DTTR_Result result = DTTR_Core_AOBFind(&ctx, "55 8B EC ??", &addr);
	assert_true(DTTR_ResultOK(result));
	assert_ptr_equal((void *)addr, sig_target);

	addr = 0;
	result = DTTR_Core_SignatureFind(&ctx, "\x8B\xEC", "xx", &addr);
	assert_true(DTTR_ResultOK(result));
	assert_ptr_equal((void *)addr, &sig_target[1]);

	result = DTTR_Core_AOBFind(&ctx, "55 8", &addr);
	assert_int_equal(result.status, DTTR_ERR_INVALID_ARGUMENT);
}

// Covers direct patches, pointer hooks, rel32 jumps, and patch-group rollback behavior.
static void test_patch_and_group_helpers_restore_memory(void **state) {
	DTTR_Core_HookCleanupAll();
	DTTR_Core_Context ctx = runtime_context();
	const uint8_t direct_original[] = {0x10, 0x20, 0x30, 0x40};
	const uint8_t direct_patch[] = {0xAA, 0xBB, 0xCC, 0xDD};
	const uint8_t group_original[] = {0x01, 0x02};
	const uint8_t group_patch[] = {0x90, 0x90};
	const uint8_t sig_original[] = {0x55, 0x8B, 0xEC, 0x90, 0x90, 0xC3, 0x33, 0xC0};
	const uint8_t sig_patch[] = {0xCC};
	const uint8_t rel32_original[] = {0x40, 0x41, 0x42, 0x43, 0x44};
	void *pointer_original = (void *)0x11112222u;
	void *pointer_replacement = (void *)0x33334444u;
	void *out_original = NULL;
	DTTR_Core_Patch *patch = NULL;
	DTTR_Core_PatchGroup *group = NULL;
	DTTR_Core_TargetReport report = {0};
	DTTR_Core_Patch *rel32_patch = NULL;

	memcpy(sig_target, sig_original, sizeof(sig_original));
	memcpy(patch_target, direct_original, sizeof(direct_original));
	memcpy(rel32_target, rel32_original, sizeof(rel32_original));
	memcpy(group_patch_target, group_original, sizeof(group_original));
	pointer_target = pointer_original;

	DTTR_Result result = DTTR_Core_PatchBytes(
		&ctx,
		(uintptr_t)patch_target,
		direct_patch,
		sizeof(direct_patch),
		&patch
	);
	assert_true(DTTR_ResultOK(result));
	assert_memory_equal(patch_target, direct_patch, sizeof(direct_patch));
	DTTR_Core_Unpatch(patch);
	assert_memory_equal(patch_target, direct_original, sizeof(direct_original));

	result = DTTR_Core_PatchRel32Jump(
		&ctx,
		(uintptr_t)rel32_target,
		rel32_detour,
		&rel32_patch
	);
	assert_true(DTTR_ResultOK(result));
	assert_rel32_jump(rel32_target, rel32_detour);
	DTTR_Core_Unpatch(rel32_patch);
	assert_memory_equal(rel32_target, rel32_original, sizeof(rel32_original));

	const DTTR_Core_TargetSpec targets[] = {
		{
			.kind = DTTR_TARGET_ADDRESS_PATCH,
			.required = true,
			.address = (uintptr_t)group_patch_target,
			.patch_bytes = group_patch,
			.patch_size = sizeof(group_patch),
		},
		{
			.kind = DTTR_TARGET_POINTER_HOOK,
			.required = true,
			.address = (uintptr_t)&pointer_target,
			.new_value = pointer_replacement,
			.out_original = &out_original,
		},
		{
			.kind = DTTR_TARGET_AOB_REL32_JMP,
			.required = true,
			.aob = "55 8B EC ?? 90 C3",
			.offset = 3,
			.detour = rel32_detour,
		},
		{
			.kind = DTTR_TARGET_AOB_PATCH,
			.required = true,
			.aob = "55 8B EC ??",
			.offset = 1,
			.patch_bytes = sig_patch,
			.patch_size = sizeof(sig_patch),
		},
		{
			.kind = DTTR_TARGET_AOB_PATCH,
			.required = false,
			.aob = "AA BB CC DD",
			.patch_bytes = sig_patch,
			.patch_size = sizeof(sig_patch),
		},
	};

	result = DTTR_Core_PatchGroupCreate(&ctx, &group);
	assert_true(DTTR_ResultOK(result));
	result = DTTR_Core_PatchGroupInstallTargets(
		group,
		targets,
		sizeof(targets) / sizeof(targets[0]),
		&report
	);
	assert_true(DTTR_ResultOK(result));
	assert_int_equal(report.attempted, 5);
	assert_int_equal(report.installed, 4);
	assert_int_equal(report.skipped_optional, 1);
	assert_memory_equal(group_patch_target, group_patch, sizeof(group_patch));
	assert_rel32_jump(&sig_target[3], rel32_detour);
	assert_memory_equal(&sig_target[1], sig_patch, sizeof(sig_patch));
	assert_ptr_equal(pointer_target, pointer_replacement);
	assert_ptr_equal(out_original, pointer_original);

	result = DTTR_Core_PatchGroupUninstall(group);
	assert_true(DTTR_ResultOK(result));
	DTTR_Core_PatchGroupDestroy(group);
	assert_memory_equal(group_patch_target, group_original, sizeof(group_original));
	assert_memory_equal(sig_target, sig_original, sizeof(sig_original));
	assert_ptr_equal(pointer_target, pointer_original);
	assert_null(out_original);

	out_original = (void *)0x1;
	DTTR_Core_Hook *null_pointer_hook = NULL;
	result = DTTR_Core_HookPointer(
		&ctx,
		(uintptr_t)&pointer_target,
		NULL,
		&out_original,
		&null_pointer_hook
	);
	assert_true(DTTR_ResultOK(result));
	assert_non_null(null_pointer_hook);
	assert_null(pointer_target);
	assert_ptr_equal(out_original, pointer_original);
	result = DTTR_Core_Unhook(null_pointer_hook);
	assert_true(DTTR_ResultOK(result));
	assert_ptr_equal(pointer_target, pointer_original);

	DTTR_Core_HookCleanupAll();
}

// Covers patch-group helpers, grouped teardown, and failed required target rollback.
static void test_patch_group_helpers_restore_memory_and_roll_back(void **state) {
	DTTR_Core_HookCleanupAll();
	DTTR_Core_Context ctx = runtime_context();
	DTTR_Core_PatchGroup *group = NULL;
	const uint8_t patch_original[] = {0x11, 0x22, 0x33, 0x44};
	const uint8_t patch_bytes[] = {0xAA, 0xBB, 0xCC, 0xDD};
	const uint8_t group_original[] = {0x01, 0x02};
	const uint8_t group_patch[] = {0x90, 0x90};
	const uint8_t sig_original[] = {0x55, 0x8B, 0xEC, 0x90, 0x90, 0xC3, 0x33, 0xC0};
	const uint8_t rel32_original[] = {0x40, 0x41, 0x42, 0x43, 0x44};
	void *pointer_original = (void *)0x12345678u;
	void *pointer_replacement = (void *)0x87654321u;
	void *out_original = NULL;
	DTTR_Core_TargetReport report = {0};

	memcpy(patch_target, patch_original, sizeof(patch_original));
	memcpy(group_patch_target, group_original, sizeof(group_original));
	memcpy(sig_target, sig_original, sizeof(sig_original));
	memcpy(rel32_target, rel32_original, sizeof(rel32_original));
	pointer_target = pointer_original;

	DTTR_Result result = DTTR_Core_PatchGroupCreate(&ctx, &group);
	assert_true(DTTR_ResultOK(result));
	assert_non_null(group);

	result = DTTR_Core_PatchGroupPatchBytes(
		group,
		(uintptr_t)patch_target,
		patch_bytes,
		sizeof(patch_bytes),
		NULL
	);
	assert_true(DTTR_ResultOK(result));
	assert_memory_equal(patch_target, patch_bytes, sizeof(patch_bytes));

	result = DTTR_Core_PatchGroupHookPointer(
		group,
		(uintptr_t)&pointer_target,
		pointer_replacement,
		&out_original,
		NULL
	);
	assert_true(DTTR_ResultOK(result));
	assert_ptr_equal(pointer_target, pointer_replacement);
	assert_ptr_equal(out_original, pointer_original);

	result = DTTR_Core_PatchGroupPatchRel32Jump(
		group,
		(uintptr_t)rel32_target,
		rel32_detour,
		NULL
	);
	assert_true(DTTR_ResultOK(result));
	assert_rel32_jump(rel32_target, rel32_detour);

	const DTTR_Core_TargetSpec targets[] = {
		{
			.kind = DTTR_TARGET_ADDRESS_PATCH,
			.required = true,
			.address = (uintptr_t)group_patch_target,
			.patch_bytes = group_patch,
			.patch_size = sizeof(group_patch),
		},
		{
			.kind = DTTR_TARGET_AOB_PATCH,
			.required = false,
			.aob = "AA BB CC DD",
			.patch_bytes = group_patch,
			.patch_size = sizeof(group_patch),
		},
	};

	result = DTTR_Core_PatchGroupInstallTargets(
		group,
		targets,
		sizeof(targets) / sizeof(targets[0]),
		&report
	);
	assert_true(DTTR_ResultOK(result));
	assert_int_equal(report.attempted, 2);
	assert_int_equal(report.installed, 1);
	assert_int_equal(report.skipped_optional, 1);
	assert_memory_equal(group_patch_target, group_patch, sizeof(group_patch));

	result = DTTR_Core_PatchGroupUninstall(group);
	assert_true(DTTR_ResultOK(result));
	assert_memory_equal(patch_target, patch_original, sizeof(patch_original));
	assert_memory_equal(group_patch_target, group_original, sizeof(group_original));
	assert_memory_equal(sig_target, sig_original, sizeof(sig_original));
	assert_memory_equal(rel32_target, rel32_original, sizeof(rel32_original));
	assert_ptr_equal(pointer_target, pointer_original);
	assert_null(out_original);

	result = DTTR_Core_PatchGroupUninstall(group);
	assert_true(DTTR_ResultOK(result));
	DTTR_Core_PatchGroupDestroy(group);
	DTTR_Core_HookCleanupAll();
}

// Covers transactional rollback of only the entries added by a failing install call.
static void test_patch_group_target_failure_rolls_back_only_new_entries(void **state) {
	DTTR_Core_HookCleanupAll();
	DTTR_Core_Context ctx = runtime_context();
	DTTR_Core_PatchGroup *group = NULL;
	const uint8_t first_original[] = {0x10, 0x20, 0x30, 0x40};
	const uint8_t first_patch[] = {0xA0, 0xB0, 0xC0, 0xD0};
	const uint8_t added_original[] = {0x01, 0x02};
	const uint8_t added_patch[] = {0x90, 0x90};
	DTTR_Core_TargetReport report = {0};

	memcpy(patch_target, first_original, sizeof(first_original));
	memcpy(group_patch_target, added_original, sizeof(added_original));

	DTTR_Result result = DTTR_Core_PatchGroupCreate(&ctx, &group);
	assert_true(DTTR_ResultOK(result));
	result = DTTR_Core_PatchGroupPatchBytes(
		group,
		(uintptr_t)patch_target,
		first_patch,
		sizeof(first_patch),
		NULL
	);
	assert_true(DTTR_ResultOK(result));

	const DTTR_Core_TargetSpec targets[] = {
		{
			.kind = DTTR_TARGET_ADDRESS_PATCH,
			.required = true,
			.address = (uintptr_t)group_patch_target,
			.patch_bytes = added_patch,
			.patch_size = sizeof(added_patch),
		},
		{
			.kind = DTTR_TARGET_AOB_PATCH,
			.required = true,
			.aob = "AA BB CC DD",
			.patch_bytes = added_patch,
			.patch_size = sizeof(added_patch),
		},
	};

	result = DTTR_Core_PatchGroupInstallTargets(
		group,
		targets,
		sizeof(targets) / sizeof(targets[0]),
		&report
	);
	assert_int_equal(result.status, DTTR_ERR_NOT_FOUND);
	assert_int_equal(report.failed_index, 1);
	assert_memory_equal(patch_target, first_patch, sizeof(first_patch));
	assert_memory_equal(group_patch_target, added_original, sizeof(added_original));

	DTTR_Core_PatchGroupDestroy(group);
	assert_memory_equal(patch_target, first_original, sizeof(first_original));
	DTTR_Core_HookCleanupAll();
}

// Covers rollback failure being reported instead of hidden behind the install error.
static void test_patch_group_target_failure_reports_rollback_failure(void **state) {
	DTTR_Core_HookCleanupAll();
	DTTR_Core_Context ctx = runtime_context_with_unhook_failure();
	DTTR_Core_PatchGroup *group = NULL;
	const uint8_t original[] = {0x01, 0x02};
	const uint8_t patch[] = {0x90, 0x90};
	DTTR_Core_TargetReport report = {0};

	memcpy(group_patch_target, original, sizeof(original));
	fail_next_unhook_count = 1;

	DTTR_Result result = DTTR_Core_PatchGroupCreate(&ctx, &group);
	assert_true(DTTR_ResultOK(result));

	const DTTR_Core_TargetSpec targets[] = {
		{
			.kind = DTTR_TARGET_ADDRESS_PATCH,
			.required = true,
			.address = (uintptr_t)group_patch_target,
			.patch_bytes = patch,
			.patch_size = sizeof(patch),
		},
		{
			.kind = DTTR_TARGET_AOB_PATCH,
			.required = true,
			.aob = "AA BB CC DD",
			.patch_bytes = patch,
			.patch_size = sizeof(patch),
		},
	};

	result = DTTR_Core_PatchGroupInstallTargets(
		group,
		targets,
		DTTR_ARRAY_COUNT(targets),
		&report
	);
	assert_int_equal(result.status, DTTR_ERR_MEMORY_PROTECTION);
	assert_int_equal(report.failed_index, 1);
	assert_int_equal(report.status, DTTR_ERR_MEMORY_PROTECTION);
	assert_memory_equal(group_patch_target, patch, sizeof(patch));

	result = DTTR_Core_PatchGroupRelease(&group);
	assert_true(DTTR_ResultOK(result));
	assert_memory_equal(group_patch_target, original, sizeof(original));
	DTTR_Core_HookCleanupAll();
}

// Covers result release retaining the caller pointer when teardown cannot restore memory.
static void test_patch_group_result_release_retains_group_after_restore_failure(
	void **state
) {
	DTTR_Core_HookCleanupAll();
	DTTR_Core_Context ctx = runtime_context();
	void **slot = (void **)
		VirtualAlloc(NULL, sizeof(void *), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	assert_non_null(slot);
	void *slot_address = slot;
	void *pointer_original = (void *)0x10203040u;
	void *pointer_replacement = (void *)0x50607080u;
	void *out_original = NULL;
	*slot = pointer_original;

	DTTR_Core_PatchGroup *group = NULL;
	DTTR_Result result = DTTR_Core_PatchGroupCreate(&ctx, &group);
	assert_true(DTTR_ResultOK(result));
	result = DTTR_Core_PatchGroupHookPointer(
		group,
		(uintptr_t)slot,
		pointer_replacement,
		&out_original,
		NULL
	);
	assert_true(DTTR_ResultOK(result));
	assert_ptr_equal(*slot, pointer_replacement);
	assert_ptr_equal(out_original, pointer_original);

	assert_true(VirtualFree(slot, 0, MEM_RELEASE));
	result = DTTR_Core_PatchGroupRelease(&group);
	assert_int_equal(result.status, DTTR_ERR_MEMORY_PROTECTION);
	assert_non_null(group);
	assert_ptr_equal(out_original, pointer_original);

	slot = (void **)VirtualAlloc(
		slot_address,
		sizeof(void *),
		MEM_COMMIT | MEM_RESERVE,
		PAGE_READWRITE
	);
	assert_ptr_equal(slot, slot_address);
	result = DTTR_Core_PatchGroupRelease(&group);
	assert_true(DTTR_ResultOK(result));
	assert_null(group);
	assert_ptr_equal(*slot, pointer_original);
	assert_null(out_original);

	VirtualFree(slot, 0, MEM_RELEASE);
	DTTR_Core_HookCleanupAll();
}

// Covers generated PCDOGS patch specs installed through the core patch-group path.
static void test_pcdogs_patch_specs_install_custom_patches(void **state) {
	DTTR_Core_HookCleanupAll();
	DTTR_Core_Context ctx = runtime_context();
	DTTR_Core_PatchGroup *group = NULL;
	const uint8_t address_original[] = {0x11, 0x22, 0x33, 0x44};
	const uint8_t sig_original[] = {0x55, 0x8B, 0xEC, 0x90, 0x90, 0xC3, 0x33, 0xC0};
	const uint8_t rel32_original[] = {0x40, 0x41, 0x42, 0x43, 0x44};
	DTTR_PCDOGS_T_Patch_Report report = {0};

	memcpy(patch_target, address_original, sizeof(address_original));
	memcpy(sig_target, sig_original, sizeof(sig_original));
	memcpy(rel32_target, rel32_original, sizeof(rel32_original));

	const DTTR_PCDOGS_T_Patch_Spec specs[] = {
		DTTR_PCDOGS_PATCH_SPEC_ADDRESS_BYTES(
			true,
			(uintptr_t)patch_target,
			0xAA,
			0xBB,
			0xCC,
			0xDD
		),
		DTTR_PCDOGS_PATCH_SPEC_AOB_BYTES(true, "55 8B EC ??", 1, 0xCC),
		DTTR_PCDOGS_PATCH_SPEC_AOB_REL32_JMP(true, "40 41 42 43 44", 0, rel32_detour),
		DTTR_PCDOGS_PATCH_SPEC_AOB_BYTES(false, "AA BB CC DD", 0, 0xCC),
	};

	DTTR_Result result = DTTR_PCDOGS_PatchGroup_Install(
		&ctx,
		specs,
		DTTR_ARRAY_COUNT(specs),
		&group,
		&report
	);
	assert_true(DTTR_ResultOK(result));
	assert_non_null(group);
	assert_int_equal(report.attempted, 4);
	assert_int_equal(report.installed, 3);
	assert_int_equal(report.skipped_optional, 1);
	assert_memory_equal(patch_target, ((const uint8_t[]){0xAA, 0xBB, 0xCC, 0xDD}), 4);
	assert_int_equal(sig_target[1], 0xCC);
	assert_rel32_jump(rel32_target, rel32_detour);

	DTTR_Core_PatchGroupRelease(&group);
	assert_null(group);
	assert_memory_equal(patch_target, address_original, sizeof(address_original));
	assert_memory_equal(sig_target, sig_original, sizeof(sig_original));
	assert_memory_equal(rel32_target, rel32_original, sizeof(rel32_original));
	DTTR_Core_HookCleanupAll();
}

// Covers PCDOGS install preserving a retriable group when required-spec cleanup fails.
static void test_pcdogs_patch_specs_cleanup_failure_retains_group(void **state) {
	DTTR_Core_HookCleanupAll();
	DTTR_Core_Context ctx = runtime_context_with_unhook_failure();
	const uint8_t original[] = {0x11, 0x22};
	const uint8_t patch[] = {0xAA, 0xBB};
	DTTR_PCDOGS_T_Patch_Report report = {0};
	DTTR_Core_PatchGroup *group = NULL;

	memcpy(group_patch_target, original, sizeof(original));
	fail_next_unhook_count = 1;

	const DTTR_PCDOGS_T_Patch_Spec unsupported_required = {
		.required = true,
	};

	const DTTR_PCDOGS_T_Patch_Spec specs[] = {
		DTTR_PCDOGS_PATCH_SPEC_ADDRESS_BYTES(
			true,
			(uintptr_t)group_patch_target,
			0xAA,
			0xBB
		),
		unsupported_required,
	};

	DTTR_Result result = DTTR_PCDOGS_PatchGroup_Install(
		&ctx,
		specs,
		DTTR_ARRAY_COUNT(specs),
		&group,
		&report
	);
	assert_int_equal(result.status, DTTR_ERR_MEMORY_PROTECTION);
	assert_non_null(group);
	assert_int_equal(report.attempted, 2);
	assert_int_equal(report.installed, 1);
	assert_int_equal(report.failed_index, 1);
	assert_int_equal(report.status, DTTR_ERR_MEMORY_PROTECTION);
	assert_memory_equal(group_patch_target, patch, sizeof(patch));

	result = DTTR_Core_PatchGroupRelease(&group);
	assert_true(DTTR_ResultOK(result));
	assert_null(group);
	assert_memory_equal(group_patch_target, original, sizeof(original));
	DTTR_Core_HookCleanupAll();
}

// Covers the fixed-array PCDOGS install macro forwarding count and report state.
static void test_pcdogs_patch_specs_install_macro_counts_arrays(void **state) {
	DTTR_Core_HookCleanupAll();
	DTTR_Core_Context ctx = runtime_context();
	DTTR_Core_PatchGroup *group = NULL;
	const uint8_t original[] = {0x11, 0x22, 0x33, 0x44};
	DTTR_PCDOGS_T_Patch_Report report = {0};

	memcpy(patch_target, original, sizeof(original));

	const DTTR_PCDOGS_T_Patch_Spec specs[] = {
		DTTR_PCDOGS_PATCH_SPEC_ADDRESS_BYTES(
			true,
			(uintptr_t)patch_target,
			0x01,
			0x02,
			0x03,
			0x04
		),
	};

	DTTR_Result result = DTTR_PCDOGS_INSTALL_PATCHES(&ctx, specs, &group, &report);
	assert_true(DTTR_ResultOK(result));
	assert_non_null(group);
	assert_int_equal(report.attempted, DTTR_ARRAY_COUNT(specs));
	assert_int_equal(report.installed, 1);
	assert_memory_equal(patch_target, ((const uint8_t[]){0x01, 0x02, 0x03, 0x04}), 4);

	result = DTTR_Core_PatchGroupRelease(&group);
	assert_true(DTTR_ResultOK(result));
	assert_null(group);
	assert_memory_equal(patch_target, original, sizeof(original));
	DTTR_Core_HookCleanupAll();
}

// Covers generated typed hook specs installing through the fixed-array helper.
static void test_pcdogs_typed_patch_hook_spec_installs_and_clears_original(void **state) {
	DTTR_Core_HookCleanupAll();
	DTTR_Core_Context ctx = symbol_runtime_context();
	uint8_t *site = (uint8_t *)
		VirtualAlloc(NULL, 32, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	assert_non_null(site);
	memset(site, 0x90, 32);
	site[31] = 0xC3;
	const uint8_t original_prefix[5] = {0x90, 0x90, 0x90, 0x90, 0x90};

	assert_true(resolve_symbol_to_address(
		DTTR_PCDOGS_SYMBOL_FUNCTION_ID_MOVIE_PLAY_FILE,
		(uintptr_t)site
	));
	DTTR_PCDOGS_F_MoviePlayFile_proto original = NULL;
	DTTR_Core_PatchGroup *group = NULL;
	DTTR_PCDOGS_T_Patch_Report report = {0};
	const DTTR_PCDOGS_T_Patch_Spec specs[] = {
		DTTR_PCDOGS_F_MoviePlayFile->PatchSpec(true, movie_playfile_detour, &original),
	};

	DTTR_Result result = DTTR_PCDOGS_INSTALL_PATCHES(&ctx, specs, &group, &report);
	assert_true(DTTR_ResultOK(result));
	assert_non_null(group);
	assert_int_equal(report.attempted, DTTR_ARRAY_COUNT(specs));
	assert_int_equal(report.installed, 1);
	assert_non_null(original);
	assert_rel32_jump(site, movie_playfile_detour);

	DTTR_Core_PatchGroupRelease(&group);
	assert_null(group);
	assert_null(original);
	assert_memory_equal(site, original_prefix, sizeof(original_prefix));

	VirtualFree(site, 0, MEM_RELEASE);
	DTTR_Core_HookCleanupAll();
	DTTR_PCDOGS_Reset();
}

// Covers multiple generated PatchSpec groups composing on one function hook site.
static void test_pcdogs_typed_patch_hook_specs_chain_and_uninstall(void **state) {
	DTTR_Core_HookCleanupAll();
	DTTR_PCDOGS_Reset();
	DTTR_Core_Context ctx = symbol_runtime_context();
	pcdogs_chain_original_a = NULL;
	pcdogs_chain_original_b = NULL;

	assert_true(resolve_symbol_to_address(
		DTTR_PCDOGS_SYMBOL_FUNCTION_ID_TIMER_GET_RAW_TICK_COUNT,
		(uintptr_t)pcdogs_chain_target
	));

	DTTR_Core_PatchGroup *group_a = NULL;
	DTTR_Core_PatchGroup *group_b = NULL;
	DTTR_PCDOGS_T_Patch_Report report = {0};
	const DTTR_PCDOGS_T_Patch_Spec spec_a[] = {
		DTTR_PCDOGS_F_TimerGetRawTickCount
			->PatchSpec(true, pcdogs_chain_detour_a, &pcdogs_chain_original_a),
	};

	DTTR_Result result = DTTR_PCDOGS_INSTALL_PATCHES(&ctx, spec_a, &group_a, &report);
	assert_true(DTTR_ResultOK(result));
	assert_non_null(group_a);
	assert_non_null(pcdogs_chain_original_a);
	assert_int_equal(pcdogs_chain_target(), 17);

	memset(&report, 0, sizeof(report));
	const DTTR_PCDOGS_T_Patch_Spec spec_b[] = {
		DTTR_PCDOGS_F_TimerGetRawTickCount
			->PatchSpec(true, pcdogs_chain_detour_b, &pcdogs_chain_original_b),
	};

	result = DTTR_PCDOGS_INSTALL_PATCHES(&ctx, spec_b, &group_b, &report);
	assert_true(DTTR_ResultOK(result));
	assert_non_null(group_b);
	assert_non_null(pcdogs_chain_original_b);
	assert_int_equal(pcdogs_chain_target(), 117);

	result = DTTR_Core_PatchGroupRelease(&group_b);
	assert_true(DTTR_ResultOK(result));
	assert_null(group_b);
	assert_null(pcdogs_chain_original_b);
	assert_non_null(pcdogs_chain_original_a);
	assert_int_equal(pcdogs_chain_target(), 17);

	result = DTTR_Core_PatchGroupRelease(&group_a);
	assert_true(DTTR_ResultOK(result));
	assert_null(group_a);
	assert_null(pcdogs_chain_original_a);
	assert_int_equal(pcdogs_chain_target(), 7);

	DTTR_Core_HookCleanupAll();
	DTTR_PCDOGS_Reset();
}

// Covers result-returning core hook APIs surfacing unsupported chain conflicts clearly.
static void test_core_function_hook_overlap_reports_chain_unsupported(void **state) {
	DTTR_Core_HookCleanupAll();
	DTTR_Core_Context ctx = runtime_context();
	uint8_t *site = (uint8_t *)
		VirtualAlloc(NULL, 32, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	assert_non_null(site);
	memset(site, 0x90, 32);
	site[31] = 0xC3;

	const uint8_t trap = 0xCC;
	DTTR_Core_Patch *patch = NULL;
	DTTR_Result result = DTTR_Core_PatchBytes(
		&ctx,
		(uintptr_t)site,
		&trap,
		sizeof(trap),
		&patch
	);
	assert_true(DTTR_ResultOK(result));
	assert_non_null(patch);

	DTTR_Core_Hook *hook = NULL;
	result = DTTR_Core_HookFunction(&ctx, (uintptr_t)site, 5, rel32_detour, NULL, &hook);
	assert_int_equal(result.status, DTTR_ERR_HOOK_CHAIN_UNSUPPORTED);
	assert_string_equal(DTTR_StatusName(result.status), "DTTR_ERR_HOOK_CHAIN_UNSUPPORTED");
	assert_null(hook);

	result = DTTR_Core_Unpatch(patch);
	assert_true(DTTR_ResultOK(result));
	VirtualFree(site, 0, MEM_RELEASE);
	DTTR_Core_HookCleanupAll();
}

// Covers pointer-valued PCDOGS data helpers accepting NULL replacements, matching core
// pointer-hook semantics for clearing a slot.
static void test_pcdogs_data_pointer_hooks_accept_null_replacement(void **state) {
	DTTR_Core_HookCleanupAll();
	DTTR_Core_Context ctx = symbol_runtime_context();
	uint8_t *site = (uint8_t *)
		VirtualAlloc(NULL, 1024, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	assert_non_null(site);
	memset(site, 0x90, 1024);

	void *pointer_original = (void *)0x12345678u;
	void *out_original = (void *)0x1;
	pointer_target = pointer_original;
	uint32_t pointer_slot = (uint32_t)(uintptr_t)&pointer_target;
	memcpy(site + 727, &pointer_slot, sizeof(pointer_slot));

	assert_true(resolve_typed_function_to_address(
		&ctx,
		DTTR_PCDOGS_SYMBOL_FUNCTION_ID_D3D_CREATE_TEXTURE_SURFACE,
		(uintptr_t)site
	));

	DTTR_Core_Hook *hook = NULL;
	DTTR_Result result = DTTR_PCDOGS_Hook_DataPointer(
		&ctx,
		DTTR_PCDOGS_DATA_DDRAW_OBJECT,
		NULL,
		&out_original,
		&hook
	);
	assert_true(DTTR_ResultOK(result));
	assert_non_null(hook);
	assert_null(pointer_target);
	assert_ptr_equal(out_original, pointer_original);
	result = DTTR_Core_Unhook(hook);
	assert_true(DTTR_ResultOK(result));
	assert_ptr_equal(pointer_target, pointer_original);

	out_original = (void *)0x1;
	DTTR_Core_PatchGroup *group = NULL;
	DTTR_PCDOGS_T_Patch_Report report = {0};
	const DTTR_PCDOGS_T_Patch_Spec specs[] = {
		DTTR_PCDOGS_D_DDrawObject->PatchSpec(true, NULL, &out_original),
	};

	result = DTTR_PCDOGS_INSTALL_PATCHES(&ctx, specs, &group, &report);
	assert_true(DTTR_ResultOK(result));
	assert_non_null(group);
	assert_int_equal(report.attempted, 1);
	assert_int_equal(report.installed, 1);
	assert_null(pointer_target);
	assert_ptr_equal(out_original, pointer_original);
	result = DTTR_Core_PatchGroupRelease(&group);
	assert_true(DTTR_ResultOK(result));
	assert_null(group);
	assert_ptr_equal(pointer_target, pointer_original);
	assert_null(out_original);

	VirtualFree(site, 0, MEM_RELEASE);
	DTTR_Core_HookCleanupAll();
	DTTR_PCDOGS_Reset();
}

// Covers stateless actor utility helpers.
static void test_pcdogs_active_actor_helpers(void **state) {
	uint8_t actor_storage[1] = {0};
	uint8_t other_actor_storage[1] = {0};
	DTTR_PCDOGS_T_Actor_State *actor = (DTTR_PCDOGS_T_Actor_State *)actor_storage;
	DTTR_PCDOGS_T_Actor_State *other_actor = (DTTR_PCDOGS_T_Actor_State *)
		other_actor_storage;

	assert_true(DTTR_Util_SameActor(actor, actor));
	assert_false(DTTR_Util_SameActor(actor, other_actor));
	assert_false(DTTR_Util_SameActor(actor, NULL));
	assert_false(DTTR_Util_SameActor(NULL, actor));
	assert_false(DTTR_Util_SameActor(NULL, NULL));

	DTTR_Core_Context ctx = symbol_runtime_context();
	DTTR_PCDOGS_Reset();
	active_actor_result = actor;
	assert_null(DTTR_Util_GetActiveActor(&ctx));
	assert_true(resolve_typed_function_to_address(
		&ctx,
		DTTR_PCDOGS_SYMBOL_FUNCTION_ID_ENTITY_GET_ACTIVE_ACTOR_FROM_LIST,
		(uintptr_t)active_actor_stub
	));
	assert_true(DTTR_PCDOGS_F_EntityGetActiveActorFromList->IsResolved());
	assert_ptr_equal(DTTR_Util_GetActiveActor(&ctx), actor);
	active_actor_result = NULL;
	assert_null(DTTR_Util_GetActiveActor(&ctx));
	assert_null(DTTR_Util_GetActiveActor(NULL));
	DTTR_PCDOGS_Reset();
	active_actor_result = NULL;
}

// Covers Reset preserving hook handles so a following UnhookAll can restore bytes.
static void test_pcdogs_reset_preserves_unhookable_handles(void **state) {
	DTTR_Core_HookCleanupAll();
	DTTR_Core_Context ctx = symbol_runtime_context();
	uint8_t *site = (uint8_t *)
		VirtualAlloc(NULL, 32, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	assert_non_null(site);
	memset(site, 0x90, 32);
	site[31] = 0xC3;
	const uint8_t original_prefix[5] = {0x90, 0x90, 0x90, 0x90, 0x90};

	assert_true(resolve_typed_function_to_address(
		&ctx,
		DTTR_PCDOGS_SYMBOL_FUNCTION_ID_MOVIE_PLAY_FILE,
		(uintptr_t)site
	));
	DTTR_PCDOGS_F_MoviePlayFile_proto original = NULL;
	assert_true(DTTR_PCDOGS_F_MoviePlayFile->Hook(&ctx, movie_playfile_detour, &original));
	assert_rel32_jump(site, movie_playfile_detour);

	DTTR_PCDOGS_Reset();
	DTTR_PCDOGS_Unhook_All(&ctx);
	assert_memory_equal(site, original_prefix, sizeof(original_prefix));

	VirtualFree(site, 0, MEM_RELEASE);
	DTTR_Core_HookCleanupAll();
	DTTR_PCDOGS_Reset();
}

// Covers generated object metadata plus direct resolve, hooks, and custom patch specs.
static void test_pcdogs_symbol_facade_exposes_object_metadata(void **state) {
	DTTR_Core_Context ctx = runtime_context();

	assert_int_equal(
		DTTR_PCDOGS_F_MoviePlayFile->FunctionId,
		DTTR_PCDOGS_FUNCTION_MOVIE_PLAY_FILE
	);
	assert_int_equal(
		DTTR_PCDOGS_F_MoviePlayFile->SymbolId,
		DTTR_PCDOGS_SYMBOL_FUNCTION_ID_MOVIE_PLAY_FILE
	);
	assert_int_equal(DTTR_PCDOGS_D_PkgBasePath->DataId, DTTR_PCDOGS_DATA_PKG_BASE_PATH);
	assert_int_equal(
		DTTR_PCDOGS_D_PkgBasePath->SymbolId,
		DTTR_PCDOGS_SYMBOL_DATA_ID_PKG_BASE_PATH
	);

	const DTTR_PCDOGS_T_Symbol_Function *fn = DTTR_PCDOGS_SymbolFunctionAt(
		(uint32_t)DTTR_PCDOGS_F_MoviePlayFile->SymbolId
	);
	assert_non_null(fn);
	assert_int_equal(fn->hook_kind, DTTR_PCDOGS_HOOK_REL32);

	const DTTR_PCDOGS_T_Symbol_Function *hotpatch_fn = DTTR_PCDOGS_SymbolFunctionAt(
		(uint32_t)DTTR_PCDOGS_F_LevelNormalizeIndex->SymbolId
	);
	assert_non_null(hotpatch_fn);
	assert_int_equal(hotpatch_fn->hook_kind, DTTR_PCDOGS_HOOK_HOTPATCH);
	assert_int_equal(
		DTTR_PCDOGS_F_LevelNormalizeIndex->HookKind(),
		DTTR_PCDOGS_HOOK_HOTPATCH
	);
	assert_int_equal(DTTR_PCDOGS_F_LevelNormalizeIndex->HookPrologueSize(), 2);
	DTTR_PCDOGS_F_LevelNormalizeIndex_proto hotpatch_original = NULL;
	assert_false(DTTR_PCDOGS_F_LevelNormalizeIndex
					 ->Hook(&ctx, level_normalize_index_detour, &hotpatch_original));
	assert_null(hotpatch_original);
	DTTR_PCDOGS_T_Patch_Spec hotpatch_spec = DTTR_PCDOGS_F_LevelNormalizeIndex->PatchSpec(
		true,
		level_normalize_index_detour,
		&hotpatch_original
	);
	assert_int_equal(hotpatch_spec.kind, DTTR_PCDOGS_PATCH_UNSUPPORTED);
	assert_true(hotpatch_spec.required);

	const DTTR_PCDOGS_T_Symbol_Data *data = DTTR_PCDOGS_SymbolDataAt(
		(uint32_t)DTTR_PCDOGS_D_PkgBasePath->SymbolId
	);
	assert_non_null(data);

	DTTR_Core_Hook *hook = (DTTR_Core_Hook *)0x1;
	DTTR_Result result = DTTR_PCDOGS_Hook_DataPointer(
		NULL,
		DTTR_PCDOGS_DATA_PKG_BASE_PATH,
		rel32_detour,
		NULL,
		&hook
	);
	assert_int_equal(result.status, DTTR_ERR_INVALID_ARGUMENT);
	assert_null(hook);

	result = DTTR_PCDOGS_PatchGroup_HookDataPointer(
		NULL,
		DTTR_PCDOGS_DATA_PKG_BASE_PATH,
		rel32_detour,
		NULL
	);
	assert_int_equal(result.status, DTTR_ERR_INVALID_ARGUMENT);

	result = DTTR_PCDOGS_PatchGroup_HookFunction(
		NULL,
		DTTR_PCDOGS_F_MoviePlayFile->FunctionId,
		rel32_detour,
		NULL
	);
	assert_int_equal(result.status, DTTR_ERR_INVALID_ARGUMENT);

	const DTTR_Core_TargetSpec raw_target = {
		.kind = DTTR_TARGET_ADDRESS_HOOK,
		.required = true,
		.address = 0x1234u,
		.detour = rel32_detour,
		.prologue_size = 5,
	};

	const DTTR_PCDOGS_T_Patch_Spec specs[] = {
		DTTR_PCDOGS_PATCH_SPEC_TARGET(true, raw_target),
		DTTR_PCDOGS_PATCH_SPEC_ADDRESS_BYTES(false, 0x1234u, 0x90),
		DTTR_PCDOGS_PATCH_SPEC_AOB_BYTES(false, "90 90", 0, 0x90),
		DTTR_PCDOGS_PATCH_SPEC_AOB_REL32_JMP(false, "90 90", 0, rel32_detour),
	};

	assert_int_equal(specs[0].kind, DTTR_PCDOGS_PATCH_TARGET);
	assert_int_equal(specs[1].kind, DTTR_PCDOGS_PATCH_ADDRESS_BYTES);
	assert_int_equal(specs[2].kind, DTTR_PCDOGS_PATCH_AOB_BYTES);
	assert_int_equal(specs[3].kind, DTTR_PCDOGS_PATCH_AOB_REL32_JMP);

	result = DTTR_PCDOGS_PatchGroup_Install(
		NULL,
		specs,
		DTTR_ARRAY_COUNT(specs),
		NULL,
		NULL
	);
	assert_int_equal(result.status, DTTR_ERR_INVALID_ARGUMENT);

	DTTR_Core_PatchGroup *existing_group = NULL;
	result = DTTR_Core_PatchGroupCreate(&ctx, &existing_group);
	assert_true(DTTR_ResultOK(result));
	DTTR_PCDOGS_T_Patch_Report report = {0};
	result = DTTR_PCDOGS_PatchGroup_Install(&ctx, specs, 0, &existing_group, &report);
	assert_int_equal(result.status, DTTR_ERR_ALREADY_INSTALLED);
	assert_int_equal(report.failed_index, 0);
	result = DTTR_Core_PatchGroupRelease(&existing_group);
	assert_true(DTTR_ResultOK(result));
	assert_null(existing_group);

	DTTR_PCDOGS_T_Patch_Spec unsupported = {0};
	unsupported.required = false;
	DTTR_Core_PatchGroup *optional_group = NULL;
	memset(&report, 0, sizeof(report));
	result = DTTR_PCDOGS_PatchGroup_Install(
		&ctx,
		&unsupported,
		1,
		&optional_group,
		&report
	);
	assert_true(DTTR_ResultOK(result));
	assert_non_null(optional_group);
	assert_int_equal(report.attempted, 1);
	assert_int_equal(report.installed, 0);
	assert_int_equal(report.skipped_optional, 1);
	result = DTTR_Core_PatchGroupRelease(&optional_group);
	assert_true(DTTR_ResultOK(result));
	assert_null(optional_group);

	unsupported.required = true;
	DTTR_Core_PatchGroup *required_group = NULL;
	memset(&report, 0, sizeof(report));
	result = DTTR_PCDOGS_PatchGroup_Install(
		&ctx,
		&unsupported,
		1,
		&required_group,
		&report
	);
	assert_int_equal(result.status, DTTR_ERR_UNSUPPORTED);
	assert_null(required_group);
	assert_int_equal(report.attempted, 1);
	assert_int_equal(report.failed_index, 0);
	assert_int_equal(report.status, DTTR_ERR_UNSUPPORTED);
}

typedef struct {
	DTTR_PCDOGS_T_Pkg_TOCEntry toc[2];
	uint8_t payloads[2][8];
	int load_count;
	int free_count;
	int failed_loads;
	int toc_visits;
	int loaded_visits;
	int unsupported_visits;
	int status_visits;
	int max_depth;
	const void *last_freed;
	bool stop_after_first_toc;
} pkg_walk_test_state;

static bool load_fixture_entry(
	const DTTR_Core_Context *ctx,
	int32_t toc_index,
	const DTTR_PCDOGS_T_Pkg_TOCEntry *entry,
	void *userdata,
	void **out_entry,
	size_t *out_size,
	DTTR_Util_PkgVisitStatus *out_status
) {
	pkg_walk_test_state *state = userdata;
	if (!state || !entry || !out_entry || !out_size || toc_index < 0 || toc_index >= 2) {
		if (out_status) {
			*out_status = DTTR_UTIL_PKG_STATUS_INVALID_ARGUMENT;
		}

		return false;
	}

	state->load_count++;
	*out_entry = state->payloads[toc_index];
	*out_size = entry->size;
	return true;
}

static bool fail_first_fixture_entry(
	const DTTR_Core_Context *ctx,
	int32_t toc_index,
	const DTTR_PCDOGS_T_Pkg_TOCEntry *entry,
	void *userdata,
	void **out_entry,
	size_t *out_size,
	DTTR_Util_PkgVisitStatus *out_status
) {
	pkg_walk_test_state *state = userdata;
	if (toc_index == 0) {
		state->failed_loads++;
		*out_entry = NULL;
		*out_size = 0;
		*out_status = DTTR_UTIL_PKG_STATUS_LOAD_FAILED;
		return false;
	}

	return load_fixture_entry(
		ctx,
		toc_index,
		entry,
		userdata,
		out_entry,
		out_size,
		out_status
	);
}

static void free_fixture_entry(
	const DTTR_Core_Context *ctx,
	int32_t toc_index,
	const DTTR_PCDOGS_T_Pkg_TOCEntry *entry,
	void *entry_data,
	void *userdata
) {
	pkg_walk_test_state *state = userdata;
	state->free_count++;
	state->last_freed = entry_data;
}

static DTTR_Util_PkgVisitAction count_visits(
	const DTTR_Util_PkgVisit *visit,
	void *userdata
) {
	pkg_walk_test_state *state = userdata;
	assert_non_null(visit);
	assert_ptr_equal(visit->userdata, state);
	if ((int)visit->depth > state->max_depth) {
		state->max_depth = (int)visit->depth;
	}

	state->status_visits += visit->status != DTTR_UTIL_PKG_STATUS_OK;
	if (visit->kind == DTTR_UTIL_PKG_VISIT_TOC_ENTRY) {
		state->toc_visits++;
		return state->stop_after_first_toc ? DTTR_UTIL_PKG_VISIT_STOP
										   : DTTR_UTIL_PKG_VISIT_RECURSE;
	}

	if (visit->kind == DTTR_UTIL_PKG_VISIT_LOADED_ENTRY) {
		state->loaded_visits++;
		assert_non_null(DTTR_Util_PkgVisit_AsLoadedEntry(visit));
		return DTTR_UTIL_PKG_VISIT_RECURSE;
	}

	state->unsupported_visits += visit->kind == DTTR_UTIL_PKG_VISIT_UNSUPPORTED;
	return DTTR_UTIL_PKG_VISIT_CONTINUE;
}

static pkg_walk_test_state make_state() {
	pkg_walk_test_state state = {0};
	state.toc[0].offset = 0x1000;
	state.toc[0].size = sizeof(state.payloads[0]);
	state.toc[1].offset = 0x2000;
	state.toc[1].size = sizeof(state.payloads[1]);
	memset(state.payloads[0], 0xA5, sizeof(state.payloads[0]));
	memset(state.payloads[1], 0x5A, sizeof(state.payloads[1]));
	return state;
}

static DTTR_Util_PkgWalkOptions make_options(pkg_walk_test_state *state) {
	DTTR_Util_PkgWalkOptions options = DTTR_Util_PkgWalk_DefaultOptions();
	options.toc_entries = state->toc;
	options.toc_count = 2;
	options.load_entry = load_fixture_entry;
	options.free_entry = free_fixture_entry;
	options.io_userdata = state;
	return options;
}

static DTTR_Util_PkgWalkResult walk_fixture(
	const DTTR_Util_PkgWalkOptions *options,
	pkg_walk_test_state *state
) {
	return DTTR_Util_PkgWalk(NULL, options, count_visits, state);
}

static void test_pkg_walk_core_behaviors(void **test_state) {
	{
		pkg_walk_test_state state = make_state();
		DTTR_Util_PkgWalkOptions options = make_options(&state);

		DTTR_Util_PkgWalkResult result = walk_fixture(&options, &state);

		assert_int_equal(result.status, DTTR_UTIL_PKG_STATUS_OK);
		assert_false(result.stopped);
		assert_int_equal(state.toc_visits, 2);
		assert_int_equal(state.load_count, 2);
		assert_int_equal(state.loaded_visits, 2);
		assert_int_equal(state.unsupported_visits, 2);
		assert_int_equal(state.free_count, 2);
		assert_ptr_equal(state.last_freed, state.payloads[1]);
		assert_true(result.visited_count >= 6);
		assert_int_equal(state.max_depth, 2);
	}

	{
		pkg_walk_test_state state = make_state();
		state.stop_after_first_toc = true;
		DTTR_Util_PkgWalkOptions options = make_options(&state);

		DTTR_Util_PkgWalkResult result = walk_fixture(&options, &state);

		assert_int_equal(result.status, DTTR_UTIL_PKG_STATUS_OK);
		assert_true(result.stopped);
		assert_int_equal(state.toc_visits, 1);
		assert_int_equal(state.load_count, 0);
		assert_int_equal(state.loaded_visits, 0);
		assert_int_equal(state.free_count, 0);
	}

	{
		pkg_walk_test_state state = make_state();
		DTTR_Util_PkgWalkOptions options = make_options(&state);
		options.load_entry = fail_first_fixture_entry;

		DTTR_Util_PkgWalkResult result = walk_fixture(&options, &state);

		assert_int_equal(result.status, DTTR_UTIL_PKG_STATUS_OK);
		assert_false(result.stopped);
		assert_int_equal(state.failed_loads, 1);
		assert_int_equal(state.status_visits, 1);
		assert_int_equal(state.toc_visits, 2);
		assert_int_equal(state.loaded_visits, 1);
		assert_int_equal(state.free_count, 1);
	}
}

static void test_pkg_visit_accessors_and_status_names(void **test_state) {
	DTTR_PCDOGS_T_Pkg_TOCEntry toc = {0};
	DTTR_PCDOGS_T_Mesh_Node mesh = {0};
	DTTR_PCDOGS_T_Level_RuntimeData level = {0};
	DTTR_Util_PkgVisit visit = {0};

	visit.kind = DTTR_UTIL_PKG_VISIT_TOC_ENTRY;
	visit.ptr = &toc;
	assert_ptr_equal(DTTR_Util_PkgVisit_AsTOCEntry(&visit), &toc);
	assert_null(DTTR_Util_PkgVisit_AsMeshNode(&visit));

	visit.kind = DTTR_UTIL_PKG_VISIT_MESH_NODE;
	visit.ptr = &mesh;
	assert_ptr_equal(DTTR_Util_PkgVisit_AsMeshNode(&visit), &mesh);
	assert_null(DTTR_Util_PkgVisit_AsLevelRuntimeData(&visit));

	visit.kind = DTTR_UTIL_PKG_VISIT_LEVEL_RUNTIME_DATA;
	visit.ptr = &level;
	assert_ptr_equal(DTTR_Util_PkgVisit_AsLevelRuntimeData(&visit), &level);
	assert_null(DTTR_Util_PkgVisit_AsTOCEntry(NULL));

	assert_string_equal(DTTR_Util_PkgVisitStatusName(DTTR_UTIL_PKG_STATUS_OK), "ok");
	assert_string_equal(
		DTTR_Util_PkgVisitStatusName(DTTR_UTIL_PKG_STATUS_LOAD_FAILED),
		"load failed"
	);
	assert_string_equal(
		DTTR_Util_PkgVisitStatusName((DTTR_Util_PkgVisitStatus)999),
		"unknown status"
	);
}

static const DTTR_TestCase TEST_CASES[] = {
	{"core-sdk-validation", test_core_validation},
	{"core-sdk-signatures", test_signature_helpers_resolve_aob_patterns},
	{"core-sdk-patches-and-groups", test_patch_and_group_helpers_restore_memory},
	{"core-sdk-patch-group", test_patch_group_helpers_restore_memory_and_roll_back},
	{"core-sdk-patch-group-rollback",
	 test_patch_group_target_failure_rolls_back_only_new_entries},
	{"core-sdk-patch-group-rollback-failure",
	 test_patch_group_target_failure_reports_rollback_failure},
	{"core-sdk-patch-group-result-release-failure",
	 test_patch_group_result_release_retains_group_after_restore_failure},
	{"pkg-walk-core-behaviors", test_pkg_walk_core_behaviors},
	{"pkg-walk-accessors-and-status-names", test_pkg_visit_accessors_and_status_names},
	{"pcdogs-patch-specs", test_pcdogs_patch_specs_install_custom_patches},
	{"pcdogs-patch-spec-cleanup-failure",
	 test_pcdogs_patch_specs_cleanup_failure_retains_group},
	{"pcdogs-patch-spec-install-macro",
	 test_pcdogs_patch_specs_install_macro_counts_arrays},
	{"pcdogs-typed-patch-hook-spec",
	 test_pcdogs_typed_patch_hook_spec_installs_and_clears_original},
	{"pcdogs-typed-patch-hook-chain",
	 test_pcdogs_typed_patch_hook_specs_chain_and_uninstall},
	{"core-sdk-hook-chain-unsupported",
	 test_core_function_hook_overlap_reports_chain_unsupported},
	{"pcdogs-data-pointer-null-hook",
	 test_pcdogs_data_pointer_hooks_accept_null_replacement},
	{"pcdogs-active-actor-helpers", test_pcdogs_active_actor_helpers},
	{"pcdogs-reset-unhookable", test_pcdogs_reset_preserves_unhookable_handles},
	{"pcdogs-symbol-facade", test_pcdogs_symbol_facade_exposes_object_metadata},
};

DTTR_TEST_MAIN(TEST_CASES)
