#include "hook_registry_internal.h"

#include <dttr_log.h>

#include <Zydis/Zydis.h>
#include <khash.h>
#include <kvec.h>
#include <psapi.h>
#include <xxhash.h>

#include <limits.h>
#include <stdio.h>
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

#define DTTR_HOOK_PATCH_SIZE 5u
#define DTTR_HOOK_MIN_PROLOGUE 5u
#define DTTR_HOOK_MAX_PROLOGUE 64u
#define DTTR_HOOK_MAX_INSN 32u

typedef struct {
	uint8_t m_offset;
	uint8_t m_length;
	uint8_t m_rel_offset;
	uint8_t m_rel_size;
} S_TrampolineInsn;

static ZydisDecoder s_decoder;
static bool s_decoder_initialized = false;

static bool s_decoder_init(void) {
	if (s_decoder_initialized) {
		return true;
	}

	const ZyanStatus status = ZydisDecoderInit(
		&s_decoder,
		ZYDIS_MACHINE_MODE_LEGACY_32,
		ZYDIS_STACK_WIDTH_32
	);
	if (!ZYAN_SUCCESS(status)) {
		DTTR_LOG_ERROR(
			"hook_attach_function: ZydisDecoderInit failed (status=0x%08X)",
			(unsigned)status
		);
		return false;
	}

	s_decoder_initialized = true;
	return true;
}

static bool s_is_readable_page_protect(DWORD protect) {
	switch (protect & 0xFFu) {
	case PAGE_READONLY:
	case PAGE_READWRITE:
	case PAGE_WRITECOPY:
	case PAGE_EXECUTE:
	case PAGE_EXECUTE_READ:
	case PAGE_EXECUTE_READWRITE:
	case PAGE_EXECUTE_WRITECOPY:
		return true;
	default:
		return false;
	}
}

static bool s_copy_memory_checked(uintptr_t addr, uint8_t *out, size_t size) {
	size_t copied = 0;

	while (copied < size) {
		const uintptr_t cur = addr + copied;
		MEMORY_BASIC_INFORMATION mbi;
		if (!VirtualQuery((const void *)cur, &mbi, sizeof(mbi))) {
			DTTR_LOG_ERROR(
				"hook_attach_function: VirtualQuery failed at 0x%08X (err=%lu)",
				(unsigned)cur,
				GetLastError()
			);
			return false;
		}

		if (mbi.State != MEM_COMMIT) {
			DTTR_LOG_ERROR(
				"hook_attach_function: unreadable memory state=0x%lX at 0x%08X",
				(unsigned long)mbi.State,
				(unsigned)cur
			);
			return false;
		}
		if ((mbi.Protect & PAGE_GUARD) || (mbi.Protect & PAGE_NOACCESS)
			|| !s_is_readable_page_protect(mbi.Protect)) {
			DTTR_LOG_ERROR(
				"hook_attach_function: unreadable memory protect=0x%lX at 0x%08X",
				(unsigned long)mbi.Protect,
				(unsigned)cur
			);
			return false;
		}

		const uintptr_t region_end = (uintptr_t)mbi.BaseAddress + mbi.RegionSize;
		size_t chunk = (size_t)(region_end - cur);
		if (chunk > size - copied) {
			chunk = size - copied;
		}

		memcpy(out + copied, (const void *)cur, chunk);
		copied += chunk;
	}

	return true;
}

static void s_log_prologue_bytes(uintptr_t site, const uint8_t *bytes, size_t size) {
	char hex[(DTTR_HOOK_MAX_PROLOGUE * 3u) + 1u];
	size_t pos = 0;

	for (size_t i = 0; i < size && i < DTTR_HOOK_MAX_PROLOGUE; i++) {
		const int wrote = snprintf(
			hex + pos,
			sizeof(hex) - pos,
			"%02X%s",
			(unsigned)bytes[i],
			(i + 1 < size) ? " " : ""
		);
		if (wrote <= 0) {
			break;
		}
		const size_t w = (size_t)wrote;
		if (w >= sizeof(hex) - pos) {
			pos = sizeof(hex) - 1;
			break;
		}
		pos += w;
	}

	hex[pos] = '\0';
	DTTR_LOG_DEBUG(
		"hook_validate: site=0x%08X prologue_bytes[%u]=%s",
		(unsigned)site,
		(unsigned)size,
		hex
	);
}

