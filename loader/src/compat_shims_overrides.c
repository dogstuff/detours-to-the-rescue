/// Builds an in-memory SDB (shim database) and packs it into a binary blob
/// that can be injected into a child process using PEB->m_shim_data.
///
/// This overrides any compatibility shims applied by Windows, which
/// resolves the EmulateHeap corruption issue that occurs on Intel
/// integrated graphics.

#include <dttr_errors.h>
#include <dttr_loader.h>
#include <log.h>
#include <sds.h>
#include <string.h>
#include <windows.h>

#define S_RESOLVE(module, type, name)                                                    \
	((type)DTTR_UNWRAP_WINAPI_EXISTS(GetProcAddress(module, name)))

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
#define SDB_MAX_SDB_S 16
#define HID_DOS_PATHS 0x00000001
#define HID_DATABASE_FULLPATH 0x00000002

typedef PVOID S_Pdb;
typedef PVOID S_Hsdb;
typedef DWORD S_Tagref;

typedef S_Pdb(WINAPI *S_SDB_CreateDatabase)(LPCWSTR, int);
typedef void(WINAPI *S_SDB_CloseDatabaseWrite)(S_Pdb);
typedef DWORD(WINAPI *S_SDB_BeginWriteListTag)(S_Pdb, DWORD);
typedef BOOL(WINAPI *S_SDB_EndWriteListTag)(S_Pdb, DWORD);
typedef BOOL(WINAPI *S_SDB_WriteStringTag)(S_Pdb, DWORD, LPCWSTR);
typedef BOOL(WINAPI *S_SDB_WriteDwordTag)(S_Pdb, DWORD, DWORD);
typedef BOOL(WINAPI *S_SDB_WriteBinaryTag)(S_Pdb, DWORD, BYTE *, DWORD);

typedef struct {
	S_Tagref m_exes[SDB_MAX_EXES];
	DWORD m_exe_flags[SDB_MAX_EXES];
	S_Tagref m_layers[SDB_MAX_LAYERS];
	DWORD m_layer_flags;
	S_Tagref m_apphelp;
	DWORD m_exe_count;
	DWORD m_layer_count;
	GUID m_guid_id;
	DWORD m_flags;
	DWORD m_custom_sdb_map;
	GUID m_guid_db[SDB_MAX_SDB_S];
} S_SDB_QueryResult;

typedef S_Hsdb(WINAPI *S_SDB_InitDatabase)(DWORD, LPCWSTR);
typedef void(WINAPI *S_SDB_ReleaseDatabase)(S_Hsdb);
typedef BOOL(WINAPI *S_SDB_GetMatchingExe)(
	S_Hsdb,
	LPCWSTR,
	LPCWSTR,
	LPCWSTR,
	DWORD,
	S_SDB_QueryResult *
);
typedef BOOL(WINAPI *S_SDB_PackAppCompatData)(
	S_Hsdb,
	S_SDB_QueryResult *,
	PVOID *,
	DWORD *
);

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
	const S_SDB_CreateDatabase sdb_create = S_RESOLVE(
		apphelp,
		S_SDB_CreateDatabase,
		"SdbCreateDatabase"
	);
	const S_SDB_CloseDatabaseWrite sdb_close = S_RESOLVE(
		apphelp,
		S_SDB_CloseDatabaseWrite,
		"SdbCloseDatabaseWrite"
	);
	const S_SDB_BeginWriteListTag sdb_begin_list = S_RESOLVE(
		apphelp,
		S_SDB_BeginWriteListTag,
		"SdbBeginWriteListTag"
	);
	const S_SDB_EndWriteListTag sdb_end_list = S_RESOLVE(
		apphelp,
		S_SDB_EndWriteListTag,
		"SdbEndWriteListTag"
	);
	const S_SDB_WriteStringTag sdb_string = S_RESOLVE(
		apphelp,
		S_SDB_WriteStringTag,
		"SdbWriteStringTag"
	);
	const S_SDB_WriteDwordTag sdb_dword = S_RESOLVE(
		apphelp,
		S_SDB_WriteDwordTag,
		"SdbWriteDWORDTag"
	);
	const S_SDB_WriteBinaryTag sdb_binary = S_RESOLVE(
		apphelp,
		S_SDB_WriteBinaryTag,
		"SdbWriteBinaryTag"
	);

	WCHAR tmp_dir[MAX_PATH];
	DTTR_UNWRAP_WINAPI_NONZERO(GetTempPathW(MAX_PATH, tmp_dir));

	DTTR_UNWRAP_WINAPI_NONZERO(GetTempFileNameW(tmp_dir, L"sdb", 0, out_path));

	// GetTempFileNameW creates the file, so remove it before SDB_CreateDatabase recreates
	// it.
	DeleteFileW(out_path);

	S_Pdb pdb = sdb_create(out_path, 0);
	if (!pdb) {
		DTTR_FATAL("SDB_CreateDatabase failed");
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

	const S_SDB_InitDatabase sdb_init = S_RESOLVE(
		apphelp,
		S_SDB_InitDatabase,
		"SdbInitDatabase"
	);

	const S_SDB_ReleaseDatabase sdb_release = S_RESOLVE(
		apphelp,
		S_SDB_ReleaseDatabase,
		"SdbReleaseDatabase"
	);

	const S_SDB_GetMatchingExe sdb_match = S_RESOLVE(
		apphelp,
		S_SDB_GetMatchingExe,
		"SdbGetMatchingExe"
	);

	const S_SDB_PackAppCompatData sdb_pack = S_RESOLVE(
		apphelp,
		S_SDB_PackAppCompatData,
		"SdbPackAppCompatData"
	);

	S_Hsdb hsdb = sdb_init(HID_DOS_PATHS | HID_DATABASE_FULLPATH, sdb_path);
	if (!hsdb) {
		DTTR_FATAL("SDB_InitDatabase failed for %ls", sdb_path);
	}

	WCHAR full_exe_path[MAX_PATH];
	DTTR_UNWRAP_WINAPI_NONZERO(
		GetFullPathNameW(image_name, MAX_PATH, full_exe_path, NULL)
	);

	S_SDB_QueryResult query = {0};
	if (!sdb_match(hsdb, full_exe_path, NULL, NULL, 0, &query)) {
		DTTR_FATAL("SDB_GetMatchingExe found no matches for %ls", full_exe_path);
	}

	log_debug(
		"SDB query matched: %lu exe entries, %lu layer entries",
		query.m_exe_count,
		query.m_layer_count
	);

	PVOID packed = NULL;
	DWORD packed_size = 0;
	if (!sdb_pack(hsdb, &query, &packed, &packed_size)) {
		DTTR_FATAL("SDB_PackAppCompatData failed");
	}

	sds result = sdsnewlen(packed, packed_size);
	HeapFree(GetProcessHeap(), 0, packed);

	sdb_release(hsdb);
	DeleteFileW(sdb_path);
	FreeLibrary(apphelp);

	log_debug("Built shim data (%lu bytes)", (unsigned long)packed_size);
	return result;
}
