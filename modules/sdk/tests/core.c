#define DTTR_SDK_ENABLE_UNSTABLE

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <windows.h>

#include <dttr_core.h>
#include <dttr_pcdogs.h>
#include <dttr_runtime.h>
#include <dttr_sigscan.h>
#include <dttr_test_support.h>

static uint8_t sig_target[] = {0x55, 0x8B, 0xEC, 0x90, 0x90, 0xC3, 0x33, 0xC0};
static uint8_t sig_all_target[] = {0x90, 0x90, 0x90, 0xCC, 0x90};
static uint8_t patch_target[4];
static uint8_t group_patch_target[2];

static uintptr_t sigscan(HMODULE mod, const char *sig, const char *mask) {
	return (uintptr_t)DTTR_Sigscan_Bytes(sig_target, sizeof(sig_target), sig, mask);
}

static DTTR_Result sigscan_all(
	HMODULE mod,
	const char *sig,
	const char *mask,
	uintptr_t *out_addrs,
	size_t addrs_cap,
	size_t *out_count
) {
	if (!mod || !sig || !mask || !out_count || (!out_addrs && addrs_cap)) {
		return (DTTR_Result){DTTR_ERR_INVALID_ARGUMENT, "invalid test sigscan-all"};
	}

	*out_count = DTTR_Sigscan_BytesAll(
		sig_all_target,
		sizeof(sig_all_target),
		sig,
		mask,
		out_addrs,
		addrs_cap
	);
	if (!*out_count) {
		return (DTTR_Result){DTTR_ERR_NOT_FOUND, "test signature not found"};
	}

	return (DTTR_Result){DTTR_OK, "ok"};
}

static const DTTR_Core_API RUNTIME = {
	.sigscan = sigscan,
	.sigscan_all = sigscan_all,
	.hook_function = DTTR_Core_HookAttachFunction,
	.hook_pointer = DTTR_Core_HookAttachPointer,
	.patch_bytes = DTTR_Core_HookPatchBytes,
	.unhook = DTTR_Core_HookDetach,
	.struct_size = sizeof(DTTR_Core_API),
	.abi_version = DTTR_SDK_ABI_VERSION,
	.hook_is_active = DTTR_Core_HookIsActive,
	.unhook_checked = DTTR_Core_HookDetachChecked,
};

static DTTR_Core_Context runtime_context() {
	return (DTTR_Core_Context){
		.game_module = (HMODULE)1,
		.api = &RUNTIME,
	};
}

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

static void test_aob_find_all_returns_every_match(void **state) {
	DTTR_Core_Context ctx = runtime_context();
	uintptr_t matches[2] = {0};
	size_t count = 0;

	DTTR_Result result = DTTR_Core_AOBFindAll(
		&ctx,
		"90 90",
		matches,
		DTTR_ARRAY_COUNT(matches),
		&count
	);

	assert_true(DTTR_ResultOK(result));
	assert_int_equal(count, 2);
	assert_ptr_equal((void *)matches[0], &sig_all_target[0]);
	assert_ptr_equal((void *)matches[1], &sig_all_target[1]);
}

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
		DTTR_TEST_ARRAY_COUNT(targets),
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

static DTTR_PCDOGS_T_File_Handle *__cdecl pcdogs_file_open_detour(
	const char *filename,
	const char *mode
) {
	return NULL;
}

static void __cdecl pcdogs_cleanup_title_resources_detour() {}

static HRESULT __stdcall pcdogs_ddraw_create_ex_detour(
	DTTR_PCDOGS_T_Win32_GUID *guid,
	void **ddraw_out,
	DTTR_PCDOGS_T_Win32_GUID *iid,
	DTTR_PCDOGS_T_COM_IUnknown *outer
) {
	return S_OK;
}

static void test_pcdogs_unstable_patch_specs_match_stable_shape(void **state) {
	DTTR_PCDOGS_F_DDraw_CreateEx_proto ddraw_original = NULL;
	void *movie_key_state_original = NULL;
	void *replacement = (void *)0x33333333u;

	assert_int_equal(
		DTTR_PCDOGS_F_DDraw_CreateEx->FunctionID,
		DTTR_PCDOGS_FUNCTION_DDRAW_CREATE_EX
	);
	DTTR_PCDOGS_T_Patch_Spec ddraw = DTTR_PCDOGS_F_DDraw_CreateEx->PatchSpec(
		true,
		pcdogs_ddraw_create_ex_detour,
		&ddraw_original
	);
	assert_int_equal(ddraw.kind, DTTR_PCDOGS_PATCH_FUNCTION_HOOK);
	assert_true(ddraw.required);
	assert_int_equal(ddraw.function, DTTR_PCDOGS_FUNCTION_DDRAW_CREATE_EX);
	assert_ptr_equal(ddraw.detour, pcdogs_ddraw_create_ex_detour);
	assert_ptr_equal(ddraw.out_original, &ddraw_original);

	assert_int_equal(
		DTTR_PCDOGS_D_Video_PlayMovieLoop_GetAsyncKeyStateThunk->DataID,
		DTTR_PCDOGS_DATA_VIDEO_PLAY_MOVIE_LOOP_GET_ASYNC_KEY_STATE_THUNK
	);
	DTTR_PCDOGS_T_Patch_Spec
		movie_key_state = DTTR_PCDOGS_D_Video_PlayMovieLoop_GetAsyncKeyStateThunk
							  ->PatchSpec(false, replacement, &movie_key_state_original);
	assert_int_equal(movie_key_state.kind, DTTR_PCDOGS_PATCH_DATA_POINTER_HOOK);
	assert_false(movie_key_state.required);
	assert_int_equal(
		movie_key_state.global,
		DTTR_PCDOGS_DATA_VIDEO_PLAY_MOVIE_LOOP_GET_ASYNC_KEY_STATE_THUNK
	);
	assert_ptr_equal(movie_key_state.new_value, replacement);
	assert_ptr_equal(movie_key_state.out_original, &movie_key_state_original);
}