static bool s_decode_prologue(
	uintptr_t addr,
	int requested_size,
	S_TrampolineInsn *insns,
	size_t *out_insn_count,
	size_t *out_prologue_size,
	uint8_t *out_prologue_bytes,
	size_t out_prologue_bytes_cap
) {
	if (!s_decoder_init()) {
		return false;
	}
	if (!insns || !out_insn_count || !out_prologue_size || !out_prologue_bytes
		|| out_prologue_bytes_cap == 0) {
		DTTR_LOG_ERROR(
			"hook_attach_function: invalid decode parameters for 0x%08X",
			(unsigned)addr
		);
		return false;
	}

	size_t need = DTTR_HOOK_MIN_PROLOGUE;
	if (requested_size > 0) {
		need = (size_t)requested_size;
		if (need < DTTR_HOOK_MIN_PROLOGUE) {
			need = DTTR_HOOK_MIN_PROLOGUE;
		}
	} else if (requested_size < 0) {
		DTTR_LOG_WARN(
			"hook_attach_function: negative requested prologue=%d at 0x%08X; using auto",
			requested_size,
			(unsigned)addr
		);
	}

	if (need > DTTR_HOOK_MAX_PROLOGUE) {
		DTTR_LOG_ERROR(
			"hook_attach_function: invalid requested prologue=%u at 0x%08X",
			(unsigned)need,
			(unsigned)addr
		);
		return false;
	}

	size_t decode_window = need + (size_t)ZYDIS_MAX_INSTRUCTION_LENGTH - 1u;
	if (decode_window > DTTR_HOOK_MAX_PROLOGUE) {
		decode_window = DTTR_HOOK_MAX_PROLOGUE;
	}

	uint8_t code_window[DTTR_HOOK_MAX_PROLOGUE] = {0};
	if (!s_copy_memory_checked(addr, code_window, decode_window)) {
		DTTR_LOG_ERROR(
			"hook_attach_function: failed to read decode window at 0x%08X (size=%u)",
			(unsigned)addr,
			(unsigned)decode_window
		);
		return false;
	}

	size_t offset = 0;
	size_t count = 0;

	while (offset < need) {
		if (count >= DTTR_HOOK_MAX_INSN) {
			DTTR_LOG_ERROR(
				"hook_attach_function: too many instructions while decoding 0x%08X",
				(unsigned)addr
			);
			return false;
		}

		ZydisDecodedInstruction inst;
		const ZyanStatus status = ZydisDecoderDecodeInstruction(
			&s_decoder,
			NULL,
			code_window + offset,
			decode_window - offset,
			&inst
		);
		if (!ZYAN_SUCCESS(status) || inst.length == 0) {
			DTTR_LOG_ERROR(
				"hook_attach_function: decode failed at 0x%08X+0x%X (status=0x%08X)",
				(unsigned)addr,
				(unsigned)offset,
				(unsigned)status
			);
			return false;
		}

		const char *mnemonic = ZydisMnemonicGetString(inst.mnemonic);
		if (!mnemonic) {
			mnemonic = "unknown";
		}

		if (offset + inst.length > DTTR_HOOK_MAX_PROLOGUE) {
			DTTR_LOG_ERROR(
				"hook_attach_function: decoded prologue exceeded %u bytes at 0x%08X",
				(unsigned)DTTR_HOOK_MAX_PROLOGUE,
				(unsigned)addr
			);
			return false;
		}

		S_TrampolineInsn *out = &insns[count];
		out->m_offset = (uint8_t)offset;
		out->m_length = inst.length;
		out->m_rel_offset = 0;
		out->m_rel_size = 0;

		for (size_t imm_idx = 0; imm_idx < 2; imm_idx++) {
			if (!inst.raw.imm[imm_idx].size || !inst.raw.imm[imm_idx].is_relative) {
				continue;
			}

			out->m_rel_offset = inst.raw.imm[imm_idx].offset;
			out->m_rel_size = (uint8_t)(inst.raw.imm[imm_idx].size / 8);
			break;
		}

		if (out->m_rel_size != 0 && out->m_rel_size != 4) {
			DTTR_LOG_ERROR(
				"hook_attach_function: unsupported relative immediate size=%u at "
				"0x%08X+0x%X (%s)",
				(unsigned)out->m_rel_size,
				(unsigned)addr,
				(unsigned)offset,
				mnemonic
			);
			return false;
		}
		if (out->m_rel_size == 4
			&& (size_t)out->m_rel_offset + out->m_rel_size > out->m_length) {
			DTTR_LOG_ERROR(
				"hook_attach_function: invalid relative-immediate layout at 0x%08X+0x%X",
				(unsigned)addr,
				(unsigned)offset
			);
			return false;
		}

		DTTR_LOG_DEBUG(
			"hook_decode: site=0x%08X off=0x%02X len=%u mnemonic=%s rel_off=%u "
			"rel_size=%u",
			(unsigned)addr,
			(unsigned)offset,
			(unsigned)inst.length,
			mnemonic,
			(unsigned)out->m_rel_offset,
			(unsigned)out->m_rel_size
		);

		offset += inst.length;
		count++;
	}

	if (offset > out_prologue_bytes_cap) {
		DTTR_LOG_ERROR(
			"hook_attach_function: prologue output overflow at 0x%08X (%u > %u)",
			(unsigned)addr,
			(unsigned)offset,
			(unsigned)out_prologue_bytes_cap
		);
		return false;
	}
	memcpy(out_prologue_bytes, code_window, offset);
	s_log_prologue_bytes(addr, out_prologue_bytes, offset);

	*out_insn_count = count;
	*out_prologue_size = offset;
	return true;
}

