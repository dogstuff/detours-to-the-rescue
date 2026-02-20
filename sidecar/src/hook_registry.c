#include "hook_registry_internal.h"

#include <khash.h>
#include <kvec.h>
#include <log.h>
#include <psapi.h>
#include <xxhash.h>

#include <stdlib.h>
#include <string.h>

uintptr_t dttr_hook_sigscan(HMODULE mod, const char *sig, const char *mask) {
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

// Relocates E8 (CALL rel32) and E9 (JMP rel32) instructions in a copied code buffer.
static void s_trampoline_relocate(uint8_t *buf, uintptr_t site, int n) {
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

KHASH_MAP_INIT_INT64(sigscan_cache, uintptr_t)

static khash_t(sigscan_cache) *s_cache = NULL;

static uint64_t s_sigscan_key(HMODULE mod, const char *sig, const char *mask) {
	const size_t mask_len = strlen(mask);
	XXH3_state_t state;
	XXH3_64bits_reset(&state);
	XXH3_64bits_update(&state, &mod, sizeof(mod));
	XXH3_64bits_update(&state, sig, mask_len);
	XXH3_64bits_update(&state, mask, mask_len);
	return XXH3_64bits_digest(&state);
}

uintptr_t dttr_hook_cached_sigscan(HMODULE mod, const char *sig, const char *mask) {
	if (!s_cache) {
		s_cache = kh_init(sigscan_cache);
	}

	const uint64_t key = s_sigscan_key(mod, sig, mask);
	khiter_t it = kh_get(sigscan_cache, s_cache, key);

	if (it != kh_end(s_cache)) {
		return kh_val(s_cache, it);
	}

	const uintptr_t result = dttr_hook_sigscan(mod, sig, mask);

	int ret;
	it = kh_put(sigscan_cache, s_cache, key, &ret);
	kh_val(s_cache, it) = result;

	return result;
}

typedef kvec_t(DTTR_Hook *) S_HookVec;

static S_HookVec s_hooks;

static void s_check_overlap(uintptr_t addr, size_t size) {
	const uintptr_t end = addr + size;

	for (size_t i = 0; i < kv_size(s_hooks); i++) {
		DTTR_Hook *h = kv_A(s_hooks, i);
		const uintptr_t h_end = h->m_addr + h->m_size;

		if (addr < h_end && h->m_addr < end) {
			log_warn(
				"hook overlap: [0x%08X, +%zu) conflicts with [0x%08X, +%zu)",
				(unsigned)addr,
				size,
				(unsigned)h->m_addr,
				h->m_size
			);
		}
	}
}

static bool s_write_bytes(uintptr_t addr, const uint8_t *bytes, size_t size) {
	DWORD old_protect;
	if (!VirtualProtect((void *)addr, size, PAGE_EXECUTE_READWRITE, &old_protect)) {
		return false;
	}

	memcpy((void *)addr, bytes, size);
	VirtualProtect((void *)addr, size, old_protect, &old_protect);
	return true;
}

DTTR_Hook *dttr_hook_attach_function(
	uintptr_t addr,
	int prologue_size,
	void *handler,
	void **out_original
) {
	s_check_overlap(addr, 5);

	uint8_t *trampoline = (uint8_t *)VirtualAlloc(
		NULL,
		prologue_size + 5,
		MEM_COMMIT | MEM_RESERVE,
		PAGE_EXECUTE_READWRITE
	);

	if (!trampoline) {
		log_error(
			"hook_attach_function: trampoline alloc failed for 0x%08X",
			(unsigned)addr
		);
		return NULL;
	}

	memcpy(trampoline, (void *)addr, prologue_size);
	s_trampoline_relocate(trampoline, addr, prologue_size);

	trampoline[prologue_size] = 0xE9;
	int32_t jmp_back = (int32_t)((addr + prologue_size)
								 - ((uintptr_t)trampoline + prologue_size + 5));
	memcpy(trampoline + prologue_size + 1, &jmp_back, 4);

	DTTR_Hook *hook = (DTTR_Hook *)calloc(1, sizeof(DTTR_Hook));
	hook->m_addr = addr;
	hook->m_size = 5;
	hook->m_original = (uint8_t *)malloc(5);
	memcpy(hook->m_original, (void *)addr, 5);
	hook->m_trampoline = trampoline;

	uint8_t jmp[5];
	jmp[0] = 0xE9;
	int32_t rel = (int32_t)((uintptr_t)handler - (addr + 5));
	memcpy(jmp + 1, &rel, 4);

	if (!s_write_bytes(addr, jmp, 5)) {
		log_error("hook_attach_function: write failed for 0x%08X", (unsigned)addr);
		VirtualFree(trampoline, 0, MEM_RELEASE);
		free(hook->m_original);
		free(hook);
		return NULL;
	}

	if (out_original) {
		*out_original = trampoline;
	}

	kv_push(DTTR_Hook *, s_hooks, hook);
	return hook;
}

DTTR_Hook *dttr_hook_attach_pointer(uintptr_t addr, void *new_value, void **out_original) {
	if (out_original) {
		*out_original = *(void **)addr;
	}

	uintptr_t val = (uintptr_t)new_value;
	return dttr_hook_patch_bytes(addr, (const uint8_t *)&val, sizeof(void *));
}

DTTR_Hook *dttr_hook_patch_bytes(uintptr_t addr, const uint8_t *bytes, size_t size) {
	s_check_overlap(addr, size);

	DTTR_Hook *hook = (DTTR_Hook *)calloc(1, sizeof(DTTR_Hook));
	hook->m_addr = addr;
	hook->m_size = size;
	hook->m_original = (uint8_t *)malloc(size);

	DWORD old_protect;
	if (!VirtualProtect((void *)addr, size, PAGE_EXECUTE_READWRITE, &old_protect)) {
		log_error("hook_patch_bytes: VirtualProtect failed for 0x%08X", (unsigned)addr);
		free(hook->m_original);
		free(hook);
		return NULL;
	}

	memcpy(hook->m_original, (void *)addr, size);
	memcpy((void *)addr, bytes, size);
	VirtualProtect((void *)addr, size, old_protect, &old_protect);

	kv_push(DTTR_Hook *, s_hooks, hook);
	return hook;
}

void dttr_hook_detach(DTTR_Hook *hook) {
	if (!hook) {
		return;
	}

	s_write_bytes(hook->m_addr, hook->m_original, hook->m_size);

	if (hook->m_trampoline) {
		VirtualFree(hook->m_trampoline, 0, MEM_RELEASE);
	}

	free(hook->m_original);

	for (size_t i = 0; i < kv_size(s_hooks); i++) {
		if (kv_A(s_hooks, i) == hook) {
			kv_A(s_hooks, i) = kv_A(s_hooks, kv_size(s_hooks) - 1);
			s_hooks.n--;
			break;
		}
	}

	free(hook);
}

void dttr_hook_cleanup_all(void) {
	for (size_t i = kv_size(s_hooks); i > 0; i--) {
		DTTR_Hook *hook = kv_A(s_hooks, i - 1);
		s_write_bytes(hook->m_addr, hook->m_original, hook->m_size);

		if (hook->m_trampoline) {
			VirtualFree(hook->m_trampoline, 0, MEM_RELEASE);
		}

		free(hook->m_original);
		free(hook);
	}

	kv_destroy(s_hooks);
	kv_init(s_hooks);

	if (s_cache) {
		kh_destroy(sigscan_cache, s_cache);
		s_cache = NULL;
	}
}
