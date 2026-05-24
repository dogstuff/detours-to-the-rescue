#include <dttr_bootstrap.h>
#include <dttr_errors.h>
#include <dttr_loader.h>
#include <dttr_log.h>
#include <dttr_path.h>
#include <gen/asm.h>
#include <sds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

static const char *const SIDECAR_DLL_RELATIVE_PATH = "modules\\libdttr_sidecar.dll";
static const char LOAD_LIBRARY_EX_NAME[] = "LoadLibraryExA";
static const char EXIT_THREAD_NAME[] = "ExitThread";
static const char GET_LAST_ERROR_NAME[] = "GetLastError";

static const uintptr_t PEB_IMAGE_BASE_OFFSET = 0x8;

static void log_win32_failure(const char *operation) {
	const DWORD error_code = GetLastError();
	if (error_code == ERROR_SUCCESS) {
		DTTR_LOG_ERROR("%s failed", operation);
		return;
	}

	LPSTR message = NULL;
	FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM
			| FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		error_code,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPSTR)&message,
		0,
		NULL
	);

	DTTR_LOG_ERROR(
		"%s failed (Win32 API Error 0x%lX: %s)",
		operation,
		error_code,
		message ? message : "unknown"
	);

	if (message) {
		LocalFree(message);
	}
}

static bool read_remote_bytes(
	HANDLE process,
	uintptr_t address,
	void *out,
	SIZE_T out_size,
	const char *name
) {
	SIZE_T bytes_read = 0;

	if (!ReadProcessMemory(process, (LPCVOID)address, out, out_size, &bytes_read)
		|| bytes_read != out_size) {
		log_win32_failure("ReadProcessMemory");
		DTTR_LOG_ERROR(
			"Could not read %s from child process (address=0x%08X, expected=%u, got=%u)",
			name,
			(unsigned)address,
			(unsigned)out_size,
			(unsigned)bytes_read
		);
		return false;
	}

	return true;
}

static bool read_remote_image_base_from_thread_context(
	HANDLE process,
	const CONTEXT *thread_context,
	uintptr_t *out_image_base
) {
	const uintptr_t peb_address = (uintptr_t)thread_context->Ebx;
	uintptr_t image_base = 0;

	DTTR_LOG_DEBUG("Reading image base from PEB at 0x%08X", (unsigned)peb_address);

	if (!read_remote_bytes(
			process,
			peb_address + PEB_IMAGE_BASE_OFFSET,
			&image_base,
			sizeof(image_base),
			"PEB image base"
		)) {
		return false;
	}

	DTTR_LOG_DEBUG("Image base: 0x%08X", (unsigned)image_base);
	*out_image_base = image_base;
	return true;
}

static bool read_entry_point_rva_from_remote_image(
	HANDLE process,
	uintptr_t image_base,
	uintptr_t *out_entry_point_rva
) {
	IMAGE_DOS_HEADER dos = {0};
	if (!read_remote_bytes(process, image_base, &dos, sizeof(dos), "remote DOS header")) {
		return false;
	}

	if (dos.e_magic != IMAGE_DOS_SIGNATURE) {
		DTTR_LOG_ERROR("Invalid DOS header in child process image");
		return false;
	}

	if (dos.e_lfanew <= 0) {
		DTTR_LOG_ERROR("Invalid NT header offset in child process image");
		return false;
	}

	const uintptr_t nt_headers_address = image_base + (uintptr_t)dos.e_lfanew;
	DTTR_LOG_DEBUG(
		"DOS header valid; remote NT headers at 0x%08X",
		(unsigned)nt_headers_address
	);

	IMAGE_NT_HEADERS32 nt = {0};
	if (!read_remote_bytes(
			process,
			nt_headers_address,
			&nt,
			sizeof(nt),
			"remote NT headers"
		)) {
		return false;
	}

	if (nt.Signature != IMAGE_NT_SIGNATURE) {
		DTTR_LOG_ERROR("Invalid NT header in child process image");
		return false;
	}

	if (nt.OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
		DTTR_LOG_ERROR("Unsupported PE optional header in child process image");
		return false;
	}

	const uintptr_t rva = (uintptr_t)nt.OptionalHeader.AddressOfEntryPoint;
	DTTR_LOG_DEBUG("Entry point RVA: 0x%08X", (unsigned)rva);
	*out_entry_point_rva = rva;
	return true;
}

