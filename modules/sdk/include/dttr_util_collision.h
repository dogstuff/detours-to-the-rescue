/// Helpers for decoding live collision nodes, polygons, edge adjacency, and
/// wall edges.
///
/// This header is exposed through dttr_sdk.h only when DTTR_SDK_ENABLE_UNSTABLE
/// is set. It depends on PCDOGS layouts that are still being mapped, so source
/// and ABI details may change without notice.

#ifndef DTTR_UTIL_COLLISION_H
#define DTTR_UTIL_COLLISION_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <dttr_core.h>
#ifndef DTTR_SDK_ENABLE_UNSTABLE
#error "Define DTTR_SDK_ENABLE_UNSTABLE before including dttr_util_collision.h"
#endif
#include <dttr_pcdogs.h>
#include <dttr_util_mem.h>

#ifdef __cplusplus
extern "C" {
#endif

/// Collision node transform matrices use this Q12 fixed-point unit.
#define DTTR_UTIL_COLLISION_Q12_ONE 4096

/// Raw collision vertex components are local units at 1/64 of world fixed
/// point, so a world position is the node origin plus the (optionally
/// rotated) raw vertex times this scale.
#define DTTR_UTIL_COLLISION_VERTEX_WORLD_SCALE 64

/// Bounds for node array counts. Live nodes stay far below these values;
/// larger counts usually mean a stale or misread node.
#define DTTR_UTIL_COLLISION_MAX_VERTICES 4096u
#define DTTR_UTIL_COLLISION_MAX_POLYGONS 8192u

/// A face-plane record must be readable through at least this size, which
/// covers the header through the adjacency words at +0xC.
#define DTTR_UTIL_COLLISION_ADJ_FACE_MIN_SIZE 0x14u

/// The per-vertex wall tag sits at this byte offset within a collision
/// vertex record.
#define DTTR_UTIL_COLLISION_VERTEX_WALL_TAG_OFFSET 0x0bu

/// Return a polygon's edge count, which is 4 unless flags bit `0x1` marks a
/// triangle. The polygon must be readable.
static inline uint32_t DTTR_Util_CollisionPolygonEdgeCount(
	const DTTR_PCDOGS_T_Collision_Polygon *polygon
) {
	return (polygon->flags & 0x1u) != 0u ? 3u : 4u;
}

/// Convert a raw collision vertex to world fixed-point coordinates (Y is
/// the vertical axis).
///
/// The node's Q12 rotation matrix is applied first when node flags 0x22
/// request transformed coordinates, then the raw-to-world scale and the
/// node origin, matching the engine's own vertex handling in collision
/// tests.
static inline bool DTTR_Util_CollisionVertexWorld(
	const DTTR_PCDOGS_T_Collision_Node *node,
	const DTTR_PCDOGS_T_Collision_Vertex *vertex,
	DTTR_PCDOGS_T_Math_Vec3I32 *out_world
) {
	if (!node || !vertex || !out_world) {
		return false;
	}

	int32_t x = (int32_t)vertex->x;
	int32_t y = (int32_t)vertex->y;
	int32_t z = (int32_t)vertex->z;
	if ((node->flags & 0x22u) != 0u) {
		const int32_t rx = x;
		const int32_t ry = y;
		const int32_t rz = z;
		const DTTR_PCDOGS_T_Math_Matrix3x3I16 *m = &node->transform_matrix;
		x = (int32_t)(((int64_t)rx * m->m00 + (int64_t)ry * m->m01 + (int64_t)rz * m->m02)
					  / DTTR_UTIL_COLLISION_Q12_ONE);
		y = (int32_t)(((int64_t)rx * m->m10 + (int64_t)ry * m->m11 + (int64_t)rz * m->m12)
					  / DTTR_UTIL_COLLISION_Q12_ONE);
		z = (int32_t)(((int64_t)rx * m->m20 + (int64_t)ry * m->m21 + (int64_t)rz * m->m22)
					  / DTTR_UTIL_COLLISION_Q12_ONE);
	}

	const int64_t wx = (int64_t)node->origin.x
					   + (int64_t)x * DTTR_UTIL_COLLISION_VERTEX_WORLD_SCALE;
	const int64_t wy = (int64_t)node->origin.y
					   + (int64_t)y * DTTR_UTIL_COLLISION_VERTEX_WORLD_SCALE;
	const int64_t wz = (int64_t)node->origin.z
					   + (int64_t)z * DTTR_UTIL_COLLISION_VERTEX_WORLD_SCALE;
	if (wx < INT32_MIN || wx > INT32_MAX || wy < INT32_MIN || wy > INT32_MAX
		|| wz < INT32_MIN || wz > INT32_MAX) {
		return false;
	}

	*out_world = (DTTR_PCDOGS_T_Math_Vec3I32){
		.x = (int32_t)wx,
		.y = (int32_t)wy,
		.z = (int32_t)wz,
	};
	return true;
}

/// Report whether a polygon pointer belongs to a node's polygon array and
/// the node's vertex array can satisfy the polygon's indices.
///
/// A zero polygon_count is normal on live nodes, in which case vertex_count
/// stands in as the membership upper bound.
static inline bool DTTR_Util_CollisionPolygonInNode(
	const DTTR_PCDOGS_T_Collision_Node *node,
	const DTTR_PCDOGS_T_Collision_Polygon *polygon
) {
	if (!node || !polygon || !DTTR_Util_MemReadable(node, sizeof(*node))
		|| !node->polygons || !node->vertices || node->vertex_count == 0
		|| node->vertex_count > DTTR_UTIL_COLLISION_MAX_VERTICES) {
		return false;
	}

	const uint32_t polygon_count = node->polygon_count != 0u ? node->polygon_count
															 : node->vertex_count;

	if (polygon_count > DTTR_UTIL_COLLISION_MAX_POLYGONS
		|| !DTTR_Util_MemReadable(
			node->polygons,
			sizeof(node->polygons[0]) * polygon_count
		)
		|| !DTTR_Util_MemReadable(
			node->vertices,
			sizeof(node->vertices[0]) * node->vertex_count
		)) {
		return false;
	}

	const uintptr_t polygon_addr = (uintptr_t)polygon;
	const uintptr_t polygons_begin = (uintptr_t)node->polygons;
	const uintptr_t polygons_end = polygons_begin
								   + sizeof(node->polygons[0]) * polygon_count;
	if (polygon_addr < polygons_begin || polygon_addr >= polygons_end) {
		return false;
	}

	if ((polygon_addr - polygons_begin) % sizeof(node->polygons[0]) != 0u) {
		return false;
	}

	// Records are quad-width, so all four indices must be valid, even for triangles.
	for (uint32_t i = 0; i < 4u; ++i) {
		if (polygon->vertex_indices[i] >= node->vertex_count) {
			return false;
		}
	}

	return true;
}

/// Decode a polygon's packed edge adjacency locally and return the neighbor.
///
/// The packed words live at +0xC of the polygon's face-plane record: the low
/// two bits hold the neighbor edge, and the rest is a one-based index into the
/// owning node's polygon array. Local decoding avoids the native helper's
/// stale-record crashes and leaves walkability filtering to callers.
static inline DTTR_PCDOGS_T_Collision_Polygon *DTTR_Util_CollisionAdjacentPolygon(
	const DTTR_PCDOGS_T_Collision_Node *node,
	const DTTR_PCDOGS_T_Collision_Polygon *polygon,
	uint32_t edge_index,
	int32_t *out_neighbor_edge
) {
	if (!node || !polygon || edge_index >= 4u || !polygon->adj_face_ptr
		|| !DTTR_Util_MemReadable(
			polygon->adj_face_ptr,
			DTTR_UTIL_COLLISION_ADJ_FACE_MIN_SIZE
		)) {
		return NULL;
	}

	const uint16_t *adj_edges = (const uint16_t *)((const uint8_t *)polygon->adj_face_ptr
												   + 0x0c);
	const uint16_t encoded = adj_edges[edge_index];
	const uint32_t one_based_index = (uint32_t)encoded >> 2;
	if (one_based_index == 0u || !node->polygons
		|| (node->polygon_count != 0u && one_based_index > node->polygon_count)) {
		return NULL;
	}

	DTTR_PCDOGS_T_Collision_Polygon *neighbor = node->polygons + (one_based_index - 1u);
	if (!DTTR_Util_MemReadable(neighbor, sizeof(*neighbor))) {
		return NULL;
	}

	if (out_neighbor_edge) {
		*out_neighbor_edge = (int32_t)(encoded & 0x3u);
	}

	return neighbor;
}

// Resolve a polygon edge to its endpoint vertex indices.
static inline bool dttr_util_collision_edge_vertices(
	const DTTR_PCDOGS_T_Collision_Node *node,
	const DTTR_PCDOGS_T_Collision_Polygon *polygon,
	uint32_t edge_index,
	uint16_t *out_i0,
	uint16_t *out_i1
) {
	if (edge_index >= 4u) {
		return false;
	}

	const uint16_t i0 = polygon->vertex_indices[edge_index];
	const uint16_t i1 = polygon->vertex_indices[(edge_index + 1u) & 3u];
	if (i0 >= node->vertex_count || i1 >= node->vertex_count) {
		return false;
	}

	*out_i0 = i0;
	*out_i1 = i1;
	return true;
}

// Read the per-vertex wall tag byte from a collision vertex record.
static inline uint8_t dttr_util_collision_vertex_wall_tag(
	const DTTR_PCDOGS_T_Collision_Vertex *vertex
) {
	return ((const uint8_t *)vertex)[DTTR_UTIL_COLLISION_VERTEX_WALL_TAG_OFFSET];
}

/// Return whether a polygon edge is a wall, per the engine's rule in
/// Collision_ProcessActorGroundCheck. Make sure you validate polygon
/// membership with DTTR_Util_CollisionPolygonInNode first.
///
/// A wall edge belongs to a boundary polygon (flags 0x4000), has at least
/// one endpoint with a nonzero wall tag (byte +0xB of the vertex record),
/// and its neighbor across the edge is missing or not walkable (neighbor
/// flags bit 0x4 clear), unless flags 0x400 plus bit 0 of the face-plane
/// record mark it as a one-way or disabled barrier. The engine only
/// extrudes such edges into vertical planes near a touching actor, while
/// this predicate classifies them anywhere.
static inline bool DTTR_Util_CollisionEdgeIsWall(
	const DTTR_PCDOGS_T_Collision_Node *node,
	const DTTR_PCDOGS_T_Collision_Polygon *polygon,
	uint32_t edge_index
) {
	if (!node || !polygon || !node->vertices
		|| edge_index >= DTTR_Util_CollisionPolygonEdgeCount(polygon)
		|| (polygon->flags & 0x4000u) == 0u) {
		return false;
	}

	if ((polygon->flags & 0x400u) != 0u && polygon->plane_data
		&& DTTR_Util_MemReadable(polygon->plane_data, 1)
		&& (((const uint8_t *)polygon->plane_data)[0] & 0x1u) != 0u) {
		return false;
	}

	uint16_t i0 = 0;
	uint16_t i1 = 0;
	if (!dttr_util_collision_edge_vertices(node, polygon, edge_index, &i0, &i1)
		|| i0 == i1) {
		return false;
	}

	const uint8_t tag0 = dttr_util_collision_vertex_wall_tag(&node->vertices[i0]);
	const uint8_t tag1 = dttr_util_collision_vertex_wall_tag(&node->vertices[i1]);
	if (tag0 == 0u && tag1 == 0u) {
		return false;
	}

	const DTTR_PCDOGS_T_Collision_Polygon
		*neighbor = DTTR_Util_CollisionAdjacentPolygon(node, polygon, edge_index, NULL);
	return neighbor == NULL || (neighbor->flags & 0x4u) == 0u;
}

/// Build a vertical world-space quad over a wall edge. The base follows the
/// edge, and the top is height_fp world fixed-point units above it.
static inline bool DTTR_Util_CollisionWallEdgeQuad(
	const DTTR_PCDOGS_T_Collision_Node *node,
	const DTTR_PCDOGS_T_Collision_Polygon *polygon,
	uint32_t edge_index,
	int32_t height_fp,
	DTTR_PCDOGS_T_Math_Vec3I32 out_world[4]
) {
	if (!node || !polygon || !out_world
		|| edge_index >= DTTR_Util_CollisionPolygonEdgeCount(polygon)) {
		return false;
	}

	uint16_t i0 = 0;
	uint16_t i1 = 0;
	if (!dttr_util_collision_edge_vertices(node, polygon, edge_index, &i0, &i1)) {
		return false;
	}

	DTTR_PCDOGS_T_Math_Vec3I32 base0 = {0};
	DTTR_PCDOGS_T_Math_Vec3I32 base1 = {0};
	if (!DTTR_Util_CollisionVertexWorld(node, &node->vertices[i0], &base0)
		|| !DTTR_Util_CollisionVertexWorld(node, &node->vertices[i1], &base1)) {
		return false;
	}

	const int64_t top0 = (int64_t)base0.y + height_fp;
	const int64_t top1 = (int64_t)base1.y + height_fp;
	if (top0 < INT32_MIN || top0 > INT32_MAX || top1 < INT32_MIN || top1 > INT32_MAX) {
		return false;
	}

	out_world[0] = base0;
	out_world[1] = base1;
	out_world[2] = (DTTR_PCDOGS_T_Math_Vec3I32){base1.x, (int32_t)top1, base1.z};
	out_world[3] = (DTTR_PCDOGS_T_Math_Vec3I32){base0.x, (int32_t)top0, base0.z};
	return true;
}

#ifdef __cplusplus
}
#endif

#endif // DTTR_UTIL_COLLISION_H
