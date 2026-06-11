#include <dttr_errors.h>
#include <dttr_loader.h>
#include <dttr_log.h>
#include <string.h>
#include <windows.h>

#define PS_ATTRIBUTE_IMAGE_NAME 0x00020005
#define PS_ATTRIBUTE_CLIENT_ID 0x00010003
#define RTL_USER_PROC_PARAMS_NORMALIZED 0x01
#define THREAD_CREATE_FLAGS_CREATE_SUSPENDED 0x00000001
#define PEB_SHIM_DATA_OFFSET 0x1E8

static const WCHAR NT_PATH_PREFIX[] = L"\\??\\";

typedef LONG NTSTATUS;
#define NT_SUCCESS(s) ((NTSTATUS)(s) >= 0)

typedef struct {
	USHORT length;
	USHORT max_length;
	PWSTR buffer;
} unicode_string;

typedef struct {
	ULONG length;
	HANDLE root_directory;
	unicode_string *object_name;
	ULONG attributes;
	PVOID security_descriptor;
	PVOID security_qos;
} object_attributes;

typedef struct {
	HANDLE process;
	HANDLE thread;
} client_id;

typedef struct {
	ULONG ps_attribute;
	SIZE_T size;
	union {
		ULONG value;
		PVOID value_ptr;
	};

	PSIZE_T return_length;
} attribute;

typedef struct {
	SIZE_T total_length;
	attribute attributes[2];
} attribute_list;

typedef struct {
	SIZE_T size;
	ULONG state;
	union {
		struct {
			ULONG init_flags;
			ULONG additional_file_access;
		} init_state;
		UCHAR reserved[64];
	};
} create_info;

typedef NTSTATUS(NTAPI *nt_create_user_process_fn)(
	PHANDLE,
	PHANDLE,
	ACCESS_MASK,
	ACCESS_MASK,
	object_attributes *,
	object_attributes *,
	ULONG,
	ULONG,
	PVOID,
	create_info *,
	attribute_list *
);

typedef NTSTATUS(NTAPI *rtl_create_process_parameters_ex_fn)(
	PVOID *,
	unicode_string *,
	unicode_string *,
	unicode_string *,
	unicode_string *,
	PVOID,
	unicode_string *,
	unicode_string *,
	unicode_string *,
	unicode_string *,
	ULONG
);

typedef NTSTATUS(NTAPI *rtl_destroy_process_parameters_fn)(PVOID);
typedef VOID(NTAPI *rtl_init_unicode_string_fn)(unicode_string *, PCWSTR);

#define RESOLVE(module, type, name)                                                      \
	((type)DTTR_UNWRAP_WINAPI_EXISTS(GetProcAddress(module, name)))

static void resolve_nt_path_and_cwd(
	WCHAR *nt_path,
	size_t nt_path_size,
	WCHAR *cwd,
	size_t cwd_size,
	const WCHAR *image_name
) {
	WCHAR full_path[MAX_PATH];
	const DWORD resolved_len = GetFullPathNameW(image_name, MAX_PATH, full_path, NULL);
	if (resolved_len == 0 || resolved_len >= MAX_PATH) {
		DTTR_FATAL("Game path is too long");
	}

	const size_t full_path_len = wcslen(full_path);
	if (full_path_len + 5 > nt_path_size || full_path_len + 1 > cwd_size) {
		DTTR_FATAL("Game path is too long");
	}

	memcpy(nt_path, NT_PATH_PREFIX, 4 * sizeof(WCHAR));
	memcpy(nt_path + 4, full_path, (full_path_len + 1) * sizeof(WCHAR));

	memcpy(cwd, full_path, (full_path_len + 1) * sizeof(WCHAR));
	WCHAR *last_sep = wcsrchr(cwd, L'\\');
	if (!last_sep) {
		DTTR_FATAL("Game path is missing a parent directory");
	}

	last_sep[1] = L'\0';
}

static void write_remote_shim_data(
	HANDLE process,
	uintptr_t peb_addr,
	const char *shim_data,
	size_t shim_data_len
) {
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
		(LPVOID)(peb_addr + PEB_SHIM_DATA_OFFSET),
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

void DTTR_Compat_CreateProcess(
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

	const nt_create_user_process_fn nt_create_user_process = RESOLVE(
		ntdll,
		nt_create_user_process_fn,
		"NtCreateUserProcess"
	);

	const rtl_create_process_parameters_ex_fn rtl_create_process_parameters_ex = RESOLVE(
		ntdll,
		rtl_create_process_parameters_ex_fn,
		"RtlCreateProcessParametersEx"
	);

	const rtl_destroy_process_parameters_fn rtl_destroy_process_parameters = RESOLVE(
		ntdll,
		rtl_destroy_process_parameters_fn,
		"RtlDestroyProcessParameters"
	);

	const rtl_init_unicode_string_fn rtl_init_unicode_string = RESOLVE(
		ntdll,
		rtl_init_unicode_string_fn,
		"RtlInitUnicodeString"
	);

	WCHAR nt_path[MAX_PATH + 8];
	WCHAR cwd[MAX_PATH];
	resolve_nt_path_and_cwd(
		nt_path,
		sizeof(nt_path) / sizeof(nt_path[0]),
		cwd,
		sizeof(cwd) / sizeof(cwd[0]),
		image_name
	);

	unicode_string us_image, us_cmd, us_cwd;
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

	client_id client_id = {0};
	attribute_list attr_list = {0};

	attr_list.total_length = sizeof(attr_list);
	attr_list.attributes[0] = (attribute){
		PS_ATTRIBUTE_IMAGE_NAME,
		us_image.length,
		{.value_ptr = us_image.buffer},
		NULL
	};
	attr_list.attributes[1] = (attribute){
		PS_ATTRIBUTE_CLIENT_ID,
		sizeof(client_id),
		{.value_ptr = &client_id},
		NULL
	};

	create_info create_info = {0};
	create_info.size = sizeof(create_info);

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
	child_info->dwProcessId = (DWORD)(ULONG_PTR)client_id.process;
	child_info->dwThreadId = (DWORD)(ULONG_PTR)client_id.thread;
	DTTR_LOG_DEBUG(
		"Process created: PID=%lu, TID=%lu",
		child_info->dwProcessId,
		child_info->dwThreadId
	);

	CONTEXT thread_context = {.ContextFlags = CONTEXT_INTEGER};
	DTTR_UNWRAP_WINAPI_NONZERO(GetThreadContext(thread, &thread_context));

	const uintptr_t peb_addr = (uintptr_t)thread_context.Ebx;
	write_remote_shim_data(process, peb_addr, shim_data, shim_data_len);
}
