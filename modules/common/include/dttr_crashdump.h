#ifndef DTTR_CRASHDUMP_H
#define DTTR_CRASHDUMP_H

#include <sds.h>
#include <windows.h>

/// Formats a stack trace from a thread context; caller frees the returned sds.
sds DTTR_Crashdump_FormatStackTrace(HANDLE process, HANDLE thread, const CONTEXT *context);

/// Writes a process minidump and returns the dump filename; caller frees the returned sds.
sds DTTR_Crashdump_Write(
	HANDLE process,
	DWORD pid,
	DWORD tid,
	EXCEPTION_POINTERS *exception_info
);

/// Installs an exception filter that writes dumps under the given directory and shows a
/// crash dialog.
void DTTR_Crashdump_Init(const char *dump_dir);

#endif // DTTR_CRASHDUMP_H
