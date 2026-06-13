#define DTTR_TEST_BINARY_SUPPORT
#include "dttr_test_support.h"
#undef DTTR_TEST_BINARY_SUPPORT

#include <dttr_path.h>
#include <dttr_sigscan.h>

#include <Zydis/Utils.h>
#include <physfs.h>
#include <xxhash.h>

#include <stdlib.h>
#include <string.h>

static void dttr_test_pe_free_image(DTTR_TestPEImage *image);
static bool dttr_test_pe_fixture_hash_matches(
	const DTTR_TestBinaryFixture *fixture,
	const DTTR_TestPEImage *image
);

// Returns mapped PE bytes only after the requested RVA range is proven in bounds.
static const uint8_t *pe_rva_bytes(
	const DTTR_TestPEImage *image,
	uintptr_t rva,
	size_t size
) {
	if (!image || !dttr_test_range_valid((size_t)rva, size, image->image_size)) {
		return NULL;
	}

	return image->image + rva;
}

static sds dttr_test_join_path(const char *dir, const char *file) {
	sds path = sdsnew(dir ? dir : "");

	if (!path) {
		return NULL;
	}

	if (!DTTR_Path_AppendSegment(&path, file ? file : "", '/')) {
		sdsfree(path);
		return NULL;
	}

	return path;
}

// Mounts fixture storage and records whether this helper owns the PhysicsFS lifetime.
static bool mount_fixture_dir(const char *fixture_dir, bool *out_initialized) {
	if (!fixture_dir || !out_initialized) {
		return false;
	}

	*out_initialized = false;
	if (PHYSFS_isInit()) {
		return PHYSFS_mount(fixture_dir, NULL, 1) != 0;
	}

	if (!PHYSFS_init("dttr-tests")) {
		return false;
	}

	*out_initialized = true;
	if (PHYSFS_mount(fixture_dir, NULL, 1)) {
		return true;
	}

	PHYSFS_deinit();
	*out_initialized = false;
	return false;
}

// Unmounts fixture storage and tears down PhysicsFS when owned.
static void unmount_fixture_dir(const char *fixture_dir, bool initialized) {
	if (fixture_dir) {
		PHYSFS_unmount(fixture_dir);
	}

	if (initialized) {
		PHYSFS_deinit();
	}
}

// Reads a mounted fixture file into an owned buffer before PE validation maps it.
static bool read_fixture_file(const char *filename, uint8_t **out, size_t *out_size) {
	if (!filename || !out || !out_size) {
		return false;
	}

	PHYSFS_File *file = PHYSFS_openRead(filename);

	if (!file) {
		return false;
	}

	const PHYSFS_sint64 len = PHYSFS_fileLength(file);

	if (len < 0 || (PHYSFS_uint64)len > (PHYSFS_uint64)SIZE_MAX) {
		PHYSFS_close(file);
		return false;
	}

	const size_t size = (size_t)len;
	uint8_t *data = malloc(size ? size : 1u);

	if (!data) {
		PHYSFS_close(file);
		return false;
	}

	const bool read_ok = PHYSFS_readBytes(file, data, size) == (PHYSFS_sint64)size;
	const bool close_ok = PHYSFS_close(file) != 0;

	if (!read_ok || !close_ok) {
		free(data);
		return false;
	}

	*out = data;
	*out_size = size;
	return true;
}

bool dttr_test_range_valid(size_t offset, size_t size, size_t total) {
	return offset <= total && size <= total - offset;
}

// Converts signed target offsets without overflowing INT32_MIN.
static uintptr_t signed_offset_magnitude(int32_t offset) {
	return offset < 0 ? (uintptr_t)(-(int64_t)offset) : (uintptr_t)offset;
}

bool dttr_test_signed_range_valid(
	uintptr_t base,
	int32_t offset,
	size_t size,
	size_t total
) {
	const uintptr_t magnitude = signed_offset_magnitude(offset);

	if (offset < 0) {
		if (base < magnitude) {
			return false;
		}

		base -= magnitude;
		return dttr_test_range_valid((size_t)base, size, total);
	}

	if (base > (uintptr_t)SIZE_MAX - magnitude) {
		return false;
	}

	base += magnitude;
	return dttr_test_range_valid((size_t)base, size, total);
}

