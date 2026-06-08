#include <dttr_test.h>

#include <string.h>

static const uint8_t PATCH_FAST_PATH[] = {0xE9, 0xBA, 0x00, 0x00, 0x00, 0x90};
static const uint8_t PATCH_NOP2[] = {0x90, 0x90};
static const uint8_t PATCH_FTOL_X[] = {0xD9, 0x1F, 0x90, 0x90, 0x90};
static const uint8_t PATCH_NOP4[] = {0x90, 0x90, 0x90, 0x90};
static const uint8_t PATCH_FTOL_Y[] = {0xD9, 0x5D, 0x00, 0x90, 0x90};
static const uint8_t PATCH_NOP3[] = {0x90, 0x90, 0x90};
static const uint8_t PATCH_RET[] = {0xC3};

static const uint8_t ORIGINAL_FAST_PATH[] = {0x0F, 0x85, 0xB9, 0x00, 0x00, 0x00};
static const uint8_t ORIGINAL_BATCH_LIMIT_A[] = {0x7D, 0x39};
static const uint8_t ORIGINAL_BATCH_LIMIT_B[] = {0x7D, 0x59};
static const uint8_t ORIGINAL_FTOL_CALL[] = {0xE8, 0, 0, 0, 0};
static const uint8_t ORIGINAL_MOV_XY[] = {0x89, 0x44, 0x24, 0x30};
static const uint8_t ORIGINAL_FSTP_X[] = {0xD9, 0x1F};
static const uint8_t ORIGINAL_FILD_X[] = {0xDB, 0x44, 0x24, 0x30};
static const uint8_t ORIGINAL_FSTP_Y[] = {0xD9, 0x5D, 0x00};
static const uint8_t ORIGINAL_FILD_Y[] = {0xDB, 0x44, 0x24, 0x34};
static const uint8_t ORIGINAL_PUSH_EBX[] = {0x53};

#define SIDECAR_JMP_HOOK_TARGET(NAME, SIG, MASK, REQUIRED)                               \
	{NAME,                                                                               \
	 TARGET_JMP_HOOK,                                                                    \
	 (const uint8_t *)(SIG),                                                             \
	 MASK,                                                                               \
	 REQUIRED,                                                                           \
	 0,                                                                                  \
	 NULL,                                                                               \
	 0,                                                                                  \
	 NULL,                                                                               \
	 NULL}
#define SIDECAR_BYTE_PATCH_TARGET(NAME, SIG, MASK, OFFSET, PATCH, ORIGINAL, ORIGINAL_MASK) \
	{                                                                                      \
		NAME,                                                                              \
		TARGET_BYTE_PATCH,                                                                 \
		(const uint8_t *)(SIG),                                                            \
		MASK,                                                                              \
		DTTR_TEST_PCDOGS_REQUIRED_ALL,                                                     \
		OFFSET,                                                                            \
		PATCH,                                                                             \
		sizeof(PATCH),                                                                     \
		ORIGINAL,                                                                          \
		ORIGINAL_MASK,                                                                     \
	}

