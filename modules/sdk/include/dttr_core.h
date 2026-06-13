/// @file dttr_core.h
/// Core SDK APIs for status handling, signature scans, patches, hooks, and patch
/// groups.

#ifndef DTTR_Core_H
#define DTTR_Core_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <windows.h>

#include <dttr_result.h>
#include <dttr_runtime.h>

#ifndef DTTR_ARRAY_COUNT
#define DTTR_ARRAY_COUNT(array) (sizeof(array) / sizeof((array)[0]))
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct DTTR_Core_PatchGroup DTTR_Core_PatchGroup;
typedef DTTR_Core_Hook DTTR_Core_Patch;

typedef enum DTTR_Core_TargetKind {
	DTTR_TARGET_ADDRESS_PATCH = 1,
	DTTR_TARGET_AOB_PATCH = 2,
	DTTR_TARGET_ADDRESS_HOOK = 3,
	DTTR_TARGET_AOB_HOOK = 4,
	DTTR_TARGET_POINTER_HOOK = 5,
	DTTR_TARGET_ADDRESS_REL32_JMP = 6,
	DTTR_TARGET_AOB_REL32_JMP = 7,
} DTTR_Core_TargetKind;

typedef struct DTTR_Core_TargetSpec {
	DTTR_Core_TargetKind kind;
	bool required;
	uintptr_t address;
	const char *aob;
	intptr_t offset;
	const uint8_t *patch_bytes;
	size_t patch_size;
	void *detour;
	void **out_original;
	int prologue_size;
	void *new_value;
} DTTR_Core_TargetSpec;

typedef struct DTTR_Core_TargetReport {
	size_t attempted;
	size_t installed;
	size_t skipped_optional;
	size_t failed_index;
	DTTR_Status status;
	const char *message;
} DTTR_Core_TargetReport;

/// Resolve a textual AOB pattern through the context runtime scanner.
/// @param ctx Runtime context that supplies the runtime scanner and target module.
/// @param aob Space-separated byte pattern. Wildcards follow the runtime scanner.
/// @param out_addr Receives the resolved address on success.
/// @return `DTTR_OK` when the pattern is found, otherwise an error status.
DTTR_Result DTTR_Core_AOBFind(
	const DTTR_Core_Context *ctx,
	const char *aob,
	uintptr_t *out_addr
);

/// Resolve every textual AOB pattern match through the context runtime scanner.
/// @param ctx Runtime context that supplies the runtime scanner and target module.
/// @param aob Space-separated byte pattern. Wildcards follow the runtime scanner.
/// @param out_addrs Optional caller buffer receiving up to `addrs_cap` matches.
/// @param addrs_cap Number of entries available in `out_addrs`.
/// @param out_count Receives the total number of matches found.
/// @return `DTTR_OK` when at least one pattern match is found, otherwise an error status.
DTTR_Result DTTR_Core_AOBFindAll(
	const DTTR_Core_Context *ctx,
	const char *aob,
	uintptr_t *out_addrs,
	size_t addrs_cap,
	size_t *out_count
);

/// Resolve a raw signature and mask through the context runtime scanner.
/// @param ctx Runtime context that supplies the runtime scanner and target module.
/// @param sig Raw byte signature buffer.
/// @param mask Mask string where implementation-defined wildcard characters skip bytes.
/// @param out_addr Receives the resolved address on success.
/// @return `DTTR_OK` when the signature is found, otherwise an error status.
DTTR_Result DTTR_Core_SignatureFind(
	const DTTR_Core_Context *ctx,
	const char *sig,
	const char *mask,
	uintptr_t *out_addr
);

/// Resolve a textual AOB pattern in an explicit module without an SDK context.
/// @param mod Module to scan.
/// @param aob Space-separated byte pattern.
/// @param out_addr Receives the resolved address on success.
/// @return `DTTR_OK` when the pattern is found, otherwise an error status.
DTTR_Result DTTR_Core_AOBFindInModule(HMODULE mod, const char *aob, uintptr_t *out_addr);

/// Resolve every textual AOB pattern match in an explicit module without an SDK context.
/// @param mod Module to scan.
/// @param aob Space-separated byte pattern.
/// @param out_addrs Optional caller buffer receiving up to `addrs_cap` matches.
/// @param addrs_cap Number of entries available in `out_addrs`.
/// @param out_count Receives the total number of matches found.
/// @return `DTTR_OK` when at least one pattern match is found, otherwise an error status.
DTTR_Result DTTR_Core_AOBFindAllInModule(
	HMODULE mod,
	const char *aob,
	uintptr_t *out_addrs,
	size_t addrs_cap,
	size_t *out_count
);

