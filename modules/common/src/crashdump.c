#include <dttr_config.h>
#include <dttr_crashdump.h>
#include <dttr_errors.h>
#include <dttr_imgui.h>
#include <dttr_path.h>
#include <dttr_sdl.h>

#include <dbghelp.h>
#include <Zydis/Zydis.h>

#include <dttr_log.h>
#include <sds.h>

#include <stdint.h>

static char crash_dump_dir[MAX_PATH];

static INIT_ONCE dbghelp_lock_once = INIT_ONCE_STATIC_INIT;
static CRITICAL_SECTION dbghelp_lock;
static DTTR_CrashDump_SymbolProvider crash_symbol_provider;
static void *crash_symbol_provider_context;

static BOOL CALLBACK init_dbghelp_lock(PINIT_ONCE, PVOID, PVOID *) {
	InitializeCriticalSection(&dbghelp_lock);
	return TRUE;
}

static void enter_dbghelp_lock() {
	InitOnceExecuteOnce(&dbghelp_lock_once, init_dbghelp_lock, NULL, NULL);
	EnterCriticalSection(&dbghelp_lock);
}

static void leave_dbghelp_lock() { LeaveCriticalSection(&dbghelp_lock); }

static void invoke_symbol_provider(HANDLE process) {
	if (crash_symbol_provider) {
		crash_symbol_provider(process, crash_symbol_provider_context);
	}
}

void DTTR_CrashDump_SetSymbolProvider(
	DTTR_CrashDump_SymbolProvider provider,
	void *context
) {
	enter_dbghelp_lock();
	crash_symbol_provider = provider;
	crash_symbol_provider_context = context;
	leave_dbghelp_lock();
}

void DTTR_CrashDump_ClearSymbolProvider() {
	DTTR_CrashDump_SetSymbolProvider(NULL, NULL);
}

#define MAX_STACK_FRAMES 64
#define SYMBOL_NAME_CAPACITY 256
#define SYMBOL_BUFFER_SIZE (sizeof(IMAGEHLP_SYMBOL) + SYMBOL_NAME_CAPACITY)
#define DISASM_BYTES_BEFORE 16u
#define DISASM_BYTES_AFTER 32u
#define DISASM_MAX_BYTES (DISASM_BYTES_BEFORE + DISASM_BYTES_AFTER)
#define DISASM_MAX_INSTRUCTIONS 6
#define DISASM_TEXT_CAPACITY 160

static bool memory_protection_is_readable(DWORD protect) {
	if (protect & (PAGE_GUARD | PAGE_NOACCESS)) {
		return false;
	}

	protect &= 0xFF;
	switch (protect) {
		case PAGE_READONLY:
		case PAGE_READWRITE:
		case PAGE_WRITECOPY:
		case PAGE_EXECUTE_READ:
		case PAGE_EXECUTE_READWRITE:
		case PAGE_EXECUTE_WRITECOPY:
			return true;
		default:
			return false;
	}
}

static sds append_disassembly_unavailable(sds message, const char *reason) {
	return sdscatprintf(
		message,
		"\n    <disassembly unavailable: %s>",
		reason ? reason : "unknown"
	);
}

typedef struct {
	DWORD address;
	char text[DISASM_TEXT_CAPACITY];
} CrashDisasmLine;

static sds append_disassembly_line(
	sds message,
	DWORD address,
	const char *text,
	bool failed
) {
	return sdscatprintf(
		message,
		"\n    %s 0x%08lX  %s",
		failed ? "=>" : "  ",
		address,
		text
	);
}

static bool format_instruction_at(
	ZydisDecoder *decoder,
	ZydisFormatter *formatter,
	const uint8_t *bytes,
	size_t size,
	DWORD address,
	ZydisDecodedInstruction *instruction,
	char *text,
	size_t text_size
) {
	ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT] = {0};
	if (!ZYAN_SUCCESS(ZydisDecoderDecodeFull(
			decoder,
			bytes,
			size,
			instruction,
			operands
		))
		|| instruction->length == 0) {
		return false;
	}

	return ZYAN_SUCCESS(ZydisFormatterFormatInstruction(
		formatter,
		instruction,
		operands,
		instruction->operand_count_visible,
		text,
		text_size,
		address,
		ZYAN_NULL
	));
}

