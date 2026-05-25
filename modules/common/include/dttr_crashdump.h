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

/// Clears the registered crash stack symbol provider.
void DTTR_CrashDump_ClearSymbolProvider();

/// Formats a stack trace from a thread context; caller frees the returned sds.
sds DTTR_CrashDump_FormatStackTrace(HANDLE process, HANDLE thread, const CONTEXT *context);

/// Writes a process minidump and returns the dump filename; caller frees the returned sds.
sds DTTR_CrashDump_Write(
	HANDLE process,
	DWORD pid,
	DWORD tid,
	EXCEPTION_POINTERS *exception_info
);

/// Emits a complete crash report to the log and Windows debug trace stream.
void DTTR_CrashDump_LogAndTraceReport(const char *message);

/// Installs an exception filter that writes dumps under the given directory and shows a
/// crash dialog.
void DTTR_CrashDump_Init(const char *dump_dir);

#endif // DTTR_CRASHDUMP_H