static bool s_trampoline_relocate(
	uint8_t *trampoline,
	uintptr_t site,
	const S_TrampolineInsn *insns,
	size_t insn_count
) {
	for (size_t i = 0; i < insn_count; i++) {
		const S_TrampolineInsn *insn = &insns[i];
		if (insn->m_rel_size == 0) {
			continue;
		}

		if (insn->m_rel_size != 4) {
			DTTR_LOG_ERROR(
				"hook_attach_function: relocate unsupported rel_size=%u at 0x%08X+0x%X",
				(unsigned)insn->m_rel_size,
				(unsigned)site,
				(unsigned)insn->m_offset
			);
			return false;
		}

		int32_t old_rel = 0;
		memcpy(
			&old_rel,
			trampoline + insn->m_offset + insn->m_rel_offset,
			sizeof(old_rel)
		);

		const intptr_t old_next = (intptr_t)(site + insn->m_offset + insn->m_length);
		const intptr_t old_target = old_next + (intptr_t)old_rel;
		const intptr_t new_next = (intptr_t)((uintptr_t)trampoline + insn->m_offset
											 + insn->m_length);
		const int64_t new_rel64 = (int64_t)(old_target - new_next);
		if (new_rel64 < INT32_MIN || new_rel64 > INT32_MAX) {
			DTTR_LOG_ERROR(
				"hook_attach_function: relocated target out of range at 0x%08X+0x%X",
				(unsigned)site,
				(unsigned)insn->m_offset
			);
			return false;
		}

		const int32_t new_rel = (int32_t)new_rel64;
		memcpy(
			trampoline + insn->m_offset + insn->m_rel_offset,
			&new_rel,
			sizeof(new_rel)
		);

		DTTR_LOG_DEBUG(
			"hook_reloc: site=0x%08X off=0x%02X target=0x%08X rel=%d",
			(unsigned)site,
			(unsigned)insn->m_offset,
			(unsigned)old_target,
			new_rel
		);
	}

	return true;
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

static void s_hook_destroy(DTTR_Hook *hook) {
	if (!hook) {
		return;
	}

	if (hook->m_trampoline) {
		VirtualFree(hook->m_trampoline, 0, MEM_RELEASE);
	}

	free(hook->m_original);
	free(hook);
}

static void s_check_overlap(uintptr_t addr, size_t size) {
	const uintptr_t end = addr + size;

	for (size_t i = 0; i < kv_size(s_hooks); i++) {
		DTTR_Hook *h = kv_A(s_hooks, i);
		const uintptr_t h_end = h->m_addr + h->m_size;

		if (addr < h_end && h->m_addr < end) {
			DTTR_LOG_WARN(
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

static DTTR_Hook *s_hook_attach_function_common(
	uintptr_t addr,
	int prologue_size,
	void *handler,
	void **out_original
) {
	if (!addr || !handler) {
		DTTR_LOG_ERROR(
			"hook_attach_function: invalid parameters site=0x%08X handler=0x%08X",
			(unsigned)addr,
			(unsigned)(uintptr_t)handler
		);
		return NULL;
	}

	s_check_overlap(addr, DTTR_HOOK_PATCH_SIZE);

	S_TrampolineInsn insns[DTTR_HOOK_MAX_INSN];
	uint8_t prologue_bytes[DTTR_HOOK_MAX_PROLOGUE] = {0};
	size_t insn_count = 0;
	size_t actual_prologue_size = 0;
	if (!s_decode_prologue(
			addr,
			prologue_size,
			insns,
			&insn_count,
			&actual_prologue_size,
			prologue_bytes,
			sizeof(prologue_bytes)
		)) {
		return NULL;
	}

	DTTR_LOG_DEBUG(
		"hook_attach_function: site=0x%08X requested=%d aligned=%u insn_count=%u",
		(unsigned)addr,
		prologue_size,
		(unsigned)actual_prologue_size,
		(unsigned)insn_count
	);

	uint8_t *trampoline = (uint8_t *)VirtualAlloc(
		NULL,
		actual_prologue_size + DTTR_HOOK_PATCH_SIZE,
		MEM_COMMIT | MEM_RESERVE,
		PAGE_EXECUTE_READWRITE
	);

	if (!trampoline) {
		DTTR_LOG_ERROR(
			"hook_attach_function: trampoline alloc failed for 0x%08X",
			(unsigned)addr
		);
		return NULL;
	}

	memcpy(trampoline, prologue_bytes, actual_prologue_size);
	if (!s_trampoline_relocate(trampoline, addr, insns, insn_count)) {
		VirtualFree(trampoline, 0, MEM_RELEASE);
		return NULL;
	}

	trampoline[actual_prologue_size] = 0xE9;
	const int64_t jmp_back64 = (int64_t)(addr + actual_prologue_size)
							   - (int64_t)((uintptr_t)trampoline + actual_prologue_size
										   + DTTR_HOOK_PATCH_SIZE);
	if (jmp_back64 < INT32_MIN || jmp_back64 > INT32_MAX) {
		DTTR_LOG_ERROR(
			"hook_attach_function: trampoline jmp-back out of range at 0x%08X",
			(unsigned)addr
		);
		VirtualFree(trampoline, 0, MEM_RELEASE);
		return NULL;
	}
	const int32_t jmp_back = (int32_t)jmp_back64;
	memcpy(trampoline + actual_prologue_size + 1, &jmp_back, 4);

	DTTR_Hook *hook = (DTTR_Hook *)calloc(1, sizeof(DTTR_Hook));
	if (!hook) {
		DTTR_LOG_ERROR(
			"hook_attach_function: hook alloc failed for 0x%08X",
			(unsigned)addr
		);
		VirtualFree(trampoline, 0, MEM_RELEASE);
		return NULL;
	}

	hook->m_addr = addr;
	hook->m_size = DTTR_HOOK_PATCH_SIZE;
	hook->m_original = (uint8_t *)malloc(DTTR_HOOK_PATCH_SIZE);
	if (!hook->m_original) {
		DTTR_LOG_ERROR(
			"hook_attach_function: original-bytes alloc failed for 0x%08X",
			(unsigned)addr
		);
		VirtualFree(trampoline, 0, MEM_RELEASE);
		free(hook);
		return NULL;
	}

	memcpy(hook->m_original, prologue_bytes, DTTR_HOOK_PATCH_SIZE);
	hook->m_trampoline = trampoline;

	uint8_t jmp[DTTR_HOOK_PATCH_SIZE];
	jmp[0] = 0xE9;
	const int64_t rel64 = (int64_t)(uintptr_t)handler
						  - (int64_t)(addr + DTTR_HOOK_PATCH_SIZE);
	if (rel64 < INT32_MIN || rel64 > INT32_MAX) {
		DTTR_LOG_ERROR(
			"hook_attach_function: handler jump out of range at 0x%08X -> 0x%08X",
			(unsigned)addr,
			(unsigned)(uintptr_t)handler
		);
		VirtualFree(trampoline, 0, MEM_RELEASE);
		free(hook->m_original);
		free(hook);
		return NULL;
	}
	const int32_t rel = (int32_t)rel64;
	memcpy(jmp + 1, &rel, 4);

	if (!s_write_bytes(addr, jmp, DTTR_HOOK_PATCH_SIZE)) {
		DTTR_LOG_ERROR("hook_attach_function: write failed for 0x%08X", (unsigned)addr);
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

DTTR_Hook *dttr_hook_attach_function(
	uintptr_t addr,
	int prologue_size,
	void *handler,
	void **out_original
) {
	return s_hook_attach_function_common(addr, prologue_size, handler, out_original);
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
	if (!hook) {
		DTTR_LOG_ERROR("hook_patch_bytes: hook alloc failed for 0x%08X", (unsigned)addr);
		return NULL;
	}

	hook->m_addr = addr;
	hook->m_size = size;
	hook->m_original = (uint8_t *)malloc(size);
	if (!hook->m_original) {
		DTTR_LOG_ERROR(
			"hook_patch_bytes: original-bytes alloc failed for 0x%08X",
			(unsigned)addr
		);
		free(hook);
		return NULL;
	}

	DWORD old_protect;
	if (!VirtualProtect((void *)addr, size, PAGE_EXECUTE_READWRITE, &old_protect)) {
		DTTR_LOG_ERROR(
			"hook_patch_bytes: VirtualProtect failed for 0x%08X",
			(unsigned)addr
		);
		s_hook_destroy(hook);
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

	for (size_t i = 0; i < kv_size(s_hooks); i++) {
		if (kv_A(s_hooks, i) == hook) {
			kv_A(s_hooks, i) = kv_A(s_hooks, kv_size(s_hooks) - 1);
			s_hooks.n--;
			break;
		}
	}

	s_hook_destroy(hook);
}

void dttr_hook_cleanup_all(void) {
	for (size_t i = kv_size(s_hooks); i > 0; i--) {
		DTTR_Hook *hook = kv_A(s_hooks, i - 1);
		s_write_bytes(hook->m_addr, hook->m_original, hook->m_size);
		s_hook_destroy(hook);
	}

	kv_destroy(s_hooks);
	kv_init(s_hooks);

	if (s_cache) {
		kh_destroy(sigscan_cache, s_cache);
		s_cache = NULL;
	}
}