static sds append_failed_instruction_decode(
	sds message,
	ZydisDecoder *decoder,
	ZydisFormatter *formatter,
	const uint8_t *bytes,
	size_t size,
	DWORD failed_eip
) {
	size_t offset = 0;
	for (int emitted = 0; offset < size && emitted < DISASM_MAX_INSTRUCTIONS; emitted++) {
		ZydisDecodedInstruction instruction = {0};
		char text[DISASM_TEXT_CAPACITY] = {0};
		if (!format_instruction_at(
				decoder,
				formatter,
				bytes + offset,
				size - offset,
				failed_eip + (DWORD)offset,
				&instruction,
				text,
				sizeof(text)
			)) {
			return emitted > 0 ? message : append_disassembly_unavailable(message, "decode failed");
		}

		message = append_disassembly_line(
			message,
			failed_eip + (DWORD)offset,
			text,
			offset == 0
		);
		offset += instruction.length;
	}

	return message;
}

static sds append_disassembly_from_bytes(
	sds message,
	const uint8_t *bytes,
	size_t size,
	DWORD runtime_base,
	DWORD failed_eip
) {
	if (!bytes || size == 0 || failed_eip < runtime_base) {
		return append_disassembly_unavailable(message, "decode failed");
	}

	const size_t failed_offset = (size_t)(failed_eip - runtime_base);
	if (failed_offset >= size) {
		return append_disassembly_unavailable(message, "decode failed");
	}

	ZydisDecoder decoder = {0};
	if (!ZYAN_SUCCESS(ZydisDecoderInit(
			&decoder,
			ZYDIS_MACHINE_MODE_LEGACY_32,
			ZYDIS_STACK_WIDTH_32
		))) {
		return append_disassembly_unavailable(message, "decoder unavailable");
	}

	ZydisFormatter formatter = {0};
	if (!ZYAN_SUCCESS(ZydisFormatterInit(&formatter, ZYDIS_FORMATTER_STYLE_INTEL))) {
		return append_disassembly_unavailable(message, "decoder unavailable");
	}

	message = sdscatprintf(message, "\n    Disassembly around 0x%08lX:", failed_eip);

	CrashDisasmLine lines[DISASM_MAX_BYTES] = {0};
	size_t line_count = 0;
	size_t failed_index = SIZE_MAX;
	size_t offset = 0;
	while (offset < size && line_count < DISASM_MAX_BYTES) {
		ZydisDecodedInstruction instruction = {0};
		if (!format_instruction_at(
				&decoder,
				&formatter,
				bytes + offset,
				size - offset,
				runtime_base + (DWORD)offset,
				&instruction,
				lines[line_count].text,
				sizeof(lines[line_count].text)
			)) {
			break;
		}

		lines[line_count].address = runtime_base + (DWORD)offset;
		if (lines[line_count].address == failed_eip) {
			failed_index = line_count;
		}

		line_count++;
		offset += instruction.length;
	}

	if (failed_index == SIZE_MAX) {
		return append_failed_instruction_decode(
			message,
			&decoder,
			&formatter,
			bytes + failed_offset,
			size - failed_offset,
			failed_eip
		);
	}

	size_t first = failed_index > 2 ? failed_index - 2 : 0;
	size_t last = first + DISASM_MAX_INSTRUCTIONS;
	if (last > line_count) {
		last = line_count;
	}
	if (last - first < DISASM_MAX_INSTRUCTIONS && last == line_count) {
		const size_t available = last - first;
		if (available < DISASM_MAX_INSTRUCTIONS && first > 0) {
			const size_t backfill = DISASM_MAX_INSTRUCTIONS - available;
			first = first > backfill ? first - backfill : 0;
		}
	}

	for (size_t i = first; i < last; i++) {
		message = append_disassembly_line(
			message,
			lines[i].address,
			lines[i].text,
			i == failed_index
		);
	}

	return message;
}