/// Patch bytes and return a handle that can restore the original memory.
/// @param ctx Runtime context used to patch memory.
/// @param address Address to patch.
/// @param bytes Replacement bytes to write.
/// @param size Number of replacement bytes.
/// @param out_patch Required output receiving the patch handle.
/// @return `DTTR_OK` when the bytes were patched, otherwise an error status.
DTTR_Result DTTR_Core_PatchBytes(
	const DTTR_Core_Context *ctx,
	uintptr_t address,
	const uint8_t *bytes,
	size_t size,
	DTTR_Core_Patch **out_patch
);

/// Install a function detour and return both the trampoline and hook handle.
/// @param ctx Runtime context used to install the hook.
/// @param address Function entry or instruction site to hook.
/// @param prologue_size Minimum prologue bytes for the hook injection site, or
/// `0` for automatic sizing.
/// @param detour Replacement function to call.
/// @param out_original Optional output receiving the original trampoline.
/// @param out_hook Required output receiving the hook handle.
/// @return `DTTR_OK` when the hook is installed, otherwise an error status.
DTTR_Result DTTR_Core_HookFunction(
	const DTTR_Core_Context *ctx,
	uintptr_t address,
	int prologue_size,
	void *detour,
	void **out_original,
	DTTR_Core_Hook **out_hook
);

/// Resolve an AOB target and install a function detour at its offset.
/// @param ctx Runtime context used to resolve and install the hook.
/// @param aob Space-separated byte pattern to resolve.
/// @param offset Offset added to the resolved match before installing the hook.
/// @param prologue_size Minimum prologue bytes, or `0` for automatic sizing.
/// @param detour Replacement function to call.
/// @param out_original Optional output receiving the original trampoline.
/// @param out_hook Required output receiving the hook handle.
/// @return `DTTR_OK` when the target resolves and the hook installs.
DTTR_Result DTTR_Core_HookAOB(
	const DTTR_Core_Context *ctx,
	const char *aob,
	intptr_t offset,
	int prologue_size,
	void *detour,
	void **out_original,
	DTTR_Core_Hook **out_hook
);

/// Patch a site with a five-byte relative JMP when the target is in range.
/// @param ctx Runtime context used to patch memory.
/// @param address Address of the jump instruction to write.
/// @param detour Destination address for the relative jump.
/// @param out_patch Required output receiving the patch handle.
/// @return `DTTR_OK` when the jump patch is installed, otherwise an error status.
DTTR_Result DTTR_Core_PatchRel32Jump(
	const DTTR_Core_Context *ctx,
	uintptr_t address,
	void *detour,
	DTTR_Core_Patch **out_patch
);

/// Resolve an AOB target and patch it with a relative JMP.
/// @param ctx Runtime context used to resolve and patch memory.
/// @param aob Space-separated byte pattern to resolve.
/// @param offset Offset added to the resolved match before writing the jump.
/// @param detour Destination address for the relative jump.
/// @param out_patch Required output receiving the patch handle.
/// @return `DTTR_OK` when the target resolves and the jump patch installs.
DTTR_Result DTTR_Core_PatchAOBRel32Jump(
	const DTTR_Core_Context *ctx,
	const char *aob,
	intptr_t offset,
	void *detour,
	DTTR_Core_Patch **out_patch
);

/// Patch a pointer slot and optionally return the previous slot value.
/// @param ctx Runtime context used to patch the pointer slot.
/// @param address Address of the pointer slot to replace.
/// @param new_value Replacement pointer value, or `NULL` to clear the slot.
/// @param out_original Optional output receiving the previous pointer value.
/// @param out_hook Required output receiving the hook handle.
/// @return `DTTR_OK` when the slot is patched, otherwise an error status.
DTTR_Result DTTR_Core_HookPointer(
	const DTTR_Core_Context *ctx,
	uintptr_t address,
	void *new_value,
	void **out_original,
	DTTR_Core_Hook **out_hook
);

/// Detach a byte patch created by the SDK runtime.
DTTR_Result DTTR_Core_Unpatch(DTTR_Core_Patch *patch);

/// Detach a function or pointer hook created by the SDK runtime.
/// @param hook Hook handle returned by an SDK hook call, or `NULL`.
/// @return `DTTR_OK` when detached or already null, otherwise an error.
DTTR_Result DTTR_Core_Unhook(DTTR_Core_Hook *hook);

