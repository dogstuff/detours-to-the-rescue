#include <dttr_path.h>
#include <dttr_test_support.h>

static void safe_relative_rejects_absolute_and_traversal_paths(void **state) {
	assert_true(DTTR_Path_IsSafeRelative("data/save.dat"));
	assert_true(DTTR_Path_IsSafeRelative("data\\save.dat"));
	assert_false(DTTR_Path_IsSafeRelative(NULL));
	assert_false(DTTR_Path_IsSafeRelative(""));
	assert_false(DTTR_Path_IsSafeRelative("/data/save.dat"));
	assert_false(DTTR_Path_IsSafeRelative("\\data\\save.dat"));
	assert_false(DTTR_Path_IsSafeRelative("C:\\data\\save.dat"));
	assert_false(DTTR_Path_IsSafeRelative("C:data\\save.dat"));
	assert_false(DTTR_Path_IsSafeRelative("data/../save.dat"));
	assert_false(DTTR_Path_IsSafeRelative("./save.dat"));
}

static const DTTR_TestCase TEST_CASES[] = {
	{"safe-relative-rejects-absolute-and-traversal-paths",
	 safe_relative_rejects_absolute_and_traversal_paths},
};

DTTR_TEST_MAIN(TEST_CASES)
