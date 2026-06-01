#ifndef DTTR_CRASHDUMP_H
#define DTTR_CRASHDUMP_H

#include <stdbool.h>

#include <sds.h>
#include <windows.h>

/// Adds process-local symbols after DbgHelp initialization and before stack walking.
typedef bool (*DTTR_CrashDump_SymbolProvider)(HANDLE process, void *context);

/// Registers a synchronous symbol provider used by crash stack formatting.
void DTTR_CrashDump_SetSymbolProvider(
	DTTR_CrashDump_SymbolProvider provider,
	void *context
);

void DTTR_CrashDump_ClearSymbolProvider();

/// Formats a stack trace from a thread context. Caller frees the returned sds.
sds DTTR_CrashDump_FormatStackTrace(HANDLE process, HANDLE thread, const CONTEXT *context);

sds DTTR_CrashDump_BuildReportMessage(const char *summary, const char *stack_trace);

/// Writes a process minidump and returns the dump filename. The caller should free the
/// returned sds.
sds DTTR_CrashDump_Write(
	HANDLE process,
	DWORD pid,
	DWORD tid,
	EXCEPTION_POINTERS *exception_info
);

void DTTR_CrashDump_LogAndTraceReport(const char *message);

/// Installs an exception filter that writes dumps under the given directory and shows a
/// crash dialog.
void DTTR_CrashDump_Init(const char *dump_dir);

#endif // DTTR_CRASHDUMP_H
