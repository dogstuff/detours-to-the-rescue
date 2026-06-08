#ifndef DTTR_TEST_H
#define DTTR_TEST_H

#define DTTR_TEST_BINARY_SUPPORT
#include <dttr_test_support.h>
#undef DTTR_TEST_BINARY_SUPPORT

#define DTTR_TEST_PCDOGS_REQUIRED_ALL                                                    \
	DTTR_TEST_FIXTURE_MASK_ALL(DTTR_TEST_PCDOGS_FIXTURE_COUNT)
#define DTTR_TEST_PCDOGS_REQUIRED_EN DTTR_TEST_FIXTURE_BIT(DTTR_TEST_PCDOGS_EN)
#define DTTR_TEST_PCDOGS_REQUIRED_EU_SC                                                  \
	(DTTR_TEST_FIXTURE_BIT(DTTR_TEST_PCDOGS_EU)                                          \
	 | DTTR_TEST_FIXTURE_BIT(DTTR_TEST_PCDOGS_SC))
#define DTTR_TEST_PCDOGS_REQUIRED_EN_EU                                                  \
	(DTTR_TEST_FIXTURE_BIT(DTTR_TEST_PCDOGS_EN)                                          \
	 | DTTR_TEST_FIXTURE_BIT(DTTR_TEST_PCDOGS_EU))
#define DTTR_TEST_PCDOGS_REQUIRED_EN_SC                                                  \
	(DTTR_TEST_FIXTURE_BIT(DTTR_TEST_PCDOGS_EN)                                          \
	 | DTTR_TEST_FIXTURE_BIT(DTTR_TEST_PCDOGS_SC))
#define DTTR_TEST_PCDOGS_SIG_NOT_FOUND DTTR_TEST_SIG_NOT_FOUND
#define DTTR_TEST_PCDOGS_IMPORT_CAP DTTR_TEST_IMPORT_CAP

#define TARGET_RESOLVE DTTR_TEST_TARGET_RESOLVE
#define TARGET_JMP_HOOK DTTR_TEST_TARGET_JMP_HOOK
#define TARGET_TRAMPOLINE_HOOK DTTR_TEST_TARGET_TRAMPOLINE_HOOK
#define TARGET_POINTER_FF25_E8_TARGET DTTR_TEST_TARGET_POINTER_FF25_E8_TARGET
#define TARGET_POINTER_U32_AT_MATCH_PLUS_2 DTTR_TEST_TARGET_POINTER_U32_AT_MATCH_PLUS_2
#define TARGET_BYTE_PATCH DTTR_TEST_TARGET_BYTE_PATCH

typedef enum {
	DTTR_TEST_PCDOGS_EN = 0,
	DTTR_TEST_PCDOGS_EU,
	DTTR_TEST_PCDOGS_SC,
	DTTR_TEST_PCDOGS_FIXTURE_COUNT,
} DTTR_TestPCDOGSFixtureID;

typedef DTTR_TestBinaryFixture DTTR_TestPCDOGSFixture;
typedef DTTR_TestPatternExpectation DTTR_TestPCDOGSSignatureExpectation;
typedef DTTR_TestTargetKind E_TargetKind;
typedef DTTR_TestTargetExpectation pcdogs_target_expectation;

extern const DTTR_TestPCDOGSFixture DTTR_TEST_PCDOGS_FIXTURES[];
extern const pcdogs_target_expectation DTTR_TEST_PCDOGS_SIDECAR_TARGETS[];
extern const size_t DTTR_TEST_PCDOGS_SIDECAR_TARGET_COUNT;
extern const char *const DTTR_TEST_PCDOGS_SIDECAR_MSS_IMPORT_HOOKS[];
extern const size_t DTTR_TEST_PCDOGS_SIDECAR_MSS_IMPORT_HOOK_COUNT;

/// Return the fixture directory used by PCDOGS signature tests.
const char *pcdogs_fixture_dir();
/// Report whether all PCDOGS binary fixtures are available for signature tests.
bool pcdogs_fixtures_available();
/// Visit each available PCDOGS fixture image for signature and ABI checks.
bool pcdogs_for_each_fixture(DTTR_TestPEFixtureVisitor visitor, void *userdata);

/// Scan a fixture image with the same signature format used by SDK symbol rows.
static inline uintptr_t pcdogs_sigscan(
	const DTTR_TestPEImage *image,
	const uint8_t *sig,
	const char *mask
) {
	return DTTR_TestPE_Sigscan(image, sig, mask);
}

/// Count signature matches in a fixture image.
static inline size_t pcdogs_sigscan_count(
	const DTTR_TestPEImage *image,
	const uint8_t *sig,
	const char *mask
) {
	return DTTR_TestPE_SigscanCount(image, sig, mask);
}

/// Collect fixture imports for tests that validate recovered sidecar targets.
static inline size_t pcdogs_collect_imports(
	const DTTR_TestPEImage *image,
	DTTR_TestImportEntry *imports,
	size_t imports_cap
) {
	return DTTR_TestPE_CollectImports(image, imports, imports_cap);
}

#endif // DTTR_TEST_H
