#ifndef NDEBUG

#include <windows.h>

#include <dbghelp.h>

#include <log.h>
#include <sds.h>

static const char *s_dump_name;

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
				IMAGE_FILE_MACHINE_I386, process, thread, &frame, ctx, NULL, SymFunctionTableAccess, SymGetModuleBase, NULL
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

		*message = sdscatprintf(*message, "\n  %s!%s+0x%lX", mod_name, sym->Name, displacement);
	}

	SymCleanup(process);
}

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
		log_error("Failed to create dump file %s", filename);
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

	sds message;
	if (success) {
		log_error(
			"Exception 0x%08lX; dump written to %s",
			exception_info->ExceptionRecord->ExceptionCode,
			filename
		);
		message = sdscatprintf(
			sdsempty(),
			"Exception 0x%08lX\n\nDump written to:\n%s",
			exception_info->ExceptionRecord->ExceptionCode,
			filename
		);
	} else {
		log_error(
			"Exception 0x%08lX; failed to write dump",
			exception_info->ExceptionRecord->ExceptionCode
		);
		message = sdscatprintf(
			sdsempty(),
			"Exception 0x%08lX\n\nFailed to write crash dump.",
			exception_info->ExceptionRecord->ExceptionCode
		);
	}

	CONTEXT ctx_copy = *exception_info->ContextRecord;
	s_append_stack_trace(&message, &ctx_copy);

	MessageBoxA(NULL, message, "crash jumpscare", MB_OK | MB_ICONERROR);
	sdsfree(message);
	sdsfree(filename);
	return EXCEPTION_CONTINUE_SEARCH;
}

void dttr_crashdump_init(const char *const name) {
	s_dump_name = name;
	SetUnhandledExceptionFilter(s_unhandled_exception_filter);
	log_info("Crash dump handler installed");
}

#else

void dttr_crashdump_init(const char *name) {
	(void)name;
}

#endif
