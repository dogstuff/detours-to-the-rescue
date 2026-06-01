/// @file dttr_result.h
/// Shared SDK status and result helpers.

#ifndef DTTR_RESULT_H
#define DTTR_RESULT_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum DTTR_Status {
	DTTR_OK = 0,
	DTTR_ERR_INVALID_ARGUMENT,
	DTTR_ERR_NOT_FOUND,
	DTTR_ERR_UNSUPPORTED,
	DTTR_ERR_ALREADY_INSTALLED,
	DTTR_ERR_NOT_INSTALLED,
	DTTR_ERR_MEMORY_PROTECTION,
	DTTR_ERR_RUNTIME_UNAVAILABLE,
	DTTR_ERR_ABI_MISMATCH,
	DTTR_ERR_OUT_OF_MEMORY,
	DTTR_ERR_HOOK_CHAIN_UNSUPPORTED,
	DTTR_ERR_MISSING_SYMBOL,
	DTTR_ERR_UNRESOLVED,
	DTTR_ERR_NOT_CALLABLE,
	DTTR_ERR_READ_FAILED,
	DTTR_ERR_WRITE_FAILED,
	DTTR_ERR_POLICY_MISMATCH,
	DTTR_ERR_UNSUPPORTED_LAYOUT,
	DTTR_ERR_UNSUPPORTED_CONTRACT,
	DTTR_ERR_PROVENANCE_UNSAFE,
} DTTR_Status;

typedef struct DTTR_Result {
	DTTR_Status status;
	/// Optional static diagnostic text. May be `NULL` when `status` is enough.
	const char *message;
} DTTR_Result;

/// Return a stable text token for SDK status values used by logs and tests.
/// @param status Status value to name.
/// @return Static string like `DTTR_OK` or `DTTR_ERR_INVALID_ARGUMENT`.
const char *DTTR_StatusName(DTTR_Status status);

/// Report whether an SDK status represents success.
/// @param status Status value returned by an SDK operation.
/// @return `true` when `status` is `DTTR_OK`.
bool DTTR_StatusOk(DTTR_Status status);

/// Report whether an SDK status represents failure.
/// @param status Status value returned by an SDK operation.
/// @return `true` when `status` is not `DTTR_OK`.
bool DTTR_StatusFailed(DTTR_Status status);

/// Report whether an SDK result represents success.
/// @param result Result object returned by an SDK operation.
/// @return `true` when `result.status` is `DTTR_OK`.
bool DTTR_ResultOk(DTTR_Result result);

#ifdef __cplusplus
}
#endif

#endif // DTTR_RESULT_H
