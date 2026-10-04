#include <dttr_test.h>

#include <dttr_pcdogs.h>

typedef struct {
	const char *name;
	const uint8_t *sig;
	const char *mask;
	DTTR_TestFixtureMask required;
} blueprint_signature;

typedef struct {
	const char *name;
	const uint8_t *sig;
	const char *mask;
	DTTR_TestFixtureMask required;
	int32_t match_offset;
	DTTR_PCDOGS_T_Calling_Convention calling_convention;
	DTTR_PCDOGS_T_Hook_Kind hook_kind;
	uint32_t patch_size;
	uint32_t entry_patch_size;
	uint32_t param_count;
	uint32_t stack_param_bytes;
} blueprint_function;

typedef struct {
	const char *name;
	uint32_t data_index;
	uint32_t function_index;
	uint32_t instr_off;
	uint32_t addr_off;
	uint32_t indirections;
	DTTR_TestFixtureMask required;
} blueprint_data_xref;

#include "pcdogs_blueprint_test_rows.h"

#define SIGNATURE_COUNT (sizeof(DTTR_PCDOGS_SIGNATURES) / sizeof(*DTTR_PCDOGS_SIGNATURES))
#define FUNCTION_COUNT (sizeof(DTTR_PCDOGS_FUNCTIONS) / sizeof(*DTTR_PCDOGS_FUNCTIONS))

// Skip binary-backed signature tests when fixture files are unavailable.
static uintptr_t require_sigscan(
	const DTTR_TestPCDOGSFixture *fixture,
	const char *kind,
	const char *name,
	const uint8_t *sig,
	const char *mask,
	const DTTR_TestPEImage *image
) {
	const uintptr_t rva = pcdogs_sigscan(image, sig, mask);
	if (rva != DTTR_TEST_PCDOGS_SIG_NOT_FOUND) {
		const size_t matches = pcdogs_sigscan_count(image, sig, mask);
		if (matches == 1) {
			return rva;
		}

		fail_msg(
			"required %s %s resolved %zu times in %s (%s); expected exactly one match",
			kind,
			name,
			matches,
			fixture->id,
			fixture->filename
		);
		return rva;
	}

	fail_msg(
		"required %s %s did not resolve in %s (%s)",
		kind,
		name,
		fixture->id,
		fixture->filename
	);
	return DTTR_TEST_PCDOGS_SIG_NOT_FOUND;
}

// Decode one instruction at an expected fixture address for patch-window checks.
static void assert_decodes_at(
	const DTTR_TestPCDOGSFixture *fixture,
	const char *kind,
	const char *name,
	const DTTR_TestPEImage *image,
	uintptr_t rva
) {
	DTTR_TestDecodedInstruction decoded = {0};
	if (dttr_test_zydis_decode32_at(image, rva, &decoded)) {
		assert_true(decoded.instruction.length > 0);
		return;
	}

	fail_msg(
		"required %s %s did not decode in %s at 0x%08X",
		kind,
		name,
		fixture->id,
		(unsigned)rva
	);
}

// Verify a generated signature resolves in every required fixture image.
static void assert_signature_resolved(
	const DTTR_TestPCDOGSFixture *fixture,
	const blueprint_signature *sig,
	const DTTR_TestPEImage *image
) {
	const uintptr_t rva = require_sigscan(
		fixture,
		"signature",
		sig->name,
		sig->sig,
		sig->mask,
		image
	);
	assert_decodes_at(fixture, "signature", sig->name, image, rva);
}

// Run generated signature expectations against one loaded PCDOGS fixture.
static bool assert_signatures_for_fixture(
	size_t fixture_index,
	const DTTR_TestBinaryFixture *fixture,
	const char *path,
	const DTTR_TestPEImage *image,
	void *userdata
) {
	for (size_t sig_index = 0; sig_index < SIGNATURE_COUNT; sig_index++) {
		const blueprint_signature *sig = &DTTR_PCDOGS_SIGNATURES[sig_index];
		if (!dttr_test_fixture_required(sig->required, fixture_index)) {
			continue;
		}

		assert_signature_resolved(fixture, sig, image);
	}

	return true;
}

// Covers generated signature rows resolving across required PCDOGS fixtures.
static void test_expected_pcdogs_signatures_resolve(void **state) {
	dttr_test_require_available(pcdogs_fixtures_available());
	assert_true(pcdogs_for_each_fixture(assert_signatures_for_fixture, NULL));
}

// Return the matched function address adjusted by its blueprint match offset.
static uintptr_t blueprint_function_site(
	const DTTR_TestPCDOGSFixture *fixture,
	const blueprint_function *fn,
	const DTTR_TestPEImage *image
) {
	const uintptr_t match = require_sigscan(
		fixture,
		"blueprint function",
		fn->name,
		fn->sig,
		fn->mask,
		image
	);
	if (!dttr_test_signed_range_valid(match, fn->match_offset, 1u, image->image_size)) {
		fail_msg(
			"blueprint function %s match offset outside %s: match=0x%08X offset=%d",
			fn->name,
			fixture->id,
			(unsigned)match,
			(int)fn->match_offset
		);
	}

	return dttr_test_offset_site(match, fn->match_offset);
}

