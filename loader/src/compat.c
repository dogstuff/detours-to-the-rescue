#include <dttr_errors.h>
#include <dttr_loader.h>
#include <log.h>
#include <sds.h>
#include <string.h>
#include <windows.h>

#define PS_ATTRIBUTE_IMAGE_NAME 0x00020005 // PsAttributeImageName | INPUT
#define PS_ATTRIBUTE_CLIENT_ID 0x00010003  // PsAttributeClientId | THREAD
#define RTL_USER_PROC_PARAMS_NORMALIZED 0x01
#define THREAD_CREATE_FLAGS_CREATE_SUSPENDED 0x00000001
#define PEB_SHIM_DATA_OFFSET 0x1E8 // PEB->pShimData on 32-bit Windows 7+

typedef LONG NTSTATUS;
#define NT_SUCCESS(s) ((NTSTATUS)(s) >= 0)

typedef struct {
	USHORT m_length;
	USHORT m_max_length;
	PWSTR m_buffer;
} s_unicode_string;

typedef struct {
	ULONG m_length;
	HANDLE m_root_directory;
	s_unicode_string *m_object_name;
	ULONG m_attributes;
	PVOID m_security_descriptor;
	PVOID m_security_qos;
} s_object_attributes;

typedef struct {
	HANDLE m_process;
	HANDLE m_thread;
} s_client_id;

typedef struct {
	ULONG m_attribute;
	SIZE_T m_size;
	union {
		ULONG m_value;
		PVOID m_value_ptr;
	};
	PSIZE_T m_return_length;
} s_ps_attribute;

typedef struct {
	SIZE_T m_total_length;
	s_ps_attribute m_attributes[2];
} s_ps_attribute_list;

typedef struct {
	SIZE_T m_size;
	ULONG m_state;
	union {
		struct {
			ULONG m_init_flags;
			ULONG m_additional_file_access;
		} m_init_state;
		UCHAR m_reserved[64]; // Covers largest kernel output union member
	};
} s_ps_create_info;

typedef NTSTATUS(NTAPI *s_pfn_nt_create_user_process)(
	PHANDLE,
	PHANDLE,
	ACCESS_MASK,
	ACCESS_MASK,
	s_object_attributes *,
	s_object_attributes *,
	ULONG,
	ULONG,
	PVOID,
	s_ps_create_info *,
	s_ps_attribute_list *
);

typedef NTSTATUS(NTAPI *s_pfn_rtl_create_process_parameters_ex)(
	PVOID *,
	s_unicode_string *,
	s_unicode_string *,
	s_unicode_string *,
	s_unicode_string *,
	PVOID,
	s_unicode_string *,
	s_unicode_string *,
	s_unicode_string *,
	s_unicode_string *,
	ULONG
);

typedef NTSTATUS(NTAPI *s_pfn_rtl_destroy_process_parameters)(PVOID);
typedef VOID(NTAPI *s_pfn_rtl_init_unicode_string)(s_unicode_string *, PCWSTR);

#define S_RESOLVE(module, type, name) ((type)DTTR_UNWRAP_WINAPI_EXISTS(GetProcAddress(module, name)))