uintptr_t dttr_test_offset_site(uintptr_t base, int32_t offset) {
	const uintptr_t magnitude = signed_offset_magnitude(offset);
	return offset < 0 ? base - magnitude : base + magnitude;
}

static bool dttr_test_bytes_match_mask(
	const uint8_t *actual,
	const uint8_t *expected,
	const char *mask,
	size_t size
) {
	if (!actual || !expected || !mask || strlen(mask) != size) {
		return false;
	}

	for (size_t i = 0; i < size; i++) {
		if (mask[i] == 'x' && actual[i] != expected[i]) {
			return false;
		}
	}

	return true;
}

bool dttr_test_case_equal(const char *a, const char *b) {
	return a && b && PHYSFS_utf8stricmp(a, b) == 0;
}

bool dttr_test_fixture_required(DTTR_TestFixtureMask required, size_t fixture_index) {
	return fixture_index < 64u && (required & DTTR_TEST_FIXTURE_BIT(fixture_index)) != 0;
}

bool dttr_test_fixtures_available(
	const DTTR_TestBinaryFixture *fixtures,
	size_t fixture_count,
	const char *fixture_dir
) {
	if (!fixtures || !fixture_dir) {
		return false;
	}

	bool initialized = false;

	if (!mount_fixture_dir(fixture_dir, &initialized)) {
		return false;
	}

	bool ok = true;

	for (size_t i = 0; i < fixture_count; i++) {
		PHYSFS_Stat stat;

		if (!PHYSFS_stat(fixtures[i].filename, &stat)
			|| stat.filetype != PHYSFS_FILETYPE_REGULAR) {
			ok = false;
			break;
		}
	}

	unmount_fixture_dir(fixture_dir, initialized);
	return ok;
}

// Finds the PE section table only after its numeric file range is validated.
static bool pe_section_table(
	const DTTR_TestPEImage *image,
	size_t pe_offset,
	const IMAGE_NT_HEADERS32 *nt,
	const IMAGE_SECTION_HEADER **out_sections
) {
	if (!image || !nt || !out_sections) {
		return false;
	}

	const size_t optional_offset = pe_offset
								   + offsetof(IMAGE_NT_HEADERS32, OptionalHeader);
	if (optional_offset < pe_offset) {
		return false;
	}

	const size_t section_header = optional_offset + nt->FileHeader.SizeOfOptionalHeader;
	if (section_header < optional_offset) {
		return false;
	}

	const size_t section_count = nt->FileHeader.NumberOfSections;
	if (section_count > SIZE_MAX / sizeof(IMAGE_SECTION_HEADER)) {
		return false;
	}

	const size_t section_table_size = section_count * sizeof(IMAGE_SECTION_HEADER);
	if (!dttr_test_range_valid(section_header, section_table_size, image->file_size)) {
		return false;
	}

	*out_sections = (const IMAGE_SECTION_HEADER *)(image->file + section_header);
	return true;
}

// Copies one file-backed PE section to its virtual address in the mapped image.
static bool pe_copy_section(DTTR_TestPEImage *image, const IMAGE_SECTION_HEADER *section) {
	if (section->SizeOfRawData == 0) {
		return true;
	}

	if (!dttr_test_range_valid(
			section->PointerToRawData,
			section->SizeOfRawData,
			image->file_size
		)
		|| !dttr_test_range_valid(
			section->VirtualAddress,
			section->SizeOfRawData,
			image->image_size
		)) {
		return false;
	}

	memcpy(
		image->image + section->VirtualAddress,
		image->file + section->PointerToRawData,
		section->SizeOfRawData
	);
	return true;
}