// Find prologue instructions that the trampoline relocator cannot support.
static bool instruction_has_unsupported_reloc(const DTTR_TestDecodedInstruction *decoded) {
	for (size_t imm_idx = 0; imm_idx < 2; imm_idx++) {
		if (!decoded->instruction.raw.imm[imm_idx].size
			|| !decoded->instruction.raw.imm[imm_idx].is_relative) {
			continue;
		}

		const uint8_t rel_size = (uint8_t)(decoded->instruction.raw.imm[imm_idx].size
										   / 8u);
		if (rel_size != 4u) {
			return true;
		}

		if ((size_t)decoded->instruction.raw.imm[imm_idx].offset + rel_size
			> decoded->instruction.length) {
			return true;
		}
	}

	return false;
}

// Verify generated hook patch windows contain decodable and supported instructions.
static void assert_patch_window_decodes(
	const DTTR_TestPCDOGSFixture *fixture,
	const blueprint_function *fn,
	const DTTR_TestPEImage *image,
	uintptr_t site,
	uint32_t patch_size
) {
	if (patch_size == 0u) {
		return;
	}

	if (!dttr_test_range_valid((size_t)site, patch_size, image->image_size)) {
		fail_msg(
			"blueprint hook %s patch window outside %s at 0x%08X size=%u",
			fn->name,
			fixture->id,
			(unsigned)site,
			(unsigned)patch_size
		);
	}

	size_t decoded_size = 0;
	while (decoded_size < patch_size) {
		DTTR_TestDecodedInstruction decoded = {0};
		assert_true(dttr_test_zydis_decode32_at(image, site + decoded_size, &decoded));
		assert_true(decoded.instruction.length > 0);
		if (instruction_has_unsupported_reloc(&decoded)) {
			fail_msg(
				"blueprint hook %s has unsupported relative immediate in %s at 0x%08X",
				fn->name,
				fixture->id,
				(unsigned)(site + decoded_size)
			);
		}

		decoded_size += decoded.instruction.length;
	}
}

// Read stdcall stack cleanup bytes from a fixture RET instruction.
static uint32_t ret_stack_bytes(const DTTR_TestDecodedInstruction *decoded) {
	if (decoded->instruction.raw.imm[0].size) {
		return (uint32_t)decoded->instruction.raw.imm[0].value.u;
	}

	return 0u;
}

// Compare recovered ABI stack cleanup against generated blueprint metadata.
static void assert_abi_return_matches(
	const DTTR_TestPCDOGSFixture *fixture,
	const blueprint_function *fn,
	const DTTR_TestPEImage *image,
	uintptr_t site
) {
	const uint32_t expected = fn->calling_convention == DTTR_PCDOGS_CC_CDECL
								  ? 0u
								  : fn->stack_param_bytes;
	uintptr_t rva = site;
	for (size_t decoded_count = 0; decoded_count < 2048u; decoded_count++) {
		DTTR_TestDecodedInstruction decoded = {0};
		if (!dttr_test_zydis_decode32_at(image, rva, &decoded)) {
			fail_msg(
				"blueprint function %s decode failed in %s while scanning for ABI RET "
				"at "
				"0x%08X",
				fn->name,
				fixture->id,
				(unsigned)rva
			);
		}

		assert_true(decoded.instruction.length > 0);
		if (decoded.instruction.mnemonic == ZYDIS_MNEMONIC_JMP) {
			return;
		}

		if (decoded.instruction.mnemonic == ZYDIS_MNEMONIC_RET) {
			const uint32_t actual = ret_stack_bytes(&decoded);
			if (actual != expected) {
				fail_msg(
					"blueprint function %s ABI cleanup mismatch in %s at 0x%08X: "
					"expected %u got %u",
					fn->name,
					fixture->id,
					(unsigned)rva,
					(unsigned)expected,
					(unsigned)actual
				);
			}

			return;
		}

		rva += decoded.instruction.length;
		if (rva >= image->image_size) {
			break;
		}
	}

	fail_msg("blueprint function %s has no decoded RET in %s", fn->name, fixture->id);
}

// Verify one generated function row resolves and matches hook/ABI expectations.
static void assert_blueprint_function_resolved(
	const DTTR_TestPCDOGSFixture *fixture,
	const blueprint_function *fn,
	const DTTR_TestPEImage *image
) {
	const uintptr_t site = blueprint_function_site(fixture, fn, image);
	assert_decodes_at(fixture, "blueprint function", fn->name, image, site);
	assert_abi_return_matches(fixture, fn, image, site);
	if (fn->hook_kind == DTTR_PCDOGS_HOOK_REL32) {
		assert_patch_window_decodes(fixture, fn, image, site, fn->patch_size);
	} else if (fn->hook_kind == DTTR_PCDOGS_HOOK_HOTPATCH) {
		if (site < 5u) {
			fail_msg("blueprint hotpatch function %s has no pre-entry slot", fn->name);
		}

		assert_patch_window_decodes(fixture, fn, image, site - 5u, 5u);
		assert_patch_window_decodes(fixture, fn, image, site, fn->entry_patch_size);
	}
}

