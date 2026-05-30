/// @file dttr_runtime.h
/// Low-level runtime hook, patch, storage, and signature scanning helpers.

#ifndef DTTR_RUNTIME_H
#define DTTR_RUNTIME_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DTTR_RUNTIME_API_VERSION 2

typedef uintptr_t (*DTTR_Core_SigscanFn)(HMODULE mod, const char *sig, const char *mask);

typedef struct DTTR_Core_Hook DTTR_Core_Hook;
typedef struct DTTR_Core_API DTTR_Core_API;

typedef struct DTTR_Core_Context {
	HMODULE game_module;
	const DTTR_Core_API *api;
	uint32_t struct_size;
	uint32_t api_version;
	uint32_t flags;
	const void *reserved[4];
} DTTR_Core_Context;

typedef DTTR_Core_Hook *(*DTTR_Core_HookFunctionFn)(
	uintptr_t addr,
	// Minimum prologue bytes before instruction-boundary alignment. Pass 0 for auto.
	int prologue_size,
	void *handler,
	void **out_original
);

typedef DTTR_Core_Hook
	*(*DTTR_Core_HookPointerFn)(uintptr_t addr, void *new_value, void **out_original);

typedef DTTR_Core_Hook
	*(*DTTR_Core_PatchBytesFn)(uintptr_t addr, const uint8_t *bytes, size_t size);

typedef void (*DTTR_Core_UnhookFn)(DTTR_Core_Hook *hook);
typedef bool (*DTTR_Core_HookIsActiveFn)(DTTR_Core_Hook *hook);
typedef bool (*DTTR_Core_UnhookCheckedFn)(DTTR_Core_Hook *hook);

struct DTTR_Core_API {
	DTTR_Core_SigscanFn sigscan;
	DTTR_Core_HookFunctionFn hook_function;
	DTTR_Core_HookPointerFn hook_pointer;
	DTTR_Core_PatchBytesFn patch_bytes;
	DTTR_Core_UnhookFn unhook;
	uint32_t struct_size;
	uint32_t api_version;
	uint32_t flags;
	const void *reserved[4];
	DTTR_Core_HookIsActiveFn hook_is_active;
	DTTR_Core_UnhookCheckedFn unhook_checked;
};

// Purpose-agnostic declaration/definition helpers for private consumer storage.
// These helpers only declare or define storage owned by the consumer.
#define DTTR_DECLARE_STORAGE(type, name) extern type name;
#define DTTR_DEFINE_STORAGE(type, name) type name;

#define DTTR_DECLARE_HOOK_STORAGE(name)                                                  \
	extern uintptr_t name##_site;                                                        \
	extern DTTR_Core_Hook *name##_handle;
#define DTTR_DEFINE_HOOK_STORAGE(name)                                                   \
	uintptr_t name##_site;                                                               \
	DTTR_Core_Hook *name##_handle;

#define DTTR_STORAGE_SLOT(type, name) DTTR_DECLARE_STORAGE(type, name)
#define DTTR_HOOK_STORAGE_SLOT(name) DTTR_DECLARE_HOOK_STORAGE(name)

/// Install a JMP hook and optionally return the trampoline.
/// @param addr Function entry or instruction site to hook.
/// @param prologue_size Minimum prologue bytes before instruction-boundary
/// alignment, or `0` for automatic sizing.
/// @param handler Replacement function to call.
/// @param out_original Optional output receiving the original trampoline.
/// @return Hook handle on success, or `NULL` on failure.
DTTR_Core_Hook *DTTR_Core_HookAttachFunction(
	uintptr_t addr,
	int prologue_size,
	void *handler,
	void **out_original
);

/// Patch a pointer or IAT slot and optionally return the previous value.
/// @param addr Address of the pointer slot to replace.
/// @param new_value Replacement pointer value.
/// @param out_original Optional output receiving the previous pointer value.
/// @return Hook handle on success, or `NULL` on failure.
DTTR_Core_Hook *DTTR_Core_HookAttachPointer(
	uintptr_t addr,
	void *new_value,
	void **out_original
);

/// Patch bytes and keep the originals for detach.
/// @param addr Address to patch.
/// @param bytes Replacement bytes to write.
/// @param size Number of replacement bytes.
/// @return Patch handle on success, or `NULL` on failure.
DTTR_Core_Hook *DTTR_Core_HookPatchBytes(
	uintptr_t addr,
	const uint8_t *bytes,
	size_t size
);

/// Detach one hook or patch.
/// @param hook Hook or patch handle returned by this runtime, or `NULL`.
void DTTR_Core_HookDetach(DTTR_Core_Hook *hook);

/// Detach one hook or patch and report restore failure.
/// @param hook Hook or patch handle returned by this runtime, or `NULL`.
/// @return `true` when the hook is detached, already stale, or NULL.
bool DTTR_Core_HookDetachChecked(DTTR_Core_Hook *hook);

/// Report whether a hook handle is still registered with the runtime.
/// @param hook Hook or patch handle returned by this runtime, or `NULL`.
/// @return `true` while the hook is still active.
bool DTTR_Core_HookIsActive(DTTR_Core_Hook *hook);

/// Detach hooks tagged with the given owner and report restore failures.
/// @param owner Owner pointer previously active when hooks or byte patches were created.
/// @return `true` when every owned hook was detached or no owner was supplied.
bool DTTR_Core_HookDetachOwnerChecked(void *owner);

/// Set the owner tag for subsequent hooks and return the previous owner.
/// @param owner Opaque owner pointer assigned to hooks or byte patches created after
/// this call.
///
/// The owner tag is process-global runtime state. Save the returned owner and
/// restore it before returning from a callback if you set it manually.
///
/// @return Previous owner pointer.
void *DTTR_Core_HookSetOwner(void *owner);

/// Detach hooks tagged with the given owner.
/// @param owner Owner pointer previously active when hooks or byte patches were created.
void DTTR_Core_HookDetachOwner(void *owner);

/// Detach all remaining hooks and free the sigscan cache.
void DTTR_Core_HookCleanupAll();

/// Detach all remaining hooks, free the sigscan cache, and report restore failures.
/// @return `true` when every active hook was detached.
bool DTTR_Core_HookCleanupAllChecked();

/// Scan a module for the first matching signature.
/// @param mod Module to scan.
/// @param sig Raw byte signature buffer.
/// @param mask Mask string where implementation-defined wildcard characters skip bytes.
/// @return Matching address, or `0` when not found.
uintptr_t DTTR_Core_HookSigscan(HMODULE mod, const char *sig, const char *mask);

/// Scan a module with the runtime signature cache.
/// @param mod Module to scan.
/// @param sig Raw byte signature buffer.
/// @param mask Mask string where implementation-defined wildcard characters skip bytes.
/// @return Matching address, or `0` when not found.
uintptr_t DTTR_Core_HookCachedSigscan(HMODULE mod, const char *sig, const char *mask);

#ifdef __cplusplus
}
#endif

#endif // DTTR_RUNTIME_H
