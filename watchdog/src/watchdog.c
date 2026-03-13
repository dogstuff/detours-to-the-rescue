#include <dttr_bootstrap.h>

#include <dbghelp.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>

static HMODULE s_self_module = NULL;
static DTTR_BootstrapState *s_bootstrap_state = NULL;
static LPTOP_LEVEL_EXCEPTION_FILTER s_previous_filter = NULL;
static char s_dump_dir[MAX_PATH];

static void s_copy_string(char *dst, size_t dst_size, const char *src) {
	if (!dst || dst_size == 0) {
		return;
	}

	snprintf(dst, dst_size, "%s", src ? src : "");
}

static bool s_init_dump_dir(void) {
	if (!GetModuleFileNameA(s_self_module, s_dump_dir, sizeof(s_dump_dir))) {
		return false;
	}

	char *const last_sep = strrchr(s_dump_dir, '\\');
	if (!last_sep) {
		SetLastError(ERROR_BAD_PATHNAME);
		return false;
	}

	last_sep[1] = '\0';
	return true;
}

static bool s_write_dump(EXCEPTION_POINTERS *exception_info, char *out_path, size_t out_path_size) {
	SYSTEMTIME st;
	GetLocalTime(&st);

	char filename[MAX_PATH];
	const int written = snprintf(
		filename,
		sizeof(filename),
		"%sdttr_crash_%04d%02d%02d_%02d%02d%02d.dmp",
		s_dump_dir,
		st.wYear,
		st.wMonth,
		st.wDay,
		st.wHour,
		st.wMinute,
		st.wSecond
	);
	if (written <= 0 || (size_t)written >= sizeof(filename)) {
		SetLastError(ERROR_BUFFER_OVERFLOW);
		return false;
	}

	HANDLE file = CreateFileA(
		filename,
		GENERIC_WRITE,
		0,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);
	if (file == INVALID_HANDLE_VALUE) {
		return false;
	}

	MINIDUMP_EXCEPTION_INFORMATION mei = {
		.ThreadId = GetCurrentThreadId(),
		.ExceptionPointers = exception_info,
		.ClientPointers = FALSE,
	};

#ifdef NDEBUG
	const MINIDUMP_TYPE dump_type = MiniDumpNormal;
#else
	const MINIDUMP_TYPE dump_type = MiniDumpWithDataSegs
									| MiniDumpWithIndirectlyReferencedMemory;
#endif

	const BOOL success = MiniDumpWriteDump(
		GetCurrentProcess(),
		GetCurrentProcessId(),
		file,
		dump_type,
		&mei,
		NULL,
		NULL
	);
	const DWORD write_error = success ? ERROR_SUCCESS : GetLastError();
	CloseHandle(file);

	if (!success) {
		SetLastError(write_error);
		return false;
	}

	s_copy_string(out_path, out_path_size, filename);
	return true;
}

static LONG WINAPI s_unhandled_exception_filter(EXCEPTION_POINTERS *exception_info) {
	if (s_bootstrap_state) {
		s_bootstrap_state->m_phase = DTTR_BOOTSTRAP_PHASE_CRASHED;
		s_bootstrap_state->m_failure_code = DTTR_BOOTSTRAP_FAILURE_NONE;
		s_bootstrap_state->m_exception_code = exception_info->ExceptionRecord->ExceptionCode;
		s_bootstrap_state->m_flags &= ~DTTR_BOOTSTRAP_FLAG_DUMP_WRITTEN;
		s_bootstrap_state->m_dump_path[0] = '\0';

		if (s_write_dump(
				exception_info,
				s_bootstrap_state->m_dump_path,
				sizeof(s_bootstrap_state->m_dump_path)
			)) {
			s_bootstrap_state->m_flags |= DTTR_BOOTSTRAP_FLAG_DUMP_WRITTEN;
			s_bootstrap_state->m_last_error = ERROR_SUCCESS;
		} else {
			s_bootstrap_state->m_last_error = GetLastError();
		}
	}

	if (s_previous_filter) {
		return s_previous_filter(exception_info);
	}

	return EXCEPTION_CONTINUE_SEARCH;
}

__declspec(dllexport) DWORD dttr_watchdog_bootstrap(DTTR_BootstrapState *state) {
	if (!state) {
		return ERROR_INVALID_PARAMETER;
	}

	s_bootstrap_state = state;

	if (!s_init_dump_dir()) {
		return GetLastError();
	}

	s_previous_filter = SetUnhandledExceptionFilter(s_unhandled_exception_filter);
	s_bootstrap_state->m_phase = DTTR_BOOTSTRAP_PHASE_HELPER_BOOTSTRAPPED;
	s_bootstrap_state->m_failure_code = DTTR_BOOTSTRAP_FAILURE_NONE;
	s_bootstrap_state->m_last_error = ERROR_SUCCESS;
	return ERROR_SUCCESS;
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason_for_call, LPVOID reserved) {
	(void)reserved;

	if (reason_for_call == DLL_PROCESS_ATTACH) {
		s_self_module = module;
	}

	return TRUE;
}
