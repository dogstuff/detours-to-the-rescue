#ifndef DTTR_Core_INTERNAL_H
#define DTTR_Core_INTERNAL_H

#include <dttr_core.h>

static inline DTTR_Result dttr_core_result(DTTR_Status status, const char *message) {
	return (DTTR_Result){status, message};
}

const DTTR_Core_Context *dttr_core_patch_group_context(const DTTR_Core_PatchGroup *group);

// These declarations store the last SDK hook error for internal callers.
DTTR_Result dttr_core_hook_last_error();
void dttr_core_hook_set_last_error(DTTR_Status status, const char *message);

void dttr_core_report_init(DTTR_Core_TargetReport *report);
void dttr_core_report_fail(
	DTTR_Core_TargetReport *report,
	size_t index,
	DTTR_Result result
);

#endif // DTTR_Core_INTERNAL_H
