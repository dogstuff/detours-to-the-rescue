/// Builds an in-memory SDB (shim database) and packs it into a binary blob
/// that can be injected into a child process using PEB->m_shim_data.
///
/// This overrides any compatibility shims applied by shims, notably
/// the EmulateHeap corruption issue that occurs on Intel Integrated
/// Graphics.

#include <dttr_errors.h>
#include <dttr_loader.h>
#include <log.h>
#include <sds.h>
#include <string.h>
#include <windows.h>

#define S_RESOLVE(module, type, name) ((type)DTTR_UNWRAP_WINAPI_EXISTS(GetProcAddress(module, name)))

#define SDB_TAG_DATABASE 0x7001
#define SDB_TAG_EXE 0x7007
#define SDB_TAG_MATCHING_FILE 0x7008
#define SDB_TAG_SHIM_REF 0x7009
#define SDB_TAG_NAME 0x6001
#define SDB_TAG_APP_NAME 0x6006
#define SDB_TAG_VENDOR 0x6005
#define SDB_TAG_COMPILER_VERSION 0x6022
#define SDB_TAG_OS_PLATFORM 0x4023
#define SDB_TAG_DATABASE_ID 0x9007
#define SDB_TAG_EXE_ID 0x9004

#define SDB_MAX_EXES 16
#define SDB_MAX_LAYERS 8
#define SDB_MAX_SDBS 16
#define HID_DOS_PATHS 0x00000001
#define HID_DATABASE_FULLPATH 0x00000002

typedef PVOID s_pdb;
typedef PVOID s_hsdb;
typedef DWORD s_tagref;

typedef s_pdb(WINAPI *s_pfn_sdb_create_database)(LPCWSTR, int);
typedef void(WINAPI *s_pfn_sdb_close_database_write)(s_pdb);
typedef DWORD(WINAPI *s_pfn_sdb_begin_write_list_tag)(s_pdb, DWORD);
typedef BOOL(WINAPI *s_pfn_sdb_end_write_list_tag)(s_pdb, DWORD);
typedef BOOL(WINAPI *s_pfn_sdb_write_string_tag)(s_pdb, DWORD, LPCWSTR);
typedef BOOL(WINAPI *s_pfn_sdb_write_dword_tag)(s_pdb, DWORD, DWORD);
typedef BOOL(WINAPI *s_pfn_sdb_write_binary_tag)(s_pdb, DWORD, BYTE *, DWORD);

typedef struct {
	s_tagref m_exes[SDB_MAX_EXES];
	DWORD m_exe_flags[SDB_MAX_EXES];
	s_tagref m_layers[SDB_MAX_LAYERS];
	DWORD m_layer_flags;
	s_tagref m_apphelp;
	DWORD m_exe_count;
	DWORD m_layer_count;
	GUID m_guid_id;
	DWORD m_flags;
	DWORD m_custom_sdb_map;
	GUID m_guid_db[SDB_MAX_SDBS];
} s_sdb_query_result;

typedef s_hsdb(WINAPI *s_pfn_sdb_init_database)(DWORD, LPCWSTR);
typedef void(WINAPI *s_pfn_sdb_release_database)(s_hsdb);
typedef BOOL(WINAPI *s_pfn_sdb_get_matching_exe)(s_hsdb, LPCWSTR, LPCWSTR, LPCWSTR, DWORD, s_sdb_query_result *);
typedef BOOL(WINAPI *s_pfn_sdb_pack_app_compat_data)(s_hsdb, s_sdb_query_result *, PVOID *, DWORD *);

// clang-format off
static const BYTE s_database_guid[16] = {
	0xc3, 0x5c, 0xf3, 0xd4, 0xad, 0xad, 0x09, 0x4f,
	0x8f, 0x00, 0x9e, 0x69, 0x3b, 0x55, 0x4b, 0xf4,
};

static const BYTE s_exe_guid[16] = {
	0xa5, 0x19, 0x62, 0x40, 0x3f, 0x50, 0x9b, 0x47,
	0xa6, 0x87, 0xe6, 0x63, 0xa0, 0x77, 0x58, 0xe8,
};
// clang-format on

