#ifndef DTTR_CRASHDUMP_H
#define DTTR_CRASHDUMP_H

#include <sds.h>
#include <windows.h>

/// Writes a minidump for the given process and returns the dump filename on success.
/// Returns NULL on failure. Caller frees the returned sds.
sds dttr_crashdump_write(
	HANDLE process,
	DWORD pid,
	DWORD tid,
	EXCEPTION_POINTERS *exception_info
);

/// Installs an unhandled exception filter that writes a crash dump and shows a dialog.
/// Crash dumps will be written to a dumps directory inside the given directory.
void dttr_crashdump_init(const char *dump_dir);

#endif