// Creates a suspended process with the provided shim data using NT functions.
// This provides a portable way to override all Windows compatibility shims,
// which lets us avoid some weird crashes (e.g., Intel iGPU crash).
void dttr_compat_create_process(const WCHAR *image_name, sds shim_data, PROCESS_INFORMATION *child_info) {
	log_debug("Spawning game process: NtCreateUserProcess (%u bytes shim data)", (unsigned)sdslen(shim_data));

	const HMODULE ntdll = DTTR_UNWRAP_WINAPI_EXISTS(GetModuleHandleA("ntdll.dll"));

	s_pfn_nt_create_user_process nt_create_user_process
		= S_RESOLVE(ntdll, s_pfn_nt_create_user_process, "NtCreateUserProcess");

	s_pfn_rtl_create_process_parameters_ex rtl_create_process_parameters_ex
		= S_RESOLVE(ntdll, s_pfn_rtl_create_process_parameters_ex, "RtlCreateProcessParametersEx");

	s_pfn_rtl_destroy_process_parameters rtl_destroy_process_parameters
		= S_RESOLVE(ntdll, s_pfn_rtl_destroy_process_parameters, "RtlDestroyProcessParameters");

	s_pfn_rtl_init_unicode_string rtl_init_unicode_string
		= S_RESOLVE(ntdll, s_pfn_rtl_init_unicode_string, "RtlInitUnicodeString");

	// Prepend \??\ to form an NT path.
	WCHAR full_path[MAX_PATH];
	DTTR_UNWRAP_WINAPI_NONZERO(GetFullPathNameW(image_name, MAX_PATH, full_path, NULL));

	WCHAR nt_path[MAX_PATH + 8];
	memcpy(nt_path, L"\\??\\", 4 * sizeof(WCHAR));
	DWORD len = 0;
	while (full_path[len])
		len++;
	memcpy(nt_path + 4, full_path, (len + 1) * sizeof(WCHAR));

	WCHAR cwd[MAX_PATH];
	memcpy(cwd, full_path, (len + 1) * sizeof(WCHAR));
	WCHAR *const last_sep = wcsrchr(cwd, L'\\');
	if (last_sep)
		*(last_sep + 1) = L'\0';

	s_unicode_string us_image, us_cmd, us_cwd;
	rtl_init_unicode_string(&us_image, nt_path);
	rtl_init_unicode_string(&us_cmd, image_name);
	rtl_init_unicode_string(&us_cwd, cwd);

	log_debug("NT image path: %ls", nt_path);
	log_debug("Working directory: %ls", cwd);

	PVOID params = NULL;
	NTSTATUS status = rtl_create_process_parameters_ex(
		&params, &us_image, NULL, &us_cwd, &us_cmd, NULL, NULL, NULL, NULL, NULL, RTL_USER_PROC_PARAMS_NORMALIZED
	);
	if (!NT_SUCCESS(status)) {
		DTTR_FATAL("RtlCreateProcessParametersEx failed: 0x%08lX", (unsigned long)status);
	}

	s_client_id client_id = {0};
	s_ps_attribute_list attr_list = {0};

	attr_list.m_total_length = sizeof(attr_list);
	attr_list.m_attributes[0]
		= (s_ps_attribute){PS_ATTRIBUTE_IMAGE_NAME, us_image.m_length, {.m_value_ptr = us_image.m_buffer}, NULL};
	attr_list.m_attributes[1]
		= (s_ps_attribute){PS_ATTRIBUTE_CLIENT_ID, sizeof(client_id), {.m_value_ptr = &client_id}, NULL};

	s_ps_create_info create_info = {0};
	create_info.m_size = sizeof(create_info);

	HANDLE process = NULL, thread = NULL;
	status = nt_create_user_process(
		&process,
		&thread,
		PROCESS_ALL_ACCESS,
		THREAD_ALL_ACCESS,
		NULL,
		NULL,
		0,
		THREAD_CREATE_FLAGS_CREATE_SUSPENDED,
		params,
		&create_info,
		&attr_list
	);
	rtl_destroy_process_parameters(params);
	if (!NT_SUCCESS(status)) {
		DTTR_FATAL("NtCreateUserProcess failed: 0x%08lX", (unsigned long)status);
	}

	child_info->hProcess = process;
	child_info->hThread = thread;
	child_info->dwProcessId = (DWORD)(ULONG_PTR)client_id.m_process;
	child_info->dwThreadId = (DWORD)(ULONG_PTR)client_id.m_thread;
	log_debug("Process created: PID=%lu, TID=%lu", child_info->dwProcessId, child_info->dwThreadId);

	CONTEXT ctx = {.ContextFlags = CONTEXT_INTEGER};
	DTTR_UNWRAP_WINAPI_NONZERO(GetThreadContext(thread, &ctx));
	const uintptr_t peb_addr = (uintptr_t)ctx.Ebx; // EBX holds PEB in suspended 32-bit process

	const SIZE_T data_len = sdslen(shim_data);

	LPVOID remote_shim
		= DTTR_UNWRAP_WINAPI_EXISTS(VirtualAllocEx(process, NULL, data_len, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));

	DTTR_UNWRAP_WINAPI_NONZERO(WriteProcessMemory(process, remote_shim, shim_data, data_len, NULL));

	DTTR_UNWRAP_WINAPI_NONZERO(
		WriteProcessMemory(process, (LPVOID)(peb_addr + PEB_SHIM_DATA_OFFSET), &remote_shim, sizeof(PVOID), NULL)
	);

	log_debug(
		"Shim data (%u bytes) written to PEB->pShimData at 0x%08X", (unsigned)data_len, (unsigned)(uintptr_t)remote_shim
	);
}

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

