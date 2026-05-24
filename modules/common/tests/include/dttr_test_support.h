#ifndef DTTR_TEST_SUPPORT_H
#define DTTR_TEST_SUPPORT_H

#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>

#include <cmocka.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	const char *name;
	CMUnitTestFunction fn;
} DTTR_TestCase;

#define DTTR_TEST_ARRAY_COUNT(TESTS) (sizeof(TESTS) / sizeof(*(TESTS)))
#define DTTR_TEST_MAIN(TESTS)                                                            \
	int main(int argc, char **argv) {                                                    \
		return dttr_test_run_cases((TESTS), DTTR_TEST_ARRAY_COUNT(TESTS), argc, argv);   \
	}

// Runs one named cmocka case for focused local debugging.
static inline int dttr_test_run_case(const DTTR_TestCase *test_case) {
	if (!test_case) {
		return 2;
	}

	const struct CMUnitTest tests[] = {
		cmocka_unit_test(test_case->fn),
	};

	return cmocka_run_group_tests_name(test_case->name, tests, NULL, NULL);
}

// Skips fixture-dependent tests when the external binary corpus is not present.
static inline void dttr_test_require_available(bool available) {
	if (available) {
		return;
	}

	const char *required = getenv("DTTR_REQUIRE_PCDOGS_FIXTURES");
	if (required && required[0] && strcmp(required, "0") != 0) {
		fail_msg("required PCDOGS fixtures are unavailable");
	}

	skip();
}

// Looks up the optional command-line test name used for focused local debugging.
static inline const DTTR_TestCase *dttr_test_find_case(
	const DTTR_TestCase *test_cases,
	size_t test_case_count,
	const char *name
) {
	if (!test_cases || !name) {
		return NULL;
	}

	for (size_t i = 0; i < test_case_count; i++) {
		if (strcmp(name, test_cases[i].name) == 0) {
			return &test_cases[i];
		}
	}

	return NULL;
}

// Dispatches either the whole suite or one named case when requested directly.
static inline int dttr_test_run_cases(
	const DTTR_TestCase *test_cases,
	size_t test_case_count,
	int argc,
	char **argv
) {
	if (!test_cases || !argv || argc < 1) {
		return 2;
	}

	if (argc == 2) {
		const DTTR_TestCase *test_case = dttr_test_find_case(
			test_cases,
			test_case_count,
			argv[1]
		);
		if (test_case) {
			return dttr_test_run_case(test_case);
		}

		fprintf(stderr, "unknown test case: %s\n", argv[1]);
		return 2;
	}

	if (argc != 1) {
		fprintf(stderr, "usage: %s [test-case]\n", argv[0]);
		return 2;
	}

	int status = 0;
	for (size_t i = 0; i < test_case_count; i++) {
		const int test_status = dttr_test_run_case(&test_cases[i]);
		if (test_status != 0) {
			status = test_status;
		}
	}

	return status;
}

#endif // DTTR_TEST_SUPPORT_H

#if defined(DTTR_TEST_BINARY_SUPPORT) && !defined(DTTR_TEST_BINARY_SUPPORT_H)
#define DTTR_TEST_BINARY_SUPPORT_H

#include <stdint.h>

#include <windows.h>

#include <Zydis/Zydis.h>
#include <sds.h>

#define DTTR_TEST_SIG_NOT_FOUND UINTPTR_MAX
#define DTTR_TEST_IMPORT_CAP 128u
#define DTTR_TEST_FIXTURE_BIT(INDEX) (UINT64_C(1) << (INDEX))
#define DTTR_TEST_FIXTURE_MASK_ALL(COUNT)                                                \
	((DTTR_TestFixtureMask)(DTTR_TEST_FIXTURE_BIT(COUNT) - UINT64_C(1)))

typedef uint64_t DTTR_TestFixtureMask;

typedef struct {
	const char *id;
	const char *filename;
	size_t size;
	uint64_t xxh3;
} DTTR_TestBinaryFixture;

typedef struct {
	uint8_t *file;
	size_t file_size;
	uint8_t *image;
	size_t image_size;
	IMAGE_DATA_DIRECTORY imports_dir;
} DTTR_TestPEImage;

typedef struct {
	const char *name;
	const uint8_t *sig;
	const char *mask;
	DTTR_TestFixtureMask required;
} DTTR_TestPatternExpectation;

typedef struct {
	ZydisDecodedInstruction instruction;
	ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT];
} DTTR_TestDecodedInstruction;

typedef struct {
	const char *dll;
	const char *name;
	uintptr_t iat_site;
} DTTR_TestImportEntry;

typedef enum {
	DTTR_TEST_TARGET_RESOLVE,
	DTTR_TEST_TARGET_JMP_HOOK,
	DTTR_TEST_TARGET_TRAMPOLINE_HOOK,
	DTTR_TEST_TARGET_POINTER_FF25_E8_TARGET,
	DTTR_TEST_TARGET_POINTER_U32_AT_MATCH_PLUS_2,
	DTTR_TEST_TARGET_BYTE_PATCH,
} DTTR_TestTargetKind;