/// Create a patch group for rollback and teardown.
/// @param ctx Runtime context used by patch-group installs.
/// @param out_group Receives the created patch group on success.
/// @return `DTTR_OK` when the patch group is created, otherwise an error status.
DTTR_Result DTTR_Core_PatchGroupCreate(
	const DTTR_Core_Context *ctx,
	DTTR_Core_PatchGroup **out_group
);

/// Detach every active handle in a patch group while keeping it reusable.
/// @param group Patch group to uninstall.
/// @return `DTTR_OK` when all tracked handles are detached.
DTTR_Result DTTR_Core_PatchGroupUninstall(DTTR_Core_PatchGroup *group);

/// Uninstall a patch group and release its storage.
/// @param group Patch group to destroy, or `NULL`.
/// @return `DTTR_OK` when all tracked handles detached and storage was released.
/// On restore failure, the group is retained so callers can retry or diagnose.
DTTR_Result DTTR_Core_PatchGroupDestroy(DTTR_Core_PatchGroup *group);

/// Destroy a patch group pointer and clear the caller slot.
/// @param group Address of the caller-owned patch group pointer, or `NULL`.
/// @return `DTTR_OK` when all tracked handles detached and `*group` was cleared.
/// On restore failure, `*group` is retained so callers can retry or diagnose.
DTTR_Result DTTR_Core_PatchGroupRelease(DTTR_Core_PatchGroup **group);

/// Patch bytes and adopt the handle into a patch group.
/// @param group Patch group that will own the patch handle.
/// @param address Address to patch.
/// @param bytes Replacement bytes to write.
/// @param size Number of replacement bytes.
/// @param out_patch Optional output receiving the patch handle.
/// @return `DTTR_OK` when the bytes are patched and adopted.
DTTR_Result DTTR_Core_PatchGroupPatchBytes(
	DTTR_Core_PatchGroup *group,
	uintptr_t address,
	const uint8_t *bytes,
	size_t size,
	DTTR_Core_Patch **out_patch
);

/// Install a function hook and adopt the handle into a patch group.
/// @param group Patch group that will own the hook handle.
/// @param address Function entry or instruction site to hook.
/// @param prologue_size Minimum prologue bytes, or `0` for automatic sizing.
/// @param detour Replacement function to call.
/// @param out_original Optional output receiving the original trampoline.
/// @param out_hook Optional output receiving the hook handle.
/// @return `DTTR_OK` when the hook is installed and adopted.
DTTR_Result DTTR_Core_PatchGroupHookFunction(
	DTTR_Core_PatchGroup *group,
	uintptr_t address,
	int prologue_size,
	void *detour,
	void **out_original,
	DTTR_Core_Hook **out_hook
);

/// Install a pointer hook and adopt the handle into a patch group.
/// @param group Patch group that will own the hook handle.
/// @param address Address of the pointer slot to replace.
/// @param new_value Replacement pointer value, or `NULL` to clear the slot.
/// @param out_original Optional output receiving the previous pointer value.
/// @param out_hook Optional output receiving the hook handle.
/// @return `DTTR_OK` when the slot is patched and adopted.
DTTR_Result DTTR_Core_PatchGroupHookPointer(
	DTTR_Core_PatchGroup *group,
	uintptr_t address,
	void *new_value,
	void **out_original,
	DTTR_Core_Hook **out_hook
);

/// Patch a rel32 jump and adopt the handle into a patch group.
/// @param group Patch group that will own the patch handle.
/// @param address Address of the jump instruction to write.
/// @param detour Destination address for the relative jump.
/// @param out_patch Optional output receiving the patch handle.
/// @return `DTTR_OK` when the jump patch is installed and adopted.
DTTR_Result DTTR_Core_PatchGroupPatchRel32Jump(
	DTTR_Core_PatchGroup *group,
	uintptr_t address,
	void *detour,
	DTTR_Core_Patch **out_patch
);

/// Install target specs transactionally into a patch group.
/// @param group Patch group that will own installed target handles.
/// @param targets Array of target specifications to install.
/// @param target_count Number of entries in `targets`.
/// @param out_report Optional output receiving install counts and failure details.
/// @return `DTTR_OK` when all required targets install. Required-target failure rolls
/// back installed handles.
DTTR_Result DTTR_Core_PatchGroupInstallTargets(
	DTTR_Core_PatchGroup *group,
	const DTTR_Core_TargetSpec *targets,
	size_t target_count,
	DTTR_Core_TargetReport *out_report
);

#ifdef __cplusplus
}
#endif

#endif // DTTR_Core_H
