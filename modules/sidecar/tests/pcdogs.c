#include <dttr_test.h>

#include <string.h>

#include <sidecar_hook_sigs.h>

// Expands one shared byte-patch row into a test expectation. The signature, offset, and patch
// bytes come from the same sidecar_graphics_byte_patches.def the runtime installs; only the
// fixture-required mask and expected-original bytes are test-specific.
#define SIDECAR_GFX_BYTE_PATCH(                                                           \
	name,                                                                                \
	rt_required,                                                                          \
	test_required,                                                                        \
	aob,                                                                                  \
	offset,                                                                               \
	patch_seq,                                                                            \
	original_seq,                                                                         \
	original_mask                                                                         \
)                                                                                        \
	{name,                                                                               \
	 TARGET_BYTE_PATCH,                                                                  \
	 aob,                                                                                \
	 test_required,                                                                      \
	 offset,                                                                             \
	 (const uint8_t[]){DTTR_SIDECAR_UNPAREN patch_seq},                                  \
	 sizeof((const uint8_t[]){DTTR_SIDECAR_UNPAREN patch_seq}),                          \
	 (const uint8_t[]){DTTR_SIDECAR_UNPAREN original_seq},                               \
	 original_mask},

// JMP-hook targets the sidecar installs from inline AOB signatures, sourced from the same
// shared macros the runtime uses (sidecar_hook_sigs.h). The Input_ReadGamepad hook is an SDK
// blueprint symbol (DTTR_PCDOGS_F_Input_ReadGamepad); its signature is owned and asserted by
// the SDK blueprint-function suite, so it is intentionally not duplicated here. Byte-patch
// targets follow, expanded from sidecar_graphics_byte_patches.def.
const pcdogs_target_expectation DTTR_TEST_PCDOGS_SIDECAR_TARGETS[] = {
	{"dttr_hook_win_main",
	 TARGET_JMP_HOOK,
	 DTTR_SIDECAR_AOB_WIN_MAIN,
	 DTTR_TEST_PCDOGS_REQUIRED_ALL,
	 0,
	 NULL,
	 0,
	 NULL,
	 NULL},
	{"dttr_hook_resolve_pcdogs_path",
	 TARGET_JMP_HOOK,
	 DTTR_SIDECAR_AOB_RESOLVE_PCDOGS_PATH,
	 DTTR_TEST_PCDOGS_REQUIRED_EU_SC,
	 0,
	 NULL,
	 0,
	 NULL,
	 NULL},
	{"dttr_inputs_hook_dinput_poll",
	 TARGET_JMP_HOOK,
	 DTTR_SIDECAR_AOB_DINPUT_POLL,
	 DTTR_TEST_PCDOGS_REQUIRED_ALL,
	 0,
	 NULL,
	 0,
	 NULL,
	 NULL},
#include <sidecar_graphics_byte_patches.def>
};

#undef SIDECAR_GFX_BYTE_PATCH

const size_t DTTR_TEST_PCDOGS_SIDECAR_TARGET_COUNT = sizeof(
														 DTTR_TEST_PCDOGS_SIDECAR_TARGETS
													 )
													 / sizeof(
														 *DTTR_TEST_PCDOGS_SIDECAR_TARGETS
													 );

// Expanded from the same sidecar_mss_imports.def the runtime installs, so the expected import
// set never drifts from the hooked set.
#define SIDECAR_MSS_IMPORT(hook_name, import_name, callback) import_name,

const char *const DTTR_TEST_PCDOGS_SIDECAR_MSS_IMPORT_HOOKS[] = {
#include <sidecar_mss_imports.def>
};
#undef SIDECAR_MSS_IMPORT

const size_t DTTR_TEST_PCDOGS_SIDECAR_MSS_IMPORT_HOOK_COUNT
	= sizeof(DTTR_TEST_PCDOGS_SIDECAR_MSS_IMPORT_HOOKS)
	  / sizeof(*DTTR_TEST_PCDOGS_SIDECAR_MSS_IMPORT_HOOKS);

