#ifndef DTTR_COMMON_ITEROP_H
#define DTTR_COMMON_ITEROP_H

// This header provides callable wrappers for functions and variables at known offsets within a module

#include <stdint.h>
#include <string.h>
#include <windows.h>
#include <psapi.h>
#include <log.h>

#ifdef __cplusplus
extern "C" {
#endif

// Scan a module for a byte pattern with mask and return the address of the first match
// sig is a byte string, mask uses 'x' for exact match and '?' for wildcard
// Returns the absolute address of the match, or 0 if not found
static inline uintptr_t dttr_interop_sigscan(
	HMODULE mod, const char *sig, const char *mask
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
		for (size_t j = 0; ; j++) {
			if (j == len) return (uintptr_t)(base + i);
			if (mask[j] == 'x' && base[i + j] != (uint8_t)sig[j]) break;
		}
	}
	return 0;
}

// Checks that a resolved address is non-zero and falls within the module's address range
static inline int dttr_interop_validate_resolve(
	HMODULE mod, uintptr_t addr, const char *name
) {
	if (!addr) {
		log_error("%s: resolved to NULL", name);
		return 0;
	}
	MODULEINFO mi;
	if (GetModuleInformation(GetCurrentProcess(), mod, &mi, sizeof(mi))) {
		const uintptr_t mod_base = (uintptr_t)mi.lpBaseOfDll;
		const uintptr_t mod_end = mod_base + mi.SizeOfImage;
		if (addr < mod_base || addr >= mod_end) {
			log_error(
				"%s: resolved to 0x%08X (outside module 0x%08X-0x%08X)",
				name, (unsigned)addr, (unsigned)mod_base, (unsigned)mod_end
			);
			return 0;
		}
	}
	return 1;
}

// Validates and logs a resolved address, expanding log_debug at the call site for correct __FILE__ and __LINE__
#define INTERNAL_DTTR_INTEROP_VALIDATE_AND_LOG(mod, addr, name) \
	(dttr_interop_validate_resolve(mod, addr, name) ? \
		(log_debug("Resolved %s at 0x%08X", name, (unsigned)(addr)), 1) : 0)

// Address resolution helpers used internally by the macros below
// Resolves an E8 relative call at address p to its absolute target
#define DTTR_E8_TARGET(p) ((p) + 5 + *(int32_t *)((p) + 1))

// Reads the absolute jump target from an FF 25 import thunk at address p
#define DTTR_FF25_ADDR(p) (*(uint32_t *)((p) + 2))

// RESOLVE_OFFSET computes an absolute address from a module-relative offset
// RESOLVE_SIG scans for a byte pattern, binds the match address to `match`, and evaluates expr
#define INTERNAL_DTTR_INTEROP_RESOLVE_OFFSET(mod, offset) ((uintptr_t)(mod) + (offset))
#define INTERNAL_DTTR_INTEROP_RESOLVE_SIG(mod, sig, mask, expr) \
	(__extension__({ \
		const uintptr_t match = dttr_interop_sigscan((mod), (sig), (mask)); \
		match ? (uintptr_t)(expr) : 0; \
	}))

