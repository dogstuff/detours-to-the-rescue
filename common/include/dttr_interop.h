#ifndef DTTR_COMMON_ITEROP_H
#define DTTR_COMMON_ITEROP_H

// This header provides callable wrappers for functions and variables at known offsets within a module

#include <stdint.h>
#include <string.h>
#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

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
#define DTTR_INTEROP_WRAP_CACHED_STORAGE(name) uintptr_t name##_addr = 0;
#else
#define DTTR_INTEROP_WRAP_CACHED_STORAGE(name) extern uintptr_t name##_addr;
#endif

#define DTTR_INTEROP_WRAP_CACHED(name, offset, ret, params, args) \
	typedef ret(*name##_fn_t) params; \
	DTTR_INTEROP_WRAP_CACHED_STORAGE(name) \
	static inline void name##_init(HMODULE mod) { \
		name##_addr = (uintptr_t)mod + (offset); \
	} \
	static inline ret name params { return ((name##_fn_t)name##_addr)args; }

// Wraps a function with cached address resolution and an explicit calling convention
#define DTTR_INTEROP_WRAP_CACHED_CC(name, cc, offset, ret, params, args) \
	typedef ret(cc *name##_fn_t) params; \
	DTTR_INTEROP_WRAP_CACHED_STORAGE(name) \
	static inline void name##_init(HMODULE mod) { \
		name##_addr = (uintptr_t)mod + (offset); \
	} \
	static inline ret name params { return ((name##_fn_t)name##_addr)args; }

// Provides accessor functions for a module global variable
// name_init(mod), name_ptr(), name_get(), name_set(val)
#ifdef DTTR_INTEROP_IMPLEMENT
#define DTTR_INTEROP_VAR_STORAGE(name) uintptr_t name##_addr = 0;
#else
#define DTTR_INTEROP_VAR_STORAGE(name) extern uintptr_t name##_addr;
#endif

#define DTTR_INTEROP_VAR(name, type, offset) \
	DTTR_INTEROP_VAR_STORAGE(name) \
	static inline void name##_init(HMODULE mod) { \
		name##_addr = (uintptr_t)mod + (offset); \
	} \
	static inline type *name##_ptr(void) { return (type *)name##_addr; } \
	static inline type name##_get(void) { return *(type *)name##_addr; } \
	static inline void name##_set(type val) { *(type *)name##_addr = val; }

// These macros patch code to redirect CALL/JMP instructions and pointers
// The instruction opcodes are E8 for CALL rel32, E9 for JMP rel32, EB for JMP rel8, FF15 for CALL [addr], and FF25 for JMP [addr]

// Patch a relative CALL (E8) instruction
// name_patch(mod), name_unpatch()
#ifdef DTTR_INTEROP_IMPLEMENT
#define DTTR_INTEROP_PATCH_CALL_STORAGE(name) \
	uintptr_t name##_site = 0; \
	uint8_t name##_orig[5] = {0};
#else
#define DTTR_INTEROP_PATCH_CALL_STORAGE(name) \
	extern uintptr_t name##_site; \
	extern uint8_t name##_orig[5];
#endif

#define DTTR_INTEROP_HOOK_FUNC_LOG(hook, mod) \
	do { \
		hook##_install(mod); \
		log_info( \
			"hook: Applied HOOK_FUNC %s at 0x%08X", #hook, \
			(unsigned)hook##_site \
		); \
	} while (0)

#define DTTR_INTEROP_HOOK_FUNC_TRAMPOLINE_LOG(hook, mod) \
	do { \
		hook##_install(mod); \
		if (hook##_trampoline) { \
			log_info( \
				"hook: Applied HOOK_FUNC %s at 0x%08X", #hook, \
				(unsigned)hook##_site \
			); \
		} else { \
			log_error( \
				"hook: Failed to allocate trampoline for %s", #hook \
			); \
		} \
	} while (0)

#define DTTR_INTEROP_PATCH_CALL_LOG(hook, mod) \
	do { \
		hook##_patch(mod); \
		log_info( \
			"hook: Applied PATCH_CALL %s at 0x%08X", #hook, \
			(unsigned)hook##_site \
		); \
	} while (0)

#define DTTR_INTEROP_PATCH_PTR_LOG(hook, mod) \
	do { \
		hook##_patch(mod); \
		log_info( \
			"hook: Applied PATCH_PTR %s at 0x%08X", #hook, \
			(unsigned)hook##_site \
		); \
	} while (0)

#define DTTR_INTEROP_UNHOOK_LOG(hook) \
	do { \
		log_info( \
			"hook: Removed %s at 0x%08X", #hook, (unsigned)hook##_site \
		); \
		hook##_unpatch(); \
	} while (0)

#define DTTR_INTEROP_PATCH_CALL(name, call_addr, target) \
	DTTR_INTEROP_PATCH_CALL_STORAGE(name) \
	static inline void name##_patch(HMODULE mod) { \
		name##_site = (uintptr_t)mod + (call_addr); \
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

// Works the same as PATCH_CALL but writes E9 (JMP) instead of E8
#ifdef DTTR_INTEROP_IMPLEMENT
#define DTTR_INTEROP_PATCH_JMP_STORAGE(name) \
	uintptr_t name##_site = 0; \
	uint8_t name##_orig[5] = {0};
#else
#define DTTR_INTEROP_PATCH_JMP_STORAGE(name) \
	extern uintptr_t name##_site; \
	extern uint8_t name##_orig[5];
#endif

#define DTTR_INTEROP_PATCH_JMP(name, jmp_addr, target) \
	DTTR_INTEROP_PATCH_JMP_STORAGE(name) \
	static inline void name##_patch(HMODULE mod) { \
		name##_site = (uintptr_t)mod + (jmp_addr); \
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

// Patch a pointer/address (e.g. vtable entry, IAT)
// name_patch(mod), name_unpatch(), name_orig_ptr()
#ifdef DTTR_INTEROP_IMPLEMENT
#define DTTR_INTEROP_PATCH_PTR_STORAGE(name) \
	uintptr_t name##_site = 0; \
	uintptr_t name##_orig = 0;
#else
#define DTTR_INTEROP_PATCH_PTR_STORAGE(name) \
	extern uintptr_t name##_site; \
	extern uintptr_t name##_orig;
#endif

#define DTTR_INTEROP_PATCH_PTR(name, ptr_addr, target) \
	DTTR_INTEROP_PATCH_PTR_STORAGE(name) \
	static inline void name##_patch(HMODULE mod) { \
		name##_site = (uintptr_t)mod + (ptr_addr); \
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

// Overwrites `size` bytes at a fixed offset with caller-supplied content, saving the originals for restoration
// Generates name_patch(mod, bytes) to apply and name_unpatch() to restore
#ifdef DTTR_INTEROP_IMPLEMENT
#define DTTR_INTEROP_PATCH_BYTES_STORAGE(name, size) \
	uintptr_t name##_site = 0; \
	uint8_t name##_orig[size] = {0};
#else
#define DTTR_INTEROP_PATCH_BYTES_STORAGE(name, size) \
	extern uintptr_t name##_site; \
	extern uint8_t name##_orig[size];
#endif

#define DTTR_INTEROP_PATCH_BYTES(name, addr, size) \
	DTTR_INTEROP_PATCH_BYTES_STORAGE(name, size) \
	static inline void name##_patch(HMODULE mod, const uint8_t *bytes) { \
		name##_site = (uintptr_t)mod + (addr); \
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

// Replace entire function by writing a JMP at its start
#define DTTR_INTEROP_HOOK_FUNC(name, func_addr, target) \
	DTTR_INTEROP_PATCH_JMP(name, func_addr, target) \
	static inline void name##_install(HMODULE mod) { name##_patch(mod); }

// Replace entire function with a JMP and allocate a trampoline for calling through to the original
// Exposes name##_trampoline (uint8_t*) that the callback casts to the original function
#ifdef DTTR_INTEROP_IMPLEMENT
#define DTTR_INTEROP_HOOK_FUNC_TRAMPOLINE_STORAGE(name) \
	uintptr_t name##_site = 0; \
	uint8_t name##_orig[5] = {0}; \
	uint8_t *name##_trampoline = NULL;
#else
#define DTTR_INTEROP_HOOK_FUNC_TRAMPOLINE_STORAGE(name) \
	extern uintptr_t name##_site; \
	extern uint8_t name##_orig[5]; \
	extern uint8_t *name##_trampoline;
#endif

#define DTTR_INTEROP_HOOK_FUNC_TRAMPOLINE(name, func_addr, target) \
	DTTR_INTEROP_HOOK_FUNC_TRAMPOLINE_STORAGE(name) \
	static inline void name##_install(HMODULE mod) { \
		const uintptr_t site = (uintptr_t)mod + (func_addr); \
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
