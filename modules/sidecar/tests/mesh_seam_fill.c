#include <cmocka.h>
#include <math.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>

#include "graphics/graphics_private.h"
#include <dttr_config.h>
#include <dttr_test_support.h>

static DTTR_Vertex vertex(float x, float y) {
	return (DTTR_Vertex){
		.x = x,
		.y = y,
		.z = 0.25f,
		.rhw = 0.75f,
		.r = 0.1f,
		.g = 0.2f,
		.b = 0.3f,
		.a = 0.4f,
		.u = x + 10.0f,
		.v = y + 20.0f,
	};
}

static void enable_logical_subpixel() {
	DTTR_Config_SetDefaults(&dttr_config);
	dttr_config.scaling_method = DTTR_SCALING_METHOD_LOGICAL;
	dttr_config.vertex_precision = DTTR_VERTEX_PRECISION_SUBPIXEL;
}

static bool should_fill(
	DTTR_PrimitiveType type,
	bool transformed,
	bool depth_test,
	bool blend_enabled
) {
	return dttr_graphics_should_fill_mesh_seams(
		type,
		transformed,
		depth_test,
		blend_enabled
	);
}

static void assert_non_position_attrs_unchanged(
	const DTTR_Vertex *actual,
	const DTTR_Vertex *expected
) {
	assert_float_equal(actual->z, expected->z, 0.00001f);
	assert_float_equal(actual->rhw, expected->rhw, 0.00001f);
	assert_float_equal(actual->r, expected->r, 0.00001f);
	assert_float_equal(actual->g, expected->g, 0.00001f);
	assert_float_equal(actual->b, expected->b, 0.00001f);
	assert_float_equal(actual->a, expected->a, 0.00001f);
	assert_float_equal(actual->u, expected->u, 0.00001f);
	assert_float_equal(actual->v, expected->v, 0.00001f);
}

static void assert_vertex_shift_below(
	const DTTR_Vertex *actual,
	const DTTR_Vertex *before,
	float max_delta
) {
	assert_true(fabsf(actual->x - before->x) < max_delta);
	assert_true(fabsf(actual->y - before->y) < max_delta);
	assert_non_position_attrs_unchanged(actual, before);
}

static void seam_fill_predicate_accepts_only_safe_draws(void **state) {
	enable_logical_subpixel();
	assert_true(should_fill(DTTR_PRIM_TRIANGLELIST, true, true, false));
	assert_false(should_fill(DTTR_PRIM_TRIANGLESTRIP, true, true, false));
	assert_false(should_fill(DTTR_PRIM_TRIANGLELIST, false, true, false));
	assert_false(should_fill(DTTR_PRIM_TRIANGLELIST, true, false, false));
	assert_false(should_fill(DTTR_PRIM_TRIANGLELIST, true, true, true));

	dttr_config.scaling_method = DTTR_SCALING_METHOD_PRESENT;
	assert_false(should_fill(DTTR_PRIM_TRIANGLELIST, true, true, false));

	dttr_config.scaling_method = DTTR_SCALING_METHOD_LOGICAL;
	dttr_config.vertex_precision = DTTR_VERTEX_PRECISION_NATIVE;
	assert_false(should_fill(DTTR_PRIM_TRIANGLELIST, true, true, false));
}

static void seam_fill_expands_triangle_by_physical_pixels(void **state) {
	DTTR_Vertex verts[3] = {vertex(0.0f, 0.0f), vertex(4.0f, 0.0f), vertex(0.0f, 4.0f)};
	DTTR_Vertex before[3];
	memcpy(before, verts, sizeof(verts));

	dttr_graphics_fill_mesh_seams(verts, 3, 640, 480, 1280, 960);

	assert_true(verts[0].x < before[0].x);
	assert_true(verts[0].y < before[0].y);
	assert_true(verts[1].x > before[1].x);
	assert_true(verts[1].y < before[1].y);
	assert_true(verts[2].x < before[2].x);
	assert_true(verts[2].y > before[2].y);

	for (int i = 0; i < 3; i++) {
		assert_vertex_shift_below(&verts[i], &before[i], 0.40f);
	}
}

static void seam_fill_expands_near_slanted_edge_corner(void **state) {
	DTTR_Vertex verts[3] = {vertex(0.0f, 0.0f), vertex(100.0f, 1.0f), vertex(0.0f, 12.0f)};
	DTTR_Vertex before[3];
	memcpy(before, verts, sizeof(verts));

	dttr_graphics_fill_mesh_seams(verts, 3, 640, 480, 1280, 960);

	assert_true(verts[0].y < before[0].y - 0.20f);
	assert_true(verts[1].y <= before[1].y);

	for (int i = 0; i < 3; i++) {
		assert_vertex_shift_below(&verts[i], &before[i], 0.40f);
	}
}

static void seam_fill_skips_degenerate_triangles(void **state) {
	DTTR_Vertex verts[3] = {vertex(2.0f, 2.0f), vertex(2.0f, 2.0f), vertex(2.0f, 2.0f)};
	DTTR_Vertex before[3];
	memcpy(before, verts, sizeof(verts));

	dttr_graphics_fill_mesh_seams(verts, 3, 640, 480, 1280, 960);

	assert_memory_equal(verts, before, sizeof(verts));
}

static const DTTR_TestCase TEST_CASES[] = {
	{"seam-fill-predicate-accepts-only-safe-draws",
	 seam_fill_predicate_accepts_only_safe_draws},
	{"seam-fill-expands-triangle-by-physical-pixels",
	 seam_fill_expands_triangle_by_physical_pixels},
	{"seam-fill-expands-near-slanted-edge-corner",
	 seam_fill_expands_near_slanted_edge_corner},
	{"seam-fill-skips-degenerate-triangles", seam_fill_skips_degenerate_triangles},
};

DTTR_TEST_MAIN(TEST_CASES)