// Wraps a function at an offset from the module base
// First parameter of the generated function is always HMODULE
#define DTTR_INTEROP_WRAP(name, offset, ret, params, args) \
	typedef ret(*name##_fn_t) params; \
	static inline ret name params { \
		return ((name##_fn_t)((uintptr_t)mod + (offset))) args; \
	}

// Works the same as WRAP but uses an explicit calling convention
#define DTTR_INTEROP_WRAP_CC(name, cc, offset, ret, params, args) \
	typedef ret(cc *name##_fn_t) params; \
	static inline ret name params { \
		return ((name##_fn_t)((uintptr_t)mod + (offset))) args; \
	}

// Wraps a function with cached address resolution
// Call name_init(module) once, then call name() without passing module
// Define DTTR_INTEROP_IMPLEMENT in exactly one .c file to provide storage
#ifdef DTTR_INTEROP_IMPLEMENT
#define INTERNAL_DTTR_INTEROP_WRAP_CACHED_STORAGE(name) uintptr_t name##_addr = 0;
#else
#define INTERNAL_DTTR_INTEROP_WRAP_CACHED_STORAGE(name) extern uintptr_t name##_addr;
#endif

#define INTERNAL_DTTR_INTEROP_WRAP_CACHED_IMPL(name, resolve, ret, params, args) \
	typedef ret(*name##_fn_t) params; \
	INTERNAL_DTTR_INTEROP_WRAP_CACHED_STORAGE(name) \
	static inline void name##_init(HMODULE mod) { \
		name##_addr = (resolve); \
		if (!INTERNAL_DTTR_INTEROP_VALIDATE_AND_LOG(mod, name##_addr, #name)) { \
			name##_addr = 0; \
		} \
	} \
	static inline ret name params { return ((name##_fn_t)name##_addr)args; }

#define DTTR_INTEROP_WRAP_CACHED(name, offset, ret, params, args) \
	INTERNAL_DTTR_INTEROP_WRAP_CACHED_IMPL(name, INTERNAL_DTTR_INTEROP_RESOLVE_OFFSET(mod, offset), ret, params, args)

#define DTTR_INTEROP_WRAP_CACHED_SIG(name, sig, mask, expr, ret, params, args) \
	INTERNAL_DTTR_INTEROP_WRAP_CACHED_IMPL(name, INTERNAL_DTTR_INTEROP_RESOLVE_SIG(mod, sig, mask, expr), ret, params, args)

// Wraps a function with cached address resolution and an explicit calling convention
#define INTERNAL_DTTR_INTEROP_WRAP_CACHED_CC_IMPL(name, cc, resolve, ret, params, args) \
	typedef ret(cc *name##_fn_t) params; \
	INTERNAL_DTTR_INTEROP_WRAP_CACHED_STORAGE(name) \
	static inline void name##_init(HMODULE mod) { \
		name##_addr = (resolve); \
		if (!INTERNAL_DTTR_INTEROP_VALIDATE_AND_LOG(mod, name##_addr, #name)) { \
			name##_addr = 0; \
		} \
	} \
	static inline ret name params { return ((name##_fn_t)name##_addr)args; }

#define DTTR_INTEROP_WRAP_CACHED_CC(name, cc, offset, ret, params, args) \
	INTERNAL_DTTR_INTEROP_WRAP_CACHED_CC_IMPL(name, cc, INTERNAL_DTTR_INTEROP_RESOLVE_OFFSET(mod, offset), ret, params, args)

#define DTTR_INTEROP_WRAP_CACHED_CC_SIG(name, cc, sig, mask, expr, ret, params, args) \
	INTERNAL_DTTR_INTEROP_WRAP_CACHED_CC_IMPL(name, cc, INTERNAL_DTTR_INTEROP_RESOLVE_SIG(mod, sig, mask, expr), ret, params, args)

// Like WRAP_CACHED_CC_SIG but for optional functions that may not exist in all game versions
// name_init(mod) warns instead of erroring when the sig is not found
// Callers must check name##_addr before calling name()
#define INTERNAL_DTTR_INTEROP_WRAP_CACHED_CC_OPTIONAL_IMPL(name, cc, resolve, ret, params, args) \
	typedef ret(cc *name##_fn_t) params; \
	INTERNAL_DTTR_INTEROP_WRAP_CACHED_STORAGE(name) \
	static inline void name##_init(HMODULE mod) { \
		name##_addr = (resolve); \
		if (!name##_addr) { \
			log_warn("Skipped optional %s (not found)", #name); \
			return; \
		} \
		if (!INTERNAL_DTTR_INTEROP_VALIDATE_AND_LOG(mod, name##_addr, #name)) { \
			name##_addr = 0; \
		} \
	} \
	static inline ret name params { return ((name##_fn_t)name##_addr)args; }

#define DTTR_INTEROP_WRAP_CACHED_CC_OPTIONAL_SIG(name, cc, sig, mask, expr, ret, params, args) \
	INTERNAL_DTTR_INTEROP_WRAP_CACHED_CC_OPTIONAL_IMPL(name, cc, INTERNAL_DTTR_INTEROP_RESOLVE_SIG(mod, sig, mask, expr), ret, params, args)

// Provides accessor functions for a module global variable
// name_init(mod), name_ptr(), name_get(), name_set(val)
#ifdef DTTR_INTEROP_IMPLEMENT
#define INTERNAL_DTTR_INTEROP_VAR_STORAGE(name) uintptr_t name##_addr = 0;
#else
#define INTERNAL_DTTR_INTEROP_VAR_STORAGE(name) extern uintptr_t name##_addr;
#endif

#define INTERNAL_DTTR_INTEROP_VAR_IMPL(name, type, resolve) \
	INTERNAL_DTTR_INTEROP_VAR_STORAGE(name) \
	static inline void name##_init(HMODULE mod) { \
		name##_addr = (resolve); \
		if (!INTERNAL_DTTR_INTEROP_VALIDATE_AND_LOG(mod, name##_addr, #name)) { \
			name##_addr = 0; \
		} \
	} \
	static inline type *name##_ptr(void) { return (type *)name##_addr; } \
	static inline type name##_get(void) { return *(type *)name##_addr; } \
	static inline void name##_set(type val) { *(type *)name##_addr = val; }

#define DTTR_INTEROP_VAR(name, type, offset) \
	INTERNAL_DTTR_INTEROP_VAR_IMPL(name, type, INTERNAL_DTTR_INTEROP_RESOLVE_OFFSET(mod, offset))

#define DTTR_INTEROP_VAR_SIG(name, type, sig, mask, expr) \
	INTERNAL_DTTR_INTEROP_VAR_IMPL(name, type, INTERNAL_DTTR_INTEROP_RESOLVE_SIG(mod, sig, mask, expr))

// These macros patch code to redirect CALL/JMP instructions and pointers
// The instruction opcodes are E8 for CALL rel32, E9 for JMP rel32, EB for JMP rel8, FF15 for CALL [addr], and FF25 for JMP [addr]

// Patch a relative CALL (E8) instruction
// name_patch(mod), name_unpatch()
#ifdef DTTR_INTEROP_IMPLEMENT
#define INTERNAL_DTTR_INTEROP_PATCH_CALL_STORAGE(name) \
	uintptr_t name##_site = 0; \
	uint8_t name##_orig[5] = {0};
#else
#define INTERNAL_DTTR_INTEROP_PATCH_CALL_STORAGE(name) \
	extern uintptr_t name##_site; \
	extern uint8_t name##_orig[5];
#endif

#define DTTR_INTEROP_HOOK_FUNC_LOG(hook, mod) \
	do { \
		hook##_install(mod); \
		log_debug( \
			"Applied HOOK_FUNC %s at 0x%08X", #hook, \
			(unsigned)hook##_site \
		); \
	} while (0)

// Generic optional install+log for hooks/patches that may not exist in all game versions
// Calls _install, then logs success or skip based on _site
#define DTTR_INTEROP_OPTIONAL_LOG(hook, mod, type_name) \
	do { \
		hook##_install(mod); \
		if (hook##_site) { \
			log_debug("Applied " type_name " %s at 0x%08X", #hook, (unsigned)hook##_site); \
		} else { \
			log_warn("Skipped optional " type_name " %s (not found)", #hook); \
		} \
	} while (0)

#define DTTR_INTEROP_HOOK_FUNC_OPTIONAL_LOG(hook, mod) DTTR_INTEROP_OPTIONAL_LOG(hook, mod, "HOOK_FUNC")
#define DTTR_INTEROP_PATCH_CALL_OPTIONAL_LOG(hook, mod) DTTR_INTEROP_OPTIONAL_LOG(hook, mod, "PATCH_CALL")
#define DTTR_INTEROP_PATCH_PTR_OPTIONAL_LOG(hook, mod) DTTR_INTEROP_OPTIONAL_LOG(hook, mod, "PATCH_PTR")

#define DTTR_INTEROP_HOOK_FUNC_TRAMPOLINE_LOG(hook, mod) \
	do { \
		hook##_install(mod); \
		if (hook##_trampoline) { \
			log_debug( \
				"Applied HOOK_FUNC %s at 0x%08X", #hook, \
				(unsigned)hook##_site \
			); \
		} else { \
			log_error( \
				"Failed to allocate trampoline for %s", #hook \
			); \
		} \
	} while (0)

#define DTTR_INTEROP_HOOK_FUNC_TRAMPOLINE_OPTIONAL_LOG(hook, mod) \
	do { \
		hook##_install(mod); \
		if (hook##_trampoline) { \
			log_debug( \
				"Applied HOOK_FUNC %s at 0x%08X", #hook, \
				(unsigned)hook##_site \
			); \
		} else { \
			log_warn( \
				"Skipped optional HOOK_FUNC %s (not found)", #hook \
			); \
		} \
	} while (0)

#define DTTR_INTEROP_PATCH_CALL_LOG(hook, mod) \
	do { \
		hook##_patch(mod); \
		log_debug( \
			"Applied PATCH_CALL %s at 0x%08X", #hook, \
			(unsigned)hook##_site \
		); \
	} while (0)

#define DTTR_INTEROP_PATCH_PTR_LOG(hook, mod) \
	do { \
		hook##_patch(mod); \
		log_debug( \
			"Applied PATCH_PTR %s at 0x%08X", #hook, \
			(unsigned)hook##_site \
		); \
	} while (0)

#define DTTR_INTEROP_UNHOOK_LOG(hook) \
	do { \
		log_debug( \
			"Removed %s at 0x%08X", #hook, (unsigned)hook##_site \
		); \
		hook##_unpatch(); \
	} while (0)

#define DTTR_INTEROP_UNHOOK_OPTIONAL_LOG(hook) \
	do { \
		if (hook##_site) { \
			log_debug( \
				"Removed %s at 0x%08X", #hook, (unsigned)hook##_site \
			); \
			hook##_unpatch(); \
		} \
	} while (0)

#define INTERNAL_DTTR_INTEROP_PATCH_CALL_IMPL(name, resolve, target) \
	INTERNAL_DTTR_INTEROP_PATCH_CALL_STORAGE(name) \
	static inline void name##_patch(HMODULE mod) { \
		name##_site = (resolve); \
		if (!dttr_interop_validate_resolve(mod, name##_site, #name)) { \
			name##_site = 0; return; \
		} \
		DWORD old; \
		VirtualProtect((void *)name##_site, 5, PAGE_EXECUTE_READWRITE, &old); \
		memcpy(name##_orig, (void *)name##_site, 5); \
		*(uint8_t *)name##_site = 0xE8; \
		*(int32_t *)(name##_site + 1) = \
			(int32_t)((uintptr_t)(target) - (name##_site + 5)); \
		VirtualProtect((void *)name##_site, 5, old, &old); \
	} \
	static inline void name##_unpatch(void) { \
		if (name##_site) { \
			DWORD old; \
			VirtualProtect((void *)name##_site, 5, PAGE_EXECUTE_READWRITE, &old); \
			memcpy((void *)name##_site, name##_orig, 5); \
			VirtualProtect((void *)name##_site, 5, old, &old); \
		} \
	}

#define DTTR_INTEROP_PATCH_CALL(name, call_addr, target) \
	INTERNAL_DTTR_INTEROP_PATCH_CALL_IMPL(name, INTERNAL_DTTR_INTEROP_RESOLVE_OFFSET(mod, call_addr), target)

#define DTTR_INTEROP_PATCH_CALL_SIG(name, sig, mask, expr, target) \
	INTERNAL_DTTR_INTEROP_PATCH_CALL_IMPL(name, INTERNAL_DTTR_INTEROP_RESOLVE_SIG(mod, sig, mask, expr), target)

// Shared _install wrapper for optional SIG variants
// Pre-checks the signature before calling _patch, silently returning if not found
#define INTERNAL_DTTR_INTEROP_OPTIONAL_INSTALL_SIG(name, sig, mask) \
	static inline void name##_install(HMODULE mod) { \
		if (!dttr_interop_sigscan(mod, sig, mask)) { return; } \
		name##_patch(mod); \
	}

// Like PATCH_CALL_SIG but for optional patches that may not exist in all game versions
#define DTTR_INTEROP_PATCH_CALL_OPTIONAL_SIG(name, sig, mask, expr, target) \
	DTTR_INTEROP_PATCH_CALL_SIG(name, sig, mask, expr, target) \
	INTERNAL_DTTR_INTEROP_OPTIONAL_INSTALL_SIG(name, sig, mask)

// Works the same as PATCH_CALL but writes E9 (JMP) instead of E8
#ifdef DTTR_INTEROP_IMPLEMENT
#define INTERNAL_DTTR_INTEROP_PATCH_JMP_STORAGE(name) \
	uintptr_t name##_site = 0; \
	uint8_t name##_orig[5] = {0};
#else
#define INTERNAL_DTTR_INTEROP_PATCH_JMP_STORAGE(name) \
	extern uintptr_t name##_site; \
	extern uint8_t name##_orig[5];
#endif

#define INTERNAL_DTTR_INTEROP_PATCH_JMP_IMPL(name, resolve, target) \
	INTERNAL_DTTR_INTEROP_PATCH_JMP_STORAGE(name) \
	static inline void name##_patch(HMODULE mod) { \
		name##_site = (resolve); \
		if (!dttr_interop_validate_resolve(mod, name##_site, #name)) { \
			name##_site = 0; return; \
		} \
		DWORD old; \
		VirtualProtect((void *)name##_site, 5, PAGE_EXECUTE_READWRITE, &old); \
		memcpy(name##_orig, (void *)name##_site, 5); \
		*(uint8_t *)name##_site = 0xE9; \
		*(int32_t *)(name##_site + 1) = \
			(int32_t)((uintptr_t)(target) - (name##_site + 5)); \
		VirtualProtect((void *)name##_site, 5, old, &old); \
	} \
	static inline void name##_unpatch(void) { \
		if (name##_site) { \
			DWORD old; \
			VirtualProtect((void *)name##_site, 5, PAGE_EXECUTE_READWRITE, &old); \
			memcpy((void *)name##_site, name##_orig, 5); \
			VirtualProtect((void *)name##_site, 5, old, &old); \
		} \
	}

#define DTTR_INTEROP_PATCH_JMP(name, jmp_addr, target) \
	INTERNAL_DTTR_INTEROP_PATCH_JMP_IMPL(name, INTERNAL_DTTR_INTEROP_RESOLVE_OFFSET(mod, jmp_addr), target)

#define DTTR_INTEROP_PATCH_JMP_SIG(name, sig, mask, expr, target) \
	INTERNAL_DTTR_INTEROP_PATCH_JMP_IMPL(name, INTERNAL_DTTR_INTEROP_RESOLVE_SIG(mod, sig, mask, expr), target)

// Patch a pointer/address (e.g. vtable entry, IAT)
// name_patch(mod), name_unpatch(), name_orig_ptr()
#ifdef DTTR_INTEROP_IMPLEMENT
#define INTERNAL_DTTR_INTEROP_PATCH_PTR_STORAGE(name) \
	uintptr_t name##_site = 0; \
	uintptr_t name##_orig = 0;
#else
#define INTERNAL_DTTR_INTEROP_PATCH_PTR_STORAGE(name) \
	extern uintptr_t name##_site; \
	extern uintptr_t name##_orig;
#endif

#define INTERNAL_DTTR_INTEROP_PATCH_PTR_IMPL(name, resolve, target) \
	INTERNAL_DTTR_INTEROP_PATCH_PTR_STORAGE(name) \
	static inline void name##_patch(HMODULE mod) { \
		name##_site = (resolve); \
		if (!dttr_interop_validate_resolve(mod, name##_site, #name)) { \
			name##_site = 0; return; \
		} \
		DWORD old; \
		VirtualProtect( \
			(void *)name##_site, sizeof(void *), PAGE_EXECUTE_READWRITE, &old \
		); \
		name##_orig = *(uintptr_t *)name##_site; \
		*(uintptr_t *)name##_site = (uintptr_t)(target); \
		VirtualProtect((void *)name##_site, sizeof(void *), old, &old); \
	} \
	static inline void name##_unpatch(void) { \
		if (name##_site) { \
			DWORD old; \
			VirtualProtect( \
				(void *)name##_site, sizeof(void *), PAGE_EXECUTE_READWRITE, &old \
			); \
			*(uintptr_t *)name##_site = name##_orig; \
			VirtualProtect((void *)name##_site, sizeof(void *), old, &old); \
		} \
	} \
	static inline void *name##_orig_ptr(void) { return (void *)name##_orig; }

#define DTTR_INTEROP_PATCH_PTR(name, ptr_addr, target) \
	INTERNAL_DTTR_INTEROP_PATCH_PTR_IMPL(name, INTERNAL_DTTR_INTEROP_RESOLVE_OFFSET(mod, ptr_addr), target)

#define DTTR_INTEROP_PATCH_PTR_SIG(name, sig, mask, expr, target) \
	INTERNAL_DTTR_INTEROP_PATCH_PTR_IMPL(name, INTERNAL_DTTR_INTEROP_RESOLVE_SIG(mod, sig, mask, expr), target)

// Like PATCH_PTR_SIG but for optional patches that may not exist in all game versions
#define DTTR_INTEROP_PATCH_PTR_OPTIONAL_SIG(name, sig, mask, expr, target) \
	DTTR_INTEROP_PATCH_PTR_SIG(name, sig, mask, expr, target) \
	INTERNAL_DTTR_INTEROP_OPTIONAL_INSTALL_SIG(name, sig, mask)

// Overwrites `size` bytes at a fixed offset with caller-supplied content, saving the originals for restoration
// Generates name_patch(mod, bytes) to apply and name_unpatch() to restore
#ifdef DTTR_INTEROP_IMPLEMENT
#define INTERNAL_DTTR_INTEROP_PATCH_BYTES_STORAGE(name, size) \
	uintptr_t name##_site = 0; \
	uint8_t name##_orig[size] = {0};
#else
#define INTERNAL_DTTR_INTEROP_PATCH_BYTES_STORAGE(name, size) \
	extern uintptr_t name##_site; \
	extern uint8_t name##_orig[size];
#endif

#define INTERNAL_DTTR_INTEROP_PATCH_BYTES_IMPL(name, resolve, size) \
	INTERNAL_DTTR_INTEROP_PATCH_BYTES_STORAGE(name, size) \
	static inline void name##_patch(HMODULE mod, const uint8_t *bytes) { \
		name##_site = (resolve); \
		if (!dttr_interop_validate_resolve(mod, name##_site, #name)) { \
			name##_site = 0; return; \
		} \
		DWORD old; \
		VirtualProtect((void *)name##_site, size, PAGE_EXECUTE_READWRITE, &old); \
		memcpy(name##_orig, (void *)name##_site, size); \
		memcpy((void *)name##_site, bytes, size); \
		VirtualProtect((void *)name##_site, size, old, &old); \
	} \
	static inline void name##_unpatch(void) { \
		if (name##_site) { \
			DWORD old; \
			VirtualProtect( \
				(void *)name##_site, size, PAGE_EXECUTE_READWRITE, &old \
			); \
			memcpy((void *)name##_site, name##_orig, size); \
			VirtualProtect((void *)name##_site, size, old, &old); \
		} \
	}

#define DTTR_INTEROP_PATCH_BYTES(name, addr, size) \
	INTERNAL_DTTR_INTEROP_PATCH_BYTES_IMPL(name, INTERNAL_DTTR_INTEROP_RESOLVE_OFFSET(mod, addr), size)

#define DTTR_INTEROP_PATCH_BYTES_SIG(name, sig, mask, expr, size) \
	INTERNAL_DTTR_INTEROP_PATCH_BYTES_IMPL(name, INTERNAL_DTTR_INTEROP_RESOLVE_SIG(mod, sig, mask, expr), size)

// Like PATCH_BYTES_SIG but for optional patches that may not exist in all game versions
// Adds _install that silently returns if the signature is not found
#define DTTR_INTEROP_PATCH_BYTES_OPTIONAL_SIG(name, sig, mask, expr, size) \
	DTTR_INTEROP_PATCH_BYTES_SIG(name, sig, mask, expr, size) \
	static inline void name##_install(HMODULE mod, const uint8_t *bytes) { \
		if (!dttr_interop_sigscan(mod, sig, mask)) { return; } \
		name##_patch(mod, bytes); \
	}

// Replace entire function by writing a JMP at its start
#define DTTR_INTEROP_HOOK_FUNC(name, func_addr, target) \
	DTTR_INTEROP_PATCH_JMP(name, func_addr, target) \
	static inline void name##_install(HMODULE mod) { name##_patch(mod); }

#define DTTR_INTEROP_HOOK_FUNC_SIG(name, sig, mask, expr, target) \
	DTTR_INTEROP_PATCH_JMP_SIG(name, sig, mask, expr, target) \
	static inline void name##_install(HMODULE mod) { name##_patch(mod); }

// Like HOOK_FUNC_SIG but for optional hooks that may not exist in all game versions
#define DTTR_INTEROP_HOOK_FUNC_OPTIONAL_SIG(name, sig, mask, expr, target) \
	DTTR_INTEROP_PATCH_JMP_SIG(name, sig, mask, expr, target) \
	INTERNAL_DTTR_INTEROP_OPTIONAL_INSTALL_SIG(name, sig, mask)

// Replace entire function with a JMP and allocate a trampoline for calling through to the original
// Exposes name##_trampoline (uint8_t*) that the callback casts to the original function
#ifdef DTTR_INTEROP_IMPLEMENT
#define INTERNAL_DTTR_INTEROP_HOOK_FUNC_TRAMPOLINE_STORAGE(name) \
	uintptr_t name##_site = 0; \
	uint8_t name##_orig[5] = {0}; \
	uint8_t *name##_trampoline = NULL;
#else
#define INTERNAL_DTTR_INTEROP_HOOK_FUNC_TRAMPOLINE_STORAGE(name) \
	extern uintptr_t name##_site; \
	extern uint8_t name##_orig[5]; \
	extern uint8_t *name##_trampoline;
#endif

#define INTERNAL_DTTR_INTEROP_HOOK_FUNC_TRAMPOLINE_IMPL(name, resolve, target) \
	INTERNAL_DTTR_INTEROP_HOOK_FUNC_TRAMPOLINE_STORAGE(name) \
	static inline void name##_install(HMODULE mod) { \
		const uintptr_t site = (resolve); \
		if (!dttr_interop_validate_resolve(mod, site, #name)) { return; } \
		name##_trampoline = (uint8_t *)VirtualAlloc( \
			NULL, 16, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE \
		); \
		if (!name##_trampoline) { return; } \
		memcpy(name##_trampoline, (void *)site, 5); \
		name##_trampoline[5] = 0xE9; \
		*(int32_t *)(name##_trampoline + 6) = \
			(int32_t)((site + 5) - ((uintptr_t)name##_trampoline + 10)); \
		DWORD old; \
		VirtualProtect((void *)site, 5, PAGE_EXECUTE_READWRITE, &old); \
		memcpy(name##_orig, (void *)site, 5); \
		*(uint8_t *)site = 0xE9; \
		*(int32_t *)(site + 1) = \
			(int32_t)((uintptr_t)(target) - (site + 5)); \
		VirtualProtect((void *)site, 5, old, &old); \
		name##_site = site; \
	} \
	static inline void name##_unpatch(void) { \
		if (name##_site) { \
			DWORD old; \
			VirtualProtect((void *)name##_site, 5, PAGE_EXECUTE_READWRITE, &old); \
			memcpy((void *)name##_site, name##_orig, 5); \
			VirtualProtect((void *)name##_site, 5, old, &old); \
		} \
		if (name##_trampoline) { \
			VirtualFree(name##_trampoline, 0, MEM_RELEASE); \
			name##_trampoline = NULL; \
		} \
	}

#define DTTR_INTEROP_HOOK_FUNC_TRAMPOLINE(name, func_addr, target) \
	INTERNAL_DTTR_INTEROP_HOOK_FUNC_TRAMPOLINE_IMPL(name, INTERNAL_DTTR_INTEROP_RESOLVE_OFFSET(mod, func_addr), target)

#define DTTR_INTEROP_HOOK_FUNC_TRAMPOLINE_SIG(name, sig, mask, expr, target) \
	INTERNAL_DTTR_INTEROP_HOOK_FUNC_TRAMPOLINE_IMPL(name, INTERNAL_DTTR_INTEROP_RESOLVE_SIG(mod, sig, mask, expr), target)

// Like HOOK_FUNC_TRAMPOLINE_SIG but for optional hooks that may not exist in all game versions
// _install silently returns if the signature is not found, avoiding log_error from validate_resolve
#define DTTR_INTEROP_HOOK_FUNC_TRAMPOLINE_OPTIONAL_SIG(name, sig, mask, expr, target) \
	INTERNAL_DTTR_INTEROP_HOOK_FUNC_TRAMPOLINE_IMPL(name, \
		(__extension__({ \
			if (!dttr_interop_sigscan(mod, sig, mask)) return; \
			INTERNAL_DTTR_INTEROP_RESOLVE_SIG(mod, sig, mask, expr); \
		})), \
		target)

// Hook a COM method by patching its vtable entry at runtime
// https://learn.microsoft.com/en-us/windows/win32/prog-dx-with-com
// name_original, name_install(obj), name_remove()
#define DTTR_INTEROP_HOOK_COM(name, method_offset, hook_fn) \
	static void *name##_original = NULL; \
	static void **name##_entry = NULL; \
	static inline void name##_install(void **comObject) { \
		void **dispatch = *(void ***)comObject; \
		name##_entry = (void **)((uintptr_t)dispatch + (method_offset)); \
		DWORD old; \
		VirtualProtect(name##_entry, sizeof(void *), PAGE_EXECUTE_READWRITE, &old); \
		name##_original = *name##_entry; \
		*name##_entry = (void *)(hook_fn); \
		VirtualProtect(name##_entry, sizeof(void *), old, &old); \
	} \
	static inline void name##_remove(void) { \
		if (name##_entry && name##_original) { \
			DWORD old; \
			VirtualProtect( \
				name##_entry, sizeof(void *), PAGE_EXECUTE_READWRITE, &old \
			); \
			*name##_entry = name##_original; \
			VirtualProtect(name##_entry, sizeof(void *), old, &old); \
		} \
	}

#ifdef __cplusplus
}
#endif

#endif // DTTR_COMMON_ITEROP_H
