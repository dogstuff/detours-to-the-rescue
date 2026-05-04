#ifndef HOOK_REGISTRY_PRIVATE_H
#define HOOK_REGISTRY_PRIVATE_H

#include <stddef.h>
#include <stdint.h>

#include <windows.h>

typedef struct DTTR_Hook {
	uintptr_t m_addr;
	size_t m_size;
	uint8_t *m_original;
	uint8_t *m_trampoline;
	void *m_owner;
} DTTR_Hook;

/// Install a JMP hook and return the trampoline in *out_original.
/// Use prologue_size = 0 for automatic sizing.
DTTR_Hook *dttr_hook_attach_function(
	uintptr_t addr,
	int prologue_size,
	void *handler,
	void **out_original
);

/// Patch a pointer or IAT slot and return the previous value in *out_original.
DTTR_Hook *dttr_hook_attach_pointer(uintptr_t addr, void *new_value, void **out_original);

/// Patch bytes and keep the originals for detach.
DTTR_Hook *dttr_hook_patch_bytes(uintptr_t addr, const uint8_t *bytes, size_t size);

/// Detach one hook or patch.
void dttr_hook_detach(DTTR_Hook *hook);

/// Set the owner tag for subsequent hooks and return the previous owner.
void *dttr_hook_set_owner(void *owner);

/// Detach hooks tagged with the given owner.
void dttr_hook_detach_owner(void *owner);

/// Detach all remaining hooks and free the sigscan cache.
void dttr_hook_cleanup_all(void);

/// Scan a module for the first matching signature.
uintptr_t dttr_hook_sigscan(HMODULE mod, const char *sig, const char *mask);

/// Cached dttr_hook_sigscan().
uintptr_t dttr_hook_cached_sigscan(HMODULE mod, const char *sig, const char *mask);

#endif /* HOOK_REGISTRY_PRIVATE_H */