static void s_author_sdb(HMODULE apphelp, WCHAR *out_path) {
	s_pfn_sdb_create_database sdb_create = S_RESOLVE(apphelp, s_pfn_sdb_create_database, "SdbCreateDatabase");
	s_pfn_sdb_close_database_write sdb_close
		= S_RESOLVE(apphelp, s_pfn_sdb_close_database_write, "SdbCloseDatabaseWrite");
	s_pfn_sdb_begin_write_list_tag sdb_begin_list
		= S_RESOLVE(apphelp, s_pfn_sdb_begin_write_list_tag, "SdbBeginWriteListTag");
	s_pfn_sdb_end_write_list_tag sdb_end_list = S_RESOLVE(apphelp, s_pfn_sdb_end_write_list_tag, "SdbEndWriteListTag");
	s_pfn_sdb_write_string_tag sdb_string = S_RESOLVE(apphelp, s_pfn_sdb_write_string_tag, "SdbWriteStringTag");
	s_pfn_sdb_write_dword_tag sdb_dword = S_RESOLVE(apphelp, s_pfn_sdb_write_dword_tag, "SdbWriteDWORDTag");
	s_pfn_sdb_write_binary_tag sdb_binary = S_RESOLVE(apphelp, s_pfn_sdb_write_binary_tag, "SdbWriteBinaryTag");

	WCHAR tmp_dir[MAX_PATH];
	DTTR_UNWRAP_WINAPI_NONZERO(GetTempPathW(MAX_PATH, tmp_dir));

	DTTR_UNWRAP_WINAPI_NONZERO(GetTempFileNameW(tmp_dir, L"sdb", 0, out_path));

	// GetTempFileNameW creates the file, so remove it before SdbCreateDatabase recreates it.
	DeleteFileW(out_path);

	s_pdb pdb = sdb_create(out_path, 0);
	if (!pdb) {
		DTTR_FATAL("SdbCreateDatabase failed");
	}

	DWORD ti_db = sdb_begin_list(pdb, SDB_TAG_DATABASE);
	sdb_string(pdb, SDB_TAG_COMPILER_VERSION, L"3.0.0.16");
	sdb_string(pdb, SDB_TAG_NAME, L"PCDogs DttR Compatibility Fixes");
	sdb_dword(pdb, SDB_TAG_OS_PLATFORM, 1);
	sdb_binary(pdb, SDB_TAG_DATABASE_ID, (BYTE *)s_database_guid, 16);

	DWORD ti_exe = sdb_begin_list(pdb, SDB_TAG_EXE);
	sdb_string(pdb, SDB_TAG_NAME, L"pcdogs.exe");
	sdb_string(pdb, SDB_TAG_APP_NAME, L"PCDogs DttR");
	sdb_string(pdb, SDB_TAG_VENDOR, L"Cruella");
	sdb_binary(pdb, SDB_TAG_EXE_ID, (BYTE *)s_exe_guid, 16);

	DWORD ti_match = sdb_begin_list(pdb, SDB_TAG_MATCHING_FILE);
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
	const HMODULE apphelp = DTTR_UNWRAP_WINAPI_EXISTS(LoadLibraryA(apphelp_path));

	WCHAR sdb_path[MAX_PATH];
	s_author_sdb(apphelp, sdb_path);

	s_pfn_sdb_init_database sdb_init = S_RESOLVE(apphelp, s_pfn_sdb_init_database, "SdbInitDatabase");
	s_pfn_sdb_release_database sdb_release = S_RESOLVE(apphelp, s_pfn_sdb_release_database, "SdbReleaseDatabase");

	s_pfn_sdb_get_matching_exe sdb_match = S_RESOLVE(apphelp, s_pfn_sdb_get_matching_exe, "SdbGetMatchingExe");

	s_pfn_sdb_pack_app_compat_data sdb_pack
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