// Parses a 32-bit PE fixture and builds the mapped image used by signature tests.
static bool load_pe_image(uint8_t *file, size_t file_size, DTTR_TestPEImage *image) {
	if (!file || !image) {
		return false;
	}

	memset(image, 0, sizeof(*image));
	image->file = file;
	image->file_size = file_size;

	if (image->file_size < sizeof(IMAGE_DOS_HEADER)) {
		goto fail;
	}

	const IMAGE_DOS_HEADER *dos = (const IMAGE_DOS_HEADER *)image->file;

	if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew < 0) {
		goto fail;
	}

	const size_t pe_offset = (size_t)dos->e_lfanew;

	if (!dttr_test_range_valid(pe_offset, sizeof(IMAGE_NT_HEADERS32), image->file_size)) {
		goto fail;
	}

	const IMAGE_NT_HEADERS32 *nt = (const IMAGE_NT_HEADERS32 *)(image->file + pe_offset);

	if (nt->Signature != IMAGE_NT_SIGNATURE
		|| nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC
		|| nt->OptionalHeader.SizeOfImage == 0) {
		goto fail;
	}

	const IMAGE_OPTIONAL_HEADER32 *optional = &nt->OptionalHeader;
	if (optional->SizeOfHeaders > optional->SizeOfImage) {
		goto fail;
	}

	image->image = calloc(1, optional->SizeOfImage);

	if (!image->image) {
		goto fail;
	}

	image->image_size = optional->SizeOfImage;
	image->imports_dir = optional->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];

	size_t header_copy = optional->SizeOfHeaders;
	header_copy = header_copy < image->file_size ? header_copy : image->file_size;
	if (header_copy > image->image_size) {
		goto fail;
	}

	memcpy(image->image, image->file, header_copy);

	const IMAGE_SECTION_HEADER *sections = NULL;

	if (!pe_section_table(image, pe_offset, nt, &sections)) {
		goto fail;
	}

	for (uint16_t i = 0; i < nt->FileHeader.NumberOfSections; i++) {
		if (!pe_copy_section(image, &sections[i])) {
			goto fail;
		}
	}

	return true;

fail:
	dttr_test_pe_free_image(image);
	return false;
}

static bool dttr_test_pe_load_fixture(
	const DTTR_TestBinaryFixture *fixtures,
	size_t fixture_count,
	size_t fixture_index,
	const char *fixture_dir,
	sds *out_path,
	DTTR_TestPEImage *out_image
) {
	if (!fixtures || !fixture_dir || fixture_index >= fixture_count || !out_path
		|| !out_image) {
		return false;
	}

	const DTTR_TestBinaryFixture *fixture = &fixtures[fixture_index];
	sds path = dttr_test_join_path(fixture_dir, fixture->filename);

	if (!path) {
		return false;
	}

	bool initialized = false;

	if (!mount_fixture_dir(fixture_dir, &initialized)) {
		sdsfree(path);
		return false;
	}

	uint8_t *file = NULL;
	size_t file_size = 0;
	const bool loaded = read_fixture_file(fixture->filename, &file, &file_size)
						&& load_pe_image(file, file_size, out_image)
						&& dttr_test_pe_fixture_hash_matches(fixture, out_image);

	unmount_fixture_dir(fixture_dir, initialized);

	if (!loaded) {
		dttr_test_pe_free_image(out_image);
		sdsfree(path);
		return false;
	}

	*out_path = path;
	return true;
}

bool dttr_test_pe_for_each_fixture(
	const DTTR_TestBinaryFixture *fixtures,
	size_t fixture_count,
	const char *fixture_dir,
	DTTR_TestPEFixtureVisitor visitor,
	void *userdata
) {
	if (!fixtures || !fixture_dir || !visitor) {
		return false;
	}

	for (size_t i = 0; i < fixture_count; i++) {
		const DTTR_TestBinaryFixture *fixture = &fixtures[i];
		sds path = NULL;
		DTTR_TestPEImage image = {0};

		if (!dttr_test_pe_load_fixture(
				fixtures,
				fixture_count,
				i,
				fixture_dir,
				&path,
				&image
			)) {
			return false;
		}

		const bool ok = visitor(i, fixture, path, &image, userdata);
		dttr_test_pe_free_image(&image);
		sdsfree(path);

		if (!ok) {
			return false;
		}
	}

	return true;
}

static void dttr_test_pe_free_image(DTTR_TestPEImage *image) {
	if (!image) {
		return;
	}

	free(image->image);
	free(image->file);
	memset(image, 0, sizeof(*image));
}

size_t DTTR_TestPE_SigscanCount(
	const DTTR_TestPEImage *image,
	const uint8_t *sig,
	const char *mask
) {
	if (!image || !image->image || !sig || !mask) {
		return 0;
	}

	return DTTR_Sigscan_BytesAll(image->image, image->image_size, sig, mask, NULL, 0);
}