static sds append_disassembly_window(sds message, HANDLE process, DWORD failed_eip) {
	MEMORY_BASIC_INFORMATION mbi = {0};
	if (VirtualQueryEx(process, (LPCVOID)(uintptr_t)failed_eip, &mbi, sizeof(mbi))
		!= sizeof(mbi)) {
		return append_disassembly_unavailable(message, "unreadable code");
	}

	if (mbi.State != MEM_COMMIT || !memory_protection_is_readable(mbi.Protect)) {
		return append_disassembly_unavailable(message, "unreadable code");
	}

	const uintptr_t region_base = (uintptr_t)mbi.BaseAddress;
	const uintptr_t region_end = region_base + mbi.RegionSize;
	uintptr_t start = failed_eip > DISASM_BYTES_BEFORE
		? (uintptr_t)failed_eip - DISASM_BYTES_BEFORE
		: (uintptr_t)failed_eip;
	if (start < region_base) {
		start = region_base;
	}

	uintptr_t end = (uintptr_t)failed_eip + DISASM_BYTES_AFTER;
	if (end > region_end) {
		end = region_end;
	}

	if (end <= start) {
		return append_disassembly_unavailable(message, "unreadable code");
	}

	size_t size = (size_t)(end - start);
	if (size > DISASM_MAX_BYTES) {
		size = DISASM_MAX_BYTES;
	}

	uint8_t bytes[DISASM_MAX_BYTES] = {0};
	SIZE_T bytes_read = 0;
	if (!ReadProcessMemory(process, (LPCVOID)start, bytes, size, &bytes_read)
		|| bytes_read == 0) {
		return append_disassembly_unavailable(message, "unreadable code");
	}

	return append_disassembly_from_bytes(
		message,
		bytes,
		(size_t)bytes_read,
		(DWORD)start,
		failed_eip
	);
}

static MINIDUMP_TYPE minidump_type() {
	if (dttr_config.minidump_type == DTTR_MINIDUMP_DETAILED) {
		return MiniDumpWithDataSegs | MiniDumpWithIndirectlyReferencedMemory;
	}

	return MiniDumpNormal;
}

static sds build_crash_message(DWORD code, const char *filename) {
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

sds DTTR_CrashDump_BuildReportMessage(const char *summary, const char *stack_trace) {
	sds message = sdsnew(summary ? summary : "");
	if (stack_trace) {
		message = sdscat(message, stack_trace);
	}

	return sdscat(message, DTTR_REPORT_SUFFIX);
}

sds DTTR_CrashDump_Write(
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
		crash_dump_dir,
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

	enter_dbghelp_lock();
	const BOOL success = MiniDumpWriteDump(
		process,
		pid,
		file,
		minidump_type(),
		&mei,
		NULL,
		NULL
	);
	leave_dbghelp_lock();
	CloseHandle(file);

	if (!success) {
		DTTR_LOG_ERROR("Failed to write crash dump to %s", filename);
		sdsfree(filename);
		return NULL;
	}

	DTTR_LOG_INFO("Crash dump written to %s", filename);
	return filename;
}

void DTTR_CrashDump_LogAndTraceReport(const char *message) {
	if (!message) {
		return;
	}

	DTTR_LOG_ERROR("%s", message);
	OutputDebugStringA(message);
	OutputDebugStringA("\n");
}

sds DTTR_CrashDump_FormatStackTrace(HANDLE process, HANDLE thread, const CONTEXT *context) {
	sds message = sdscat(sdsempty(), "\n\nStack trace:");
	if (!process || !thread || !context) {
		return sdscat(message, "\n  <unavailable>");
	}

	CONTEXT ctx = *context;
	message = append_disassembly_window(message, process, ctx.Eip);
	enter_dbghelp_lock();
	if (!SymInitialize(process, NULL, TRUE)) {
		leave_dbghelp_lock();
		return sdscat(message, "\n  <symbols unavailable>");
	}

	invoke_symbol_provider(process);

	STACKFRAME frame = {0};
	frame.AddrPC.Offset = ctx.Eip;
	frame.AddrPC.Mode = AddrModeFlat;
	frame.AddrFrame.Offset = ctx.Ebp;
	frame.AddrFrame.Mode = AddrModeFlat;
	frame.AddrStack.Offset = ctx.Esp;
	frame.AddrStack.Mode = AddrModeFlat;

	int frame_count = 0;
	for (int i = 0; i < MAX_STACK_FRAMES; i++) {
		if (!StackWalk(
				IMAGE_FILE_MACHINE_I386,
				process,
				thread,
				&frame,
				&ctx,
				NULL,
				SymFunctionTableAccess,
				SymGetModuleBase,
				NULL
			)
			|| frame.AddrPC.Offset == 0) {
			break;
		}

		const DWORD addr = (DWORD)frame.AddrPC.Offset;

		IMAGEHLP_MODULE module_info = {.SizeOfStruct = sizeof(IMAGEHLP_MODULE)};
		const char *mod_name = "???";
		if (SymGetModuleInfo(process, addr, &module_info)) {
			mod_name = module_info.ModuleName;
		}

		frame_count++;

		_Alignas(IMAGEHLP_SYMBOL) char sym_buf[SYMBOL_BUFFER_SIZE];
		IMAGEHLP_SYMBOL *sym = (IMAGEHLP_SYMBOL *)sym_buf;
		sym->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL);
		sym->MaxNameLength = SYMBOL_NAME_CAPACITY;

		DWORD displacement = 0;
		if (!SymGetSymFromAddr(process, addr, &displacement, sym)) {
			message = sdscatprintf(message, "\n  %s!0x%08lX", mod_name, addr);
			continue;
		}

		message = sdscatprintf(
			message,
			"\n  %s!%s+0x%lX",
			mod_name,
			sym->Name,
			displacement
		);
	}

	SymCleanup(process);
	leave_dbghelp_lock();
	if (frame_count == 0) {
		message = sdscat(message, "\n  <unavailable>");
	}

	return message;
}

