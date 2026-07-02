#include "gui_mod_load_path.h"

#include <dttr_test_support.h>

static void mod_dependency_dir_valid(void **state) {
	char path[64] = {0};

	assert_true(config_ui_build_mod_dependency_dir("C:\\Game\\mods", path, sizeof(path)));
	assert_string_equal(path, "C:\\Game\\modules");

	char trailing[64] = {0};
	assert_true(
		config_ui_build_mod_dependency_dir("C:\\Game\\mods\\", trailing, sizeof(trailing))
	);

	char small[8] = "stale";
	assert_false(
		config_ui_build_mod_dependency_dir("C:\\Game\\mods", small, sizeof(small))
	);
}

static void mod_dependency_dir_invalid(void **state) {
	char out[64] = "stale";

	assert_false(config_ui_build_mod_dependency_dir(NULL, out, sizeof(out)));
	assert_string_equal(out, "");
	assert_false(config_ui_build_mod_dependency_dir("", out, sizeof(out)));
	assert_false(config_ui_build_mod_dependency_dir("\\", out, sizeof(out)));
}

static const DTTR_TestCase TEST_CASES[] = {
	{"mod-dependency-dir-valid", mod_dependency_dir_valid},
	{"mod-dependency-dir-invalid", mod_dependency_dir_invalid},
};

DTTR_TEST_MAIN(TEST_CASES)
