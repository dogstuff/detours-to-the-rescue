#ifndef DTTR_COMMON_ITEROP_H
#define DTTR_COMMON_ITEROP_H

// Interop macros for resolving and hooking game functions and variables via signature scan.

#include <dttr_components.h>

#include <psapi.h>
#include <stdint.h>
#include <string.h>
#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

// Scan a module for a byte pattern with mask and return the address of the first match
// The sig parameter is a byte string and mask uses 'x' for exact match and '?' for
// wildcard. Returns the absolute address of the match, or 0 if no match was found.
static inline uintptr_t dttr_interop_sigscan(
	HMODULE mod,
	const char *sig,
	const char *mask
) {
	MODULEINFO mi;
	if (!GetModuleInformation(GetCurrentProcess(), mod, &mi, sizeof(mi))) {
		return 0;
	}
	const uint8_t *base = (const uint8_t *)mi.lpBaseOfDll;
	const size_t size = mi.SizeOfImage;
	const size_t len = strlen(mask);
	if (len == 0 || len > size) {
		return 0;
	}
	for (size_t i = 0; i <= size - len; i++) {
		for (size_t j = 0;; j++) {
			if (j == len)
				return (uintptr_t)(base + i);
			if (mask[j] == 'x' && base[i + j] != (uint8_t)sig[j])
				break;
		}
	}
	return 0;
}

// Validate a resolved address against the module's loaded range.
// Uses GetModuleInformation directly, so it works before DTTR_ComponentContext exists.
static inline int dttr_interop_validate_resolve(
	HMODULE mod,
	uintptr_t addr,
	const char *name
) {
	if (!addr) {
		return 0;
	}
	MODULEINFO mi;
	if (GetModuleInformation(GetCurrentProcess(), mod, &mi, sizeof(mi))) {
		const uintptr_t mod_base = (uintptr_t)mi.lpBaseOfDll;
		const uintptr_t mod_end = mod_base + mi.SizeOfImage;
		if (addr < mod_base || addr >= mod_end) {
			return 0;
		}
	}
	return 1;
}

// Relocates E8 (CALL rel32) and E9 (JMP rel32) instructions in a copied code buffer
static inline void s_trampoline_relocate(uint8_t *buf, uintptr_t site, int n) {
	for (int i = 0; i < n; i++) {
		if ((buf[i] != 0xE8 && buf[i] != 0xE9) || i + 5 > n)
			continue;

		int32_t rel;
		memcpy(&rel, buf + i + 1, 4);
		rel += (int32_t)(site - (uintptr_t)buf);
		memcpy(buf + i + 1, &rel, 4);
		i += 4;
	}
}

// Validates and logs a resolved address through the component context
#define INTERNAL_DTTR_INTEROP_VALIDATE_AND_LOG(ctx, addr, name)                          \
	((addr) ? (DTTR_LOG_DEBUG(ctx, "Resolved %s at 0x%08X", name, (unsigned)(addr)), 1)  \
			: (DTTR_LOG_ERROR(ctx, "%s: resolved to NULL", name), 0))

// Address resolution helpers used internally by the macros below
// Resolves an E8 relative call at address p to its absolute target
#define DTTR_E8_TARGET(p) ((p) + 5 + *(int32_t *)((p) + 1))

// Reads the absolute jump target from an FF 25 import thunk at address p
#define DTTR_FF25_ADDR(p) (*(uint32_t *)((p) + 2))

// RESOLVE_SIG scans for a byte pattern through the component context
#define INTERNAL_DTTR_INTEROP_RESOLVE_SIG(ctx, sig, mask, expr)                          \
	(__extension__({                                                                     \
		const uintptr_t match = (ctx)->m_game_api->m_sigscan(                            \
			(ctx)->m_game_module,                                                        \
			(sig),                                                                       \
			(mask)                                                                       \
		);                                                                               \
		match ? (uintptr_t)(expr) : 0;                                                   \
	}))

// Storage for address-based interop symbols with cross-TU linkage.
// Define DTTR_INTEROP_IMPLEMENT in exactly one .c file.
#ifdef DTTR_INTEROP_IMPLEMENT
#define INTERNAL_DTTR_INTEROP_ADDR_STORAGE(name) uintptr_t name##_addr = 0;
#define INTERNAL_DTTR_INTEROP_HOOK_STORAGE(name)                                         \
	uintptr_t name##_site = 0;                                                           \
	uint8_t name##_orig[5] = {0};
