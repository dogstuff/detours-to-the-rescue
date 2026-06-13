#include "core_internal.h"

#include <dttr_log.h>
#include <dttr_sigscan.h>

#include <Zydis/Zydis.h>
#include <khash.h>
#include <kvec.h>
#include <psapi.h>
#include <xxhash.h>

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
	DTTR_HOOK_RECORD_PATCH = 0,
	DTTR_HOOK_RECORD_FUNCTION = 1,
} hook_record_kind;

typedef struct hook_chain hook_chain;

struct DTTR_Core_Hook {
	hook_record_kind kind;
	uintptr_t addr;
	size_t size;
	uint8_t *original;
	uint8_t *trampoline;
	void *owner;
	hook_chain *chain;
	DTTR_Core_Hook *prev;
	DTTR_Core_Hook *next;
	void *detour;
	uint8_t *next_thunk;
};

struct hook_chain {
	uintptr_t addr;
	size_t patch_size;
	size_t prologue_size;
	uint8_t *original;
	uint8_t *trampoline;
	DTTR_Core_Hook *head;
	DTTR_Core_Hook *tail;
};

static bool module_scan_range(HMODULE mod, const uint8_t **out_base, size_t *out_size) {
	MODULEINFO module_info;

	if (!mod || !out_base || !out_size) {
		return false;
	}

	if (!GetModuleInformation(
			GetCurrentProcess(),
			mod,
			&module_info,
			sizeof(module_info)
		)) {
		return false;
	}

	*out_base = (const uint8_t *)module_info.lpBaseOfDll;
	*out_size = module_info.SizeOfImage;
	return true;
}

uintptr_t DTTR_Core_HookSigscan(HMODULE mod, const char *sig, const char *mask) {
	const uint8_t *base = NULL;
	size_t size = 0;

	if (!module_scan_range(mod, &base, &size)) {
		return 0;
	}

	return (uintptr_t)DTTR_Sigscan_Bytes(base, size, sig, mask);
}

DTTR_Result DTTR_Core_HookSigscanAll(
	HMODULE mod,
	const char *sig,
	const char *mask,
	uintptr_t *out_addrs,
	size_t addrs_cap,
	size_t *out_count
) {
	if (!out_count || (!out_addrs && addrs_cap)) {
		return (DTTR_Result){DTTR_ERR_INVALID_ARGUMENT, "invalid sigscan-all output"};
	}

	*out_count = 0;

	if (!sig || !mask) {
		return (DTTR_Result){DTTR_ERR_INVALID_ARGUMENT, "invalid sigscan-all pattern"};
	}

	const uint8_t *base = NULL;
	size_t size = 0;
	if (!module_scan_range(mod, &base, &size)) {
		return (DTTR_Result){DTTR_ERR_INVALID_ARGUMENT, "invalid sigscan-all module"};
	}

	*out_count = DTTR_Sigscan_BytesAll(base, size, sig, mask, out_addrs, addrs_cap);
	if (!*out_count) {
		return (DTTR_Result){DTTR_ERR_NOT_FOUND, "signature not found"};
	}

	return (DTTR_Result){DTTR_OK, "ok"};
}

#define DTTR_HOOK_PATCH_SIZE 5u
#define DTTR_HOOK_MIN_PROLOGUE 5u
#define DTTR_HOOK_MAX_PROLOGUE 64u
#define DTTR_HOOK_MAX_INSN 32u

typedef struct {
	uint8_t offset;
	uint8_t length;
	uint8_t rel_offset;
	uint8_t rel_size;
} trampoline_insn;

static ZydisDecoder decoder;
static bool decoder_initialized = false;
static DTTR_Result hook_last_error = {DTTR_OK, "ok"};

DTTR_Result dttr_core_hook_last_error() {
	return hook_last_error;
}

void dttr_core_hook_set_last_error(DTTR_Status status, const char *message) {
	hook_last_error = dttr_core_result(status, message);
}

static void hook_error_clear() {
	dttr_core_hook_set_last_error(DTTR_OK, "ok");
}

