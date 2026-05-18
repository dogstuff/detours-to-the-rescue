#ifndef DTTR_Core_INTERNAL_H
#define DTTR_Core_INTERNAL_H

#include <dttr_core.h>

static inline DTTR_Core_Result dttr_core_result(
	DTTR_Core_Status status,
	const char *message
) {
	return (DTTR_Core_Result){status, message};
}

const DTTR_Core_Context *dttr_core_patch_group_context(const DTTR_Core_PatchGroup *group);
DTTR_Core_Result dttr_core_hook_last_error();
void dttr_core_hook_set_last_error(DTTR_Core_Status status, const char *message);

#endif // DTTR_Core_INTERNAL_H
