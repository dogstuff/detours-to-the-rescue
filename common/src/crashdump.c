#include <dttr_config.h>
#include <dttr_crashdump.h>
#include <dttr_errors.h>

#include <dbghelp.h>

#include <log.h>
#include <sds.h>

static char s_dump_dir[MAX_PATH];

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
		log_error("Failed to create dump file %s", filename);
		sdsfree(filename);
		return NULL;
	}

	MINIDUMP_EXCEPTION_INFORMATION mei = {
		.ThreadId = tid,
		.ExceptionPointers = exception_info,
		.ClientPointers = FALSE,
	};

	const MINIDUMP_TYPE dump_type = (g_dttr_config.m_minidump_type
									 == DTTR_MINIDUMP_DETAILED)
										? (MiniDumpWithDataSegs
										   | MiniDumpWithIndirectlyReferencedMemory)
										: MiniDumpNormal;
	const BOOL success = MiniDumpWriteDump(process, pid, file, dump_type, &mei, NULL, NULL);
	CloseHandle(file);

	if (!success) {
		log_error("Failed to write crash dump to %s", filename);
		sdsfree(filename);
		return NULL;
	}

	log_info("Crash dump written to %s", filename);
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

	for (int i = 0; i < 32; i++) {
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
			)) {
			break;
		}

		if (frame.AddrPC.Offset == 0) {
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
		_Alignas(IMAGEHLP_SYMBOL) char sym_buf[sizeof(IMAGEHLP_SYMBOL) + 256];
		IMAGEHLP_SYMBOL *sym = (IMAGEHLP_SYMBOL *)sym_buf;
		sym->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL);
		sym->MaxNameLength = 256;

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
	sds filename = dttr_crashdump_write(
		GetCurrentProcess(),
		GetCurrentProcessId(),
		GetCurrentThreadId(),
		exception_info
	);

	sds message;

	if (filename) {
		message = sdscatprintf(
			sdsempty(),
			"Exception 0x%08lX\n\nDump written to:\n%s",
			code,
			filename
		);
	} else {
		message = sdscatprintf(
			sdsempty(),
			"Exception 0x%08lX\n\nFailed to write crash dump.",
			code
		);
	}

#ifndef NDEBUG
	CONTEXT ctx_copy = *exception_info->ContextRecord;
	s_append_stack_trace(&message, &ctx_copy);
#endif

	message = sdscat(message, DTTR_REPORT_SUFFIX);
	dttr_sdl_show_simple_message_box(
		SDL_MESSAGEBOX_ERROR,
		"crash jumpscare",
		message,
		NULL
	);
	sdsfree(message);
	sdsfree(filename);
	ExitProcess(1);
	return EXCEPTION_CONTINUE_SEARCH;
}

void dttr_crashdump_init(const char *const dump_dir) {
	strncpy(s_dump_dir, dump_dir, MAX_PATH - 1);
	s_dump_dir[MAX_PATH - 1] = '\0';
	SetUnhandledExceptionFilter(s_unhandled_exception_filter);
	log_debug("Crash dump handler installed");
}
