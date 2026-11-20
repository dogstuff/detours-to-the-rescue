#define DTTR_SDK_ENABLE_UNSTABLE

#include <dttr_test_support.h>
#include <dttr_util_pkg.h>

// TOC-only visits need the native build and the resolved table, but never load game data.
static DTTR_PCDOGS_T_PKG_TOCEntry live_toc[305];
const struct dttr_pcdogs_function_accessor_PKG_LoadEntry *const
	DTTR_PCDOGS_F_PKG_LoadEntry = NULL;
const struct dttr_pcdogs_function_accessor_PKG_FreeResourceData *const
	DTTR_PCDOGS_F_PKG_FreeResourceData = NULL;

DTTR_PCDOGS_T_Build_Mask DTTR_PCDOGS_ContextBuild(const DTTR_Core_Context *ctx) {
	return mock_type(DTTR_PCDOGS_T_Build_Mask);
}

DTTR_Result DTTR_PCDOGS_DataResolve(
	const DTTR_Core_Context *ctx,
	DTTR_PCDOGS_T_Data_ID id,
	uintptr_t *out_addr
) {
	assert_int_equal(id, DTTR_PCDOGS_DATA_PKG_LOAD_ENTRY_TOC);
	*out_addr = (uintptr_t)live_toc;
	return (DTTR_Result){DTTR_OK, NULL};
}

typedef struct {
	const DTTR_PCDOGS_T_PKG_TOCEntry *entries;
	uint32_t count;
} visited_table;

static DTTR_Util_PkgVisitAction visit_entry(
	const DTTR_Util_PkgVisit *visit,
	void *userdata
) {
	visited_table *table = userdata;
	assert_int_equal(visit->kind, DTTR_UTIL_PKG_VISIT_TOC_ENTRY);
	assert_int_equal(visit->toc_index, table->count);
	assert_ptr_equal(visit->toc_entry, &table->entries[table->count]);
	++table->count;
	return DTTR_UTIL_PKG_VISIT_CONTINUE;
}

static void assert_walk(
	const DTTR_Util_PkgWalkOptions *options,
	const DTTR_PCDOGS_T_PKG_TOCEntry *entries,
	uint32_t expected_count,
	DTTR_Util_PkgVisitStatus expected_status
) {
	const DTTR_Core_Context ctx = {0};
	visited_table table = {.entries = entries};
	DTTR_Util_PkgWalkResult result = DTTR_Util_PkgWalk(&ctx, options, visit_entry, &table);
	assert_int_equal(result.status, expected_status);
	assert_int_equal(result.visited_count, expected_count);
	assert_int_equal(table.count, expected_count);
}

static void test_default_live_counts(void **state) {
	will_return(DTTR_PCDOGS_ContextBuild, DTTR_PCDOGS_BUILD_MASK_EN);
	assert_walk(NULL, live_toc, 138, DTTR_UTIL_PKG_STATUS_OK);
	will_return(DTTR_PCDOGS_ContextBuild, DTTR_PCDOGS_BUILD_MASK_EU);
	assert_walk(NULL, live_toc, 305, DTTR_UTIL_PKG_STATUS_OK);
	will_return(DTTR_PCDOGS_ContextBuild, DTTR_PCDOGS_BUILD_MASK_SC);
	assert_walk(NULL, live_toc, 235, DTTR_UTIL_PKG_STATUS_OK);
}

static void test_live_count_limit(void **state) {
	DTTR_Util_PkgWalkOptions options = DTTR_Util_PkgWalk_DefaultOptions();
	options.toc_count = 5;
	will_return(DTTR_PCDOGS_ContextBuild, DTTR_PCDOGS_BUILD_MASK_EU);
	assert_walk(&options, live_toc, 5, DTTR_UTIL_PKG_STATUS_OK);
	options.toc_count = 200;
	will_return(DTTR_PCDOGS_ContextBuild, DTTR_PCDOGS_BUILD_MASK_EN);
	assert_walk(&options, live_toc, 138, DTTR_UTIL_PKG_STATUS_OK);
}

static void test_caller_supplied_table(void **state) {
	const DTTR_PCDOGS_T_PKG_TOCEntry entries[138] = {0};
	DTTR_Util_PkgWalkOptions options = DTTR_Util_PkgWalk_DefaultOptions();
	options.toc_entries = entries;
	options.toc_count = 2;
	assert_walk(&options, entries, 2, DTTR_UTIL_PKG_STATUS_OK);
	options.toc_count = 0;
	assert_walk(&options, entries, 138, DTTR_UTIL_PKG_STATUS_OK);
}

static void test_unknown_live_build(void **state) {
	will_return(DTTR_PCDOGS_ContextBuild, DTTR_PCDOGS_BUILD_MASK_NONE);
	assert_walk(NULL, live_toc, 0, DTTR_UTIL_PKG_STATUS_UNRESOLVED_SYMBOL);
}

static const DTTR_TestCase TEST_CASES[] = {
	{"pkg-walk-live-counts", test_default_live_counts},
	{"pkg-walk-live-limit", test_live_count_limit},
	{"pkg-walk-supplied-table", test_caller_supplied_table},
	{"pkg-walk-unknown-build", test_unknown_live_build},
};

DTTR_TEST_MAIN(TEST_CASES)