static bool resolve_sidecar_dll_path(char *out_path, size_t out_path_size) {
	sds sidecar_path = DTTR_Path_ModuleSibling(NULL, SIDECAR_DLL_RELATIVE_PATH);
	const bool copied = DTTR_Path_CopySds(out_path, out_path_size, sidecar_path);
	sdsfree(sidecar_path);
	if (!copied) {
		DTTR_LOG_ERROR("Sidecar DLL path is too long");
		return false;
	}

	return true;
}

static bool initialize_shellcode_payload(
	DTTR_LoaderShellcodePayload *out_payload,
	const char *dll_path,
	uintptr_t original_entry
) {
	static const WCHAR KERNEL32_NAME[] = L"kernel32.dll";

	memset(out_payload, 0, sizeof(*out_payload));

	const size_t dll_path_len = strlen(dll_path);
	if (dll_path_len >= sizeof(out_payload->dll_path)) {
		DTTR_LOG_ERROR("Sidecar DLL path is too long for shellcode buffer: %s", dll_path);
		return false;
	}

	memcpy(out_payload->dll_path, dll_path, dll_path_len + 1);
	memcpy(out_payload->kernel32_name, KERNEL32_NAME, sizeof(out_payload->kernel32_name));
	memcpy(
		out_payload->loadlibraryex_name,
		LOAD_LIBRARY_EX_NAME,
		sizeof(out_payload->loadlibraryex_name)
	);
	memcpy(
		out_payload->exitthread_name,
		EXIT_THREAD_NAME,
		sizeof(out_payload->exitthread_name)
	);
	memcpy(
		out_payload->getlasterror_name,
		GET_LAST_ERROR_NAME,
		sizeof(out_payload->getlasterror_name)
	);
	out_payload->original_entry = (uint32_t)original_entry;
	return true;
}

static bool build_sidecar_shellcode(
	const DTTR_LoaderShellcodePayload *payload,
	uint8_t **out_buffer,
	uint32_t *out_buffer_size
) {
	const size_t out_size = (size_t)dttr_sidecar_shellcode_len + sizeof(*payload);
	if (out_size > UINT32_MAX) {
		DTTR_LOG_ERROR("Shellcode payload is too large");
		return false;
	}

	uint8_t *const buffer = malloc(out_size);
	if (!buffer) {
		DTTR_LOG_ERROR("Could not allocate shellcode payload");
		return false;
	}

	*out_buffer = buffer;
	*out_buffer_size = (uint32_t)out_size;
	memcpy(buffer, dttr_sidecar_shellcode, dttr_sidecar_shellcode_len);
	memcpy(buffer + dttr_sidecar_shellcode_len, payload, sizeof(*payload));

	DTTR_LOG_DEBUG(
		"Shellcode payload built (bytes=%u, shellcode=%u, payload=%u)",
		*out_buffer_size,
		dttr_sidecar_shellcode_len,
		(unsigned)sizeof(*payload)
	);

	return true;
}

