#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include "graphics/resize_private.h"

static void newest_pending_size_wins(void **) {
	DTTR_WindowResizePending pending = {0};
	int width = 0;
	int height = 0;

	dttr_window_resize_record(&pending, 800, 600);
	dttr_window_resize_record(&pending, 1280, 720);

	assert_true(dttr_window_resize_take(&pending, &width, &height));
	assert_int_equal(width, 1280);
	assert_int_equal(height, 720);
}

static void pending_size_drains_once(void **) {
	DTTR_WindowResizePending pending = {0};
	int width = 0;
	int height = 0;

	dttr_window_resize_record(&pending, 800, 600);

	assert_true(dttr_window_resize_take(&pending, &width, &height));
	assert_false(dttr_window_resize_take(&pending, &width, &height));
}

static void invalid_size_is_ignored_and_later_valid_size_retries(void **) {
	DTTR_WindowResizePending pending = {0};
	int width = 0;
	int height = 0;

	dttr_window_resize_record(&pending, 63, 720);
	assert_false(dttr_window_resize_take(&pending, &width, &height));

	dttr_window_resize_record(&pending, 1024, 768);
	assert_true(dttr_window_resize_take(&pending, &width, &height));
	assert_int_equal(width, 1024);
	assert_int_equal(height, 768);
}

static void logical_letterbox_uses_supplied_pixel_size(void **) {
	const DTTR_Size size = dttr_select_render_resolution(
		DTTR_SCALING_METHOD_LOGICAL,
		DTTR_SCALING_MODE_LETTERBOX,
		(DTTR_Size){640, 480},
		(DTTR_Size){640, 480},
		(DTTR_Size){1280, 720}
	);

	assert_int_equal(size.width, 960);
	assert_int_equal(size.height, 720);
}

static void logical_stretch_uses_supplied_pixel_size(void **) {
	const DTTR_Size size = dttr_select_render_resolution(
		DTTR_SCALING_METHOD_LOGICAL,
		DTTR_SCALING_MODE_STRETCH,
		(DTTR_Size){640, 480},
		(DTTR_Size){640, 480},
		(DTTR_Size){1280, 720}
	);

	assert_int_equal(size.width, 1280);
	assert_int_equal(size.height, 720);
}

static void configured_window_size_remains_minimum(void **) {
	const DTTR_Size size = dttr_select_render_resolution(
		DTTR_SCALING_METHOD_LOGICAL,
		DTTR_SCALING_MODE_STRETCH,
		(DTTR_Size){640, 480},
		(DTTR_Size){1024, 768},
		(DTTR_Size){800, 600}
	);

	assert_int_equal(size.width, 1024);
	assert_int_equal(size.height, 768);
}

static void present_scaling_keeps_logical_resolution(void **) {
	const DTTR_Size size = dttr_select_render_resolution(
		DTTR_SCALING_METHOD_PRESENT,
		DTTR_SCALING_MODE_STRETCH,
		(DTTR_Size){800, 600},
		(DTTR_Size){640, 480},
		(DTTR_Size){1920, 1080}
	);

	assert_int_equal(size.width, 800);
	assert_int_equal(size.height, 600);
}

static void present_scaling_clamps_only_dimensions_below_minimum(void **) {
	const DTTR_Size size = dttr_select_render_resolution(
		DTTR_SCALING_METHOD_PRESENT,
		DTTR_SCALING_MODE_STRETCH,
		(DTTR_Size){63, 64},
		(DTTR_Size){640, 480},
		(DTTR_Size){1920, 1080}
	);

	assert_int_equal(size.width, WINDOW_WIDTH);
	assert_int_equal(size.height, 64);
}

static void logical_letterbox_clamps_computed_dimensions_below_minimum(void **) {
	const DTTR_Size size = dttr_select_render_resolution(
		DTTR_SCALING_METHOD_LOGICAL,
		DTTR_SCALING_MODE_LETTERBOX,
		(DTTR_Size){640, 480},
		(DTTR_Size){63, 63},
		(DTTR_Size){0, 0}
	);

	assert_int_equal(size.width, WINDOW_WIDTH);
	assert_int_equal(size.height, WINDOW_HEIGHT);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(newest_pending_size_wins),
		cmocka_unit_test(pending_size_drains_once),
		cmocka_unit_test(invalid_size_is_ignored_and_later_valid_size_retries),
		cmocka_unit_test(logical_letterbox_uses_supplied_pixel_size),
		cmocka_unit_test(logical_stretch_uses_supplied_pixel_size),
		cmocka_unit_test(configured_window_size_remains_minimum),
		cmocka_unit_test(present_scaling_keeps_logical_resolution),
		cmocka_unit_test(present_scaling_clamps_only_dimensions_below_minimum),
		cmocka_unit_test(logical_letterbox_clamps_computed_dimensions_below_minimum),
	};

	return cmocka_run_group_tests(tests, NULL, NULL);
}
