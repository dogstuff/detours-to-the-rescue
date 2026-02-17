#include <dttr_crashdump.h>
#include <dttr_errors.h>
#include <dttr_loader.h>
#include <log.h>
#include <stdbool.h>
#include <string.h>
#include <windows.h>

#define WATCHDOG_TIMEOUT_MS 30000
#define WATCHDOG_SENTINEL "DTTR_SIDECAR_ENTRYPOINT"

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
			"Game crashed (exception 0x%08lX). Crash dump written to %s.",
			exception_code,
			filename
		);
		sdsfree(filename);
	} else {
		DTTR_ERROR(
			"Game crashed (exception 0x%08lX). Failed to write crash dump.",
			exception_code
		);
	}
}

void dttr_loader_watchdog_attach(const PROCESS_INFORMATION *child_info) {
	if (!DebugActiveProcess(child_info->dwProcessId)) {
		log_warn(
			"Could not attach debugger to child process; skipping early crash detection"
		);
		return;
	}

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
				"Game exited unexpectedly within %ds (code %lu).",
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