static bool write_remote_payload(
	HANDLE process,
	const void *buffer,
	SIZE_T buffer_size,
	LPVOID *out_remote_buffer
) {
	DTTR_LOG_DEBUG("Allocating %u bytes in remote process", (unsigned)buffer_size);

	LPVOID remote_buffer = VirtualAllocEx(
		process,
		NULL,
		buffer_size,
		MEM_COMMIT | MEM_RESERVE,
		PAGE_READWRITE
	);
	if (!remote_buffer) {
		log_win32_failure("VirtualAllocEx");
		return false;
	}

	DTTR_LOG_DEBUG("Remote allocation at 0x%08X", (unsigned)(uintptr_t)remote_buffer);

	SIZE_T bytes_written = 0;
	if (!WriteProcessMemory(process, remote_buffer, buffer, buffer_size, &bytes_written)
		|| bytes_written != buffer_size) {
		log_win32_failure("WriteProcessMemory");
		DTTR_LOG_ERROR(
			"Could not write shellcode to child process (expected=%u, wrote=%u)",
			(unsigned)buffer_size,
			(unsigned)bytes_written
		);
		VirtualFreeEx(process, remote_buffer, 0, MEM_RELEASE);
		return false;
	}

	DTTR_LOG_DEBUG("Shellcode written to remote process");

	DWORD old_protect = 0;
	if (!VirtualProtectEx(
			process,
			remote_buffer,
			buffer_size,
			PAGE_EXECUTE_READWRITE,
			&old_protect
		)) {
		log_win32_failure("VirtualProtectEx");
		VirtualFreeEx(process, remote_buffer, 0, MEM_RELEASE);
		return false;
	}

	DTTR_LOG_DEBUG("Remote memory protection set to PAGE_EXECUTE_READWRITE");

	*out_remote_buffer = remote_buffer;
	return true;
}

bool DTTR_Loader_InjectSidecar(const PROCESS_INFORMATION *child_info) {
	CONTEXT child_thread_context = {0};
	child_thread_context.ContextFlags = CONTEXT_INTEGER | CONTEXT_CONTROL;
	if (!GetThreadContext(child_info->hThread, &child_thread_context)) {
		log_win32_failure("GetThreadContext");
		return false;
	}

	uintptr_t image_base = 0;
	if (!read_remote_image_base_from_thread_context(
			child_info->hProcess,
			&child_thread_context,
			&image_base
		)) {
		return false;
	}

	uintptr_t entry_point_rva = 0;
	if (!read_entry_point_rva_from_remote_image(
			child_info->hProcess,
			image_base,
			&entry_point_rva
		)) {
		return false;
	}

	const uintptr_t original_entry = image_base + entry_point_rva;

	DTTR_LOG_DEBUG(
		"Resolved original entry point: 0x%08X (base=0x%08X + RVA)",
		(unsigned)original_entry,
		(unsigned)image_base
	);

	char sidecar_dll_path[MAX_PATH];
	if (!resolve_sidecar_dll_path(sidecar_dll_path, sizeof(sidecar_dll_path))) {
		return false;
	}

	DTTR_LOG_DEBUG("Sidecar DLL path: %s", sidecar_dll_path);

	DTTR_LoaderShellcodePayload payload = {0};
	if (!initialize_shellcode_payload(&payload, sidecar_dll_path, original_entry)) {
		return false;
	}

	uint8_t *shellcode_buffer = NULL;
	uint32_t shellcode_buffer_len = 0;
	if (!build_sidecar_shellcode(&payload, &shellcode_buffer, &shellcode_buffer_len)) {
		return false;
	}

	LPVOID payload_buffer = NULL;
	const bool wrote_payload = write_remote_payload(
		child_info->hProcess,
		shellcode_buffer,
		shellcode_buffer_len,
		&payload_buffer
	);
	free(shellcode_buffer);
	if (!wrote_payload) {
		return false;
	}

	child_thread_context.Eip = (DWORD)(uintptr_t)payload_buffer;
	if (!SetThreadContext(child_info->hThread, &child_thread_context)) {
		log_win32_failure("SetThreadContext");
		VirtualFreeEx(child_info->hProcess, payload_buffer, 0, MEM_RELEASE);
		return false;
	}

	DTTR_LOG_DEBUG(
		"Thread context updated: EIP=0x%08X",
		(unsigned)(uintptr_t)payload_buffer
	);

	if (ResumeThread(child_info->hThread) == (DWORD)-1) {
		log_win32_failure("ResumeThread");
		VirtualFreeEx(child_info->hProcess, payload_buffer, 0, MEM_RELEASE);
		return false;
	}

	DTTR_LOG_DEBUG("Resumed thread; game process is running");
	return true;
}
