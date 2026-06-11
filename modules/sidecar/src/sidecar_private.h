#ifndef DTTR_SIDECAR_PRIVATE_H
#define DTTR_SIDECAR_PRIVATE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <windows.h>

#include <dttr_log.h>
#include <dttr_mods.h>
#include <dttr_pcdogs.h>
#include <dttr_runtime.h>

enum { DTTR_EXE_HASH_LENGTH = 16 };

extern HINSTANCE dttr_sidecar_module;
extern char dttr_loader_dir[MAX_PATH];
extern char dttr_exe_hash[DTTR_EXE_HASH_LENGTH + 1];

static inline const char *dttr_sidecar_result_detail(DTTR_Result result) {
	return result.message ? result.message : DTTR_StatusName(result.status);
}

// Logs and rejects a failed PCDOGS operation the sidecar cannot continue without.
static inline bool dttr_sidecar_require_pcdogs_call(const char *name, DTTR_Result result) {
	if (!DTTR_ResultOK(result)) {
		DTTR_LOG_ERROR(
			"Required PCDOGS operation failed: %s (%s)",
			name,
			dttr_sidecar_result_detail(result)
		);
		return false;
	}

	return true;
}

#define REQUIRE_PCDOGS_CALL(expr) dttr_sidecar_require_pcdogs_call(#expr, (expr))

// Installs a PCDogs patch group with common sidecar logging and cleanup.
static inline bool dttr_sidecar_install_pcdogs_patch_group(
	const DTTR_Mods_Context *ctx,
	const char *label,
	const DTTR_PCDOGS_T_Patch_Spec *patches,
	size_t patch_count,
	DTTR_Core_PatchGroup **group
) {
	DTTR_PCDOGS_T_Patch_Report report = {0};
	DTTR_Result result = DTTR_PCDOGS_PatchGroup_Install(
		&ctx->runtime,
		patches,
		patch_count,
		group,
		&report
	);
	if (!DTTR_ResultOK(result)) {
		DTTR_MODS_LOG_ERROR(
			ctx,
			"%s: patch %u failed: %s",
			label,
			(unsigned)report.failed_index,
			dttr_sidecar_result_detail(result)
		);
		DTTR_Core_PatchGroupRelease(group);
		return false;
	}

	DTTR_MODS_LOG_DEBUG(
		ctx,
		"Installed %u %s patches (%u optional skipped)",
		(unsigned)report.installed,
		label,
		(unsigned)report.skipped_optional
	);
	return true;
}

#endif // DTTR_SIDECAR_PRIVATE_H
