/// Experimental helpers for walking live package structures during
/// reverse-engineering work.
///
/// This header is exposed through dttr_sdk.h only when DTTR_SDK_ENABLE_UNSTABLE
/// is set. It depends on PCDOGS layouts that are still being mapped, so source
/// and ABI details may change without notice.

#ifndef DTTR_UTIL_PKG_H
#define DTTR_UTIL_PKG_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <dttr_core.h>
#ifndef DTTR_SDK_ENABLE_UNSTABLE
#error "Define DTTR_SDK_ENABLE_UNSTABLE before including dttr_util_pkg.h"
#endif
#include <dttr_pcdogs.h>

#ifndef DTTR_UTIL_API
#define DTTR_UTIL_API extern
#endif

#ifdef __cplusplus
extern "C" {
#endif

/// Default TOC entry count when options provide neither entries nor a count.
#define DTTR_UTIL_PKG_DEFAULT_TOC_COUNT 138u

/// Bit flags for DTTR_Util_PkgWalkOptions.domains.
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

/// Values stored in DTTR_Util_PkgVisit.kind.
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

/// Status values reported by visits and the final walk result.
typedef enum DTTR_Util_PkgVisitStatus {
	DTTR_UTIL_PKG_STATUS_OK = 0,
	DTTR_UTIL_PKG_STATUS_INVALID_ARGUMENT,
	DTTR_UTIL_PKG_STATUS_UNRESOLVED_SYMBOL,
	DTTR_UTIL_PKG_STATUS_LOAD_FAILED,
	DTTR_UTIL_PKG_STATUS_UNKNOWN_ENTRY_KIND,
	DTTR_UTIL_PKG_STATUS_DECODE_UNSUPPORTED,
	DTTR_UTIL_PKG_STATUS_BOUNDS_INVALID,
} DTTR_Util_PkgVisitStatus;

/// Visitor actions that steer the next walk step.
typedef enum DTTR_Util_PkgVisitAction {
	DTTR_UTIL_PKG_VISIT_CONTINUE = 0,
	DTTR_UTIL_PKG_VISIT_RECURSE,
	DTTR_UTIL_PKG_VISIT_LOAD_AND_RECURSE,
	DTTR_UTIL_PKG_VISIT_SKIP_SUBTREE,
	DTTR_UTIL_PKG_VISIT_STOP,
} DTTR_Util_PkgVisitAction;

typedef struct DTTR_Util_PkgVisit DTTR_Util_PkgVisit;

/// Visitor called once per visit. Its return value steers traversal.
typedef DTTR_Util_PkgVisitAction (*DTTR_Util_PkgVisitor)(
	const DTTR_Util_PkgVisit *visit,
	void *userdata
);

/// Load data for a TOC entry, reporting failures through out_status.
typedef bool (*DTTR_Util_PkgLoadEntryFn)(
	const DTTR_Core_Context *ctx,
	int32_t toc_index,
	const DTTR_PCDOGS_T_PKG_TOCEntry *entry,
	void *userdata,
	void **out_entry,
	size_t *out_size,
	DTTR_Util_PkgVisitStatus *out_status
);

/// Release entry data produced by the matching load callback.
typedef void (*DTTR_Util_PkgFreeEntryFn)(
	const DTTR_Core_Context *ctx,
	int32_t toc_index,
	const DTTR_PCDOGS_T_PKG_TOCEntry *entry,
	void *entry_data,
	void *userdata
);

/// Pointers last only for the visit unless PCDOGS documents a longer lifetime.
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

/// Walk options. Start from DTTR_Util_PkgWalk_DefaultOptions() for defaults.
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

/// Non-fatal per-entry failures keep traversal moving.
typedef struct DTTR_Util_PkgWalkResult {
	DTTR_Util_PkgVisitStatus status;
	uint32_t visited_count;
	uint32_t loaded_count;
	uint32_t failed_count;
	bool stopped;
} DTTR_Util_PkgWalkResult;

/// Return default options covering all known domains, entry loading included.
DTTR_UTIL_API DTTR_Util_PkgWalkOptions DTTR_Util_PkgWalk_DefaultOptions();

/// Walk live package structures and call the visitor for each visit.
DTTR_UTIL_API DTTR_Util_PkgWalkResult DTTR_Util_PkgWalk(
	const DTTR_Core_Context *ctx,
	const DTTR_Util_PkgWalkOptions *options,
	DTTR_Util_PkgVisitor visitor,
	void *userdata
);

/// Return a static log-friendly name such as `ok` or `load failed`.
DTTR_UTIL_API const char *DTTR_Util_PkgVisitStatusName(DTTR_Util_PkgVisitStatus status);

/// Define a typed accessor that returns visit->ptr only for the matching kind.
#define DTTR_UTIL_PKG_VISIT_AS(FnSuffix, Kind, Type)                                     \
	static inline const Type *DTTR_Util_PkgVisit_As##FnSuffix(                           \
		const DTTR_Util_PkgVisit *visit                                                  \
	) {                                                                                  \
		return visit && visit->kind == (Kind) ? (const Type *)visit->ptr : NULL;         \
	}

DTTR_UTIL_PKG_VISIT_AS(TOCEntry, DTTR_UTIL_PKG_VISIT_TOC_ENTRY, DTTR_PCDOGS_T_PKG_TOCEntry)
DTTR_UTIL_PKG_VISIT_AS(LoadedEntry, DTTR_UTIL_PKG_VISIT_LOADED_ENTRY, void)
DTTR_UTIL_PKG_VISIT_AS(
	LevelRuntimeData,
	DTTR_UTIL_PKG_VISIT_LEVEL_RUNTIME_DATA,
	DTTR_PCDOGS_T_Level_RuntimeData
)
DTTR_UTIL_PKG_VISIT_AS(MeshNode, DTTR_UTIL_PKG_VISIT_MESH_NODE, DTTR_PCDOGS_T_Mesh_Node)
DTTR_UTIL_PKG_VISIT_AS(
	MaterialEntry,
	DTTR_UTIL_PKG_VISIT_MATERIAL_ENTRY,
	DTTR_PCDOGS_T_Material_Entry
)
DTTR_UTIL_PKG_VISIT_AS(SceneNode, DTTR_UTIL_PKG_VISIT_SCENE_NODE, DTTR_PCDOGS_T_Scene_Node)
DTTR_UTIL_PKG_VISIT_AS(
	SpriteContext,
	DTTR_UTIL_PKG_VISIT_SPRITE_ENTRY,
	DTTR_PCDOGS_T_Graphics_SpriteContext
)
DTTR_UTIL_PKG_VISIT_AS(
	CollisionShape,
	DTTR_UTIL_PKG_VISIT_COLLISION_SHAPE,
	DTTR_PCDOGS_T_PKG_CollisionShape
)

#ifdef __cplusplus
}
#endif

#endif // DTTR_UTIL_PKG_H
