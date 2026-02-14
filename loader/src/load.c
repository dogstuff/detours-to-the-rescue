#include <dttr_errors.h>
#include <dttr_loader.h>
#include <gen/asm.h>
#include <log.h>
#include <stdio.h>
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

	DTTR_FATAL(error_message, exe_name);
}

static uintptr_t s_read_remote_image_base_from_thread_context(HANDLE process, const CONTEXT *thread_context) {
	const uintptr_t peb_address = (uintptr_t)thread_context->Ebx; // EBX points to PEB when suspended
	uintptr_t image_base = 0;

	log_debug("Reading image base from PEB at 0x%08X", (unsigned)peb_address);

	DTTR_UNWRAP_WINAPI_NONZERO(ReadProcessMemory(
		process, (LPCVOID)(peb_address + PEB_IMAGE_BASE_OFFSET), &image_base, sizeof(image_base), NULL
	));

	log_debug("Image base: 0x%08X", (unsigned)image_base);

	return image_base;
}

static uintptr_t s_read_entry_point_rva_from_exe(const char *exe_name) {
	log_debug("Reading entry point RVA from %s", exe_name);

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

	log_debug("DOS header valid; NT headers at offset 0x%lX", dos.e_lfanew);

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

	const uintptr_t rva = (uintptr_t)nt.OptionalHeader.AddressOfEntryPoint;
	log_debug("Entry point RVA: 0x%08X", (unsigned)rva);
	return rva;
}

static uint32_t s_build_sidecar_shell_code(const char *dll_path, uintptr_t original_entry, uint8_t **out_buffer) {
	log_debug("Building shellcode: dll=%s, original_entry=0x%08X", dll_path, (unsigned)original_entry);

	const uint32_t dll_path_len = strlen(dll_path);
	const uint32_t out_size = dttr_sidecar_shellcode_len + dll_path_len + 1;
	uint8_t *const buffer = malloc(out_size);
	if (!buffer) {
		DTTR_FATAL("Could not allocate shellcode payload for %s", TARGET_EXE_NAME);
	}
	*out_buffer = buffer;
	memcpy(buffer, dttr_sidecar_shellcode, dttr_sidecar_shellcode_len);

	HMODULE kernel32 = GetModuleHandleA("kernel32.dll");
	const uintptr_t load_library_a_address
		= (uintptr_t)DTTR_UNWRAP_WINAPI_EXISTS(GetProcAddress(kernel32, "LoadLibraryA"));

	const uintptr_t exit_thread_address = (uintptr_t)DTTR_UNWRAP_WINAPI_EXISTS(GetProcAddress(kernel32, "ExitThread"));

	log_debug(
		"Resolved kernel32 APIs: LoadLibraryA=0x%08X, ExitThread=0x%08X",
		(unsigned)load_library_a_address,
		(unsigned)exit_thread_address
	);

	*(uintptr_t *)((uintptr_t)buffer + SHELLCODE_LOADLIB_OFFSET) = load_library_a_address;
	*(uintptr_t *)((uintptr_t)buffer + SHELLCODE_ENTRY_OFFSET) = original_entry;
	*(uintptr_t *)((uintptr_t)buffer + SHELLCODE_EXITTHREAD_OFFSET) = exit_thread_address;

	memcpy((void *)((uintptr_t)buffer + dttr_sidecar_shellcode_len), dll_path, dll_path_len);
	*(char *)((uintptr_t)buffer + out_size - 1) = '\0';

	log_debug(
		"Shellcode payload built (bytes=%u, shellcode=%u + dll_path=%u + 1)",
		out_size,
		dttr_sidecar_shellcode_len,
		dll_path_len
	);

	return out_size;
}

void dttr_loader_inject_sidecar(const PROCESS_INFORMATION *child_info) {
	CONTEXT child_thread_context = {0};
	child_thread_context.ContextFlags = CONTEXT_INTEGER | CONTEXT_CONTROL;
	DTTR_UNWRAP_WINAPI_NONZERO(GetThreadContext(child_info->hThread, &child_thread_context));

	const uintptr_t image_base
		= s_read_remote_image_base_from_thread_context(child_info->hProcess, &child_thread_context);

	const uintptr_t original_entry = image_base + s_read_entry_point_rva_from_exe(TARGET_EXE_NAME);

	log_debug(
		"Resolved original entry point: 0x%08X (base=0x%08X + RVA)", (unsigned)original_entry, (unsigned)image_base
	);

	uint8_t *shellcode_buffer = NULL;
	const uint32_t shellcode_buffer_len
		= s_build_sidecar_shell_code(SIDECAR_DLL_PATH, original_entry, &shellcode_buffer);

	log_debug("Allocating %u bytes in remote process", shellcode_buffer_len);

	LPVOID payload_buffer = DTTR_UNWRAP_WINAPI_EXISTS(
		VirtualAllocEx(child_info->hProcess, NULL, shellcode_buffer_len, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE)
	);

	log_debug("Remote allocation at 0x%08X", (unsigned)(uintptr_t)payload_buffer);

	DTTR_UNWRAP_WINAPI_NONZERO(
		WriteProcessMemory(child_info->hProcess, payload_buffer, shellcode_buffer, shellcode_buffer_len, NULL)
	);
	log_debug("Shellcode written to remote process");

	DWORD old_protect;

	DTTR_UNWRAP_WINAPI_NONZERO(
		VirtualProtectEx(child_info->hProcess, payload_buffer, shellcode_buffer_len, PAGE_EXECUTE_READ, &old_protect)
	);
	log_debug("Remote memory protection set to PAGE_EXECUTE_READ");

	free(shellcode_buffer);

	child_thread_context.Eip = (uintptr_t)payload_buffer;
	DTTR_UNWRAP_WINAPI_NONZERO(SetThreadContext(child_info->hThread, &child_thread_context));
	log_debug("Thread context updated: EIP=0x%08X", (unsigned)(uintptr_t)payload_buffer);

	DTTR_UNWRAP_WINAPI_NONNEGATIVE(ResumeThread(child_info->hThread));

	log_debug("Resumed thread; game process is running");
}
