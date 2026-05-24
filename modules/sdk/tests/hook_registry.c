#include <stdint.h>
#include <string.h>
#include <windows.h>

#include <dttr_runtime.h>
#include <dttr_test_support.h>

static uint8_t patch_target[4] = {0x10, 0x20, 0x30, 0x40};
static void *pointer_target = (void *)0x11112222u;

// Covers byte patch detach restoring original memory.
static void test_patch_bytes_detach_restores_original(void **state) {
	DTTR_Core_HookCleanupAll();

	const uint8_t original[] = {0x10, 0x20, 0x30, 0x40};
	const uint8_t patch[] = {0xAA, 0xBB, 0xCC, 0xDD};
	memcpy(patch_target, original, sizeof(original));

	DTTR_Core_Hook *hook = DTTR_Core_HookPatchBytes(
		(uintptr_t)patch_target,
		patch,
		sizeof(patch)
	);
	assert_non_null(hook);
	assert_memory_equal(patch_target, patch, sizeof(patch));

	DTTR_Core_HookDetach(hook);
	assert_memory_equal(patch_target, original, sizeof(original));
}

// Covers pointer-slot hook detach restoring the previous pointer.
static void test_pointer_hook_detach_restores_original(void **state) {
	DTTR_Core_HookCleanupAll();

	void *original = (void *)0x11112222u;
	void *replacement = (void *)0x33334444u;
	void *out_original = NULL;
	pointer_target = original;

	DTTR_Core_Hook *hook = DTTR_Core_HookAttachPointer(
		(uintptr_t)&pointer_target,
		replacement,
		&out_original
	);
	assert_non_null(hook);
	assert_ptr_equal(out_original, original);
	assert_ptr_equal(pointer_target, replacement);

	DTTR_Core_HookDetach(hook);
	assert_ptr_equal(pointer_target, original);
}

// Covers owner tags detaching only hooks created for the selected SDK context.
static void test_owner_detach_only_detaches_matching_owner(void **state) {
	DTTR_Core_HookCleanupAll();

	uint8_t first = 0x11;
	uint8_t second = 0x22;
	const uint8_t first_patch = 0xA1;
	const uint8_t second_patch = 0xB2;
	int owner_a = 0;
	int owner_b = 0;

	void *previous_owner = DTTR_Core_HookSetOwner(&owner_a);
	DTTR_Core_Hook *first_hook = DTTR_Core_HookPatchBytes(
		(uintptr_t)&first,
		&first_patch,
		1
	);
	assert_non_null(first_hook);
	DTTR_Core_HookSetOwner(&owner_b);
	DTTR_Core_Hook *second_hook = DTTR_Core_HookPatchBytes(
		(uintptr_t)&second,
		&second_patch,
		1
	);
	assert_non_null(second_hook);
	DTTR_Core_HookSetOwner(previous_owner);

	assert_int_equal(first, first_patch);
	assert_int_equal(second, second_patch);

	DTTR_Core_HookDetachOwner(&owner_a);
	assert_int_equal(first, 0x11);
	assert_int_equal(second, second_patch);

	DTTR_Core_HookDetachOwner(&owner_b);
	assert_int_equal(second, 0x22);
}

// Covers global cleanup restoring hooks and leaving the registry reusable.
static void test_cleanup_all_restores_hooks_and_allows_reuse(void **state) {
	DTTR_Core_HookCleanupAll();

	uint8_t target = 0x55;
	const uint8_t patch = 0x66;
	DTTR_Core_Hook *hook = DTTR_Core_HookPatchBytes((uintptr_t)&target, &patch, 1);
	assert_non_null(hook);
	assert_int_equal(target, patch);

	HMODULE module = GetModuleHandleA(NULL);
	assert_non_null(module);
	DTTR_Core_HookCachedSigscan(module, "\x4D\x5A", "xx");

	DTTR_Core_HookCleanupAll();
	assert_int_equal(target, 0x55);

	DTTR_Core_Hook *second_hook = DTTR_Core_HookPatchBytes((uintptr_t)&target, &patch, 1);
	assert_non_null(second_hook);
	assert_int_equal(target, patch);
	DTTR_Core_HookCleanupAll();
	assert_int_equal(target, 0x55);
}

// Covers overlapping byte patches being rejected before they can corrupt restore order.
static void test_overlapping_byte_patches_are_rejected(void **state) {
	DTTR_Core_HookCleanupAll();

	uint8_t target[4] = {0x10, 0x20, 0x30, 0x40};
	const uint8_t first_patch[] = {0xAA, 0xBB};
	const uint8_t overlapping_patch[] = {0xCC, 0xDD};

	DTTR_Core_Hook *first_hook = DTTR_Core_HookPatchBytes(
		(uintptr_t)&target[1],
		first_patch,
		sizeof(first_patch)
	);
	assert_non_null(first_hook);
	assert_memory_equal(&target[1], first_patch, sizeof(first_patch));

	DTTR_Core_Hook *overlapping_hook = DTTR_Core_HookPatchBytes(
		(uintptr_t)&target[2],
		overlapping_patch,
		sizeof(overlapping_patch)
	);
	assert_null(overlapping_hook);
	assert_int_equal(target[2], 0xBB);

	DTTR_Core_HookDetach(first_hook);
	assert_memory_equal(target, ((uint8_t[]){0x10, 0x20, 0x30, 0x40}), sizeof(target));
}