#define INTERNAL_DTTR_INTEROP_HOOK_TRAMPOLINE_STORAGE(name)                              \
	uintptr_t name##_site = 0;                                                           \
	uint8_t name##_orig[5] = {0};                                                        \
	uint8_t *name##_trampoline = NULL;
#define INTERNAL_DTTR_INTEROP_PATCH_PTR_STORAGE(name)                                    \
	uintptr_t name##_site = 0;                                                           \
	uintptr_t name##_orig = 0;
#define INTERNAL_DTTR_INTEROP_PATCH_BYTES_STORAGE(name, sz)                              \
	uintptr_t name##_site = 0;                                                           \
	uint8_t name##_orig[sz] = {0};
#define INTERNAL_DTTR_INTEROP_HOOK_COM_STORAGE(name)                                     \
	void *name##_original = NULL;                                                        \
	void **name##_entry = NULL;
#else
#define INTERNAL_DTTR_INTEROP_ADDR_STORAGE(name) extern uintptr_t name##_addr;
#define INTERNAL_DTTR_INTEROP_HOOK_STORAGE(name)                                         \
	extern uintptr_t name##_site;                                                        \
	extern uint8_t name##_orig[5];
#define INTERNAL_DTTR_INTEROP_HOOK_TRAMPOLINE_STORAGE(name)                              \
	extern uintptr_t name##_site;                                                        \
	extern uint8_t name##_orig[5];                                                       \
	extern uint8_t *name##_trampoline;
#define INTERNAL_DTTR_INTEROP_PATCH_PTR_STORAGE(name)                                    \
	extern uintptr_t name##_site;                                                        \
	extern uintptr_t name##_orig;
#define INTERNAL_DTTR_INTEROP_PATCH_BYTES_STORAGE(name, sz)                              \
	extern uintptr_t name##_site;                                                        \
	extern uint8_t name##_orig[sz];
#define INTERNAL_DTTR_INTEROP_HOOK_COM_STORAGE(name)                                     \
	extern void *name##_original;                                                        \
	extern void **name##_entry;
#endif

// Cached function wrappers resolved by signature scan

// Cached function wrapper resolved by signature scan.
// Call name_init(ctx) once; name() uses the cached address.
#define DTTR_INTEROP_WRAP_CACHED_SIG(name, sig, mask, expr, ret, params, args)           \
	DTTR_INTEROP_WRAP_CACHED_CC_SIG(                                                     \
		name,                                                                            \
		/* default cc */,                                                                \
		sig,                                                                             \
		mask,                                                                            \
		expr,                                                                            \
		ret,                                                                             \
		params,                                                                          \
		args                                                                             \
	)