static void test_pcdogs_generated_function_patch_specs_name_current_hooks(void **state) {
	DTTR_PCDOGS_F_File_Open_proto file_open_original = NULL;
	DTTR_PCDOGS_F_Title_CleanupScreenResources_proto cleanup_original = NULL;

	DTTR_PCDOGS_T_Patch_Spec file_open = DTTR_PCDOGS_F_File_Open->PatchSpec(
		true,
		pcdogs_file_open_detour,
		&file_open_original
	);
	assert_int_equal(file_open.kind, DTTR_PCDOGS_PATCH_FUNCTION_HOOK);
	assert_true(file_open.required);
	assert_int_equal(file_open.function, DTTR_PCDOGS_FUNCTION_FILE_OPEN);
	assert_ptr_equal(file_open.detour, pcdogs_file_open_detour);
	assert_ptr_equal(file_open.out_original, &file_open_original);

	DTTR_PCDOGS_T_Patch_Spec cleanup = DTTR_PCDOGS_F_Title_CleanupScreenResources
										   ->PatchSpec(
											   true,
											   pcdogs_cleanup_title_resources_detour,
											   &cleanup_original
										   );
	assert_int_equal(cleanup.kind, DTTR_PCDOGS_PATCH_FUNCTION_HOOK);
	assert_true(cleanup.required);
	assert_int_equal(
		cleanup.function,
		DTTR_PCDOGS_FUNCTION_TITLE_CLEANUP_SCREEN_RESOURCES
	);
	assert_ptr_equal(cleanup.detour, pcdogs_cleanup_title_resources_detour);
	assert_ptr_equal(cleanup.out_original, &cleanup_original);
}

static void test_pcdogs_generated_title_resource_patch_specs_use_current_names(
	void **state
) {
	void *handle1_original = NULL;
	void *handle0_original = NULL;
	void *replacement1 = (void *)0x11111111u;
	void *replacement0 = (void *)0x22222222u;

	DTTR_PCDOGS_T_Patch_Spec handle1 = DTTR_PCDOGS_D_Title_ResourceHandle1->PatchSpec(
		true,
		replacement1,
		&handle1_original
	);
	assert_int_equal(handle1.kind, DTTR_PCDOGS_PATCH_DATA_POINTER_HOOK);
	assert_true(handle1.required);
	assert_int_equal(handle1.global, DTTR_PCDOGS_DATA_TITLE_RESOURCE_HANDLE1);
	assert_ptr_equal(handle1.new_value, replacement1);
	assert_ptr_equal(handle1.out_original, &handle1_original);

	DTTR_PCDOGS_T_Patch_Spec handle0 = DTTR_PCDOGS_D_Title_ResourceHandle0->PatchSpec(
		false,
		replacement0,
		&handle0_original
	);
	assert_int_equal(handle0.kind, DTTR_PCDOGS_PATCH_DATA_POINTER_HOOK);
	assert_false(handle0.required);
	assert_int_equal(handle0.global, DTTR_PCDOGS_DATA_TITLE_RESOURCE_HANDLE0);
	assert_ptr_equal(handle0.new_value, replacement0);
	assert_ptr_equal(handle0.out_original, &handle0_original);
}

static const DTTR_TestCase TEST_CASES[] = {
	{"core-sdk-signatures", test_signature_helpers_resolve_aob_patterns},
	{"core-sdk-aob-find-all", test_aob_find_all_returns_every_match},
	{"core-sdk-patch-group-rollback",
	 test_patch_group_target_failure_rolls_back_only_new_entries},
	{"pcdogs-generated-function-patch-specs",
	 test_pcdogs_generated_function_patch_specs_name_current_hooks},
	{"pcdogs-generated-title-resource-patch-specs",
	 test_pcdogs_generated_title_resource_patch_specs_use_current_names},
	{"pcdogs-unstable-patch-specs", test_pcdogs_unstable_patch_specs_match_stable_shape},
};

DTTR_TEST_MAIN(TEST_CASES)
