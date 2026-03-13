#include <dttr_crashdump.h>
#include <dttr_errors.h>
#include <dttr_loader.h>
#include <log.h>
#include <stdbool.h>
#include <string.h>
#include <windows.h>

#define WATCHDOG_TIMEOUT_MS 30000
#define WATCHDOG_SENTINEL "DTTR_SIDECAR_ENTRYPOINT"

static bool s_watchdog_attached = false;

static bool s_should_disable_watchdog(void) {
	typedef BOOL(WINAPI * S_IsWow64Process2)(HANDLE, USHORT *, USHORT *);

	const HMODULE kernel32 = GetModuleHandleA("kernel32.dll");
	const S_IsWow64Process2 is_wow64_process2
		= (S_IsWow64Process2)(kernel32 ? GetProcAddress(kernel32, "IsWow64Process2")
									   : NULL);

	if (is_wow64_process2) {
		USHORT process_machine = IMAGE_FILE_MACHINE_UNKNOWN;
		USHORT native_machine = IMAGE_FILE_MACHINE_UNKNOWN;
		if (is_wow64_process2(GetCurrentProcess(), &process_machine, &native_machine)) {
			log_debug(
				"Watchdog host machine detection: process=0x%04X native=0x%04X",
				process_machine,
				native_machine
			);
			return native_machine == IMAGE_FILE_MACHINE_ARM64;
		}
	}

	SYSTEM_INFO system_info = {0};
	GetNativeSystemInfo(&system_info);
	log_debug(
		"Watchdog fallback architecture detection: native_arch=0x%04X",
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

	CONTEXT ctx = {.ContextFlags = CONTEXT_ALL};
	GetThreadContext(thread, &ctx);
	CloseHandle(thread);

	EXCEPTION_RECORD fake_record = {.ExceptionCode = exception_code};
	EXCEPTION_POINTERS ptrs = {.ExceptionRecord = &fake_record, .ContextRecord = &ctx};

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
		log_warn("Skipping watchdog debugger on ARM64 host");
		return;
	}

	if (!DebugActiveProcess(child_info->dwProcessId)) {
		log_warn(
			"Could not attach debugger to child process; skipping early crash detection"
		);
		return;
	}

	s_watchdog_attached = true;
	log_debug("Watchdog attached to PID %lu", child_info->dwProcessId);
}

/// Reads a debug string from the child process and checks if it matches the sentinel.
static bool s_is_sentinel(HANDLE process, const OUTPUT_DEBUG_STRING_INFO *info) {
	if (info->fUnicode || info->nDebugStringLength < sizeof(WATCHDOG_SENTINEL)) {
		return false;
	}

	char buf[sizeof(WATCHDOG_SENTINEL)];
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

	return bytes_read >= sizeof(WATCHDOG_SENTINEL)
		   && memcmp(buf, WATCHDOG_SENTINEL, sizeof(WATCHDOG_SENTINEL)) == 0;
}

void dttr_loader_watchdog_wait(const PROCESS_INFORMATION *child_info) {
	if (!s_watchdog_attached) {
		log_debug("Watchdog not attached; skipping early crash monitoring");
		return;
	}

	log_debug(
		"Watching for early crash or ready sentinel (timeout=%dms)",
		WATCHDOG_TIMEOUT_MS
	);

	DEBUG_EVENT evt;
	DWORD remaining = WATCHDOG_TIMEOUT_MS;

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

			if (!evt.u.Exception.dwFirstChance) {
				s_write_child_dump(
					child_info->hProcess,
					child_info->dwProcessId,
					evt.dwThreadId,
					code
				);
				done = true;
			} else if (code != EXCEPTION_BREAKPOINT) {
				continue_status = DBG_EXCEPTION_NOT_HANDLED;
			}

			break;
		}

		case OUTPUT_DEBUG_STRING_EVENT:
			if (s_is_sentinel(child_info->hProcess, &evt.u.DebugString)) {
				log_info("Sidecar confirmed entrypoint entered!");
				done = true;
			}

			break;

		case EXIT_PROCESS_DEBUG_EVENT:
			DTTR_ERROR(
				"Game exited unexpectedly within %ds (code %lu)." DTTR_REPORT_SUFFIX,
				WATCHDOG_TIMEOUT_MS / 1000,
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

	DebugActiveProcessStop(child_info->dwProcessId);
	log_debug("Watchdog detached");
}
