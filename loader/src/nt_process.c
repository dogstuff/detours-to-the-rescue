/// Extremely evil direct RTL/NT function calling to override subprocess
/// compatibility shims without making registry/non-portable changes.

#include <dttr_errors.h>
#include <dttr_loader.h>
#include <log.h>
#include <string.h>
#include <windows.h>

#define PS_ATTRIBUTE_IMAGE_NAME 0x00020005
#define PS_ATTRIBUTE_CLIENT_ID 0x00010003
#define RTL_USER_PROC_PARAMS_NORMALIZED 0x01
#define THREAD_CREATE_FLAGS_CREATE_SUSPENDED 0x00000001
#define PEB_SHIM_DATA_OFFSET 0x1E8

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

#define S_RESOLVE(module, type, name)                                                    \
	((type)DTTR_UNWRAP_WINAPI_EXISTS(GetProcAddress(module, name)))

// Creates a suspended process with the provided shim data using NT functions.
// This provides a portable way to override all Windows compatibility shims,
// which lets us avoid some weird crashes (e.g., Intel iGPU crash).
void dttr_compat_create_process(
	const WCHAR *image_name,
	const char *shim_data,
	size_t shim_data_len,
	PROCESS_INFORMATION *child_info
) {
	log_debug(
		"Spawning game process: NtCreateUserProcess (%u bytes shim data)",
		(unsigned)shim_data_len
	);

	HMODULE ntdll = DTTR_UNWRAP_WINAPI_EXISTS(GetModuleHandleA("ntdll.dll"));

	const s_pfn_nt_create_user_process nt_create_user_process = S_RESOLVE(
		ntdll,
		s_pfn_nt_create_user_process,
		"NtCreateUserProcess"
	);

	const s_pfn_rtl_create_process_parameters_ex rtl_create_process_parameters_ex = S_RESOLVE(
		ntdll,
		s_pfn_rtl_create_process_parameters_ex,
		"RtlCreateProcessParametersEx"
	);

	const s_pfn_rtl_destroy_process_parameters rtl_destroy_process_parameters = S_RESOLVE(
		ntdll,
		s_pfn_rtl_destroy_process_parameters,
		"RtlDestroyProcessParameters"
	);

	const s_pfn_rtl_init_unicode_string rtl_init_unicode_string = S_RESOLVE(
		ntdll,
		s_pfn_rtl_init_unicode_string,
		"RtlInitUnicodeString"
	);

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

	s_client_id client_id = {0};
	s_ps_attribute_list attr_list = {0};

	attr_list.m_total_length = sizeof(attr_list);
	attr_list.m_attributes[0] = (s_ps_attribute){PS_ATTRIBUTE_IMAGE_NAME,
												 us_image.m_length,
												 {.m_value_ptr = us_image.m_buffer},
												 NULL};
	attr_list.m_attributes[1] = (s_ps_attribute){PS_ATTRIBUTE_CLIENT_ID,
												 sizeof(client_id),
												 {.m_value_ptr = &client_id},
												 NULL};

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
	log_debug(
		"Process created: PID=%lu, TID=%lu",
		child_info->dwProcessId,
		child_info->dwThreadId
	);

	CONTEXT ctx = {.ContextFlags = CONTEXT_INTEGER};
	DTTR_UNWRAP_WINAPI_NONZERO(GetThreadContext(thread, &ctx));

	// EBX holds PEB in suspended 32-bit process
	const uintptr_t peb_addr = (uintptr_t)ctx.Ebx;

	const SIZE_T data_len = shim_data_len;

	LPVOID remote_shim = DTTR_UNWRAP_WINAPI_EXISTS(
		VirtualAllocEx(process, NULL, data_len, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE)
	);

	DTTR_UNWRAP_WINAPI_NONZERO(
		WriteProcessMemory(process, remote_shim, shim_data, data_len, NULL)
	);

	DTTR_UNWRAP_WINAPI_NONZERO(WriteProcessMemory(
		process,
		(LPVOID)(peb_addr + PEB_SHIM_DATA_OFFSET),
		&remote_shim,
		sizeof(PVOID),
		NULL
	));

	log_debug(
		"Shim data (%u bytes) written to PEB->pShimData at 0x%08X",
		(unsigned)data_len,
		(unsigned)(uintptr_t)remote_shim
	);
}
