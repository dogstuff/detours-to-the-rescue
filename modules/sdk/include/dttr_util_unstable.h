/// @file dttr_util_unstable.h
/// Experimental utility helpers for reverse-engineering package data.
///
/// These declarations are available through dttr_sdk.h only when
/// DTTR_SDK_ENABLE_UNSTABLE is defined. These declarations can change at source
/// or ABI level while package layouts are still being mapped.

#ifndef DTTR_UTIL_UNSTABLE_H
#define DTTR_UTIL_UNSTABLE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <dttr_core.h>
#ifndef DTTR_SDK_ENABLE_UNSTABLE
#error "Define DTTR_SDK_ENABLE_UNSTABLE before including dttr_util_unstable.h"
#endif
#include <dttr_pcdogs.h>

#ifndef DTTR_UTIL_API
#define DTTR_UTIL_API extern
#endif

#ifdef __cplusplus
extern "C" {
#endif

/// Cast a stable opaque Level_Data pointer to the unstable runtime layout.
/// The stable API keeps Level_Data opaque; this helper makes the intentional
/// bridge explicit for callers that opt into unstable layouts.
/// @param ptr_ Stable opaque Level_Data pointer to cast.
/// @return Mutable pointer using the unstable runtime layout.
static inline DTTR_PCDOGS_T_Level_RuntimeData *DTTR_Util_LevelDataAsRuntimeDataMutable(
	void *ptr_
) {
	return (DTTR_PCDOGS_T_Level_RuntimeData *)ptr_;
}

/// Const-preserving variant of the Level_Data runtime-layout cast.
/// @param ptr_ Stable opaque Level_Data pointer to cast.
/// @return Const pointer using the unstable runtime layout.
static inline const DTTR_PCDOGS_T_Level_RuntimeData *DTTR_Util_LevelDataAsRuntimeDataConst(
	const void *ptr_
) {
	return (const DTTR_PCDOGS_T_Level_RuntimeData *)ptr_;
}

/// Return the current active actor, or `NULL` when unavailable or inactive.
/// @param ctx Runtime context for generated PCDOGS symbol calls.
static inline DTTR_PCDOGS_T_Actor_State *DTTR_Util_GetActiveActor(
	const DTTR_Core_Context *ctx
) {
	DTTR_PCDOGS_T_Actor_State *actor = NULL;
	return DTTR_ResultOK(DTTR_PCDOGS_F_Entity_GetActiveActorFromList->Call(ctx, &actor))
			   ? actor
			   : NULL;
}

/// Compare non-`NULL` live actor pointers directly.
/// @param left First actor pointer.
/// @param right Second actor pointer.
static inline bool DTTR_Util_SameActor(
	const DTTR_PCDOGS_T_Actor_State *left,
	const DTTR_PCDOGS_T_Actor_State *right
) {
	return left != NULL && left == right;
}

/// Experimental package traversal helpers for reverse-engineering workflows.
/// These declarations may change while package layouts are still being mapped.
#define DTTR_UTIL_PKG_DEFAULT_TOC_COUNT 138u

/// Package subtree families that the walker can expose.
typedef enum DTTR_Util_PkgWalkDomain {
	DTTR_UTIL_PKG_DOMAIN_TOC = 1u << 0,
	DTTR_UTIL_PKG_DOMAIN_ENTRY = 1u << 1,
	DTTR_UTIL_PKG_DOMAIN_LEVEL = 1u << 2,
	DTTR_UTIL_PKG_DOMAIN_MESH = 1u << 3,
	DTTR_UTIL_PKG_DOMAIN_MATERIAL = 1u << 4,
	DTTR_UTIL_PKG_DOMAIN_SCENE = 1u << 5,
	DTTR_UTIL_PKG_DOMAIN_SPRITE = 1u << 6,
	DTTR_UTIL_PKG_DOMAIN_COLLISION = 1u << 7,
	DTTR_UTIL_PKG_DOMAIN_KNOWN_CHILDREN = DTTR_UTIL_PKG_DOMAIN_LEVEL
										  | DTTR_UTIL_PKG_DOMAIN_MESH
										  | DTTR_UTIL_PKG_DOMAIN_MATERIAL
										  | DTTR_UTIL_PKG_DOMAIN_SCENE
										  | DTTR_UTIL_PKG_DOMAIN_SPRITE
										  | DTTR_UTIL_PKG_DOMAIN_COLLISION,
	DTTR_UTIL_PKG_DOMAIN_ALL_KNOWN = DTTR_UTIL_PKG_DOMAIN_TOC | DTTR_UTIL_PKG_DOMAIN_ENTRY
									 | DTTR_UTIL_PKG_DOMAIN_KNOWN_CHILDREN,
} DTTR_Util_PkgWalkDomain;

/// Object kind reported to a package visitor.
typedef enum DTTR_Util_PkgVisitKind {
	DTTR_UTIL_PKG_VISIT_TOC_ENTRY = 1,
	DTTR_UTIL_PKG_VISIT_LOADED_ENTRY,
	DTTR_UTIL_PKG_VISIT_LEVEL_RUNTIME_DATA,
	DTTR_UTIL_PKG_VISIT_MESH_NODE,
	DTTR_UTIL_PKG_VISIT_MATERIAL_ENTRY,
	DTTR_UTIL_PKG_VISIT_SCENE_NODE,
	DTTR_UTIL_PKG_VISIT_SPRITE_ENTRY,
	DTTR_UTIL_PKG_VISIT_COLLISION_SHAPE,
	DTTR_UTIL_PKG_VISIT_UNSUPPORTED,
} DTTR_Util_PkgVisitKind;

/// Per-visit and whole-walk status values.
typedef enum DTTR_Util_PkgVisitStatus {
	DTTR_UTIL_PKG_STATUS_OK = 0,
	DTTR_UTIL_PKG_STATUS_INVALID_ARGUMENT,
	DTTR_UTIL_PKG_STATUS_UNRESOLVED_SYMBOL,
	DTTR_UTIL_PKG_STATUS_LOAD_FAILED,
	DTTR_UTIL_PKG_STATUS_UNKNOWN_ENTRY_KIND,
	DTTR_UTIL_PKG_STATUS_DECODE_UNSUPPORTED,
	DTTR_UTIL_PKG_STATUS_BOUNDS_INVALID,
} DTTR_Util_PkgVisitStatus;

/// Visitor return value controlling traversal below the current visit.
typedef enum DTTR_Util_PkgVisitAction {
	DTTR_UTIL_PKG_VISIT_CONTINUE = 0,
	DTTR_UTIL_PKG_VISIT_RECURSE,
	DTTR_UTIL_PKG_VISIT_LOAD_AND_RECURSE,
	DTTR_UTIL_PKG_VISIT_SKIP_SUBTREE,
	DTTR_UTIL_PKG_VISIT_STOP,
} DTTR_Util_PkgVisitAction;

typedef struct DTTR_Util_PkgVisit DTTR_Util_PkgVisit;

typedef DTTR_Util_PkgVisitAction (*DTTR_Util_PkgVisitor)(
	const DTTR_Util_PkgVisit *visit,
	void *userdata
);

typedef bool (*DTTR_Util_PkgLoadEntryFn)(
	const DTTR_Core_Context *ctx,
	int32_t toc_index,
	const DTTR_PCDOGS_T_PKG_TOCEntry *entry,
	void *userdata,
	void **out_entry,
	size_t *out_size,
	DTTR_Util_PkgVisitStatus *out_status
);

typedef void (*DTTR_Util_PkgFreeEntryFn)(
	const DTTR_Core_Context *ctx,
	int32_t toc_index,
	const DTTR_PCDOGS_T_PKG_TOCEntry *entry,
	void *entry_data,
	void *userdata
);

/// A single callback frame. Pointers are visit-lifetime unless documented by PCDOGS.
struct DTTR_Util_PkgVisit {
	uint32_t struct_size;
	DTTR_Util_PkgVisitKind kind;
	DTTR_Util_PkgVisitStatus status;
	uint32_t depth;
	int32_t toc_index;
	uint32_t pkg_offset;
	uint32_t pkg_size;
	const void *ptr;
	const DTTR_Util_PkgVisit *parent;
	const DTTR_PCDOGS_T_PKG_TOCEntry *toc_entry;
	const void *loaded_entry_base;
	size_t loaded_entry_size;
	void *userdata;
};

/// Walker configuration. Use DTTR_Util_PkgWalk_DefaultOptions() for defaults.
typedef struct DTTR_Util_PkgWalkOptions {
	uint32_t struct_size;
	uint32_t domains;
	uint32_t max_depth;
	bool load_entries;
	const DTTR_PCDOGS_T_PKG_TOCEntry *toc_entries;
	uint32_t toc_count;
	DTTR_Util_PkgLoadEntryFn load_entry;
	DTTR_Util_PkgFreeEntryFn free_entry;
	void *io_userdata;
} DTTR_Util_PkgWalkOptions;

/// Aggregate walk result. Non-fatal per-entry failures keep traversal moving.
typedef struct DTTR_Util_PkgWalkResult {
	DTTR_Util_PkgVisitStatus status;
	uint32_t visited_count;
	uint32_t loaded_count;
	uint32_t failed_count;
	bool stopped;
} DTTR_Util_PkgWalkResult;

DTTR_UTIL_API DTTR_Util_PkgWalkOptions DTTR_Util_PkgWalk_DefaultOptions();

DTTR_UTIL_API DTTR_Util_PkgWalkResult DTTR_Util_PkgWalk(
	const DTTR_Core_Context *ctx,
	const DTTR_Util_PkgWalkOptions *options,
	DTTR_Util_PkgVisitor visitor,
	void *userdata
);

DTTR_UTIL_API const char *DTTR_Util_PkgVisitStatusName(DTTR_Util_PkgVisitStatus status);

static inline const DTTR_PCDOGS_T_PKG_TOCEntry *DTTR_Util_PkgVisit_AsTOCEntry(
	const DTTR_Util_PkgVisit *visit
) {
	return visit && visit->kind == DTTR_UTIL_PKG_VISIT_TOC_ENTRY
			   ? (const DTTR_PCDOGS_T_PKG_TOCEntry *)visit->ptr
			   : NULL;
}

static inline const void *DTTR_Util_PkgVisit_AsLoadedEntry(
	const DTTR_Util_PkgVisit *visit
) {
	return visit && visit->kind == DTTR_UTIL_PKG_VISIT_LOADED_ENTRY ? visit->ptr : NULL;
}

static inline const DTTR_PCDOGS_T_Level_RuntimeData *DTTR_Util_PkgVisit_AsLevelRuntimeData(
	const DTTR_Util_PkgVisit *visit
) {
	return visit && visit->kind == DTTR_UTIL_PKG_VISIT_LEVEL_RUNTIME_DATA
			   ? (const DTTR_PCDOGS_T_Level_RuntimeData *)visit->ptr
			   : NULL;
}

static inline const DTTR_PCDOGS_T_Mesh_Node *DTTR_Util_PkgVisit_AsMeshNode(
	const DTTR_Util_PkgVisit *visit
) {
	return visit && visit->kind == DTTR_UTIL_PKG_VISIT_MESH_NODE
			   ? (const DTTR_PCDOGS_T_Mesh_Node *)visit->ptr
			   : NULL;
}

static inline const DTTR_PCDOGS_T_Material_Entry *DTTR_Util_PkgVisit_AsMaterialEntry(
	const DTTR_Util_PkgVisit *visit
) {
	return visit && visit->kind == DTTR_UTIL_PKG_VISIT_MATERIAL_ENTRY
			   ? (const DTTR_PCDOGS_T_Material_Entry *)visit->ptr
			   : NULL;
}

static inline const DTTR_PCDOGS_T_Scene_Node *DTTR_Util_PkgVisit_AsSceneNode(
	const DTTR_Util_PkgVisit *visit
) {
	return visit && visit->kind == DTTR_UTIL_PKG_VISIT_SCENE_NODE
			   ? (const DTTR_PCDOGS_T_Scene_Node *)visit->ptr
			   : NULL;
}

static inline const DTTR_PCDOGS_T_Graphics_SpriteContext *DTTR_Util_PkgVisit_AsSpriteContext(
	const DTTR_Util_PkgVisit *visit
) {
	return visit && visit->kind == DTTR_UTIL_PKG_VISIT_SPRITE_ENTRY
			   ? (const DTTR_PCDOGS_T_Graphics_SpriteContext *)visit->ptr
			   : NULL;
}

static inline const DTTR_PCDOGS_T_Graphics_SpriteContext *DTTR_Util_PkgVisit_AsSpriteEntry(
	const DTTR_Util_PkgVisit *visit
) {
	return DTTR_Util_PkgVisit_AsSpriteContext(visit);
}

static inline const DTTR_PCDOGS_T_PKG_CollisionShape *DTTR_Util_PkgVisit_AsCollisionShape(
	const DTTR_Util_PkgVisit *visit
) {
	return visit && visit->kind == DTTR_UTIL_PKG_VISIT_COLLISION_SHAPE
			   ? (const DTTR_PCDOGS_T_PKG_CollisionShape *)visit->ptr
			   : NULL;
}

#ifdef __cplusplus
}
#endif

#endif // DTTR_UTIL_UNSTABLE_H