// Wraps a function with cached address resolution and an explicit calling convention
#define DTTR_INTEROP_WRAP_CACHED_CC_SIG(name, cc, sig, mask, expr, ret, params, args)    \
	typedef ret(cc *name##_fn_t) params;                                                 \
	INTERNAL_DTTR_INTEROP_ADDR_STORAGE(name)                                             \
	static inline bool name##_init(const DTTR_ComponentContext *ctx) {                   \
		name##_addr = INTERNAL_DTTR_INTEROP_RESOLVE_SIG(ctx, sig, mask, expr);           \
		if (!INTERNAL_DTTR_INTEROP_VALIDATE_AND_LOG(ctx, name##_addr, #name)) {          \
			name##_addr = 0;                                                             \
			return false;                                                                \
		}                                                                                \
		return true;                                                                     \
	}                                                                                    \
	static inline ret name params { return ((name##_fn_t)name##_addr)args; }

// Optional variant that warns instead of erroring when not found.
// Callers must check name_addr before calling name().
#define DTTR_INTEROP_WRAP_CACHED_CC_OPTIONAL_SIG(                                        \
	name,                                                                                \
	cc,                                                                                  \
	sig,                                                                                 \
	mask,                                                                                \
	expr,                                                                                \
	ret,                                                                                 \
	params,                                                                              \
	args                                                                                 \
)                                                                                        \
	typedef ret(cc *name##_fn_t) params;                                                 \
	INTERNAL_DTTR_INTEROP_ADDR_STORAGE(name)                                             \
	static inline bool name##_init(const DTTR_ComponentContext *ctx) {                   \
		name##_addr = INTERNAL_DTTR_INTEROP_RESOLVE_SIG(ctx, sig, mask, expr);           \
		if (!name##_addr) {                                                              \
			DTTR_LOG_WARN(ctx, "Skipped optional %s (not found)", #name);                \
			return false;                                                                \
		}                                                                                \
		DTTR_LOG_DEBUG(ctx, "Resolved %s at 0x%08X", #name, (unsigned)name##_addr);      \
		return true;                                                                     \
	}                                                                                    \
	static inline ret name params { return ((name##_fn_t)name##_addr)args; }

// Variable accessors resolved by signature scan

// Provides accessor functions for a module global variable.
// Generates name_init(ctx), name_ptr(), name_get(), and name_set(val).
#define DTTR_INTEROP_VAR_SIG(name, type, sig, mask, expr)                                \
	INTERNAL_DTTR_INTEROP_ADDR_STORAGE(name)                                             \
	static inline bool name##_init(const DTTR_ComponentContext *ctx) {                   \
		name##_addr = INTERNAL_DTTR_INTEROP_RESOLVE_SIG(ctx, sig, mask, expr);           \
		if (!INTERNAL_DTTR_INTEROP_VALIDATE_AND_LOG(ctx, name##_addr, #name)) {          \
			name##_addr = 0;                                                             \
			return false;                                                                \
		}                                                                                \
		return true;                                                                     \
	}                                                                                    \
	static inline type *name##_ptr(void) { return (type *)name##_addr; }                 \
	static inline type name##_get(void) { return *(type *)name##_addr; }                 \
	static inline void name##_set(type val) { *(type *)name##_addr = val; }

// LOG wrappers for hook and patch install and removal.

#define DTTR_INTEROP_HOOK_FUNC_LOG(hook, ctx)                                            \
	do {                                                                                 \
		hook##_install(ctx);                                                             \
		DTTR_LOG_DEBUG(                                                                  \
			ctx,                                                                         \
			"Applied HOOK_FUNC %s at 0x%08X",                                            \
			#hook,                                                                       \
			(unsigned)hook##_site                                                        \
		);                                                                               \
	} while (0)

// Generic optional install and log for hooks and patches that may not exist in all game
// versions. Calls _install, then logs success or skip based on _site.
#define DTTR_INTEROP_OPTIONAL_LOG(hook, ctx, type_name)                                  \
	do {                                                                                 \
		hook##_install(ctx);                                                             \
		if (hook##_site) {                                                               \
			DTTR_LOG_DEBUG(                                                              \
				ctx,                                                                     \
				"Applied " type_name " %s at 0x%08X",                                    \
				#hook,                                                                   \
				(unsigned)hook##_site                                                    \
			);                                                                           \
		} else {                                                                         \
			DTTR_LOG_WARN(ctx, "Skipped optional " type_name " %s (not found)", #hook);  \
		}                                                                                \
	} while (0)

#define DTTR_INTEROP_HOOK_FUNC_OPTIONAL_LOG(hook, ctx)                                   \
	DTTR_INTEROP_OPTIONAL_LOG(hook, ctx, "HOOK_FUNC")

#define DTTR_INTEROP_HOOK_FUNC_TRAMPOLINE_LOG(hook, ctx)                                 \
	do {                                                                                 \
		hook##_install(ctx);                                                             \
		if (hook##_trampoline) {                                                         \
			DTTR_LOG_DEBUG(                                                              \
				ctx,                                                                     \
				"Applied HOOK_FUNC %s at 0x%08X",                                        \
				#hook,                                                                   \
				(unsigned)hook##_site                                                    \
			);                                                                           \
		} else {                                                                         \
			DTTR_LOG_ERROR(ctx, "Failed to allocate trampoline for %s", #hook);          \
		}                                                                                \
	} while (0)

#define DTTR_INTEROP_PATCH_PTR_LOG(hook, ctx)                                            \
	do {                                                                                 \
		hook##_patch(ctx);                                                               \
		DTTR_LOG_DEBUG(                                                                  \
			ctx,                                                                         \
			"Applied PATCH_PTR %s at 0x%08X",                                            \
			#hook,                                                                       \
			(unsigned)hook##_site                                                        \
		);                                                                               \
	} while (0)

#define DTTR_INTEROP_PATCH_BYTES_LOG(hook, ctx, bytes)                                   \
	do {                                                                                 \
		hook##_patch(ctx, bytes);                                                        \
		DTTR_LOG_DEBUG(                                                                  \
			ctx,                                                                         \
			"Applied PATCH_BYTES %s at 0x%08X",                                          \
			#hook,                                                                       \
			(unsigned)hook##_site                                                        \
		);                                                                               \
	} while (0)

#define DTTR_INTEROP_PATCH_BYTES_OPTIONAL_LOG(hook, ctx, bytes)                          \
	do {                                                                                 \
		hook##_install(ctx, bytes);                                                      \
		if (hook##_site) {                                                               \
			DTTR_LOG_DEBUG(                                                              \
				ctx,                                                                     \
				"Applied PATCH_BYTES %s at 0x%08X",                                      \
				#hook,                                                                   \
				(unsigned)hook##_site                                                    \
			);                                                                           \
		} else {                                                                         \
			DTTR_LOG_WARN(ctx, "Skipped optional PATCH_BYTES %s (not found)", #hook);    \
		}                                                                                \
	} while (0)

#define DTTR_INTEROP_UNHOOK_LOG(hook, ctx)                                               \
	do {                                                                                 \
		DTTR_LOG_DEBUG(ctx, "Removed %s at 0x%08X", #hook, (unsigned)hook##_site);       \
		hook##_unpatch(ctx);                                                             \
	} while (0)

#define DTTR_INTEROP_UNHOOK_OPTIONAL_LOG(hook, ctx)                                      \
	do {                                                                                 \
		if (hook##_site) {                                                               \
			DTTR_LOG_DEBUG(ctx, "Removed %s at 0x%08X", #hook, (unsigned)hook##_site);   \
			hook##_unpatch(ctx);                                                         \
		}                                                                                \
	} while (0)

// Patch an E9 JMP rel32 at a signature-matched site.
// Generates name_patch(ctx) and name_unpatch(ctx).
#define DTTR_INTEROP_PATCH_JMP_SIG(name, sig, mask, expr, target)                        \
	INTERNAL_DTTR_INTEROP_HOOK_STORAGE(name)                                             \
	static inline bool name##_patch(const DTTR_ComponentContext *ctx) {                  \
		name##_site = INTERNAL_DTTR_INTEROP_RESOLVE_SIG(ctx, sig, mask, expr);           \
		if (!INTERNAL_DTTR_INTEROP_VALIDATE_AND_LOG(ctx, name##_site, #name)) {          \
			name##_site = 0;                                                             \
			return false;                                                                \
		}                                                                                \
		uint8_t buf[5];                                                                  \
		buf[0] = 0xE9;                                                                   \
		int32_t rel = (int32_t)((uintptr_t)(target) - (name##_site + 5));                \
		memcpy(buf + 1, &rel, 4);                                                        \
		if (!ctx->m_game_api->m_patch_write(name##_site, buf, 5, name##_orig)) {         \
			DTTR_LOG_ERROR(ctx, "%s: patch_write failed", #name);                        \
			name##_site = 0;                                                             \
			return false;                                                                \
		}                                                                                \
		return true;                                                                     \
	}                                                                                    \
	static inline void name##_unpatch(const DTTR_ComponentContext *ctx) {                \
		if (name##_site) {                                                               \
			ctx->m_game_api->m_patch_restore(name##_site, name##_orig, 5);               \
			name##_site = 0;                                                             \
		}                                                                                \
	}

// Patch a pointer (vtable entry, IAT slot, etc.) at a signature-matched site.
// Generates name_patch(ctx), name_unpatch(ctx), and name_orig_ptr().
#define DTTR_INTEROP_PATCH_PTR_SIG(name, sig, mask, expr, target)                        \
	INTERNAL_DTTR_INTEROP_PATCH_PTR_STORAGE(name)                                        \
	static inline bool name##_patch(const DTTR_ComponentContext *ctx) {                  \
		name##_site = INTERNAL_DTTR_INTEROP_RESOLVE_SIG(ctx, sig, mask, expr);           \
		if (!INTERNAL_DTTR_INTEROP_VALIDATE_AND_LOG(ctx, name##_site, #name)) {          \
			name##_site = 0;                                                             \
			return false;                                                                \
		}                                                                                \
		uint8_t orig_bytes[sizeof(void *)];                                              \
		uintptr_t new_val = (uintptr_t)(target);                                         \
		if (!ctx->m_game_api->m_patch_write(                                             \
				name##_site,                                                             \
				(const uint8_t *)&new_val,                                               \
				sizeof(void *),                                                          \
				orig_bytes                                                               \
			)) {                                                                         \
			DTTR_LOG_ERROR(ctx, "%s: patch_write failed", #name);                        \
			name##_site = 0;                                                             \
			return false;                                                                \
		}                                                                                \
		memcpy(&name##_orig, orig_bytes, sizeof(void *));                                \
		return true;                                                                     \
	}                                                                                    \
	static inline void name##_unpatch(const DTTR_ComponentContext *ctx) {                \
		if (name##_site) {                                                               \
			ctx->m_game_api->m_patch_restore(                                            \
				name##_site,                                                             \
				(const uint8_t *)&name##_orig,                                           \
				sizeof(void *)                                                           \
			);                                                                           \
			name##_site = 0;                                                             \
		}                                                                                \
	}                                                                                    \
	static inline void *name##_orig_ptr(void) { return (void *)name##_orig; }

// Overwrite arbitrary bytes at a signature-matched site, saving originals for restore.
// Generates name_patch(ctx, bytes) and name_unpatch(ctx).
#define DTTR_INTEROP_PATCH_BYTES_SIG(name, sig, mask, expr, size)                        \
	INTERNAL_DTTR_INTEROP_PATCH_BYTES_STORAGE(name, size)                                \
	static inline bool name##_patch(                                                     \
		const DTTR_ComponentContext *ctx,                                                \
		const uint8_t *bytes                                                             \
	) {                                                                                  \
		name##_site = INTERNAL_DTTR_INTEROP_RESOLVE_SIG(ctx, sig, mask, expr);           \
		if (!INTERNAL_DTTR_INTEROP_VALIDATE_AND_LOG(ctx, name##_site, #name)) {          \
			name##_site = 0;                                                             \
			return false;                                                                \
		}                                                                                \
		if (!ctx->m_game_api->m_patch_write(name##_site, bytes, size, name##_orig)) {    \
			DTTR_LOG_ERROR(ctx, "%s: patch_write failed", #name);                        \
			name##_site = 0;                                                             \
			return false;                                                                \
		}                                                                                \
		return true;                                                                     \
	}                                                                                    \
	static inline void name##_unpatch(const DTTR_ComponentContext *ctx) {                \
		if (name##_site) {                                                               \
			ctx->m_game_api->m_patch_restore(name##_site, name##_orig, size);            \
			name##_site = 0;                                                             \
		}                                                                                \
	}

// Optional variant of PATCH_BYTES_SIG that silently skips if the signature is not found.
#define DTTR_INTEROP_PATCH_BYTES_OPTIONAL_SIG(name, sig, mask, expr, size)               \
	DTTR_INTEROP_PATCH_BYTES_SIG(name, sig, mask, expr, size)                            \
	static inline bool name##_install(                                                   \
		const DTTR_ComponentContext *ctx,                                                \
		const uint8_t *bytes                                                             \
	) {                                                                                  \
		if (!ctx->m_game_api->m_sigscan(ctx->m_game_module, sig, mask)) {                \
			return false;                                                                \
		}                                                                                \
		return name##_patch(ctx, bytes);                                                 \
	}

// Pre-check signature before calling _patch; silently returns false if not found.
#define INTERNAL_DTTR_INTEROP_OPTIONAL_INSTALL_SIG(name, sig, mask)                      \
	static inline bool name##_install(const DTTR_ComponentContext *ctx) {                \
		if (!ctx->m_game_api->m_sigscan(ctx->m_game_module, sig, mask)) {                \
			return false;                                                                \
		}                                                                                \
		return name##_patch(ctx);                                                        \
	}

// Generates name_install(ctx), name_patch(ctx), and name_unpatch(ctx).
#define DTTR_INTEROP_HOOK_FUNC_SIG(name, sig, mask, expr, target)                        \
	DTTR_INTEROP_PATCH_JMP_SIG(name, sig, mask, expr, target)                            \
	static inline bool name##_install(const DTTR_ComponentContext *ctx) {                \
		return name##_patch(ctx);                                                        \
	}

// Optional variant of HOOK_FUNC_SIG that silently skips if the signature is not found.
#define DTTR_INTEROP_HOOK_FUNC_OPTIONAL_SIG(name, sig, mask, expr, target)               \
	DTTR_INTEROP_PATCH_JMP_SIG(name, sig, mask, expr, target)                            \
	INTERNAL_DTTR_INTEROP_OPTIONAL_INSTALL_SIG(name, sig, mask)

// Hook a function with a trampoline for calling the original.
// Copies n bytes (must be instruction-aligned) and relocates E8/E9 relative branches.
// Provides name##_trampoline for casting to the original function signature.
// Generates name_install(ctx) and name_unpatch(ctx).
#define DTTR_INTEROP_HOOK_FUNC_TRAMPOLINE_SIG(name, n, sig, mask, expr, target)          \
	INTERNAL_DTTR_INTEROP_HOOK_TRAMPOLINE_STORAGE(name)                                  \
	static inline bool name##_install(const DTTR_ComponentContext *ctx) {                \
		name##_site = INTERNAL_DTTR_INTEROP_RESOLVE_SIG(ctx, sig, mask, expr);           \
		if (!INTERNAL_DTTR_INTEROP_VALIDATE_AND_LOG(ctx, name##_site, #name)) {          \
			name##_site = 0;                                                             \
			return false;                                                                \
		}                                                                                \
		name##_trampoline = (uint8_t *)VirtualAlloc(                                     \
			NULL,                                                                        \
			(n) + 5,                                                                     \
			MEM_COMMIT | MEM_RESERVE,                                                    \
			PAGE_EXECUTE_READWRITE                                                       \
		);                                                                               \
		if (!name##_trampoline) {                                                        \
			DTTR_LOG_ERROR(ctx, "%s: trampoline VirtualAlloc failed", #name);            \
			name##_site = 0;                                                             \
			return false;                                                                \
		}                                                                                \
		memcpy(name##_trampoline, (void *)name##_site, (n));                             \
		s_trampoline_relocate(name##_trampoline, name##_site, (n));                      \
		name##_trampoline[(n)] = 0xE9;                                                   \
		*(int32_t *)(name##_trampoline + (n) + 1) = (int32_t)((name##_site + (n))        \
															  - ((uintptr_t)             \
																	 name##_trampoline   \
																 + (n) + 5));            \
		uint8_t buf[5];                                                                  \
		buf[0] = 0xE9;                                                                   \
		int32_t rel = (int32_t)((uintptr_t)(target) - (name##_site + 5));                \
		memcpy(buf + 1, &rel, 4);                                                        \
		if (!ctx->m_game_api->m_patch_write(name##_site, buf, 5, name##_orig)) {         \
			DTTR_LOG_ERROR(ctx, "%s: patch_write failed", #name);                        \
			VirtualFree(name##_trampoline, 0, MEM_RELEASE);                              \
			name##_trampoline = NULL;                                                    \
			name##_site = 0;                                                             \
			return false;                                                                \
		}                                                                                \
		return true;                                                                     \
	}                                                                                    \
	static inline void name##_unpatch(const DTTR_ComponentContext *ctx) {                \
		(void)ctx;                                                                       \
		if (name##_site) {                                                               \
			ctx->m_game_api->m_patch_restore(name##_site, name##_orig, 5);               \
			name##_site = 0;                                                             \
		}                                                                                \
		if (name##_trampoline) {                                                         \
			VirtualFree(name##_trampoline, 0, MEM_RELEASE);                              \
			name##_trampoline = NULL;                                                    \
		}                                                                                \
	}

// Optional variant that returns false silently if the signature is not found.
#define DTTR_INTEROP_HOOK_FUNC_TRAMPOLINE_OPTIONAL_SIG(name, n, sig, mask, expr, target) \
	INTERNAL_DTTR_INTEROP_HOOK_TRAMPOLINE_STORAGE(name)                                  \
	static inline bool name##_install(const DTTR_ComponentContext *ctx) {                \
		name##_site = INTERNAL_DTTR_INTEROP_RESOLVE_SIG(ctx, sig, mask, expr);           \
		if (!name##_site) {                                                              \
			return false;                                                                \
		}                                                                                \
		name##_trampoline = (uint8_t *)VirtualAlloc(                                     \
			NULL,                                                                        \
			(n) + 5,                                                                     \
			MEM_COMMIT | MEM_RESERVE,                                                    \
			PAGE_EXECUTE_READWRITE                                                       \
		);                                                                               \
		if (!name##_trampoline) {                                                        \
			name##_site = 0;                                                             \
			return false;                                                                \
		}                                                                                \
		memcpy(name##_trampoline, (void *)name##_site, (n));                             \
		s_trampoline_relocate(name##_trampoline, name##_site, (n));                      \
		name##_trampoline[(n)] = 0xE9;                                                   \
		*(int32_t *)(name##_trampoline + (n) + 1) = (int32_t)((name##_site + (n))        \
															  - ((uintptr_t)             \
																	 name##_trampoline   \
																 + (n) + 5));            \
		uint8_t buf[5];                                                                  \
		buf[0] = 0xE9;                                                                   \
		int32_t rel = (int32_t)((uintptr_t)(target) - (name##_site + 5));                \
		memcpy(buf + 1, &rel, 4);                                                        \
		if (!ctx->m_game_api->m_patch_write(name##_site, buf, 5, name##_orig)) {         \
			VirtualFree(name##_trampoline, 0, MEM_RELEASE);                              \
			name##_trampoline = NULL;                                                    \
			name##_site = 0;                                                             \
			return false;                                                                \
		}                                                                                \
		return true;                                                                     \
	}                                                                                    \
	static inline void name##_unpatch(const DTTR_ComponentContext *ctx) {                \
		(void)ctx;                                                                       \
		if (name##_site) {                                                               \
			ctx->m_game_api->m_patch_restore(name##_site, name##_orig, 5);               \
			name##_site = 0;                                                             \
		}                                                                                \
		if (name##_trampoline) {                                                         \
			VirtualFree(name##_trampoline, 0, MEM_RELEASE);                              \
			name##_trampoline = NULL;                                                    \
		}                                                                                \
	}

// Patch a COM vtable entry at runtime via VirtualProtect.
#define DTTR_INTEROP_HOOK_COM(name, method_offset, hook_fn)                              \
	INTERNAL_DTTR_INTEROP_HOOK_COM_STORAGE(name)                                         \
	static inline void name##_install(void **comObject) {                                \
		void **dispatch = *(void ***)comObject;                                          \
		name##_entry = (void **)((uintptr_t)dispatch + (method_offset));                 \
		DWORD old;                                                                       \
		VirtualProtect(name##_entry, sizeof(void *), PAGE_EXECUTE_READWRITE, &old);      \
		name##_original = *name##_entry;                                                 \
		*name##_entry = (void *)(hook_fn);                                               \
		VirtualProtect(name##_entry, sizeof(void *), old, &old);                         \
	}                                                                                    \
	static inline void name##_remove(void) {                                             \
		if (name##_entry && name##_original) {                                           \
			DWORD old;                                                                   \
			VirtualProtect(name##_entry, sizeof(void *), PAGE_EXECUTE_READWRITE, &old);  \
			*name##_entry = name##_original;                                             \
			VirtualProtect(name##_entry, sizeof(void *), old, &old);                     \
		}                                                                                \
	}

// Bootstrap hook using HMODULE directly, for use in DllMain before DTTR_ComponentContext
// exists. Patches via VirtualProtect.
#define DTTR_INTEROP_HOOK_FUNC_RAW_SIG(name, sig, mask, expr, target)                    \
	INTERNAL_DTTR_INTEROP_HOOK_STORAGE(name)                                             \
	static inline void name##_install(HMODULE mod) {                                     \
		const uintptr_t site = (__extension__({                                          \
			const uintptr_t match = dttr_interop_sigscan((mod), (sig), (mask));          \
			match ? (uintptr_t)(expr) : 0;                                               \
		}));                                                                             \
		if (!dttr_interop_validate_resolve(mod, site, #name)) {                          \
			return;                                                                      \
		}                                                                                \
		name##_site = site;                                                              \
		DWORD old;                                                                       \
		VirtualProtect((void *)name##_site, 5, PAGE_EXECUTE_READWRITE, &old);            \
		memcpy(name##_orig, (void *)name##_site, 5);                                     \
		*(uint8_t *)name##_site = 0xE9;                                                  \
		*(int32_t *)(name##_site + 1) = (int32_t)((uintptr_t)(target)                    \
												  - (name##_site + 5));                  \
		VirtualProtect((void *)name##_site, 5, old, &old);                               \
	}                                                                                    \
	static inline void name##_unpatch(void) {                                            \
		if (name##_site) {                                                               \
			DWORD old;                                                                   \
			VirtualProtect((void *)name##_site, 5, PAGE_EXECUTE_READWRITE, &old);        \
			memcpy((void *)name##_site, name##_orig, 5);                                 \
			VirtualProtect((void *)name##_site, 5, old, &old);                           \
		}                                                                                \
	}

#ifdef __cplusplus
}
#endif

#endif // DTTR_COMMON_ITEROP_H
