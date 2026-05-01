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
} DTTR_Hook;

/// JMP hook with trampoline. *out_original receives a callable trampoline to the
/// previous handler (or original prologue for the first hook at this address).
/// prologue_size is a minimum byte count before alignment to instruction boundaries;
/// pass 0 to use automatic minimum sizing (5-byte patch only).
DTTR_Hook *dttr_hook_attach_function(
	uintptr_t addr,
	int prologue_size,
	void *handler,
	void **out_original
);

/// Pointer/IAT patch. *out_original receives the previous pointer value.
DTTR_Hook *dttr_hook_attach_pointer(uintptr_t addr, void *new_value, void **out_original);

/// Arbitrary byte patch. Saves originals internally.
DTTR_Hook *dttr_hook_patch_bytes(uintptr_t addr, const uint8_t *bytes, size_t size);

/// Remove a hook/patch. LIFO order required for same-address chains.
void dttr_hook_detach(DTTR_Hook *hook);

/// Detach all remaining hooks in reverse order and free the sigscan cache.
void dttr_hook_cleanup_all(void);

/// Scan a module for a byte pattern with mask and return the address of the first match.
/// The sig parameter is a byte string and mask uses 'x' for exact match and '?' for
/// wildcard. Returns the absolute address of the match, or 0 if no match was found.
uintptr_t dttr_hook_sigscan(HMODULE mod, const char *sig, const char *mask);

/// Cached signature scan that returns a cached result for repeated (sig, mask, module)
/// lookups.
uintptr_t dttr_hook_cached_sigscan(HMODULE mod, const char *sig, const char *mask);

#endif /* HOOK_REGISTRY_PRIVATE_H */
