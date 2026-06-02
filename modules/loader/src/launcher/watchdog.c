#include <dttr_config.h>
#include <dttr_crashdump.h>
#include <dttr_errors.h>
#include <dttr_loader.h>
#include <dttr_log.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <windows.h>

enum { WATCHDOG_TIMEOUT_MS = 30000 };
static const char WATCHDOG_SENTINEL[] = "DTTR_SIDECAR_ENTRYPOINT";

typedef BOOL(WINAPI *is_wow64_process2_fn)(HANDLE, USHORT *, USHORT *);

static bool watchdog_attached = false;

static void detach_watchdog(DWORD process_id) {
	if (!watchdog_attached) {
		return;
	}

	DebugActiveProcessStop(process_id);
	watchdog_attached = false;
	DTTR_LOG_DEBUG("Watchdog detached");
}

static bool should_disable_watchdog() {
	const HMODULE kernel32 = GetModuleHandleA("kernel32.dll");
	const is_wow64_process2_fn is_wow64_process2
		= (is_wow64_process2_fn)(kernel32 ? GetProcAddress(kernel32, "IsWow64Process2")
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

static void write_child_dump(HANDLE process, DWORD pid, DWORD tid, DWORD exception_code) {
	HANDLE thread = OpenThread(THREAD_ALL_ACCESS, FALSE, tid);
	if (!thread) {
		DTTR_ERROR("Failed to open crashing thread %lu", tid);
		return;
	}

	CONTEXT thread_context = {.ContextFlags = CONTEXT_ALL};
	if (!GetThreadContext(thread, &thread_context)) {
		CloseHandle(thread);
		DTTR_ERROR("Failed to read crashing thread %lu context", tid);
		return;
	}

	EXCEPTION_RECORD fake_record = {.ExceptionCode = exception_code};
	EXCEPTION_POINTERS ptrs = {
		.ExceptionRecord = &fake_record,
		.ContextRecord = &thread_context,
	};

	sds filename = DTTR_CrashDump_Write(process, pid, tid, &ptrs);
	sds stack_trace = DTTR_CrashDump_FormatStackTrace(process, thread, &thread_context);
	CloseHandle(thread);

	sds summary = sdsempty();
	if (filename) {
		summary = sdscatprintf(
			summary,
			"Game crashed (exception 0x%08lX). Crash dump written to %s.",
			exception_code,
			filename
		);
		sdsfree(filename);
	} else {
		summary = sdscatprintf(
			summary,
			"Game crashed (exception 0x%08lX). Failed to write crash dump.",
			exception_code
		);
	}

	sds report_message = DTTR_CrashDump_AppendReportMessage(summary, stack_trace);

	sdsfree(stack_trace);

	DTTR_CrashDump_LogAndTraceReport(report_message);

	if (dttr_config.show_crash_popup) {
		DTTR_Errors_ShowMessage(DTTR_ERROR_TITLE, report_message);
	}
	
	sdsfree(report_message);
}

void DTTR_Loader_WatchdogAttach(const PROCESS_INFORMATION *child_info) {
	watchdog_attached = false;

	if (should_disable_watchdog()) {
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

	watchdog_attached = true;
	DTTR_LOG_DEBUG("Watchdog attached to PID %lu", child_info->dwProcessId);
}

void DTTR_Loader_WatchdogDetach(const PROCESS_INFORMATION *child_info) {
	detach_watchdog(child_info->dwProcessId);
}

static bool is_sentinel(HANDLE process, const OUTPUT_DEBUG_STRING_INFO *info) {
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

	return bytes_read == sizeof(WATCHDOG_SENTINEL)
		   && memcmp(buf, WATCHDOG_SENTINEL, sizeof(WATCHDOG_SENTINEL)) == 0;
}

bool DTTR_Loader_WatchdogWait(const PROCESS_INFORMATION *child_info) {
	if (!watchdog_attached) {
		DTTR_LOG_DEBUG("Watchdog not attached; skipping early crash monitoring");
		return true;
	}

	DTTR_LOG_DEBUG(
		"Watching for early crash or ready sentinel (timeout=%dms)",
		WATCHDOG_TIMEOUT_MS
	);

	DEBUG_EVENT evt = {0};
	DWORD remaining = WATCHDOG_TIMEOUT_MS;
	bool saw_sentinel = false;
	bool saw_failure = false;

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

			write_child_dump(
				child_info->hProcess,
				child_info->dwProcessId,
				evt.dwThreadId,
				code
			);
			saw_failure = true;
			done = true;
			break;
		}

		case OUTPUT_DEBUG_STRING_EVENT:
			if (is_sentinel(child_info->hProcess, &evt.u.DebugString)) {
				DTTR_LOG_INFO("Sidecar confirmed entrypoint entered!");
				saw_sentinel = true;
				done = true;
			}

			break;

		case CREATE_PROCESS_DEBUG_EVENT:
			if (evt.u.CreateProcessInfo.hFile) {
				CloseHandle(evt.u.CreateProcessInfo.hFile);
			}

			break;

		case LOAD_DLL_DEBUG_EVENT:
			if (evt.u.LoadDll.hFile) {
				CloseHandle(evt.u.LoadDll.hFile);
			}

			break;

		case EXIT_PROCESS_DEBUG_EVENT:
			saw_failure = true;
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

	detach_watchdog(child_info->dwProcessId);
	if (!saw_sentinel && !saw_failure) {
		DTTR_ERROR(
			"Sidecar did not report entrypoint within %ds; aborting "
			"launch." DTTR_REPORT_SUFFIX,
			WATCHDOG_TIMEOUT_MS / 1000
		);
		return false;
	}

	return saw_sentinel && !saw_failure;
}
