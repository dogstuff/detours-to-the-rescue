#include <dttr_test.h>

#include <stdlib.h>

const DTTR_TestPCDOGSFixture DTTR_TEST_PCDOGS_FIXTURES[] = {
	[DTTR_TEST_PCDOGS_EN] = {
		.id = "en",
		.filename = "pcdogs_en.exe",
		.size = 438272,
		.xxh3 = UINT64_C(0x78b1f7ebc13a1428),
	},
	[DTTR_TEST_PCDOGS_EU] = {
		.id = "eu",
		.filename = "pcdogs_eu.exe",
		.size = 446464,
		.xxh3 = UINT64_C(0xb7addc38f431ab95),
	},
	[DTTR_TEST_PCDOGS_SC] = {
		.id = "sc",
		.filename = "pcdogs_sc.exe",
		.size = 446464,
		.xxh3 = UINT64_C(0xa4034d351b713639),
	},
};

// Return the build-configured PCDOGS fixture directory for signature tests.
const char *pcdogs_fixture_dir() {
	const char *const dir = getenv("DTTR_PCDOGS_FIXTURE_DIR");
	return (dir && dir[0]) ? dir : "fixture";
}

// Check every expected PCDOGS fixture before enabling binary-backed tests.
bool pcdogs_fixtures_available() {
	return dttr_test_fixtures_available(
		DTTR_TEST_PCDOGS_FIXTURES,
		DTTR_TEST_PCDOGS_FIXTURE_COUNT,
		pcdogs_fixture_dir()
	);
}

// Load each PCDOGS fixture image and pass it to the requested visitor.
bool pcdogs_for_each_fixture(DTTR_TestPEFixtureVisitor visitor, void *userdata) {
	return dttr_test_pe_for_each_fixture(
		DTTR_TEST_PCDOGS_FIXTURES,
		DTTR_TEST_PCDOGS_FIXTURE_COUNT,
		pcdogs_fixture_dir(),
		visitor,
		userdata
	);
}