uintptr_t DTTR_TestPE_Sigscan(
	const DTTR_TestPEImage *image,
	const uint8_t *sig,
	const char *mask
) {
	if (!image || !sig || !mask) {
		return DTTR_TEST_SIG_NOT_FOUND;
	}

	const uint8_t *match = DTTR_Sigscan_Bytes(image->image, image->image_size, sig, mask);
	return match ? (uintptr_t)(match - image->image) : DTTR_TEST_SIG_NOT_FOUND;
}

static uint64_t dttr_test_pe_file_hash(const DTTR_TestPEImage *image) {
	if (!image || !image->file) {
		return 0;
	}

	return XXH3_64bits(image->file, image->file_size);
}

static bool dttr_test_pe_fixture_hash_matches(
	const DTTR_TestBinaryFixture *fixture,
	const DTTR_TestPEImage *image
) {
	if (!fixture || !image) {
		return false;
	}

	return image->file_size == fixture->size
		   && dttr_test_pe_file_hash(image) == fixture->xxh3;
}

static const char *dttr_test_pe_cstr(const DTTR_TestPEImage *image, uintptr_t rva) {
	const char *str = (const char *)pe_rva_bytes(image, rva, 1);

	if (!str) {
		return NULL;
	}

	return memchr(str, '\0', image->image_size - (size_t)rva) ? str : NULL;
}

size_t DTTR_TestPE_CollectImports(
	const DTTR_TestPEImage *image,
	DTTR_TestImportEntry *imports,
	size_t imports_cap
) {
	if (!image || !imports) {
		return 0;
	}

	if (image->imports_dir.VirtualAddress == 0) {
		return 0;
	}

	const uintptr_t imports_rva = image->imports_dir.VirtualAddress;
	size_t count = 0;

	for (uintptr_t desc_rva = imports_rva;; desc_rva += sizeof(IMAGE_IMPORT_DESCRIPTOR)) {
		const IMAGE_IMPORT_DESCRIPTOR *desc = (const IMAGE_IMPORT_DESCRIPTOR *)
			pe_rva_bytes(image, desc_rva, sizeof(*desc));

		if (!desc || desc->Name == 0) {
			return count;
		}

		const char *dll = dttr_test_pe_cstr(image, desc->Name);
		uintptr_t name_thunk_rva = desc->OriginalFirstThunk ? desc->OriginalFirstThunk
															: desc->FirstThunk;
		uintptr_t addr_thunk_rva = desc->FirstThunk;

		if (!dll || !name_thunk_rva || !addr_thunk_rva) {
			continue;
		}

		for (;; name_thunk_rva += sizeof(IMAGE_THUNK_DATA32),
				addr_thunk_rva += sizeof(IMAGE_THUNK_DATA32)) {
			const IMAGE_THUNK_DATA32 *name_thunk = (const IMAGE_THUNK_DATA32 *)
				pe_rva_bytes(image, name_thunk_rva, sizeof(*name_thunk));

			if (!name_thunk) {
				return count;
			}

			if (name_thunk->u1.AddressOfData == 0) {
				break;
			}

			if (IMAGE_SNAP_BY_ORDINAL32(name_thunk->u1.Ordinal)) {
				continue;
			}

			const uintptr_t import_name_rva = name_thunk->u1.AddressOfData;
			const IMAGE_IMPORT_BY_NAME *import = (const IMAGE_IMPORT_BY_NAME *)
				pe_rva_bytes(image, import_name_rva, sizeof(WORD));
			const char *import_name = dttr_test_pe_cstr(
				image,
				import_name_rva + offsetof(IMAGE_IMPORT_BY_NAME, Name)
			);

			if (!import || !import_name || count >= imports_cap) {
				return count;
			}

			imports[count++] = (DTTR_TestImportEntry){
				.dll = dll,
				.name = import_name,
				.iat_site = addr_thunk_rva,
			};
		}
	}
}

