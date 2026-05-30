/// @file dttr_util.h
/// Public helper APIs for mod authors.

#ifndef DTTR_UTIL_H
#define DTTR_UTIL_H

#include <stdbool.h>
#include <stddef.h>

#include <dttr_pcdogs.h>

#ifdef __cplusplus
extern "C" {
#endif

/// Resolve the current active actor from the game.
/// @param ctx Runtime context for generated PCDOGS symbol calls.
/// @return The active actor pointer, or `NULL` when unavailable or inactive.
static inline DTTR_PCDOGS_T_Actor_State *DTTR_Util_GetActiveActor(
	const DTTR_Core_Context *ctx
) {
	DTTR_PCDOGS_T_Actor_State *actor = NULL;
	return DTTR_PCDOGS_F_EntityGetActiveActorFromList->Try(ctx, &actor) ? actor : NULL;
}

/// Compare actor pointers using live pointer identity.
/// @param left First actor pointer.
/// @param right Second actor pointer.
/// @return `true` only when both pointers are non-`NULL` and equal.
static inline bool DTTR_Util_SameActor(
	const DTTR_PCDOGS_T_Actor_State *left,
	const DTTR_PCDOGS_T_Actor_State *right
) {
	return left && right && left == right;
}

#ifdef __cplusplus
}
#endif

#endif // DTTR_UTIL_H