typedef struct {
	const char *name;
	DTTR_TestTargetKind kind;
	const uint8_t *sig;
	const char *mask;
	DTTR_TestFixtureMask required;
	int32_t site_offset;
	const uint8_t *patch_bytes;
	size_t patch_size;
	const uint8_t *expected_original;
	const char *expected_original_mask;
} DTTR_TestTargetExpectation;

typedef bool (*DTTR_TestPEFixtureVisitor)(
	size_t fixture_index,
	const DTTR_TestBinaryFixture *fixture,
	const char *path,
	const DTTR_TestPEImage *image,
	void *userdata
);

// Builds a fixture path that works with either slash style used by the test data.
sds dttr_test_join_path(const char *dir, const char *file);
// Validates PE offsets before test helpers read raw fixture or mapped image buffers.
bool dttr_test_range_valid(size_t offset, size_t size, size_t total);
// Applies hook-site relative offsets safely before checking bytes in a PE image.
bool dttr_test_signed_range_valid(
	uintptr_t base,
	int32_t offset,
	size_t size,
	size_t total
);
// Computes the concrete PE site address for a signed target offset.
uintptr_t dttr_test_offset_site(uintptr_t base, int32_t offset);
// Compares fixture bytes while allowing wildcard mask positions for unstable operands.
bool dttr_test_bytes_match_mask(
	const uint8_t *actual,
	const uint8_t *expected,
	const char *mask,
	size_t size
);
// Compares test case names with the fixture path UTF-8 rules.
bool dttr_test_case_equal(const char *a, const char *b);
// Tests whether a target expectation applies to the current binary fixture index.
bool dttr_test_fixture_required(DTTR_TestFixtureMask required, size_t fixture_index);

// Checks fixture availability before fixture-dependent tests decide whether to run or skip.
bool dttr_test_fixtures_available(
	const DTTR_TestBinaryFixture *fixtures,
	size_t fixture_count,
	const char *fixture_dir
);
// Loads a declared PE fixture and verifies its size and hash before tests inspect it.
bool dttr_test_pe_load_fixture(
	const DTTR_TestBinaryFixture *fixtures,
	size_t fixture_count,
	size_t fixture_index,
	const char *fixture_dir,
	sds *out_path,
	DTTR_TestPEImage *out_image
);
// Opens each declared PE fixture and passes the mapped image to a test visitor.
bool dttr_test_pe_for_each_fixture(
	const DTTR_TestBinaryFixture *fixtures,
	size_t fixture_count,
	const char *fixture_dir,
	DTTR_TestPEFixtureVisitor visitor,
	void *userdata
);
// Frees the raw and mapped buffers owned by a loaded PE fixture image.
void dttr_test_pe_free_image(DTTR_TestPEImage *image);
// Counts masked signature matches in a mapped PE fixture image.
size_t DTTR_TestPE_SigscanCount(
	const DTTR_TestPEImage *image,
	const uint8_t *sig,
	const char *mask
);
// Searches a mapped PE fixture image for a signature RVA.
uintptr_t DTTR_TestPE_Sigscan(
	const DTTR_TestPEImage *image,
	const uint8_t *sig,
	const char *mask
);
// Computes the raw fixture hash used to catch stale or mismatched test binaries.
uint64_t dttr_test_pe_file_hash(const DTTR_TestPEImage *image);
// Confirms the loaded fixture still matches the expected byte size and XXH3 hash.
bool dttr_test_pe_fixture_hash_matches(
	const DTTR_TestBinaryFixture *fixture,
	const DTTR_TestPEImage *image
);
// Returns a PE image string only when the RVA points to a NUL-terminated span.
const char *dttr_test_pe_cstr(const DTTR_TestPEImage *image, uintptr_t rva);
// Collects import names and IAT RVAs so hook tests can verify imported targets.
size_t DTTR_TestPE_CollectImports(
	const DTTR_TestPEImage *image,
	DTTR_TestImportEntry *imports,
	size_t imports_cap
);

// Decodes a 32-bit x86 instruction from fixture bytes for target validation.
bool dttr_test_zydis_decode32(
	const uint8_t *bytes,
	size_t size,
	DTTR_TestDecodedInstruction *out
);
// Decodes a 32-bit instruction at a mapped PE RVA for hook-site checks.
bool dttr_test_zydis_decode32_at(
	const DTTR_TestPEImage *image,
	uintptr_t rva,
	DTTR_TestDecodedInstruction *out
);
// Measures whole instructions covering a patch prefix.
bool dttr_test_zydis_decode32_prefix(
	const DTTR_TestPEImage *image,
	uintptr_t rva,
	size_t required_size,
	size_t *out_size
);
// Resolves an instruction operand to an absolute pointer-pattern target.
bool dttr_test_zydis_absolute_operand(
	const DTTR_TestDecodedInstruction *decoded,
	size_t operand_index,
	uintptr_t runtime_address,
	uintptr_t *out_address
);
// Verifies that a fixture signature satisfies the expected hook kind.
void dttr_test_assert_target_resolved(
	const DTTR_TestBinaryFixture *fixture,
	const DTTR_TestTargetExpectation *target,
	const DTTR_TestPEImage *image
);

#endif // DTTR_TEST_BINARY_SUPPORT_H