// Lazily initializes the shared 32-bit Zydis decoder.
static ZydisDecoder *zydis_decoder32() {
	static ZydisDecoder decoder;
	static bool initialized = false;

	if (!initialized) {
		if (!ZYAN_SUCCESS(ZydisDecoderInit(
				&decoder,
				ZYDIS_MACHINE_MODE_LEGACY_32,
				ZYDIS_STACK_WIDTH_32
			))) {
			return NULL;
		}

		initialized = true;
	}

	return &decoder;
}

static bool dttr_test_zydis_decode32(
	const uint8_t *bytes,
	size_t size,
	DTTR_TestDecodedInstruction *out
) {
	ZydisDecoder *decoder = zydis_decoder32();

	if (!decoder || !bytes || !out) {
		return false;
	}

	memset(out, 0, sizeof(*out));
	return ZYAN_SUCCESS(
		ZydisDecoderDecodeFull(decoder, bytes, size, &out->instruction, out->operands)
	);
}

bool dttr_test_zydis_decode32_at(
	const DTTR_TestPEImage *image,
	uintptr_t rva,
	DTTR_TestDecodedInstruction *out
) {
	if (!image || rva >= image->image_size) {
		return false;
	}

	return dttr_test_zydis_decode32(
		image->image + rva,
		image->image_size - (size_t)rva,
		out
	);
}

static bool dttr_test_zydis_decode32_prefix(
	const DTTR_TestPEImage *image,
	uintptr_t rva,
	size_t required_size,
	size_t *out_size
) {
	if (!image || rva >= image->image_size) {
		return false;
	}

	size_t decoded_size = 0;

	while (decoded_size < required_size) {
		DTTR_TestDecodedInstruction decoded = {0};

		if (!dttr_test_zydis_decode32_at(image, rva + decoded_size, &decoded)
			|| decoded.instruction.length == 0) {
			return false;
		}

		decoded_size += decoded.instruction.length;
	}

	if (out_size) {
		*out_size = decoded_size;
	}

	return true;
}

static bool dttr_test_zydis_absolute_operand(
	const DTTR_TestDecodedInstruction *decoded,
	size_t operand_index,
	uintptr_t runtime_address,
	uintptr_t *out_address
) {
	if (!decoded || !out_address || operand_index >= ZYDIS_MAX_OPERAND_COUNT) {
		return false;
	}

	ZyanU64 address = 0;

	if (!ZYAN_SUCCESS(ZydisCalcAbsoluteAddress(
			&decoded->instruction,
			&decoded->operands[operand_index],
			(ZyanU64)runtime_address,
			&address
		))) {
		return false;
	}

	if (address > (ZyanU64)UINTPTR_MAX) {
		return false;
	}

	*out_address = (uintptr_t)address;
	return true;
}

// Validates the CALL-to-JMP pattern used by pointer targets.
static void assert_pointer_ff25_e8_target(
	const DTTR_TestBinaryFixture *fixture,
	const DTTR_TestTargetExpectation *target,
	const DTTR_TestPEImage *image,
	uintptr_t match
) {
	DTTR_TestDecodedInstruction call = {0};
	assert_true(dttr_test_zydis_decode32_at(image, match, &call));
	assert_int_equal(call.instruction.mnemonic, ZYDIS_MNEMONIC_CALL);
	assert_int_equal(call.operands[0].type, ZYDIS_OPERAND_TYPE_IMMEDIATE);

	uintptr_t thunk = 0;
	assert_true(dttr_test_zydis_absolute_operand(&call, 0, match, &thunk));

	if (!dttr_test_range_valid((size_t)thunk, 6, image->image_size)) {
		fail_msg(
			"%s resolved outside image in %s: match=0x%08X thunk=0x%08X",
			target->name,
			fixture->id,
			(unsigned)match,
			(unsigned)thunk
		);
	}

	DTTR_TestDecodedInstruction jmp = {0};
	assert_true(dttr_test_zydis_decode32_at(image, thunk, &jmp));
	assert_int_equal(jmp.instruction.mnemonic, ZYDIS_MNEMONIC_JMP);
	assert_int_equal(jmp.operands[0].type, ZYDIS_OPERAND_TYPE_MEMORY);

	uintptr_t target_address = 0;
	assert_true(dttr_test_zydis_absolute_operand(&jmp, 0, thunk, &target_address));
	assert_true(target_address != 0);
}

