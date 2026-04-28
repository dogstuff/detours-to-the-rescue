#ifndef DTTR_BOOTSTRAP_H
#define DTTR_BOOTSTRAP_H

#include <stddef.h>
#include <stdint.h>
#include <windows.h>

typedef enum {
	DTTR_BOOTSTRAP_PHASE_INIT = 0,
	DTTR_BOOTSTRAP_PHASE_KERNEL32_READY = 1,
	DTTR_BOOTSTRAP_PHASE_API_READY = 2,
	DTTR_BOOTSTRAP_PHASE_HELPER_LOADED = 3,
	DTTR_BOOTSTRAP_PHASE_HELPER_BOOTSTRAPPED = 4,
	DTTR_BOOTSTRAP_PHASE_SIDECAR_LOADED = 5,
	DTTR_BOOTSTRAP_PHASE_SIDECAR_BOUND = 6,
	DTTR_BOOTSTRAP_PHASE_READY = 7,
	DTTR_BOOTSTRAP_PHASE_FAILED = 0x80,
	DTTR_BOOTSTRAP_PHASE_CRASHED = 0x81,
} DTTR_BootstrapPhase;

typedef enum {
	DTTR_BOOTSTRAP_FAILURE_NONE = 0,
	DTTR_BOOTSTRAP_FAILURE_KERNEL32_NOT_FOUND = 1,
	DTTR_BOOTSTRAP_FAILURE_LOADLIBRARYEX_NOT_FOUND = 2,
	DTTR_BOOTSTRAP_FAILURE_EXITTHREAD_NOT_FOUND = 3,
	DTTR_BOOTSTRAP_FAILURE_GETLASTERROR_NOT_FOUND = 4,
	DTTR_BOOTSTRAP_FAILURE_HELPER_LOAD_FAILED = 5,
	DTTR_BOOTSTRAP_FAILURE_HELPER_BOOTSTRAP_EXPORT_NOT_FOUND = 6,
	DTTR_BOOTSTRAP_FAILURE_HELPER_BOOTSTRAP_FAILED = 7,
	DTTR_BOOTSTRAP_FAILURE_SIDECAR_LOAD_FAILED = 8,
	DTTR_BOOTSTRAP_FAILURE_SIDECAR_BIND_EXPORT_NOT_FOUND = 9,
} DTTR_BootstrapFailureCode;

enum {
	DTTR_BOOTSTRAP_FLAG_DUMP_WRITTEN = 1u << 0,
};

typedef struct {
	uint32_t m_phase;			// 0x0
	uint32_t m_failure_code;	// 0x4
	uint32_t m_last_error;		// 0x8
	uint32_t m_exception_code;	// 0xC
	uint32_t m_flags;			// 0x10
	uint32_t m_helper_module;	// 0x14
	uint32_t m_sidecar_module;	// 0x18
	uint32_t m_kernel32_module; // 0x1C
	uint32_t m_ntdll_module;	// 0x20
	char m_dump_path[MAX_PATH]; // 0x24
} DTTR_BootstrapState;

enum {
	DTTR_LOADER_SHELLCODE_STATUS_KERNEL32_READY = 1,
	DTTR_LOADER_SHELLCODE_STATUS_LOADLIBRARY_READY = 2,
	DTTR_LOADER_SHELLCODE_STATUS_EXITTHREAD_READY = 3,
	DTTR_LOADER_SHELLCODE_STATUS_GETLASTERROR_READY = 4,
	DTTR_LOADER_SHELLCODE_STATUS_LOADED = 5,
	DTTR_LOADER_SHELLCODE_STATUS_KERNEL32_NOT_FOUND = 0x11,
	DTTR_LOADER_SHELLCODE_STATUS_LOADLIBRARY_NOT_FOUND = 0x12,
	DTTR_LOADER_SHELLCODE_STATUS_EXITTHREAD_NOT_FOUND = 0x13,
	DTTR_LOADER_SHELLCODE_STATUS_GETLASTERROR_NOT_FOUND = 0x14,
	DTTR_LOADER_SHELLCODE_STATUS_LOADLIBRARY_FAILED = 0x80,
};

typedef struct {
	// Absolute sidecar DLL path
	char m_dll_path[MAX_PATH]; // 0x0

	// Diagnostics
	uint32_t m_status;	   // 0x104
	uint32_t m_module;	   // 0x108
	uint32_t m_last_error; // 0x10C

	// Original game entrypoint
	uint32_t m_original_entry; // 0x110

	// Embedded kernel32.dll as UTF-16LE
	WCHAR m_kernel32_name[sizeof(L"kernel32.dll") / sizeof(WCHAR)]; // 0x114

	// Embedded ASCII kernel32 exports
	char m_loadlibraryex_name[sizeof("LoadLibraryExA")]; // 0x12E
	char m_exitthread_name[sizeof("ExitThread")];		 // 0x13D
	char m_getlasterror_name[sizeof("GetLastError")];	 // 0x148

	// Sidecar-resolved ExitThread pointer used after the entrypoint returns
	uint32_t m_exitthread; // 0x158
} DTTR_LoaderShellcodePayload;

#endif /* DTTR_BOOTSTRAP_H */
