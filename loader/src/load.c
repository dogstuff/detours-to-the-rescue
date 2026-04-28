#include <dttr_bootstrap.h>
#include <dttr_errors.h>
#include <dttr_loader.h>
#include <gen/asm.h>
#include <log.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

static const char *SIDECAR_DLL_NAME = "libdttr_sidecar.dll";

static const uintptr_t PEB_IMAGE_BASE_OFFSET = 0x8;

static void s_read_remote_bytes(
	HANDLE process,
	uintptr_t address,
	void *out,
	SIZE_T out_size,
	const char *name
) {
	SIZE_T bytes_read = 0;

	if (!ReadProcessMemory(process, (LPCVOID)address, out, out_size, &bytes_read)
		|| bytes_read != out_size) {
		DTTR_FATAL("Could not read %s from child process", name);
	}
}

static uintptr_t s_read_remote_image_base_from_thread_context(
	HANDLE process,
	const CONTEXT *thread_context
) {
	// EBX points to the suspended PEB.
	const uintptr_t peb_address = (uintptr_t)thread_context->Ebx;
	uintptr_t image_base = 0;

	log_debug("Reading image base from PEB at 0x%08X", (unsigned)peb_address);

	s_read_remote_bytes(
		process,
		peb_address + PEB_IMAGE_BASE_OFFSET,
		&image_base,
		sizeof(image_base),
		"PEB image base"
	);

	log_debug("Image base: 0x%08X", (unsigned)image_base);

	return image_base;
}

static uintptr_t s_read_entry_point_rva_from_remote_image(
	HANDLE process,
	const uintptr_t image_base
) {
	IMAGE_DOS_HEADER dos = {0};
	s_read_remote_bytes(process, image_base, &dos, sizeof(dos), "remote DOS header");

	if (dos.e_magic != IMAGE_DOS_SIGNATURE) {
		DTTR_FATAL("Invalid DOS header in child process image");
	}

	if (dos.e_lfanew <= 0) {
		DTTR_FATAL("Invalid NT header offset in child process image");
	}

	const uintptr_t nt_headers_address = image_base + (uintptr_t)dos.e_lfanew;
	log_debug(
		"DOS header valid; remote NT headers at 0x%08X",
		(unsigned)nt_headers_address
	);

	IMAGE_NT_HEADERS32 nt = {0};
	s_read_remote_bytes(process, nt_headers_address, &nt, sizeof(nt), "remote NT headers");

	if (nt.Signature != IMAGE_NT_SIGNATURE) {
		DTTR_FATAL("Invalid NT header in child process image");
	}

	if (nt.OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
		DTTR_FATAL("Unsupported PE optional header in child process image");
	}

	const uintptr_t rva = (uintptr_t)nt.OptionalHeader.AddressOfEntryPoint;
	log_debug("Entry point RVA: 0x%08X", (unsigned)rva);
	return rva;
}

static void s_resolve_loader_dir(char *out_dir, size_t out_dir_size) {
	const DWORD loader_path_len = GetModuleFileNameA(NULL, out_dir, (DWORD)out_dir_size);
	if (loader_path_len == 0 || loader_path_len >= out_dir_size) {
		DTTR_FATAL("Could not resolve loader path");
	}

	char *const last_sep = strrchr(out_dir, '\\');
	if (!last_sep) {
		DTTR_FATAL("Could not resolve loader directory");
	}

	last_sep[1] = '\0';
}

static void s_resolve_sidecar_dll_path(char *out_path, size_t out_path_size) {
	char loader_dir[MAX_PATH];
	s_resolve_loader_dir(loader_dir, sizeof(loader_dir));

	const int written = snprintf(
		out_path,
		out_path_size,
		"%s%s",
		loader_dir,
		SIDECAR_DLL_NAME
	);
	if (written < 0 || (size_t)written >= out_path_size) {
		DTTR_FATAL("Sidecar DLL path is too long");
	}
}

static void s_initialize_shellcode_payload(
	DTTR_LoaderShellcodePayload *out_payload,
	const char *dll_path,
	uintptr_t original_entry
) {
	static const WCHAR KERNEL32_NAME[] = L"kernel32.dll";

	memset(out_payload, 0, sizeof(*out_payload));

	const size_t dll_path_len = strlen(dll_path);
	if (dll_path_len >= sizeof(out_payload->m_dll_path)) {
		DTTR_FATAL("Sidecar DLL path is too long for shellcode buffer: %s", dll_path);
	}

	memcpy(out_payload->m_dll_path, dll_path, dll_path_len + 1);
	memcpy(
		out_payload->m_kernel32_name,
		KERNEL32_NAME,
		sizeof(out_payload->m_kernel32_name)
	);
	memcpy(
		out_payload->m_loadlibraryex_name,
		"LoadLibraryExA",
		sizeof(out_payload->m_loadlibraryex_name)
	);
	memcpy(
		out_payload->m_exitthread_name,
		"ExitThread",
		sizeof(out_payload->m_exitthread_name)
	);
	memcpy(
		out_payload->m_getlasterror_name,
		"GetLastError",
		sizeof(out_payload->m_getlasterror_name)
	);
	out_payload->m_original_entry = (uint32_t)original_entry;
}