static void s_generate_sdb(HMODULE apphelp, WCHAR *out_path) {
	const s_pfn_sdb_create_database sdb_create = S_RESOLVE(apphelp, s_pfn_sdb_create_database, "SdbCreateDatabase");
	const s_pfn_sdb_close_database_write sdb_close
		= S_RESOLVE(apphelp, s_pfn_sdb_close_database_write, "SdbCloseDatabaseWrite");
	const s_pfn_sdb_begin_write_list_tag sdb_begin_list
		= S_RESOLVE(apphelp, s_pfn_sdb_begin_write_list_tag, "SdbBeginWriteListTag");
	const s_pfn_sdb_end_write_list_tag sdb_end_list
		= S_RESOLVE(apphelp, s_pfn_sdb_end_write_list_tag, "SdbEndWriteListTag");
	const s_pfn_sdb_write_string_tag sdb_string = S_RESOLVE(apphelp, s_pfn_sdb_write_string_tag, "SdbWriteStringTag");
	const s_pfn_sdb_write_dword_tag sdb_dword = S_RESOLVE(apphelp, s_pfn_sdb_write_dword_tag, "SdbWriteDWORDTag");
	const s_pfn_sdb_write_binary_tag sdb_binary = S_RESOLVE(apphelp, s_pfn_sdb_write_binary_tag, "SdbWriteBinaryTag");

	WCHAR tmp_dir[MAX_PATH];
	DTTR_UNWRAP_WINAPI_NONZERO(GetTempPathW(MAX_PATH, tmp_dir));

	DTTR_UNWRAP_WINAPI_NONZERO(GetTempFileNameW(tmp_dir, L"sdb", 0, out_path));

	// GetTempFileNameW creates the file, so remove it before SdbCreateDatabase recreates it.
	DeleteFileW(out_path);

	s_pdb pdb = sdb_create(out_path, 0);
	if (!pdb) {
		DTTR_FATAL("SdbCreateDatabase failed");
	}

	const DWORD ti_db = sdb_begin_list(pdb, SDB_TAG_DATABASE);
	sdb_string(pdb, SDB_TAG_COMPILER_VERSION, L"3.0.0.16");
	sdb_string(pdb, SDB_TAG_NAME, L"PCDogs DttR Compatibility Fixes");
	sdb_dword(pdb, SDB_TAG_OS_PLATFORM, 1);
	sdb_binary(pdb, SDB_TAG_DATABASE_ID, (BYTE *)s_database_guid, 16);

	const DWORD ti_exe = sdb_begin_list(pdb, SDB_TAG_EXE);
	sdb_string(pdb, SDB_TAG_NAME, L"pcdogs.exe");
	sdb_string(pdb, SDB_TAG_APP_NAME, L"PCDogs DttR");
	sdb_string(pdb, SDB_TAG_VENDOR, L"Cruella");
	sdb_binary(pdb, SDB_TAG_EXE_ID, (BYTE *)s_exe_guid, 16);

	const DWORD ti_match = sdb_begin_list(pdb, SDB_TAG_MATCHING_FILE);
	sdb_string(pdb, SDB_TAG_NAME, L"*");
	sdb_end_list(pdb, ti_match);

	DWORD ti_ref = sdb_begin_list(pdb, SDB_TAG_SHIM_REF);
	sdb_string(pdb, SDB_TAG_NAME, L"EmulateHeap");
	sdb_end_list(pdb, ti_ref);

	ti_ref = sdb_begin_list(pdb, SDB_TAG_SHIM_REF);
	sdb_string(pdb, SDB_TAG_NAME, L"EmulateIME");
	sdb_end_list(pdb, ti_ref);

	sdb_end_list(pdb, ti_exe);
	sdb_end_list(pdb, ti_db);
	sdb_close(pdb);

	log_debug("SDB authored to %ls", out_path);
}

sds dttr_compat_build_shim_data(const WCHAR *image_name) {
	log_debug("Building shim data via apphelp.dll");

	char apphelp_path[MAX_PATH];
	GetSystemDirectoryA(apphelp_path, MAX_PATH);
	strcat(apphelp_path, "\\apphelp.dll");
	HMODULE apphelp = DTTR_UNWRAP_WINAPI_EXISTS(LoadLibraryA(apphelp_path));

	WCHAR sdb_path[MAX_PATH];
	s_generate_sdb(apphelp, sdb_path);

	const s_pfn_sdb_init_database sdb_init = S_RESOLVE(apphelp, s_pfn_sdb_init_database, "SdbInitDatabase");
	const s_pfn_sdb_release_database sdb_release = S_RESOLVE(apphelp, s_pfn_sdb_release_database, "SdbReleaseDatabase");

	const s_pfn_sdb_get_matching_exe sdb_match = S_RESOLVE(apphelp, s_pfn_sdb_get_matching_exe, "SdbGetMatchingExe");

	const s_pfn_sdb_pack_app_compat_data sdb_pack
		= S_RESOLVE(apphelp, s_pfn_sdb_pack_app_compat_data, "SdbPackAppCompatData");

	s_hsdb hsdb = sdb_init(HID_DOS_PATHS | HID_DATABASE_FULLPATH, sdb_path);
	if (!hsdb) {
		DTTR_FATAL("SdbInitDatabase failed for %ls", sdb_path);
	}

	WCHAR full_exe_path[MAX_PATH];
	DTTR_UNWRAP_WINAPI_NONZERO(GetFullPathNameW(image_name, MAX_PATH, full_exe_path, NULL));

	s_sdb_query_result query = {0};
	if (!sdb_match(hsdb, full_exe_path, NULL, NULL, 0, &query)) {
		DTTR_FATAL("SdbGetMatchingExe found no matches for %ls", full_exe_path);
	}

	log_debug("SDB query matched: %lu exe entries, %lu layer entries", query.m_exe_count, query.m_layer_count);

	PVOID packed = NULL;
	DWORD packed_size = 0;
	if (!sdb_pack(hsdb, &query, &packed, &packed_size)) {
		DTTR_FATAL("SdbPackAppCompatData failed");
	}

	sds result = sdsnewlen(packed, packed_size);
	HeapFree(GetProcessHeap(), 0, packed);

	sdb_release(hsdb);
	DeleteFileW(sdb_path);
	FreeLibrary(apphelp);

	log_debug("Built shim data (%lu bytes)", (unsigned long)packed_size);
	return result;
}