static bool decoder_init() {
	if (decoder_initialized) {
		return true;
	}

	const ZyanStatus status = ZydisDecoderInit(
		&decoder,
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

	decoder_initialized = true;
	return true;
}

// Accept page protections that can be safely copied before patching.
static bool is_readable_page_protect(DWORD protect) {
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

// Copy original code only from committed readable pages to avoid unsafe trampolines.
static bool copy_memory_checked(uintptr_t addr, uint8_t *out, size_t size) {
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
			|| !is_readable_page_protect(mbi.Protect)) {
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

// Log the candidate prologue bytes when instruction decoding cannot build a trampoline.
static void log_prologue_bytes(uintptr_t site, const uint8_t *bytes, size_t size) {
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

// Decode enough whole instructions to cover the requested patch window.
static bool decode_prologue(
	uintptr_t addr,
	int requested_size,
	trampoline_insn *insns,
	size_t *out_insn_count,
	size_t *out_prologue_size,
	uint8_t *out_prologue_bytes,
	size_t out_prologue_bytes_cap
) {
	if (!decoder_init()) {
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
			"hook_attach_function: negative requested prologue=%d at 0x%08X; using "
			"auto",
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
	if (!copy_memory_checked(addr, code_window, decode_window)) {
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
			&decoder,
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

		trampoline_insn *out = &insns[count];
		out->offset = (uint8_t)offset;
		out->length = inst.length;
		out->rel_offset = 0;
		out->rel_size = 0;

		for (size_t imm_idx = 0; imm_idx < 2; imm_idx++) {
			if (!inst.raw.imm[imm_idx].size || !inst.raw.imm[imm_idx].is_relative) {
				continue;
			}

			out->rel_offset = inst.raw.imm[imm_idx].offset;
			out->rel_size = (uint8_t)(inst.raw.imm[imm_idx].size / 8);
			break;
		}

		if (out->rel_size != 0 && out->rel_size != 4) {
			DTTR_LOG_ERROR(
				"hook_attach_function: unsupported relative immediate size=%u at "
				"0x%08X+0x%X (%s)",
				(unsigned)out->rel_size,
				(unsigned)addr,
				(unsigned)offset,
				mnemonic
			);
			return false;
		}

		if (out->rel_size == 4 && (size_t)out->rel_offset + out->rel_size > out->length) {
			DTTR_LOG_ERROR(
				"hook_attach_function: invalid relative-immediate layout at "
				"0x%08X+0x%X",
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
			(unsigned)out->rel_offset,
			(unsigned)out->rel_size
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
	log_prologue_bytes(addr, out_prologue_bytes, offset);

	*out_insn_count = count;
	*out_prologue_size = offset;
	return true;
}

// Copy decoded prologue instructions into a trampoline and fix supported relative
// operands.
static bool trampoline_relocate(
	uint8_t *trampoline,
	uintptr_t site,
	const trampoline_insn *insns,
	size_t insn_count
) {
	for (size_t i = 0; i < insn_count; i++) {
		const trampoline_insn *insn = &insns[i];
		if (insn->rel_size == 0) {
			continue;
		}

		if (insn->rel_size != 4) {
			DTTR_LOG_ERROR(
				"hook_attach_function: relocate unsupported rel_size=%u at 0x%08X+0x%X",
				(unsigned)insn->rel_size,
				(unsigned)site,
				(unsigned)insn->offset
			);
			return false;
		}

		int32_t old_rel = 0;
		memcpy(&old_rel, trampoline + insn->offset + insn->rel_offset, sizeof(old_rel));

		const intptr_t old_next = (intptr_t)(site + insn->offset + insn->length);
		const intptr_t old_target = old_next + (intptr_t)old_rel;
		const intptr_t new_next = (intptr_t)((uintptr_t)trampoline + insn->offset
											 + insn->length);
		const int64_t new_rel64 = (int64_t)(old_target - new_next);
		if (new_rel64 < INT32_MIN || new_rel64 > INT32_MAX) {
			DTTR_LOG_ERROR(
				"hook_attach_function: relocated target out of range at 0x%08X+0x%X",
				(unsigned)site,
				(unsigned)insn->offset
			);
			return false;
		}

		const int32_t new_rel = (int32_t)new_rel64;
		memcpy(trampoline + insn->offset + insn->rel_offset, &new_rel, sizeof(new_rel));

		DTTR_LOG_DEBUG(
			"hook_reloc: site=0x%08X off=0x%02X target=0x%08X rel=%d",
			(unsigned)site,
			(unsigned)insn->offset,
			(unsigned)old_target,
			new_rel
		);
	}

	return true;
}

KHASH_MAP_INIT_INT64(sigscan_cache, uintptr_t)

static khash_t(sigscan_cache) *cache = NULL;

// Build a cache key from the module base and signature bytes.
static uint64_t sigscan_key(HMODULE mod, const char *sig, const char *mask) {
	const size_t mask_len = strlen(mask);
	XXH3_state_t state;
	XXH3_64bits_reset(&state);
	XXH3_64bits_update(&state, &mod, sizeof(mod));
	XXH3_64bits_update(&state, sig, mask_len);
	XXH3_64bits_update(&state, mask, mask_len);
	return XXH3_64bits_digest(&state);
}

// Reuse module signature scan results so generated symbol resolution stays cheap.
uintptr_t DTTR_Core_HookCachedSigscan(HMODULE mod, const char *sig, const char *mask) {
	if (!mod || !sig || !mask) {
		return 0;
	}

	if (!cache) {
		cache = kh_init(sigscan_cache);
		if (!cache) {
			return DTTR_Core_HookSigscan(mod, sig, mask);
		}
	}

	const uint64_t key = sigscan_key(mod, sig, mask);
	khiter_t it = kh_get(sigscan_cache, cache, key);

	if (it != kh_end(cache)) {
		return kh_val(cache, it);
	}

	const uintptr_t result = DTTR_Core_HookSigscan(mod, sig, mask);

	int ret;
	it = kh_put(sigscan_cache, cache, key, &ret);
	if (ret < 0) {
		return result;
	}

	kh_val(cache, it) = result;

	return result;
}

typedef kvec_t(DTTR_Core_Hook *) hook_vec;

static hook_vec hooks;
static void *hook_owner = NULL;

static void hook_destroy(DTTR_Core_Hook *hook) {
	if (!hook) {
		return;
	}

	if (hook->kind == DTTR_HOOK_RECORD_FUNCTION) {
		if (hook->next_thunk) {
			VirtualFree(hook->next_thunk, 0, MEM_RELEASE);
		}

		free(hook);
		return;
	}

	if (hook->trampoline) {
		VirtualFree(hook->trampoline, 0, MEM_RELEASE);
	}

	free(hook->original);
	free(hook);
}

static void hook_chain_destroy(hook_chain *chain) {
	if (!chain) {
		return;
	}

	if (chain->trampoline) {
		VirtualFree(chain->trampoline, 0, MEM_RELEASE);
	}

	free(chain->original);
	free(chain);
}

// Reject overlapping patch ranges so the registry can restore bytes deterministically.
static bool check_overlap(uintptr_t addr, size_t size) {
	const uintptr_t end = addr + size;

	for (size_t i = 0; i < kv_size(hooks); i++) {
		DTTR_Core_Hook *h = kv_A(hooks, i);
		const uintptr_t h_end = h->addr + h->size;

		if (addr < h_end && h->addr < end) {
			DTTR_LOG_WARN(
				"hook overlap: [0x%08X, +%zu) conflicts with [0x%08X, +%zu)",
				(unsigned)addr,
				size,
				(unsigned)h->addr,
				h->size
			);
			return false;
		}
	}

	return true;
}

// Allocate and register one hook record after overlap checks pass.
static DTTR_Core_Hook *hook_create(const char *op, uintptr_t addr, size_t size) {
	DTTR_Core_Hook *hook = (DTTR_Core_Hook *)calloc(1, sizeof(DTTR_Core_Hook));
	if (!hook) {
		DTTR_LOG_ERROR("%s: hook alloc failed for 0x%08X", op, (unsigned)addr);
		return NULL;
	}

	hook->addr = addr;
	hook->size = size;
	hook->owner = hook_owner;
	hook->original = (uint8_t *)malloc(size);
	if (!hook->original) {
		DTTR_LOG_ERROR("%s: original-bytes alloc failed for 0x%08X", op, (unsigned)addr);
		free(hook);
		return NULL;
	}

	return hook;
}

// Find a registered hook handle before detach or overlap checks mutate state.
static size_t hook_find_index(DTTR_Core_Hook *hook) {
	for (size_t i = 0; i < kv_size(hooks); i++) {
		if (kv_A(hooks, i) == hook) {
			return i;
		}
	}

	return kv_size(hooks);
}

static hook_chain *hook_find_function_chain(uintptr_t addr) {
	for (size_t i = 0; i < kv_size(hooks); i++) {
		DTTR_Core_Hook *hook = kv_A(hooks, i);
		if (hook->kind == DTTR_HOOK_RECORD_FUNCTION && hook->chain
			&& hook->chain->addr == addr) {
			return hook->chain;
		}
	}

	return NULL;
}

// Flush the CPU instruction cache for a freshly patched range.
static void flush_patched_range(const char *op, uintptr_t addr, size_t size) {
	if (!FlushInstructionCache(GetCurrentProcess(), (const void *)addr, size)) {
		DTTR_LOG_WARN("%s: FlushInstructionCache failed for 0x%08X", op, (unsigned)addr);
	}
}

static bool write_bytes(const char *op, uintptr_t addr, const uint8_t *bytes, size_t size) {
	DWORD old_protect;
	if (!VirtualProtect((void *)addr, size, PAGE_EXECUTE_READWRITE, &old_protect)) {
		DTTR_LOG_ERROR("%s: VirtualProtect failed for 0x%08X", op, (unsigned)addr);
		return false;
	}

	memcpy((void *)addr, bytes, size);
	VirtualProtect((void *)addr, size, old_protect, &old_protect);
	flush_patched_range(op, addr, size);
	return true;
}

static bool build_rel32_jump(
	uintptr_t site,
	void *target,
	uint8_t out[DTTR_HOOK_PATCH_SIZE]
) {
	out[0] = 0xE9;
	const int64_t rel64 = (int64_t)(uintptr_t)target
						  - (int64_t)(site + DTTR_HOOK_PATCH_SIZE);
	if (rel64 < INT32_MIN || rel64 > INT32_MAX) {
		return false;
	}

	const int32_t rel = (int32_t)rel64;
	memcpy(out + 1, &rel, sizeof(rel));
	return true;
}

static bool write_function_jump(uintptr_t site, void *target) {
	uint8_t jmp[DTTR_HOOK_PATCH_SIZE];
	if (!build_rel32_jump(site, target, jmp)) {
		DTTR_LOG_ERROR(
			"hook_attach_function: handler jump out of range at 0x%08X -> 0x%08X",
			(unsigned)site,
			(unsigned)(uintptr_t)target
		);
		return false;
	}

	return write_bytes("hook_attach_function", site, jmp, sizeof(jmp));
}

static bool hook_thunk_set_target(uint8_t *thunk, void *target) {
	if (!thunk || !target) {
		return false;
	}

	thunk[0] = 0xFF;
	thunk[1] = 0x25;
	const uint32_t slot = (uint32_t)(uintptr_t)(thunk + 6);
	const uint32_t target32 = (uint32_t)(uintptr_t)target;
	memcpy(thunk + 2, &slot, sizeof(slot));
	memcpy(thunk + 6, &target32, sizeof(target32));
	flush_patched_range("hook_chain_thunk", (uintptr_t)thunk, 10u);
	return true;
}

static void *function_link_next_target(const DTTR_Core_Hook *hook) {
	if (hook->next) {
		return hook->next->detour;
	}

	return hook->chain ? hook->chain->trampoline : NULL;
}

static DTTR_Core_Hook *function_link_create(
	hook_chain *chain,
	void *detour,
	void *next_target
) {
	DTTR_Core_Hook *hook = (DTTR_Core_Hook *)calloc(1, sizeof(DTTR_Core_Hook));
	if (!hook) {
		DTTR_LOG_ERROR(
			"hook_attach_function: hook link alloc failed for 0x%08X",
			(unsigned)(chain ? chain->addr : 0u)
		);
		return NULL;
	}

	hook->next_thunk = (uint8_t *)
		VirtualAlloc(NULL, 10u, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	if (!hook->next_thunk) {
		DTTR_LOG_ERROR(
			"hook_attach_function: hook link thunk alloc failed for 0x%08X",
			(unsigned)(chain ? chain->addr : 0u)
		);
		free(hook);
		return NULL;
	}

	hook->kind = DTTR_HOOK_RECORD_FUNCTION;
	hook->addr = chain->addr;
	hook->size = chain->patch_size;
	hook->owner = hook_owner;
	hook->chain = chain;
	hook->detour = detour;
	if (!hook_thunk_set_target(hook->next_thunk, next_target)) {
		hook_destroy(hook);
		return NULL;
	}

	return hook;
}

static bool function_chain_push_head(
	hook_chain *chain,
	DTTR_Core_Hook *hook,
	void **out_original
) {
	if (chain->head) {
		hook->next = chain->head;
		chain->head->prev = hook;
	} else {
		chain->tail = hook;
	}

	chain->head = hook;

	if (!write_function_jump(chain->addr, hook->detour)) {
		chain->head = hook->next;
		if (chain->head) {
			chain->head->prev = NULL;
		} else {
			chain->tail = NULL;
		}

		hook->next = NULL;
		return false;
	}

	if (out_original) {
		*out_original = hook->next_thunk;
	}

	return true;
}

static DTTR_Core_Hook *function_chain_append(
	hook_chain *chain,
	void *handler,
	void **out_original
) {
	DTTR_Core_Hook *hook = function_link_create(
		chain,
		handler,
		chain->head ? chain->head->detour : chain->trampoline
	);
	if (!hook) {
		return NULL;
	}

	if (!function_chain_push_head(chain, hook, out_original)) {
		hook_destroy(hook);
		return NULL;
	}

	kv_push(DTTR_Core_Hook *, hooks, hook);
	return hook;
}

static bool function_link_detach(DTTR_Core_Hook *hook) {
	hook_chain *chain = hook->chain;
	if (!chain) {
		return false;
	}

	const bool removing_head = chain->head == hook;
	const bool removing_last = !hook->prev && !hook->next;

	if (removing_last) {
		if (!write_bytes("hook_detach", chain->addr, chain->original, chain->patch_size)) {
			DTTR_LOG_ERROR(
				"hook_detach: leaving function hook registered because restore failed "
				"at "
				"0x%08X",
				(unsigned)chain->addr
			);
			return false;
		}
	} else if (removing_head) {
		if (!write_function_jump(chain->addr, hook->next->detour)) {
			DTTR_LOG_ERROR(
				"hook_detach: leaving function hook registered because relink failed "
				"at "
				"0x%08X",
				(unsigned)chain->addr
			);
			return false;
		}
	}

	DTTR_Core_Hook *prev = hook->prev;
	DTTR_Core_Hook *next = hook->next;
	if (prev) {
		prev->next = next;
		hook_thunk_set_target(prev->next_thunk, function_link_next_target(prev));
	} else {
		chain->head = next;
	}

	if (next) {
		next->prev = prev;
	} else {
		chain->tail = prev;
	}

	hook->prev = NULL;
	hook->next = NULL;
	hook->chain = NULL;

	if (!chain->head) {
		hook_chain_destroy(chain);
	}

	return true;
}

// Detach one registered hook by index while keeping the registry dense.
static bool hook_detach_index(size_t index) {
	DTTR_Core_Hook *hook = kv_A(hooks, index);
	if (hook->kind == DTTR_HOOK_RECORD_FUNCTION) {
		if (!function_link_detach(hook)) {
			return false;
		}

		kv_A(hooks, index) = kv_A(hooks, kv_size(hooks) - 1);
		kv_pop(hooks);
		hook_destroy(hook);
		return true;
	}

	if (!write_bytes("hook_detach", hook->addr, hook->original, hook->size)) {
		DTTR_LOG_ERROR(
			"hook_detach: leaving hook registered because restore failed at 0x%08X",
			(unsigned)hook->addr
		);
		return false;
	}

	kv_A(hooks, index) = kv_A(hooks, kv_size(hooks) - 1);
	kv_pop(hooks);
	hook_destroy(hook);
	return true;
}

// Install a JMP detour and build a trampoline for the overwritten prologue.
DTTR_Core_Hook *DTTR_Core_HookAttachFunction(
	uintptr_t addr,
	int prologue_size,
	void *handler,
	void **out_original
) {
	hook_error_clear();
	if (!addr || !handler) {
		DTTR_LOG_ERROR(
			"hook_attach_function: invalid parameters site=0x%08X handler=0x%08X",
			(unsigned)addr,
			(unsigned)(uintptr_t)handler
		);
		return NULL;
	}

	hook_chain *existing_chain = hook_find_function_chain(addr);
	if (existing_chain) {
		if (prologue_size > 0 && (size_t)prologue_size > existing_chain->prologue_size) {
			DTTR_LOG_WARN(
				"hook chain unsupported: site=0x%08X requested prologue %d exceeds "
				"active prologue %u",
				(unsigned)addr,
				prologue_size,
				(unsigned)existing_chain->prologue_size
			);
			dttr_core_hook_set_last_error(
				DTTR_ERR_HOOK_CHAIN_UNSUPPORTED,
				"function hook chain does not support a larger prologue"
			);
			return NULL;
		}

		return function_chain_append(existing_chain, handler, out_original);
	}

	if (!check_overlap(addr, DTTR_HOOK_PATCH_SIZE)) {
		dttr_core_hook_set_last_error(
			DTTR_ERR_HOOK_CHAIN_UNSUPPORTED,
			"function hook chain is unsupported for an overlapping hook range"
		);
		return NULL;
	}

	trampoline_insn insns[DTTR_HOOK_MAX_INSN];
	uint8_t prologue_bytes[DTTR_HOOK_MAX_PROLOGUE] = {0};
	size_t insn_count = 0;
	size_t actual_prologue_size = 0;
	if (!decode_prologue(
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
	if (!trampoline_relocate(trampoline, addr, insns, insn_count)) {
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

	hook_chain *chain = (hook_chain *)calloc(1, sizeof(hook_chain));
	if (!chain) {
		DTTR_LOG_ERROR(
			"hook_attach_function: chain alloc failed for 0x%08X",
			(unsigned)addr
		);
		VirtualFree(trampoline, 0, MEM_RELEASE);
		return NULL;
	}

	chain->original = (uint8_t *)malloc(DTTR_HOOK_PATCH_SIZE);
	if (!chain->original) {
		DTTR_LOG_ERROR(
			"hook_attach_function: original-bytes alloc failed for 0x%08X",
			(unsigned)addr
		);
		VirtualFree(trampoline, 0, MEM_RELEASE);
		free(chain);
		return NULL;
	}

	memcpy(chain->original, prologue_bytes, DTTR_HOOK_PATCH_SIZE);
	chain->addr = addr;
	chain->patch_size = DTTR_HOOK_PATCH_SIZE;
	chain->prologue_size = actual_prologue_size;
	chain->trampoline = trampoline;

	DTTR_Core_Hook *hook = function_link_create(chain, handler, trampoline);
	if (!hook) {
		hook_chain_destroy(chain);
		return NULL;
	}

	if (!function_chain_push_head(chain, hook, out_original)) {
		hook_destroy(hook);
		hook_chain_destroy(chain);
		return NULL;
	}

	kv_push(DTTR_Core_Hook *, hooks, hook);
	return hook;
}

// Patch a pointer slot and return the previous slot value when requested.
DTTR_Core_Hook *DTTR_Core_HookAttachPointer(
	uintptr_t addr,
	void *new_value,
	void **out_original
) {
	if (!addr) {
		DTTR_LOG_ERROR("hook_attach_pointer: target address is NULL");
		return NULL;
	}

	if (out_original) {
		*out_original = *(void **)addr;
	}

	const uintptr_t value = (uintptr_t)new_value;
	return DTTR_Core_HookPatchBytes(addr, (const uint8_t *)&value, sizeof(value));
}

// Patch arbitrary bytes while retaining originals for later detach.
DTTR_Core_Hook *DTTR_Core_HookPatchBytes(
	uintptr_t addr,
	const uint8_t *bytes,
	size_t size
) {
	if (!addr || !bytes || !size) {
		DTTR_LOG_ERROR(
			"hook_patch_bytes: invalid parameters addr=0x%08X bytes=0x%08X size=%zu",
			(unsigned)addr,
			(unsigned)(uintptr_t)bytes,
			size
		);
		return NULL;
	}

	if (!check_overlap(addr, size)) {
		return NULL;
	}

	DTTR_Core_Hook *hook = hook_create("hook_patch_bytes", addr, size);
	if (!hook) {
		return NULL;
	}

	memcpy(hook->original, (const void *)addr, size);
	if (!write_bytes("hook_patch_bytes", addr, bytes, size)) {
		hook_destroy(hook);
		return NULL;
	}

	kv_push(DTTR_Core_Hook *, hooks, hook);
	return hook;
}

bool DTTR_Core_HookDetachChecked(DTTR_Core_Hook *hook) {
	if (!hook) {
		return true;
	}

	const size_t index = hook_find_index(hook);
	if (index == kv_size(hooks)) {
		DTTR_LOG_DEBUG("hook_detach: ignoring stale or already detached hook handle");
		return true;
	}

	return hook_detach_index(index);
}

// Detach one registered hook or patch if the handle is still active.
void DTTR_Core_HookDetach(DTTR_Core_Hook *hook) {
	DTTR_Core_HookDetachChecked(hook);
}

bool DTTR_Core_HookIsActive(DTTR_Core_Hook *hook) {
	return hook && hook_find_index(hook) != kv_size(hooks);
}

bool DTTR_Core_HookDetachOwnerChecked(void *owner) {
	if (!owner) {
		return true;
	}

	bool ok = true;
	for (size_t i = kv_size(hooks); i > 0; i--) {
		if (kv_A(hooks, i - 1)->owner != owner) {
			continue;
		}

		ok = hook_detach_index(i - 1) && ok;
	}

	return ok;
}

// Set the owner tag applied to subsequent hooks so SDK contexts can clean up their work.
void *DTTR_Core_HookSetOwner(void *owner) {
	void *previous = hook_owner;
	hook_owner = owner;
	return previous;
}

// Detach every hook tagged with an owner during context teardown.
void DTTR_Core_HookDetachOwner(void *owner) {
	DTTR_Core_HookDetachOwnerChecked(owner);
}

static void cleanup_sigscan_cache() {
	if (cache) {
		kh_destroy(sigscan_cache, cache);
		cache = NULL;
	}
}

bool DTTR_Core_HookCleanupAllChecked() {
	bool ok = true;
	while (kv_size(hooks) > 0) {
		const size_t previous_count = kv_size(hooks);
		if (!hook_detach_index(previous_count - 1)) {
			ok = false;
			break;
		}
	}

	if (kv_size(hooks) == 0) {
		kv_destroy(hooks);
		kv_init(hooks);
		hook_owner = NULL;
	}

	cleanup_sigscan_cache();
	return ok;
}

void DTTR_Core_HookCleanupAll() {
	if (!DTTR_Core_HookCleanupAllChecked()) {
		DTTR_LOG_ERROR(
			"hook_cleanup_all: stopped after restore failure; hooks remain registered"
		);
	}
}