static uint32_t s_build_sidecar_shellcode(
	const DTTR_LoaderShellcodePayload *payload,
	uint8_t **out_buffer
) {
	const uint32_t out_size = dttr_sidecar_shellcode_len + sizeof(*payload);
	uint8_t *const buffer = malloc(out_size);
	if (!buffer) {
		DTTR_FATAL("Could not allocate shellcode payload");
	}

	*out_buffer = buffer;
	memcpy(buffer, dttr_sidecar_shellcode, dttr_sidecar_shellcode_len);
	memcpy(buffer + dttr_sidecar_shellcode_len, payload, sizeof(*payload));

	log_debug(
		"Shellcode payload built (bytes=%u, shellcode=%u, payload=%u)",
		out_size,
		dttr_sidecar_shellcode_len,
		(unsigned)sizeof(*payload)
	);

	return out_size;
}

void dttr_loader_inject_sidecar(const PROCESS_INFORMATION *child_info) {
	CONTEXT child_thread_context = {0};
	child_thread_context.ContextFlags = CONTEXT_INTEGER | CONTEXT_CONTROL;
	DTTR_UNWRAP_WINAPI_NONZERO(
		GetThreadContext(child_info->hThread, &child_thread_context)
	);

	const uintptr_t image_base = s_read_remote_image_base_from_thread_context(
		child_info->hProcess,
		&child_thread_context
	);

	const uintptr_t entry_point_rva = s_read_entry_point_rva_from_remote_image(
		child_info->hProcess,
		image_base
	);
	const uintptr_t original_entry = image_base + entry_point_rva;

	log_debug(
		"Resolved original entry point: 0x%08X (base=0x%08X + RVA)",
		(unsigned)original_entry,
		(unsigned)image_base
	);

	char sidecar_dll_path[MAX_PATH];
	s_resolve_sidecar_dll_path(sidecar_dll_path, sizeof(sidecar_dll_path));
	log_debug("Sidecar DLL path: %s", sidecar_dll_path);

	DTTR_LoaderShellcodePayload payload = {0};
	s_initialize_shellcode_payload(&payload, sidecar_dll_path, original_entry);

	uint8_t *shellcode_buffer = NULL;
	const uint32_t shellcode_buffer_len = s_build_sidecar_shellcode(
		&payload,
		&shellcode_buffer
	);

	log_debug("Allocating %u bytes in remote process", shellcode_buffer_len);

	LPVOID payload_buffer = DTTR_UNWRAP_WINAPI_EXISTS(VirtualAllocEx(
		child_info->hProcess,
		NULL,
		shellcode_buffer_len,
		MEM_COMMIT | MEM_RESERVE,
		PAGE_READWRITE
	));

	log_debug("Remote allocation at 0x%08X", (unsigned)(uintptr_t)payload_buffer);

	DTTR_UNWRAP_WINAPI_NONZERO(WriteProcessMemory(
		child_info->hProcess,
		payload_buffer,
		shellcode_buffer,
		shellcode_buffer_len,
		NULL
	));
	log_debug("Shellcode written to remote process");

	DWORD old_protect;

	DTTR_UNWRAP_WINAPI_NONZERO(VirtualProtectEx(
		child_info->hProcess,
		payload_buffer,
		shellcode_buffer_len,
		PAGE_EXECUTE_READWRITE,
		&old_protect
	));
	log_debug("Remote memory protection set to PAGE_EXECUTE_READWRITE");

	free(shellcode_buffer);

	child_thread_context.Eip = (uintptr_t)payload_buffer;
	DTTR_UNWRAP_WINAPI_NONZERO(
		SetThreadContext(child_info->hThread, &child_thread_context)
	);
	log_debug("Thread context updated: EIP=0x%08X", (unsigned)(uintptr_t)payload_buffer);

	DTTR_UNWRAP_WINAPI_NONNEGATIVE(ResumeThread(child_info->hThread));
	log_debug("Resumed thread; game process is running");
}