// Validates MOV-based pointer targets whose address is loaded from fixture memory.
static void assert_pointer_u32_at_match_plus_2(
	const DTTR_TestPEImage *image,
	uintptr_t match
) {
	DTTR_TestDecodedInstruction mov = {0};
	assert_true(dttr_test_zydis_decode32_at(image, match, &mov));
	assert_int_equal(mov.operands[1].type, ZYDIS_OPERAND_TYPE_MEMORY);

	uintptr_t target_address = 0;
	assert_true(dttr_test_zydis_absolute_operand(&mov, 1, match, &target_address));
	assert_true(target_address != 0);
}

// Verifies byte-patch targets still expose the expected original instruction bytes.
static void assert_byte_patch_site(
	const DTTR_TestBinaryFixture *fixture,
	const DTTR_TestTargetExpectation *target,
	const DTTR_TestPEImage *image,
	uintptr_t match
) {
	assert_non_null(target->patch_bytes);
	assert_true(target->patch_size > 0);
	assert_non_null(target->expected_original);
	assert_non_null(target->expected_original_mask);
	assert_true(dttr_test_signed_range_valid(
		match,
		target->site_offset,
		target->patch_size,
		image->image_size
	));

	const uintptr_t site = dttr_test_offset_site(match, target->site_offset);
	DTTR_TestDecodedInstruction decoded = {0};
	assert_true(dttr_test_zydis_decode32_at(image, site, &decoded));
	assert_true(decoded.instruction.length > 0);
	assert_true(decoded.instruction.length <= target->patch_size);

	const uint8_t *actual = image->image + site;

	if (!dttr_test_bytes_match_mask(
			actual,
			target->expected_original,
			target->expected_original_mask,
			target->patch_size
		)) {
		fail_msg(
			"%s original bytes mismatch in %s at 0x%08X",
			target->name,
			fixture->id,
			(unsigned)site
		);
	}

	assert_memory_not_equal(actual, target->patch_bytes, target->patch_size);
}

void dttr_test_assert_target_resolved(
	const DTTR_TestBinaryFixture *fixture,
	const DTTR_TestTargetExpectation *target,
	const DTTR_TestPEImage *image
) {
	assert_non_null(fixture);
	assert_non_null(target);
	assert_non_null(image);
	assert_non_null(target->aob);

	char sig[256];
	char mask[256];
	if (DTTR_Sigscan_ParseAob(target->aob, sig, mask, sizeof(sig)) == 0) {
		fail_msg("target %s has a malformed AOB pattern: %s", target->name, target->aob);
	}

	const uintptr_t match = DTTR_TestPE_Sigscan(image, (const uint8_t *)sig, mask);

	if (match == DTTR_TEST_SIG_NOT_FOUND) {
		fail_msg(
			"required target %s did not resolve in %s (%s)",
			target->name,
			fixture->id,
			fixture->filename
		);
	}

	const size_t matches = DTTR_TestPE_SigscanCount(image, (const uint8_t *)sig, mask);
	if (matches != 1) {
		fail_msg(
			"required target %s resolved %zu times in %s (%s); expected exactly one "
			"match",
			target->name,
			matches,
			fixture->id,
			fixture->filename
		);
	}

	DTTR_TestDecodedInstruction decoded = {0};
	assert_true(dttr_test_zydis_decode32_at(image, match, &decoded));
	assert_true(decoded.instruction.length > 0);

	switch (target->kind) {
	case DTTR_TEST_TARGET_RESOLVE:
		break;
	case DTTR_TEST_TARGET_JMP_HOOK:
	case DTTR_TEST_TARGET_TRAMPOLINE_HOOK: {
		size_t decoded_size = 0;
		assert_true(dttr_test_zydis_decode32_prefix(image, match, 5, &decoded_size));
		assert_true(decoded_size >= 5);
		break;
	}
	case DTTR_TEST_TARGET_POINTER_FF25_E8_TARGET:
		assert_pointer_ff25_e8_target(fixture, target, image, match);
		break;
	case DTTR_TEST_TARGET_POINTER_U32_AT_MATCH_PLUS_2:
		assert_pointer_u32_at_match_plus_2(image, match);
		break;
	case DTTR_TEST_TARGET_BYTE_PATCH:
		assert_byte_patch_site(fixture, target, image, match);
		break;
	}
}