const pcdogs_target_expectation DTTR_TEST_PCDOGS_SIDECAR_TARGETS[] = {
	SIDECAR_JMP_HOOK_TARGET(
		"dttr_hook_win_main",
		"\x83\xEC\x40\x53\x8B\x5C\x24",
		"xxxxxxx",
		DTTR_TEST_PCDOGS_REQUIRED_ALL
	),
	SIDECAR_JMP_HOOK_TARGET(
		"dttr_hook_resolve_pcdogs_path",
		"\x51\x8D\x44\x24?\x57",
		"xxxx?x",
		DTTR_TEST_PCDOGS_REQUIRED_EU_SC
	),
	SIDECAR_JMP_HOOK_TARGET(
		"dttr_inputs_hook_dinput_poll",
		"\x56\x8B\x74\x24?\x56\x8B\x06",
		"xxxx?xxx",
		DTTR_TEST_PCDOGS_REQUIRED_ALL
	),
	SIDECAR_BYTE_PATCH_TARGET(
		"dttr_hook_precision_fast_path",
		"\x83\xF8?\x7C?\xD9\x43?\xD8\x1D????\xDF\xE0\xF6\xC4\x41\x0F????",
		"xx?x?xx?xx????xxxxxx????",
		19,
		PATCH_FAST_PATH,
		ORIGINAL_FAST_PATH,
		"xxxxxx"
	),
	SIDECAR_BYTE_PATCH_TARGET(
		"dttr_hook_precision_batch_limit_a",
		"\x8B\x08\xEB?\xA1????\x8B\x0D????\x3B\xC1",
		"xxx?x????xx????xx",
		17,
		PATCH_NOP2,
		ORIGINAL_BATCH_LIMIT_A,
		"xx"
	),
	SIDECAR_BYTE_PATCH_TARGET(
		"dttr_hook_precision_batch_limit_b",
		"\x83\xC1\x14\x4E\x75?\xA1????\x8B\x0D????\x3B\xC1",
		"xxxxx?x????xx????xx",
		19,
		PATCH_NOP2,
		ORIGINAL_BATCH_LIMIT_B,
		"xx"
	),
	SIDECAR_BYTE_PATCH_TARGET(
		"dttr_hook_precision_ftol_x",
		"\xDB\x44\x24\x30\xD9\x1F",
		"xxxxxx",
		-15,
		PATCH_FTOL_X,
		ORIGINAL_FTOL_CALL,
		"x????"
	),
	SIDECAR_BYTE_PATCH_TARGET(
		"dttr_hook_precision_mov_x",
		"\xDB\x44\x24\x30\xD9\x1F",
		"xxxxxx",
		-10,
		PATCH_NOP4,
		ORIGINAL_MOV_XY,
		"xxxx"
	),
	SIDECAR_BYTE_PATCH_TARGET(
		"dttr_hook_precision_fstp2_x",
		"\x8D\xAE????\xDB\x44\x24\x30\xD9\x1F",
		"xx????xxxxxx",
		10,
		PATCH_NOP2,
		ORIGINAL_FSTP_X,
		"xx"
	),
	SIDECAR_BYTE_PATCH_TARGET(
		"dttr_hook_precision_fild_x",
		"\x8D\xAE????\xDB\x44\x24\x30",
		"xx????xxxx",
		6,
		PATCH_NOP4,
		ORIGINAL_FILD_X,
		"xxxx"
	),
	SIDECAR_BYTE_PATCH_TARGET(
		"dttr_hook_precision_ftol_y",
		"\x8B\x54\x24\x18\x89\x44\x24\x30",
		"xxxxxxxx",
		-5,
		PATCH_FTOL_Y,
		ORIGINAL_FTOL_CALL,
		"x????"
	),
	SIDECAR_BYTE_PATCH_TARGET(
		"dttr_hook_precision_mov_y",
		"\x8B\x54\x24\x18\x89\x44\x24\x30",
		"xxxxxxxx",
		4,
		PATCH_NOP4,
		ORIGINAL_MOV_XY,
		"xxxx"
	),
	SIDECAR_BYTE_PATCH_TARGET(
		"dttr_hook_precision_fstp2_y",
		"\x83\xC0\x14\x50\x55\xD9\x5D\x00",
		"xxxxxxxx",
		5,
		PATCH_NOP3,
		ORIGINAL_FSTP_Y,
		"xxx"
	),
	SIDECAR_BYTE_PATCH_TARGET(
		"dttr_hook_precision_fild_y",
		"\x52\xDB\x44\x24\x34",
		"xxxxx",
		1,
		PATCH_NOP4,
		ORIGINAL_FILD_Y,
		"xxxx"
	),
	SIDECAR_BYTE_PATCH_TARGET(
		"dttr_hook_render_quad_snap",
		"\x53\x8B\x5C\x24\x14\x55\x33\xC9\x56\x57\x85\xDB",
		"xxxxxxxxxxxx",
		0,
		PATCH_RET,
		ORIGINAL_PUSH_EBX,
		"x"
	),
};

const size_t DTTR_TEST_PCDOGS_SIDECAR_TARGET_COUNT = sizeof(
														 DTTR_TEST_PCDOGS_SIDECAR_TARGETS
													 )
													 / sizeof(
														 *DTTR_TEST_PCDOGS_SIDECAR_TARGETS
													 );

const char *const DTTR_TEST_PCDOGS_SIDECAR_MSS_IMPORT_HOOKS[] = {
	"_AIL_allocate_sample_handle@4",
	"_AIL_close_stream@4",
	"_AIL_end_sample@4",
	"_AIL_get_preference@4",
	"_AIL_init_sample@4",
	"_AIL_open_stream@12",
	"_AIL_pause_stream@8",
	"_AIL_release_sample_handle@4",
	"_AIL_sample_playback_rate@4",
	"_AIL_sample_status@4",
	"_AIL_set_digital_master_volume@8",
	"_AIL_set_preference@8",
	"_AIL_set_sample_file@12",
	"_AIL_set_sample_loop_count@8",
	"_AIL_set_sample_pan@8",
	"_AIL_set_sample_playback_rate@8",
	"_AIL_set_sample_volume@8",
	"_AIL_set_stream_loop_count@8",
	"_AIL_set_stream_volume@8",
	"_AIL_shutdown@0",
	"_AIL_start_sample@4",
	"_AIL_start_stream@4",
	"_AIL_startup@0",
	"_AIL_stop_sample@4",
	"_AIL_stream_status@4",
	"_AIL_waveOutClose@4",
	"_AIL_waveOutOpen@16",
};

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
