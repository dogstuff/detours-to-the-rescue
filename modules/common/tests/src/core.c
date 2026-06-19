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

static void resolve_relative_normalizes_segments_with_windows_separators(void **state) {
	sds resolved = DTTR_Path_ResolveRelativeTo("C:\\base\\dir", "nested/../file.txt");
	assert_non_null(resolved);
	assert_string_equal(resolved, "C:\\base\\dir\\file.txt");
	sdsfree(resolved);
}

static void resolve_relative_without_base_stays_relative(void **state) {
	sds resolved = DTTR_Path_ResolveRelativeTo(NULL, ".\\nested\\..\\file.txt");
	assert_non_null(resolved);
	assert_string_equal(resolved, "file.txt");
	sdsfree(resolved);
}

static void native_root_preserves_share_root(void **state) {
	const char *rest = NULL;
	sds root = DTTR_Path_NativeRoot("\\\\server\\share\\folder\\file.txt", &rest);
	assert_non_null(root);
	assert_string_equal(root, "\\\\server\\share\\");
	assert_string_equal(rest, "folder\\file.txt");
	sdsfree(root);
}

static void append_segment_normalizes_joined_path(void **state) {
	sds path = sdsnew("C:\\base\\dir");
	assert_non_null(path);
	assert_true(DTTR_Path_AppendSegment(&path, ".\\nested\\..\\file.txt", '\\'));
	assert_string_equal(path, "C:\\base\\dir\\file.txt");
	sdsfree(path);
}

static void normalized_match_resolves_equivalent_segments(void **state) {
	assert_true(DTTR_Path_MatchesNormalized(".\\data\\..\\save.dat", "save.dat"));
	assert_true(DTTR_Path_MatchesNormalized("data//save.dat", "data\\save.dat"));
}

static const DTTR_TestCase TEST_CASES[] = {
	{"safe-relative-rejects-absolute-and-traversal-paths",
	 safe_relative_rejects_absolute_and_traversal_paths},
	{"resolve-relative-normalizes-segments-with-windows-separators",
	 resolve_relative_normalizes_segments_with_windows_separators},
	{"resolve-relative-without-base-stays-relative",
	 resolve_relative_without_base_stays_relative},
	{"native-root-preserves-unc-share-root", native_root_preserves_share_root},
	{"append-segment-normalizes-joined-path", append_segment_normalizes_joined_path},
	{"normalized-match-resolves-equivalent-segments",
	 normalized_match_resolves_equivalent_segments},
};

DTTR_TEST_MAIN(TEST_CASES)
