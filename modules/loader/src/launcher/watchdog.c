#include <dttr_crashdump.h>
#include <dttr_errors.h>
#include <dttr_loader.h>
#include <dttr_log.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <windows.h>

enum { S_WATCHDOG_TIMEOUT_MS = 30000 };
static const char S_WATCHDOG_SENTINEL[] = "DTTR_SIDECAR_ENTRYPOINT";

typedef BOOL(WINAPI *S_IsWow64Process2)(HANDLE, USHORT *, USHORT *);

static bool s_watchdog_attached = false;

static void s_detach_watchdog(DWORD process_id) {
	if (!s_watchdog_attached) {
		return;
	}

	DebugActiveProcessStop(process_id);
	s_watchdog_attached = false;
	DTTR_LOG_DEBUG("Watchdog detached");
}

static bool s_should_disable_watchdog(void) {
	const HMODULE kernel32 = GetModuleHandleA("kernel32.dll");
	const S_IsWow64Process2 is_wow64_process2
		= (S_IsWow64Process2)(kernel32 ? GetProcAddress(kernel32, "IsWow64Process2")
									   : NULL);

	if (is_wow64_process2) {
		uint16_t process_machine = IMAGE_FILE_MACHINE_UNKNOWN;

		uint16_t native_machine = IMAGE_FILE_MACHINE_UNKNOWN;
		if (is_wow64_process2(GetCurrentProcess(), &process_machine, &native_machine)) {
			DTTR_LOG_DEBUG(
				"Watchdog host machine detection: process=0x%X native=0x%X",
				process_machine,
				native_machine
			);
			return native_machine == IMAGE_FILE_MACHINE_ARM64;
		}
	}

	SYSTEM_INFO system_info = {0};
	GetNativeSystemInfo(&system_info);
	DTTR_LOG_DEBUG(
		"Watchdog fallback architecture detection: native_arch=0x%X",
		system_info.wProcessorArchitecture
	);

	return system_info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM64;
}

static void s_write_child_dump(HANDLE process, DWORD pid, DWORD tid, DWORD exception_code) {
	HANDLE thread = OpenThread(THREAD_ALL_ACCESS, FALSE, tid);
	if (!thread) {
		DTTR_ERROR("Failed to open crashing thread %lu", tid);
		return;
	}

	CONTEXT thread_context = {.ContextFlags = CONTEXT_ALL};
	GetThreadContext(thread, &thread_context);
	CloseHandle(thread);

	EXCEPTION_RECORD fake_record = {.ExceptionCode = exception_code};
	EXCEPTION_POINTERS ptrs = {
		.ExceptionRecord = &fake_record,
		.ContextRecord = &thread_context,
	};

	sds filename = dttr_crashdump_write(process, pid, tid, &ptrs);

	if (filename) {
		DTTR_ERROR(
			"Game crashed (exception 0x%08lX). Crash dump written to "
			"%s." DTTR_REPORT_SUFFIX,
			exception_code,
			filename
		);
		sdsfree(filename);
	} else {
		DTTR_ERROR(
			"Game crashed (exception 0x%08lX). Failed to write crash "
			"dump." DTTR_REPORT_SUFFIX,
			exception_code
		);
	}
}

void dttr_loader_watchdog_attach(const PROCESS_INFORMATION *child_info) {
	s_watchdog_attached = false;

	if (s_should_disable_watchdog()) {
		DTTR_LOG_WARN(
			"Skipping watchdog debugger because debugging is not available on this "
			"machine"
		);
		return;
	}

	if (!DebugActiveProcess(child_info->dwProcessId)) {
		DTTR_LOG_WARN(
			"Could not attach debugger to child process; skipping early crash detection"
		);
		return;
	}

	s_watchdog_attached = true;
	DTTR_LOG_DEBUG("Watchdog attached to PID %lu", child_info->dwProcessId);
}

static bool s_is_sentinel(HANDLE process, const OUTPUT_DEBUG_STRING_INFO *info) {
	if (info->fUnicode || info->nDebugStringLength < sizeof(S_WATCHDOG_SENTINEL)) {
		return false;
	}

	char buf[sizeof(S_WATCHDOG_SENTINEL)];
	SIZE_T bytes_read = 0;

	if (!ReadProcessMemory(
			process,
			info->lpDebugStringData,
			buf,
			sizeof(buf),
			&bytes_read
		)) {
		return false;
	}

	return bytes_read == sizeof(S_WATCHDOG_SENTINEL)
		   && memcmp(buf, S_WATCHDOG_SENTINEL, sizeof(S_WATCHDOG_SENTINEL)) == 0;
}

void dttr_loader_watchdog_wait(const PROCESS_INFORMATION *child_info) {
	if (!s_watchdog_attached) {
		DTTR_LOG_DEBUG("Watchdog not attached; skipping early crash monitoring");
		return;
	}

	DTTR_LOG_DEBUG(
		"Watching for early crash or ready sentinel (timeout=%dms)",
		S_WATCHDOG_TIMEOUT_MS
	);

	DEBUG_EVENT evt = {0};
	DWORD remaining = S_WATCHDOG_TIMEOUT_MS;

	while (remaining > 0) {
		const DWORD start = GetTickCount();

		if (!WaitForDebugEvent(&evt, remaining)) {
			break;
		}

		DWORD continue_status = DBG_CONTINUE;
		bool done = false;

		switch (evt.dwDebugEventCode) {
		case EXCEPTION_DEBUG_EVENT: {
			const DWORD code = evt.u.Exception.ExceptionRecord.ExceptionCode;

			if (evt.u.Exception.dwFirstChance) {
				if (code != EXCEPTION_BREAKPOINT) {
					continue_status = DBG_EXCEPTION_NOT_HANDLED;
				}
				break;
			}

			s_write_child_dump(
				child_info->hProcess,
				child_info->dwProcessId,
				evt.dwThreadId,
				code
			);
			done = true;
			break;
		}

		case OUTPUT_DEBUG_STRING_EVENT:
			if (s_is_sentinel(child_info->hProcess, &evt.u.DebugString)) {
				DTTR_LOG_INFO("Sidecar confirmed entrypoint entered!");
				done = true;
			}

			break;

		case EXIT_PROCESS_DEBUG_EVENT:
			DTTR_ERROR(
				"Game exited unexpectedly within %ds (code %lu)." DTTR_REPORT_SUFFIX,
				S_WATCHDOG_TIMEOUT_MS / 1000,
				evt.u.ExitProcess.dwExitCode
			);
			done = true;
			break;

		default:
			break;
		}

		ContinueDebugEvent(evt.dwProcessId, evt.dwThreadId, continue_status);

		if (done) {
			break;
		}

		const DWORD elapsed = GetTickCount() - start;
		remaining -= (elapsed < remaining) ? elapsed : remaining;
	}

	s_detach_watchdog(child_info->dwProcessId);
}
