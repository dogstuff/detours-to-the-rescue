#include <dttr_errors.h>
#include <gen/asm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

static const char *SIDECAR_DLL_PATH = "libdttr_sidecar.dll";
static const char *TARGET_EXE_NAME = "pcdogs.exe";
static const uintptr_t PEB_IMAGE_BASE_OFFSET = 0x8;
static const uintptr_t SHELLCODE_LOADLIB_OFFSET = 0x7;
static const uintptr_t SHELLCODE_ENTRY_OFFSET = 0x10;
static const uintptr_t SHELLCODE_EXITTHREAD_OFFSET = 0x18;

static void s_exit_with_file_error(FILE *file, const char *error_message, const char *exe_name) {
	if (file) {
		fclose(file);
	}

	s_raise_error(error_message, exe_name);
	exit(EXIT_FAILURE);
}

static uintptr_t
s_read_remote_image_base_from_thread_context(HANDLE process, const CONTEXT *thread_context) {
	// In a suspended 32-bit process, EBX points to the PEB.
	const uintptr_t peb_address = (uintptr_t)thread_context->Ebx;
	uintptr_t image_base = 0;

	DTTR_UNWRAP_WINAPI_NONZERO(ReadProcessMemory(
		process,
		(LPCVOID)(peb_address + PEB_IMAGE_BASE_OFFSET),
		&image_base,
		sizeof(image_base),
		NULL
	));
	return image_base;
}

static uintptr_t s_read_entry_point_rva_from_exe(const char *exe_name) {
	FILE *file = fopen(exe_name, "rb");
	if (!file) {
		s_exit_with_file_error(NULL, "Could not open %s", exe_name);
	}

	IMAGE_DOS_HEADER dos = {0};
	if (fread(&dos, sizeof(dos), 1, file) != 1) {
		s_exit_with_file_error(file, "Could not read DOS header from %s", exe_name);
	}

	if (dos.e_magic != IMAGE_DOS_SIGNATURE) {
		s_exit_with_file_error(file, "Invalid DOS header in %s", exe_name);
	}

	if (fseek(file, dos.e_lfanew, SEEK_SET) != 0) {
		s_exit_with_file_error(file, "Could not seek NT headers in %s", exe_name);
	}

	IMAGE_NT_HEADERS32 nt = {0};
	if (fread(&nt, sizeof(nt), 1, file) != 1) {
		s_exit_with_file_error(file, "Could not read NT headers from %s", exe_name);
	}

	if (nt.Signature != IMAGE_NT_SIGNATURE) {
		s_exit_with_file_error(file, "Invalid NT header in %s", exe_name);
	}

	fclose(file);

	return (uintptr_t)nt.OptionalHeader.AddressOfEntryPoint;
}

static uint32_t
s_build_sidecar_shell_code(const char *dll_path, uintptr_t original_entry, uint8_t **out_buffer) {
	const uint32_t dll_path_len = strlen(dll_path);
	const uint32_t out_size = dttr_sidecar_shellcode_len + dll_path_len + 1;
	uint8_t *const buffer = malloc(out_size);
	if (!buffer) {
		s_raise_error("Could not allocate shellcode payload for %s", TARGET_EXE_NAME);
		exit(EXIT_FAILURE);
	}
	*out_buffer = buffer;
	memcpy(buffer, dttr_sidecar_shellcode, dttr_sidecar_shellcode_len);

	const HMODULE kernel32 = GetModuleHandleA("kernel32.dll");
	const uintptr_t load_library_a_address
		= (uintptr_t)DTTR_UNWRAP_WINAPI_EXISTS(GetProcAddress(kernel32, "LoadLibraryA"));
	const uintptr_t exit_thread_address
		= (uintptr_t)DTTR_UNWRAP_WINAPI_EXISTS(GetProcAddress(kernel32, "ExitThread"));

	*(uintptr_t *)((uintptr_t)buffer + SHELLCODE_LOADLIB_OFFSET) = load_library_a_address;
	*(uintptr_t *)((uintptr_t)buffer + SHELLCODE_ENTRY_OFFSET) = original_entry;
	*(uintptr_t *)((uintptr_t)buffer + SHELLCODE_EXITTHREAD_OFFSET) = exit_thread_address;

	memcpy((void *)((uintptr_t)buffer + dttr_sidecar_shellcode_len), dll_path, dll_path_len);
	*(char *)((uintptr_t)buffer + out_size - 1) = '\0';

	return out_size;
}

int main(void) {
	STARTUPINFOW child_startup_info = {sizeof(STARTUPINFOW)};
	PROCESS_INFORMATION child_info = {0};
	WCHAR cmd_line[] = L"pcdogs.exe";

	DTTR_UNWRAP_WINAPI_NONZERO(CreateProcessW(
		NULL,
		cmd_line,
		NULL,
		NULL,
		FALSE,
		CREATE_SUSPENDED,
		NULL,
		NULL,
		&child_startup_info,
		&child_info
	));

	CONTEXT child_thread_context = {0};
	child_thread_context.ContextFlags = CONTEXT_INTEGER | CONTEXT_CONTROL;
	DTTR_UNWRAP_WINAPI_NONZERO(GetThreadContext(child_info.hThread, &child_thread_context));

	const uintptr_t image_base
		= s_read_remote_image_base_from_thread_context(child_info.hProcess, &child_thread_context);
	const uintptr_t original_entry = image_base + s_read_entry_point_rva_from_exe(TARGET_EXE_NAME);
	uint8_t *shellcode_buffer = NULL;
	const uint32_t shellcode_buffer_len
		= s_build_sidecar_shell_code(SIDECAR_DLL_PATH, original_entry, &shellcode_buffer);

	const LPVOID payload_buffer = DTTR_UNWRAP_WINAPI_EXISTS(VirtualAllocEx(
		child_info.hProcess, NULL, shellcode_buffer_len, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE
	));

	DTTR_UNWRAP_WINAPI_NONZERO(WriteProcessMemory(
		child_info.hProcess, payload_buffer, shellcode_buffer, shellcode_buffer_len, NULL
	));

	DWORD old_protect;
	DTTR_UNWRAP_WINAPI_NONZERO(VirtualProtectEx(
		child_info.hProcess, payload_buffer, shellcode_buffer_len, PAGE_EXECUTE_READ, &old_protect
	));

	free(shellcode_buffer);
	child_thread_context.Eip = (uintptr_t)payload_buffer;
	DTTR_UNWRAP_WINAPI_NONZERO(SetThreadContext(child_info.hThread, &child_thread_context));
	DTTR_UNWRAP_WINAPI_NONNEGATIVE(ResumeThread(child_info.hThread));
	CloseHandle(child_info.hThread);
	CloseHandle(child_info.hProcess);
	return 0;
}
