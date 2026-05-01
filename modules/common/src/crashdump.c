#include <dttr_config.h>
#include <dttr_crashdump.h>
#include <dttr_errors.h>
#include <dttr_imgui.h>
#include <dttr_path.h>
#include <dttr_sdl.h>

#include <dbghelp.h>

#include <dttr_log.h>
#include <sds.h>

static char s_dump_dir[MAX_PATH];

#ifndef NDEBUG

#define S_MAX_STACK_FRAMES 32
#define S_SYMBOL_NAME_CAPACITY 256
#define S_SYMBOL_BUFFER_SIZE (sizeof(IMAGEHLP_SYMBOL) + S_SYMBOL_NAME_CAPACITY)

#endif

static MINIDUMP_TYPE s_minidump_type(void) {
	if (g_dttr_config.m_minidump_type == DTTR_MINIDUMP_DETAILED) {
		return MiniDumpWithDataSegs | MiniDumpWithIndirectlyReferencedMemory;
	}

	return MiniDumpNormal;
}

static sds s_build_crash_message(DWORD code, const char *filename) {
	if (filename) {
		return sdscatprintf(
			sdsempty(),
			"Exception 0x%08lX\n\nDump written to:\n%s",
			code,
			filename
		);
	}

	return sdscatprintf(
		sdsempty(),
		"Exception 0x%08lX\n\nFailed to write crash dump.",
		code
	);
}

sds dttr_crashdump_write(
	HANDLE process,
	DWORD pid,
	DWORD tid,
	EXCEPTION_POINTERS *exception_info
) {
	SYSTEMTIME st;
	GetLocalTime(&st);

	sds filename = sdscatprintf(
		sdsempty(),
		"%sdttr_crash_%04d%02d%02d_%02d%02d%02d.dmp",
		s_dump_dir,
		st.wYear,
		st.wMonth,
		st.wDay,
		st.wHour,
		st.wMinute,
		st.wSecond
	);

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
		DTTR_LOG_ERROR("Failed to create dump file %s", filename);
		sdsfree(filename);
		return NULL;
	}

	MINIDUMP_EXCEPTION_INFORMATION mei = {
		.ThreadId = tid,
		.ExceptionPointers = exception_info,
		.ClientPointers = FALSE,
	};

	const BOOL success = MiniDumpWriteDump(
		process,
		pid,
		file,
		s_minidump_type(),
		&mei,
		NULL,
		NULL
	);
	CloseHandle(file);

	if (!success) {
		DTTR_LOG_ERROR("Failed to write crash dump to %s", filename);
		sdsfree(filename);
		return NULL;
	}

	DTTR_LOG_INFO("Crash dump written to %s", filename);
	return filename;
}

#ifndef NDEBUG

static void s_append_stack_trace(sds *message, CONTEXT *ctx) {
	HANDLE process = GetCurrentProcess();
	HANDLE thread = GetCurrentThread();

	SymInitialize(process, NULL, TRUE);

	STACKFRAME frame = {0};
	frame.AddrPC.Offset = ctx->Eip;
	frame.AddrPC.Mode = AddrModeFlat;
	frame.AddrFrame.Offset = ctx->Ebp;
	frame.AddrFrame.Mode = AddrModeFlat;
	frame.AddrStack.Offset = ctx->Esp;
	frame.AddrStack.Mode = AddrModeFlat;

	*message = sdscat(*message, "\n\nStack trace:");

	for (int i = 0; i < S_MAX_STACK_FRAMES; i++) {
		if (!StackWalk(
				IMAGE_FILE_MACHINE_I386,
				process,
				thread,
				&frame,
				ctx,
				NULL,
				SymFunctionTableAccess,
				SymGetModuleBase,
				NULL
			)
			|| frame.AddrPC.Offset == 0) {
			break;
		}

		const DWORD addr = (DWORD)frame.AddrPC.Offset;

		// Resolve module name
		IMAGEHLP_MODULE module_info = {.SizeOfStruct = sizeof(IMAGEHLP_MODULE)};
		const char *mod_name = "???";
		if (SymGetModuleInfo(process, addr, &module_info)) {
			mod_name = module_info.ModuleName;
		}

		// Resolve symbol name
		_Alignas(IMAGEHLP_SYMBOL) char sym_buf[S_SYMBOL_BUFFER_SIZE];
		IMAGEHLP_SYMBOL *sym = (IMAGEHLP_SYMBOL *)sym_buf;
		sym->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL);
		sym->MaxNameLength = S_SYMBOL_NAME_CAPACITY;

		DWORD displacement = 0;
		if (!SymGetSymFromAddr(process, addr, &displacement, sym)) {
			*message = sdscatprintf(*message, "\n  %s!0x%08lX", mod_name, addr);
			continue;
		}

		*message = sdscatprintf(
			*message,
			"\n  %s!%s+0x%lX",
			mod_name,
			sym->Name,
			displacement
		);
	}

	SymCleanup(process);
}

#endif

static LONG WINAPI s_unhandled_exception_filter(EXCEPTION_POINTERS *const exception_info) {
	const DWORD code = exception_info->ExceptionRecord->ExceptionCode;
	const HANDLE process = GetCurrentProcess();
	const DWORD pid = GetCurrentProcessId();
	const DWORD tid = GetCurrentThreadId();
	sds filename = dttr_crashdump_write(process, pid, tid, exception_info);

	sds message = s_build_crash_message(code, filename);

#ifndef NDEBUG
	CONTEXT ctx_copy = *exception_info->ContextRecord;
	s_append_stack_trace(&message, &ctx_copy);
#endif

	message = sdscat(message, DTTR_REPORT_SUFFIX);
	if (!dttr_imgui_error_show("DttR: Crash", message)) {
		dttr_sdl_show_simple_message_box(
			SDL_MESSAGEBOX_ERROR,
			"DttR: Crash",
			message,
			NULL
		);
	}
	sdsfree(message);
	sdsfree(filename);
	ExitProcess(1);
	return EXCEPTION_CONTINUE_SEARCH;
}

static bool s_set_dump_dir(const char *base_dir) {
	sds dump_dir = sdsnew(base_dir);
	if (!dump_dir
		|| !dttr_path_append_segment(&dump_dir, "dumps", DTTR_PATH_NATIVE_SEPARATOR)) {
		sdsfree(dump_dir);
		return false;
	}

	if (!CreateDirectoryA(dump_dir, NULL) && GetLastError() != ERROR_ALREADY_EXISTS) {
		DTTR_LOG_ERROR("Failed to create crash dump directory %s", dump_dir);
		sdsfree(dump_dir);
		return false;
	}

	if (!dttr_path_append_separator(&dump_dir, DTTR_PATH_NATIVE_SEPARATOR)
		|| !dttr_path_copy_sds(s_dump_dir, sizeof(s_dump_dir), dump_dir)) {
		sdsfree(dump_dir);
		return false;
	}

	sdsfree(dump_dir);
	return true;
}

void dttr_crashdump_init(const char *const dump_dir) {
	if (!s_set_dump_dir(dump_dir)) {
		DTTR_LOG_ERROR("Could not initialize crash dump directory");
		s_dump_dir[0] = '\0';
	}

	SetUnhandledExceptionFilter(s_unhandled_exception_filter);
	DTTR_LOG_DEBUG("Crash dump handler installed");
}
