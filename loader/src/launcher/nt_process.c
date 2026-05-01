/// Direct RTL/NT calls to override subprocess compatibility shims.

#include <dttr_errors.h>
#include <dttr_loader.h>
#include <dttr_log.h>
#include <string.h>
#include <windows.h>

#define PS_ATTRIBUTE_IMAGE_NAME 0x00020005
#define PS_ATTRIBUTE_CLIENT_ID 0x00010003
#define RTL_USER_PROC_PARAMS_NORMALIZED 0x01
#define THREAD_CREATE_FLAGS_CREATE_SUSPENDED 0x00000001
#define S_PEB_SHIM_DATA_OFFSET 0x1E8

typedef LONG NTSTATUS;
#define NT_SUCCESS(s) ((NTSTATUS)(s) >= 0)

typedef struct {
	USHORT m_length;
	USHORT m_max_length;
	PWSTR m_buffer;
} S_UnicodeString;

typedef struct {
	ULONG m_length;
	HANDLE m_root_directory;
	S_UnicodeString *m_object_name;
	ULONG m_attributes;
	PVOID m_security_descriptor;
	PVOID m_security_qos;
} S_ObjectAttributes;

typedef struct {
	HANDLE m_process;
	HANDLE m_thread;
} S_ClientId;

typedef struct {
	ULONG m_attribute;
	SIZE_T m_size;
	union {
		ULONG m_value;
		PVOID m_value_ptr;
	};
	PSIZE_T m_return_length;
} S_Attribute;

typedef struct {
	SIZE_T m_total_length;
	S_Attribute m_attributes[2];
} S_AttributeList;

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
} S_CreateInfo;

typedef NTSTATUS(NTAPI *S_NtCreateUserProcess)(
	PHANDLE,
	PHANDLE,
	ACCESS_MASK,
	ACCESS_MASK,
	S_ObjectAttributes *,
	S_ObjectAttributes *,
	ULONG,
	ULONG,
	PVOID,
	S_CreateInfo *,
	S_AttributeList *
);

typedef NTSTATUS(NTAPI *S_RtlCreateProcessParametersEx)(
	PVOID *,
	S_UnicodeString *,
	S_UnicodeString *,
	S_UnicodeString *,
	S_UnicodeString *,
	PVOID,
	S_UnicodeString *,
	S_UnicodeString *,
	S_UnicodeString *,
	S_UnicodeString *,
	ULONG
);

typedef NTSTATUS(NTAPI *S_RtlDestroyProcessParameters)(PVOID);
typedef VOID(NTAPI *S_RtlInitUnicodeString)(S_UnicodeString *, PCWSTR);

#define S_RESOLVE(module, type, name)                                                    \
	((type)DTTR_UNWRAP_WINAPI_EXISTS(GetProcAddress(module, name)))

static void s_resolve_nt_path_and_cwd(
	WCHAR *nt_path,
	size_t nt_path_size,
	WCHAR *cwd,
	size_t cwd_size,
	const WCHAR *image_name
) {
	WCHAR full_path[MAX_PATH];
	DTTR_UNWRAP_WINAPI_NONZERO(GetFullPathNameW(image_name, MAX_PATH, full_path, NULL));

	const size_t full_path_len = wcslen(full_path);
	if (full_path_len + 5 > nt_path_size || full_path_len + 1 > cwd_size) {
		DTTR_FATAL("Game path is too long");
	}

	memcpy(nt_path, L"\\??\\", 4 * sizeof(WCHAR));
	memcpy(nt_path + 4, full_path, (full_path_len + 1) * sizeof(WCHAR));

	memcpy(cwd, full_path, (full_path_len + 1) * sizeof(WCHAR));
	WCHAR *const last_sep = wcsrchr(cwd, L'\\');
	if (!last_sep) {
		DTTR_FATAL("Game path is missing a parent directory");
	}
	last_sep[1] = L'\0';
}