static LONG WINAPI unhandled_exception_filter(EXCEPTION_POINTERS *const exception_info) {
	const DWORD code = exception_info->ExceptionRecord->ExceptionCode;
	const HANDLE process = GetCurrentProcess();
	const DWORD pid = GetCurrentProcessId();
	const DWORD tid = GetCurrentThreadId();
	sds filename = DTTR_CrashDump_Write(process, pid, tid, exception_info);

	sds summary = build_crash_message(code, filename);
	sds stack_trace = DTTR_CrashDump_FormatStackTrace(
		process,
		GetCurrentThread(),
		exception_info->ContextRecord
	);
	sds log_message = DTTR_CrashDump_BuildReportMessage(summary, stack_trace);
	sds popup_message = DTTR_CrashDump_BuildReportMessage(summary, stack_trace);
	sdsfree(stack_trace);
	DTTR_CrashDump_LogAndTraceReport(log_message);

	if (dttr_config.show_crash_popup
		&& !DTTR_ImGui_ErrorShow("DttR: Crash", popup_message)) {
		DTTR_SDL_ShowSimpleMessageBox(
			SDL_MESSAGEBOX_ERROR,
			"DttR: Crash",
			popup_message,
			NULL
		);
	}

	sdsfree(popup_message);
	sdsfree(log_message);
	sdsfree(summary);
	sdsfree(filename);
	ExitProcess(1);
	return EXCEPTION_CONTINUE_SEARCH;
}

static bool set_dump_dir(const char *base_dir) {
	sds dump_dir_path = sdsnew(base_dir);
	if (!dump_dir_path
		|| !DTTR_Path_AppendSegment(&dump_dir_path, "dumps", DTTR_PATH_NATIVE_SEPARATOR)) {
		sdsfree(dump_dir_path);
		return false;
	}

	if (!CreateDirectoryA(dump_dir_path, NULL)
		&& GetLastError() != ERROR_ALREADY_EXISTS) {
		DTTR_LOG_ERROR("Failed to create crash dump directory %s", dump_dir_path);
		sdsfree(dump_dir_path);
		return false;
	}

	if (!DTTR_Path_AppendSeparator(&dump_dir_path, DTTR_PATH_NATIVE_SEPARATOR)
		|| !DTTR_Path_CopySds(crash_dump_dir, sizeof(crash_dump_dir), dump_dir_path)) {
		sdsfree(dump_dir_path);
		return false;
	}

	sdsfree(dump_dir_path);
	return true;
}

void DTTR_CrashDump_Init(const char *const dump_dir) {
	if (!set_dump_dir(dump_dir)) {
		DTTR_LOG_ERROR("Could not initialize crash dump directory");
		crash_dump_dir[0] = '\0';
	}

	SetUnhandledExceptionFilter(unhandled_exception_filter);
	DTTR_LOG_DEBUG("Crash dump handler installed");
}