// Run generated function expectations against one loaded PCDOGS fixture.
static bool assert_blueprint_functions_for_fixture(
	size_t fixture_index,
	const DTTR_TestBinaryFixture *fixture,
	const char *path,
	const DTTR_TestPEImage *image,
	void *userdata
) {
	for (size_t fn_index = 0; fn_index < FUNCTION_COUNT; fn_index++) {
		const blueprint_function *fn = &DTTR_PCDOGS_FUNCTIONS[fn_index];
		if (dttr_test_fixture_required(fn->required, fixture_index)) {
			assert_blueprint_function_resolved(fixture, fn, image);
		}
	}

	return true;
}

// Covers generated function rows resolving with matching hook windows and ABI data.
static void test_blueprint_functions_resolve_and_match_abi(void **state) {
	dttr_test_require_available(pcdogs_fixtures_available());
	assert_true(pcdogs_for_each_fixture(assert_blueprint_functions_for_fixture, NULL));
}

// Check every declared reference, including alternatives hidden by first-match selection.
static bool assert_global_xrefs_for_fixture(
	size_t fixture_index,
	const DTTR_TestBinaryFixture *fixture,
	const char *path,
	const DTTR_TestPEImage *image,
	void *userdata
) {
	uintptr_t functions[FUNCTION_COUNT] = {0};
	uint32_t targets[BLUEPRINT_DATA_COUNT] = {0};

	const IMAGE_DOS_HEADER *dos = (const IMAGE_DOS_HEADER *)image->image;
	const IMAGE_NT_HEADERS32 *nt = (const IMAGE_NT_HEADERS32 *)(image->image
																+ dos->e_lfanew);
	const uint32_t base = nt->OptionalHeader.ImageBase;

	for (size_t i = 0; i < DTTR_ARRAY_COUNT(DTTR_PCDOGS_DATA_XREFS); ++i) {
		const blueprint_data_xref *ref = &DTTR_PCDOGS_DATA_XREFS[i];

		if (!dttr_test_fixture_required(ref->required, fixture_index)) {
			continue;
		}

		const blueprint_function *fn = &DTTR_PCDOGS_FUNCTIONS[ref->function_index];

		if (!functions[ref->function_index]) {
			functions[ref->function_index] = blueprint_function_site(fixture, fn, image);
		}

		const uintptr_t entry = functions[ref->function_index];
		const uintptr_t site = entry + ref->instr_off;
		uintptr_t cursor = entry;
		DTTR_TestDecodedInstruction decoded = {0};

		while (cursor < site) {
			assert_true(dttr_test_zydis_decode32_at(image, cursor, &decoded));
			cursor += decoded.instruction.length;
		}

		if (cursor != site || !dttr_test_zydis_decode32_at(image, site, &decoded)) {
			fail_msg("%s %s xref is not an instruction boundary", fixture->id, ref->name);
		}

		bool operand = decoded.instruction.raw.disp.size == 32
					   && decoded.instruction.raw.disp.offset == ref->addr_off;

		for (unsigned j = 0; j < 2; ++j) {
			operand |= decoded.instruction.raw.imm[j].size == 32
					   && !decoded.instruction.raw.imm[j].is_relative
					   && decoded.instruction.raw.imm[j].offset == ref->addr_off;
		}

		if (!operand) {
			fail_msg(
				"%s %s xref does not select an absolute 32-bit operand",
				fixture->id,
				ref->name
			);
		}

		assert_true(
			dttr_test_signed_range_valid(site, ref->addr_off, 4, image->image_size)
		);

		uint32_t target = 0;
		memcpy(&target, image->image + site + ref->addr_off, 4);

		for (uint32_t j = 0; j < ref->indirections; ++j) {
			assert_true(
				target >= base && (size_t)(target - base) <= image->image_size - 4
			);
			memcpy(&target, image->image + target - base, 4);
		}

		if (target < base || (size_t)(target - base) >= image->image_size) {
			fail_msg(
				"%s %s xref points outside module: 0x%08X",
				fixture->id,
				ref->name,
				target
			);
		}

		if (targets[ref->data_index] && targets[ref->data_index] != target) {
			fail_msg(
				"%s %s xrefs disagree: 0x%08X / 0x%08X",
				fixture->id,
				ref->name,
				targets[ref->data_index],
				target
			);
		}

		targets[ref->data_index] = target;
	}

	return true;
}

static void test_global_xrefs_select_consistent_objects(void **state) {
	dttr_test_require_available(pcdogs_fixtures_available());
	assert_true(pcdogs_for_each_fixture(assert_global_xrefs_for_fixture, NULL));
}

static const DTTR_TestCase TEST_CASES[] = {
	{"signatures", test_expected_pcdogs_signatures_resolve},
	{"global-xrefs", test_global_xrefs_select_consistent_objects},
	{"blueprint-functions", test_blueprint_functions_resolve_and_match_abi},
};

DTTR_TEST_MAIN(TEST_CASES)