typedef struct {
	E_TargetKind kind;
} target_filter;

// Checks sidecar target signatures against every available PCDogs fixture.
static bool assert_targets_for_fixture(
	size_t fixture_index,
	const DTTR_TestBinaryFixture *fixture,
	const char *path,
	const DTTR_TestPEImage *image,
	void *userdata
) {
	const target_filter *filter = userdata;

	for (size_t target_index = 0; target_index < DTTR_TEST_PCDOGS_SIDECAR_TARGET_COUNT;
		 target_index++) {
		const pcdogs_target_expectation
			*target = &DTTR_TEST_PCDOGS_SIDECAR_TARGETS[target_index];

		if (target->kind != filter->kind
			|| !dttr_test_fixture_required(target->required, fixture_index)) {
			continue;
		}

		dttr_test_assert_target_resolved(fixture, target, image);
	}

	return true;
}

// Runs one target-kind fixture pass.
static void test_targets_matching(E_TargetKind kind) {
	target_filter filter = {.kind = kind};
	assert_true(pcdogs_for_each_fixture(assert_targets_for_fixture, &filter));
}

// Verifies expected PCDogs jump-hook targets.
static void test_expected_pcdogs_jmp_hook_targets_resolve(void **state) {
	dttr_test_require_available(pcdogs_fixtures_available());
	test_targets_matching(TARGET_JMP_HOOK);
}

// Verifies expected PCDogs byte-patch targets.
static void test_expected_pcdogs_byte_patch_targets_resolve(void **state) {
	dttr_test_require_available(pcdogs_fixtures_available());
	test_targets_matching(TARGET_BYTE_PATCH);
}

// Checks whether an mss32.dll import is expected to be hooked.
static bool import_hook_expected(const char *name) {
	for (size_t i = 0; i < DTTR_TEST_PCDOGS_SIDECAR_MSS_IMPORT_HOOK_COUNT; i++) {
		if (strcmp(name, DTTR_TEST_PCDOGS_SIDECAR_MSS_IMPORT_HOOKS[i]) == 0) {
			return true;
		}
	}

	return false;
}

// Verifies each fixture exposes exactly the mss32.dll imports hooked by the sidecar.
static bool assert_imports_for_fixture(
	size_t fixture_index,
	const DTTR_TestBinaryFixture *fixture,
	const char *path,
	const DTTR_TestPEImage *image,
	void *userdata
) {
	DTTR_TestImportEntry imports[DTTR_TEST_PCDOGS_IMPORT_CAP] = {0};
	const size_t import_count = pcdogs_collect_imports(
		image,
		imports,
		DTTR_TEST_PCDOGS_IMPORT_CAP
	);

	size_t mss32_count = 0;

	for (size_t i = 0; i < import_count; i++) {
		const DTTR_TestImportEntry *entry = &imports[i];

		if (!dttr_test_case_equal(entry->dll, "mss32.dll")) {
			continue;
		}

		mss32_count++;

		if (!import_hook_expected(entry->name)) {
			fail_msg(
				"unhandled MSS32 import in %s (%s): %s",
				fixture->id,
				fixture->filename,
				entry->name
			);
		}

		assert_true(entry->iat_site != 0);
	}

	assert_int_equal(mss32_count, DTTR_TEST_PCDOGS_SIDECAR_MSS_IMPORT_HOOK_COUNT);
	return true;
}

// Verifies expected mss32.dll imports are hooked.
static void test_expected_mss32_imports_are_hooked(void **state) {
	dttr_test_require_available(pcdogs_fixtures_available());
	assert_true(pcdogs_for_each_fixture(assert_imports_for_fixture, NULL));
}

static const DTTR_TestCase TEST_CASES[] = {
	{"jmp-hooks", test_expected_pcdogs_jmp_hook_targets_resolve},
	{"byte-patches", test_expected_pcdogs_byte_patch_targets_resolve},
	{"mss32-imports", test_expected_mss32_imports_are_hooked},
};

DTTR_TEST_MAIN(TEST_CASES)