typedef int(__cdecl *hook_target_fn)(int value);
static hook_target_fn original_hook_target = NULL;
static hook_target_fn chain_original_a = NULL;
static hook_target_fn chain_original_b = NULL;
static hook_target_fn chain_original_c = NULL;
static int chain_call_log[8];
static size_t chain_call_count = 0;

// Provide a real call target so the trampoline test exercises executable code.
__attribute__((noinline)) static int __cdecl hook_target(int value) {
	volatile int extra = 7;
	return value + extra;
}

// Provide a detour target for trampoline hook tests.
static int __cdecl hook_detour(int value) {
	assert_non_null(original_hook_target);
	return original_hook_target(value) + 100;
}

static void chain_log(int id) {
	assert_true(chain_call_count < DTTR_TEST_ARRAY_COUNT(chain_call_log));
	chain_call_log[chain_call_count++] = id;
}

static void chain_reset() {
	chain_original_a = NULL;
	chain_original_b = NULL;
	chain_original_c = NULL;
	memset(chain_call_log, 0, sizeof(chain_call_log));
	chain_call_count = 0;
}

static int __cdecl chain_detour_a(int value) {
	chain_log(1);
	assert_non_null(chain_original_a);
	return chain_original_a(value) + 10;
}

static int __cdecl chain_detour_b(int value) {
	chain_log(2);
	assert_non_null(chain_original_b);
	return chain_original_b(value) + 100;
}

static int __cdecl chain_detour_c(int value) {
	chain_log(3);
	assert_non_null(chain_original_c);
	return chain_original_c(value) + 1000;
}

// Covers function hook patching, trampoline execution, and detach restoration.
static void test_function_hook_trampoline_and_detach(void **state) {
	DTTR_Core_HookCleanupAll();
	original_hook_target = NULL;

	const int before = hook_target(5);
	assert_int_equal(before, 12);

	DTTR_Core_Hook *hook = DTTR_Core_HookAttachFunction(
		(uintptr_t)hook_target,
		0,
		hook_detour,
		(void **)&original_hook_target
	);
	assert_non_null(hook);
	assert_non_null(original_hook_target);
	assert_int_equal(hook_target(5), 112);

	DTTR_Core_HookDetach(hook);
	original_hook_target = NULL;
	assert_int_equal(hook_target(5), 12);
}

// Covers same-site function hooks chaining newest first through each out_original.
static void test_function_hook_chain_calls_newest_first(void **state) {
	DTTR_Core_HookCleanupAll();
	chain_reset();

	assert_int_equal(hook_target(5), 12);

	DTTR_Core_Hook *hook_a = DTTR_Core_HookAttachFunction(
		(uintptr_t)hook_target,
		0,
		chain_detour_a,
		(void **)&chain_original_a
	);
	assert_non_null(hook_a);
	assert_non_null(chain_original_a);

	DTTR_Core_Hook *hook_b = DTTR_Core_HookAttachFunction(
		(uintptr_t)hook_target,
		0,
		chain_detour_b,
		(void **)&chain_original_b
	);
	assert_non_null(hook_b);
	assert_non_null(chain_original_b);
	assert_ptr_not_equal(chain_original_a, chain_original_b);

	assert_int_equal(hook_target(5), 122);
	assert_int_equal(chain_call_count, 2);
	assert_int_equal(chain_call_log[0], 2);
	assert_int_equal(chain_call_log[1], 1);

	DTTR_Core_HookDetach(hook_b);
	chain_original_b = NULL;
	chain_call_count = 0;
	assert_int_equal(hook_target(5), 22);
	assert_int_equal(chain_call_count, 1);
	assert_int_equal(chain_call_log[0], 1);

	DTTR_Core_HookDetach(hook_a);
	chain_original_a = NULL;
	assert_int_equal(hook_target(5), 12);
}

