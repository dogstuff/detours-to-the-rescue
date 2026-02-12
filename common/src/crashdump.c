#ifndef NDEBUG

#include <windows.h>

#include <dbghelp.h>

#include <log.h>
#include <sds.h>

#define DTTR_PREFIX_CRASH "[crash] "

static const char *s_dump_name;

static LONG WINAPI s_unhandled_exception_filter(EXCEPTION_POINTERS *const exception_info) {
	SYSTEMTIME st;
	GetLocalTime(&st);

	const sds filename = sdscatprintf(
		sdsempty(),
		"%s_crash_%04d%02d%02d_%02d%02d%02d.dmp",
		s_dump_name,
		st.wYear,
		st.wMonth,
		st.wDay,
		st.wHour,
		st.wMinute,
		st.wSecond
	);

	const HANDLE file
		= CreateFileA(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (file == INVALID_HANDLE_VALUE) {
		log_error(DTTR_PREFIX_CRASH "Failed to create dump file %s", filename);
		sdsfree(filename);
		return EXCEPTION_CONTINUE_SEARCH;
	}

	MINIDUMP_EXCEPTION_INFORMATION mei;
	mei.ThreadId = GetCurrentThreadId();
	mei.ExceptionPointers = exception_info;
	mei.ClientPointers = FALSE;

	const BOOL success = MiniDumpWriteDump(
		GetCurrentProcess(), GetCurrentProcessId(), file, MiniDumpNormal, &mei, NULL, NULL
	);

	CloseHandle(file);

	if (success) {
		log_error(
			DTTR_PREFIX_CRASH "Exception 0x%08lX; dump written to %s",
			exception_info->ExceptionRecord->ExceptionCode,
			filename
		);
	} else {
		log_error(
			DTTR_PREFIX_CRASH "Exception 0x%08lX; failed to write dump",
			exception_info->ExceptionRecord->ExceptionCode
		);
	}

	sdsfree(filename);
	return EXCEPTION_CONTINUE_SEARCH;
}

void dttr_crashdump_init(const char *const name) {
	s_dump_name = name;
	SetUnhandledExceptionFilter(s_unhandled_exception_filter);
	log_info(DTTR_PREFIX_CRASH "Crash dump handler installed");
}

#else

void dttr_crashdump_init(const char *name) {
	(void)name;
}

#endif
