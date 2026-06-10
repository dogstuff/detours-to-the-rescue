/// Helpers for live actor lookup and identity checks.
///
/// This header is exposed through dttr_sdk.h only when DTTR_SDK_ENABLE_UNSTABLE
/// is set. It depends on PCDOGS layouts that are still being mapped, so source
/// and ABI details may change without notice.

#ifndef DTTR_UTIL_ACTOR_H
#define DTTR_UTIL_ACTOR_H

#include <stdbool.h>
#include <stddef.h>

#include <dttr_core.h>
#ifndef DTTR_SDK_ENABLE_UNSTABLE
#error "Define DTTR_SDK_ENABLE_UNSTABLE before including dttr_util_actor.h"
#endif
#include <dttr_pcdogs.h>

#ifdef __cplusplus
extern "C" {
#endif

/// Return the current active actor, or `NULL` when unavailable or inactive.
static inline DTTR_PCDOGS_T_Actor_State *DTTR_Util_GetActiveActor(
	const DTTR_Core_Context *ctx
) {
	DTTR_PCDOGS_T_Actor_State *actor = NULL;
	return DTTR_ResultOK(DTTR_PCDOGS_F_Entity_GetActiveActorFromList->Call(ctx, &actor))
			   ? actor
			   : NULL;
}

/// Report whether `left` and `right` are the same non-`NULL` actor.
static inline bool DTTR_Util_SameActor(
	const DTTR_PCDOGS_T_Actor_State *left,
	const DTTR_PCDOGS_T_Actor_State *right
) {
	return left != NULL && left == right;
}

#ifdef __cplusplus
}
#endif

#endif // DTTR_UTIL_ACTOR_H