// Covers removing a middle chain link rewiring the predecessor's original thunk.
static void test_function_hook_chain_middle_detach_relinks_next(void **state) {
	DTTR_Core_HookCleanupAll();
	chain_reset();

	DTTR_Core_Hook *hook_a = DTTR_Core_HookAttachFunction(
		(uintptr_t)hook_target,
		0,
		chain_detour_a,
		(void **)&chain_original_a
	);
	DTTR_Core_Hook *hook_b = DTTR_Core_HookAttachFunction(
		(uintptr_t)hook_target,
		0,
		chain_detour_b,
		(void **)&chain_original_b
	);
	DTTR_Core_Hook *hook_c = DTTR_Core_HookAttachFunction(
		(uintptr_t)hook_target,
		0,
		chain_detour_c,
		(void **)&chain_original_c
	);
	assert_non_null(hook_a);
	assert_non_null(hook_b);
	assert_non_null(hook_c);

	assert_int_equal(hook_target(5), 1122);

	DTTR_Core_HookDetach(hook_b);
	chain_original_b = NULL;
	chain_call_count = 0;
	assert_int_equal(hook_target(5), 1022);
	assert_int_equal(chain_call_count, 2);
	assert_int_equal(chain_call_log[0], 3);
	assert_int_equal(chain_call_log[1], 1);

	DTTR_Core_HookDetach(hook_c);
	DTTR_Core_HookDetach(hook_a);
	assert_int_equal(hook_target(5), 12);
}

// Covers owner cleanup removing one chain link while preserving another owner's hook.
static void test_function_hook_chain_owner_detach_preserves_other_links(void **state) {
	DTTR_Core_HookCleanupAll();
	chain_reset();
	int owner_a = 0;
	int owner_b = 0;

	void *previous_owner = DTTR_Core_HookSetOwner(&owner_a);
	DTTR_Core_Hook *hook_a = DTTR_Core_HookAttachFunction(
		(uintptr_t)hook_target,
		0,
		chain_detour_a,
		(void **)&chain_original_a
	);
	DTTR_Core_HookSetOwner(&owner_b);
	DTTR_Core_Hook *hook_b = DTTR_Core_HookAttachFunction(
		(uintptr_t)hook_target,
		0,
		chain_detour_b,
		(void **)&chain_original_b
	);
	DTTR_Core_HookSetOwner(previous_owner);
	assert_non_null(hook_a);
	assert_non_null(hook_b);
	assert_int_equal(hook_target(5), 122);

	DTTR_Core_HookDetachOwner(&owner_b);
	chain_original_b = NULL;
	chain_call_count = 0;
	assert_int_equal(hook_target(5), 22);
	assert_int_equal(chain_call_count, 1);
	assert_int_equal(chain_call_log[0], 1);

	DTTR_Core_HookDetachOwner(&owner_a);
	chain_original_a = NULL;
	assert_int_equal(hook_target(5), 12);
}

// Covers guard clauses for invalid runtime hook inputs.
static void test_runtime_hooks_reject_invalid_arguments(void **state) {
	DTTR_Core_HookCleanupAll();

	const uint8_t patch = 0xAA;
	void *out_original = (void *)0x1;

	assert_null(DTTR_Core_HookAttachPointer(0, (void *)0x2, &out_original));
	assert_ptr_equal(out_original, (void *)0x1);
	assert_null(DTTR_Core_HookPatchBytes(0, &patch, sizeof(patch)));
	assert_null(DTTR_Core_HookPatchBytes((uintptr_t)patch_target, NULL, sizeof(patch)));
	assert_null(DTTR_Core_HookPatchBytes((uintptr_t)patch_target, &patch, 0));
}

// Covers cached sigscan rejecting incomplete scanner arguments.
static void test_cached_sigscan_rejects_invalid_arguments(void **state) {
	DTTR_Core_HookCleanupAll();

	assert_int_equal(DTTR_Core_HookCachedSigscan(NULL, "\x4D\x5A", "xx"), 0);
	assert_int_equal(DTTR_Core_HookCachedSigscan(GetModuleHandleA(NULL), NULL, "xx"), 0);
	assert_int_equal(
		DTTR_Core_HookCachedSigscan(GetModuleHandleA(NULL), "\x4D\x5A", NULL),
		0
	);
}

static const DTTR_TestCase TEST_CASES[] = {
	{"hook-registry-patch-bytes", test_patch_bytes_detach_restores_original},
	{"hook-registry-pointer", test_pointer_hook_detach_restores_original},
	{"hook-registry-owner-detach", test_owner_detach_only_detaches_matching_owner},
	{"hook-registry-cleanup-all", test_cleanup_all_restores_hooks_and_allows_reuse},
	{"hook-registry-overlap", test_overlapping_byte_patches_are_rejected},
	{"hook-registry-function-hook", test_function_hook_trampoline_and_detach},
	{"hook-registry-function-chain", test_function_hook_chain_calls_newest_first},
	{"hook-registry-function-chain-middle-detach",
	 test_function_hook_chain_middle_detach_relinks_next},
	{"hook-registry-function-chain-owner-detach",
	 test_function_hook_chain_owner_detach_preserves_other_links},
	{"hook-registry-invalid-arguments", test_runtime_hooks_reject_invalid_arguments},
	{"hook-registry-cached-sigscan-invalid-arguments",
	 test_cached_sigscan_rejects_invalid_arguments},
};

DTTR_TEST_MAIN(TEST_CASES)