// Creates a suspended process with the provided shim data using NT functions.
// This provides a portable way to override all Windows compatibility shims,
// which lets us avoid some weird crashes (e.g., Intel iGPU crash).
void dttr_compat_create_process(
	const WCHAR *image_name,
	const char *shim_data,
	size_t shim_data_len,
	PROCESS_INFORMATION *child_info
) {
	DTTR_LOG_DEBUG(
		"Spawning game process: NtCreateUserProcess (%u bytes shim data)",
		(unsigned)shim_data_len
	);

	HMODULE ntdll = DTTR_UNWRAP_WINAPI_EXISTS(GetModuleHandleA("ntdll.dll"));

	const S_NtCreateUserProcess nt_create_user_process = S_RESOLVE(
		ntdll,
		S_NtCreateUserProcess,
		"NtCreateUserProcess"
	);

	const S_RtlCreateProcessParametersEx rtl_create_process_parameters_ex = S_RESOLVE(
		ntdll,
		S_RtlCreateProcessParametersEx,
		"RtlCreateProcessParametersEx"
	);

	const S_RtlDestroyProcessParameters rtl_destroy_process_parameters = S_RESOLVE(
		ntdll,
		S_RtlDestroyProcessParameters,
		"RtlDestroyProcessParameters"
	);

	const S_RtlInitUnicodeString rtl_init_unicode_string = S_RESOLVE(
		ntdll,
		S_RtlInitUnicodeString,
		"RtlInitUnicodeString"
	);

	WCHAR nt_path[MAX_PATH + 8];
	WCHAR cwd[MAX_PATH];
	s_resolve_nt_path_and_cwd(
		nt_path,
		sizeof(nt_path) / sizeof(nt_path[0]),
		cwd,
		sizeof(cwd) / sizeof(cwd[0]),
		image_name
	);

	S_UnicodeString us_image, us_cmd, us_cwd;
	rtl_init_unicode_string(&us_image, nt_path);
	rtl_init_unicode_string(&us_cmd, image_name);
	rtl_init_unicode_string(&us_cwd, cwd);

	DTTR_LOG_DEBUG("NT image path: %ls", nt_path);
	DTTR_LOG_DEBUG("Working directory: %ls", cwd);

	PVOID params = NULL;
	NTSTATUS status = rtl_create_process_parameters_ex(
		&params,
		&us_image,
		NULL,
		&us_cwd,
		&us_cmd,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		RTL_USER_PROC_PARAMS_NORMALIZED
	);
	if (!NT_SUCCESS(status)) {
		DTTR_FATAL("RtlCreateProcessParametersEx failed: 0x%08lX", (unsigned long)status);
	}

	S_ClientId client_id = {0};
	S_AttributeList attr_list = {0};

	attr_list.m_total_length = sizeof(attr_list);
	attr_list.m_attributes[0] = (S_Attribute){PS_ATTRIBUTE_IMAGE_NAME,
											  us_image.m_length,
											  {.m_value_ptr = us_image.m_buffer},
											  NULL};
	attr_list.m_attributes[1] = (S_Attribute){PS_ATTRIBUTE_CLIENT_ID,
											  sizeof(client_id),
											  {.m_value_ptr = &client_id},
											  NULL};

	S_CreateInfo create_info = {0};
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
	DTTR_LOG_DEBUG(
		"Process created: PID=%lu, TID=%lu",
		child_info->dwProcessId,
		child_info->dwThreadId
	);

	CONTEXT thread_context = {.ContextFlags = CONTEXT_INTEGER};
	DTTR_UNWRAP_WINAPI_NONZERO(GetThreadContext(thread, &thread_context));

	// EBX holds PEB in suspended 32-bit process
	const uintptr_t peb_addr = (uintptr_t)thread_context.Ebx;

	LPVOID remote_shim = DTTR_UNWRAP_WINAPI_EXISTS(VirtualAllocEx(
		process,
		NULL,
		shim_data_len,
		MEM_COMMIT | MEM_RESERVE,
		PAGE_READWRITE
	));

	DTTR_UNWRAP_WINAPI_NONZERO(
		WriteProcessMemory(process, remote_shim, shim_data, shim_data_len, NULL)
	);

	DTTR_UNWRAP_WINAPI_NONZERO(WriteProcessMemory(
		process,
		(LPVOID)(peb_addr + S_PEB_SHIM_DATA_OFFSET),
		&remote_shim,
		sizeof(PVOID),
		NULL
	));

	DTTR_LOG_DEBUG(
		"Shim data (%u bytes) written to PEB->pShimData at 0x%08X",
		(unsigned)shim_data_len,
		(unsigned)(uintptr_t)remote_shim
	);
}
