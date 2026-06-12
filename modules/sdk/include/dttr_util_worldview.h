/// Helpers for extracting the camera frame and projecting world geometry to
/// draw space with near-plane clipping and sub-pixel output.
///
/// This header is exposed through dttr_sdk.h only when DTTR_SDK_ENABLE_UNSTABLE
/// is set. It depends on PCDOGS layouts that are still being mapped, so source
/// and ABI details may change without notice.
///
/// World positions are int32 fixed point at 4096 units per logical unit,
/// and draw coordinates are in the game's logical resolution.
/// Graphics_ListState stores eye/target as Math_Vec3I32XZY, whose .z field
/// holds logical Y and whose .y field holds logical Z.

#ifndef DTTR_UTIL_WORLDVIEW_H
#define DTTR_UTIL_WORLDVIEW_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <dttr_core.h>
#ifndef DTTR_SDK_ENABLE_UNSTABLE
#error "Define DTTR_SDK_ENABLE_UNSTABLE before including dttr_util_worldview.h"
#endif
#include <dttr_pcdogs.h>
#include <dttr_util_mem.h>

#ifdef __cplusplus
extern "C" {
#endif

/// One logical unit spans this many world fixed-point units.
#define DTTR_UTIL_WORLDVIEW_FIXED_ONE 4096

/// Polygon projection emits at most this many screen vertices. A quad
/// clipped against one plane gains at most one vertex, so 8 leaves headroom.
#define DTTR_UTIL_WORLDVIEW_MAX_POLYGON_POINTS 8u

/// Cached camera frame in screen-draw space. Build it once per frame with
/// DTTR_Util_WorldView_Refresh and reuse it for each projection.
typedef struct DTTR_Util_WorldView {
	bool valid;

	/// The camera eye is logical world XYZ, already converted from XZY storage.
	DTTR_PCDOGS_T_Math_Vec3I32 eye;

	/// The view basis vectors (Q12).
	DTTR_PCDOGS_T_Math_Vec3I16 forward;
	DTTR_PCDOGS_T_Math_Vec3I16 right;
	DTTR_PCDOGS_T_Math_Vec3I16 up;

	/// The near plane in world fixed point.
	int32_t near_fp;

	/// The screen center and per-axis focal lengths are in draw-space pixels.
	DTTR_PCDOGS_T_Math_Vec2F center;
	DTTR_PCDOGS_T_Math_Vec2F focal;
} DTTR_Util_WorldView;

static inline uint32_t dttr_util_worldview_isqrt(uint64_t value) {
	uint64_t result = 0;
	uint64_t bit = (uint64_t)1 << 62;

	while (bit > value) {
		bit >>= 2;
	}

	while (bit != 0) {
		if (value >= result + bit) {
			value -= result + bit;
			result = (result >> 1) + bit;
		} else {
			result >>= 1;
		}

		bit >>= 2;
	}

	return (uint32_t)result;
}

static inline int16_t dttr_util_worldview_norm_q12(int64_t component, uint32_t length) {
	return (int16_t)((component * DTTR_UTIL_WORLDVIEW_FIXED_ONE) / (int64_t)length);
}

/// Rebuild the camera frame from live Graphics_ListState data for a draw target.
///
/// The basis is built from the eye and target positions because view_matrix
/// does not hold a plain basis, with the right vector flipped horizontally
/// and up = forward * right. The focal length is rescaled from the camera's
/// reference resolution to the draw rectangle, given in draw-space pixels.
static inline bool DTTR_Util_WorldView_Refresh(
	DTTR_Util_WorldView *view,
	const DTTR_PCDOGS_T_Math_RectI32 *target
) {
	if (!view) {
		return false;
	}

	view->valid = false;
	if (!target || target->w == 0 || target->h == 0
		|| !DTTR_PCDOGS_D_Graphics_AdjustLevelScale_ListState
		|| !DTTR_PCDOGS_D_Graphics_AdjustLevelScale_ListState->Read) {
		return false;
	}

	DTTR_PCDOGS_T_Graphics_ListState *list_state = NULL;
	if (!DTTR_StatusOK(
			DTTR_PCDOGS_D_Graphics_AdjustLevelScale_ListState->Read(&list_state).status
		)
		|| !list_state || !DTTR_Util_MemReadable(list_state, sizeof(*list_state))) {
		return false;
	}

	const DTTR_PCDOGS_T_Math_Vec3I32 eye = {
		.x = list_state->eye_pos.x,
		.y = list_state->eye_pos.z,
		.z = list_state->eye_pos.y,
	};

	const DTTR_PCDOGS_T_Math_Vec3I32 look_at = {
		.x = list_state->target_pos.x,
		.y = list_state->target_pos.z,
		.z = list_state->target_pos.y,
	};

	const int64_t fx_raw = (int64_t)look_at.x - eye.x;
	const int64_t fy_raw = (int64_t)look_at.y - eye.y;
	const int64_t fz_raw = (int64_t)look_at.z - eye.z;
	const uint64_t fx2 = (uint64_t)(fx_raw * fx_raw);
	const uint64_t fy2 = (uint64_t)(fy_raw * fy_raw);
	const uint64_t fz2 = (uint64_t)(fz_raw * fz_raw);
	const uint32_t f_len = dttr_util_worldview_isqrt(fx2 + fy2 + fz2);
	const uint32_t h_len = dttr_util_worldview_isqrt(fx2 + fz2);

	if (f_len == 0 || h_len == 0) {
		return false;
	}

	const DTTR_PCDOGS_T_Math_Vec3I16 forward = {
		.x = dttr_util_worldview_norm_q12(fx_raw, f_len),
		.y = dttr_util_worldview_norm_q12(fy_raw, f_len),
		.z = dttr_util_worldview_norm_q12(fz_raw, f_len),
	};

	// The right vector stays horizontal, flipped for the game's handedness.
	const DTTR_PCDOGS_T_Math_Vec3I16 right = {
		.x = (int16_t)-dttr_util_worldview_norm_q12(fz_raw, h_len),
		.y = 0,
		.z = dttr_util_worldview_norm_q12(fx_raw, h_len),
	};

	const DTTR_PCDOGS_T_Math_Vec3I16 up = {
		.x = (int16_t)(((int32_t)forward.y * right.z) / DTTR_UTIL_WORLDVIEW_FIXED_ONE),
		.y = (int16_t)(((int32_t)forward.z * right.x - (int32_t)forward.x * right.z)
					   / DTTR_UTIL_WORLDVIEW_FIXED_ONE),
		.z = (int16_t)(-((int32_t)forward.y * right.x) / DTTR_UTIL_WORLDVIEW_FIXED_ONE),
	};
	int32_t near_fp = list_state->projection_near_fp;
	if (near_fp < DTTR_UTIL_WORLDVIEW_FIXED_ONE / 4
		|| near_fp > DTTR_UTIL_WORLDVIEW_FIXED_ONE * 64) {
		near_fp = DTTR_UTIL_WORLDVIEW_FIXED_ONE / 4;
	}

	int32_t focal = list_state->focal_distance;
	if (focal <= 0 || focal > DTTR_UTIL_WORLDVIEW_FIXED_ONE * 4096) {
		focal = DTTR_UTIL_WORLDVIEW_FIXED_ONE * 240;
	}

	// Rescale focal length from reference resolution to draw rectangle, per axis.
	double focal_x = (double)focal;
	double focal_y = (double)focal;
	const int32_t cam_ref_w = list_state->screen_half.width;
	const int32_t cam_ref_h = list_state->screen_half.height;
	if (cam_ref_w >= 64 && cam_ref_w <= 4096) {
		focal_x = (double)focal * (double)target->w / (double)cam_ref_w;
	}

	if (cam_ref_h >= 64 && cam_ref_h <= 4096) {
		focal_y = (double)focal * (double)target->h / (double)cam_ref_h;
	}

	*view = (DTTR_Util_WorldView){
		.valid = true,
		.eye = eye,
		.forward = forward,
		.right = right,
		.up = up,
		.near_fp = near_fp,
		.center = {
			.x = (float)target->x + (float)target->w * 0.5f,
			.y = (float)target->y + (float)target->h * 0.5f,
		},
		.focal = {
			.x = (float)focal_x,
			.y = (float)focal_y,
		}
	};

	return true;
}

/// Transform a world fixed-point point into view space, where z extends into
/// the screen.
static inline bool DTTR_Util_WorldView_ToView(
	const DTTR_Util_WorldView *view,
	const DTTR_PCDOGS_T_Math_Vec3I32 *world,
	DTTR_PCDOGS_T_Math_Vec3I32 *out
) {
	if (!view || !view->valid || !world || !out) {
		return false;
	}

	const int64_t dx = (int64_t)world->x - view->eye.x;
	const int64_t dy = (int64_t)world->y - view->eye.y;
	const int64_t dz = (int64_t)world->z - view->eye.z;

	out->x = (int32_t)((dx * view->right.x + dy * view->right.y + dz * view->right.z)
					   / DTTR_UTIL_WORLDVIEW_FIXED_ONE);
	out->y = (int32_t)((dx * view->up.x + dy * view->up.y + dz * view->up.z)
					   / DTTR_UTIL_WORLDVIEW_FIXED_ONE);
	out->z = (int32_t)((dx * view->forward.x + dy * view->forward.y
						+ dz * view->forward.z)
					   / DTTR_UTIL_WORLDVIEW_FIXED_ONE);

	return true;
}

/// Perspective-divide a view-space point to draw-space coordinates with
/// sub-pixel precision. The call fails for points at or behind the camera
/// plane.
static inline bool DTTR_Util_WorldView_ViewToScreen(
	const DTTR_Util_WorldView *view,
	const DTTR_PCDOGS_T_Math_Vec3I32 *p,
	DTTR_PCDOGS_T_Math_Vec2F *out
) {
	if (!view || !view->valid || !p || !out || p->z <= 0) {
		return false;
	}

	const double max_screen_coord = 1.0e9;
	// Scale the depth to match the focal length's Q12 units.
	const double depth = (double)p->z * DTTR_UTIL_WORLDVIEW_FIXED_ONE;
	const double inv_z = 1.0 / depth;
	const double sx = view->center.x + (double)p->x * view->focal.x * inv_z;
	const double sy = view->center.y + (double)p->y * view->focal.y * inv_z;
	if (sx < -max_screen_coord || sx > max_screen_coord || sy < -max_screen_coord
		|| sy > max_screen_coord) {
		return false;
	}

	out->x = (float)sx;
	out->y = (float)sy;
	return true;
}

/// Project a single world point to draw space, failing when the point is
/// behind the near plane.
static inline bool DTTR_Util_WorldView_ProjectPoint(
	const DTTR_Util_WorldView *view,
	const DTTR_PCDOGS_T_Math_Vec3I32 *world,
	DTTR_PCDOGS_T_Math_Vec2F *out,
	float *out_view_z
) {
	DTTR_PCDOGS_T_Math_Vec3I32 p;
	if (!DTTR_Util_WorldView_ToView(view, world, &p) || p.z <= view->near_fp
		|| !DTTR_Util_WorldView_ViewToScreen(view, &p, out)) {
		return false;
	}

	if (out_view_z) {
		*out_view_z = (float)p.z;
	}

	return true;
}

// Interpolate along segment a-b to the near-plane crossing.
static inline DTTR_PCDOGS_T_Math_Vec3I32 dttr_util_worldview_clip_segment(
	const DTTR_PCDOGS_T_Math_Vec3I32 *a,
	const DTTR_PCDOGS_T_Math_Vec3I32 *b,
	int32_t near_z
) {
	const int64_t dz = (int64_t)b->z - a->z;
	if (dz == 0) {
		return *a;
	}

	const int64_t t_num = (int64_t)near_z - a->z;
	return (DTTR_PCDOGS_T_Math_Vec3I32){
		.x = (int32_t)(a->x + (((int64_t)b->x - a->x) * t_num) / dz),
		.y = (int32_t)(a->y + (((int64_t)b->y - a->y) * t_num) / dz),
		.z = near_z,
	};
}

/// Project a convex world polygon to draw space, clipping it against the near
/// plane so camera-crossing geometry still draws its visible part. Offscreen
/// extent is left to the rasterizer.
static inline uint32_t DTTR_Util_WorldView_ProjectPolygon(
	const DTTR_Util_WorldView *view,
	const DTTR_PCDOGS_T_Math_Vec3I32 *world,
	uint32_t world_count,
	DTTR_PCDOGS_T_Math_Vec2F *out_points,
	float *out_view_z
) {
	if (!view || !view->valid || !world || !out_points || world_count < 3u
		|| world_count >= DTTR_UTIL_WORLDVIEW_MAX_POLYGON_POINTS) {
		return 0;
	}

	DTTR_PCDOGS_T_Math_Vec3I32 in[DTTR_UTIL_WORLDVIEW_MAX_POLYGON_POINTS];
	for (uint32_t i = 0; i < world_count; ++i) {
		if (!DTTR_Util_WorldView_ToView(view, &world[i], &in[i])) {
			return 0;
		}
	}

	const int32_t near_z = view->near_fp + 1;
	DTTR_PCDOGS_T_Math_Vec3I32 clipped[DTTR_UTIL_WORLDVIEW_MAX_POLYGON_POINTS];
	uint32_t clipped_count = 0;
	for (uint32_t i = 0; i < world_count; ++i) {
		const DTTR_PCDOGS_T_Math_Vec3I32 *a = &in[i];
		const DTTR_PCDOGS_T_Math_Vec3I32 *b = &in[(i + 1u) % world_count];
		const bool a_in = a->z >= near_z;
		const bool b_in = b->z >= near_z;
		if (a_in && clipped_count < DTTR_UTIL_WORLDVIEW_MAX_POLYGON_POINTS) {
			clipped[clipped_count++] = *a;
		}

		if (a_in != b_in && clipped_count < DTTR_UTIL_WORLDVIEW_MAX_POLYGON_POINTS) {
			clipped[clipped_count++] = dttr_util_worldview_clip_segment(a, b, near_z);
		}
	}

	if (clipped_count < 3u) {
		return 0;
	}

	uint32_t projected = 0;
	for (uint32_t i = 0; i < clipped_count; ++i) {
		DTTR_PCDOGS_T_Math_Vec2F s;
		if (!DTTR_Util_WorldView_ViewToScreen(view, &clipped[i], &s)) {
			return 0;
		}

		// Same clip inputs yield bit-identical floats, so == is safe for dedup.
		if (projected > 0 && s.x == out_points[projected - 1u].x
			&& s.y == out_points[projected - 1u].y) {
			continue;
		}

		out_points[projected] = s;
		if (out_view_z) {
			out_view_z[projected] = (float)clipped[i].z;
		}

		++projected;
	}

	if (projected >= 3u && out_points[0].x == out_points[projected - 1u].x
		&& out_points[0].y == out_points[projected - 1u].y) {
		--projected;
	}

	return projected >= 3u ? projected : 0;
}

#ifdef __cplusplus
}
#endif

#endif // DTTR_UTIL_WORLDVIEW_H
