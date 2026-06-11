#include "crash_private.h"

#include <string.h>

#include <dbghelp.h>

#include <dttr_crashdump.h>
#include <dttr_pcdogs.h>
#include <sds.h>

#include "sidecar_private.h"

// Copies an sds into fixed report storage and releases the source.
static bool dttr_sidecar_store_sds(char *dst, size_t dst_size, sds src) {
	if (!src) {
		return false;
	}

	if (dst_size > 0) {
		size_t copy_len = sdslen(src);
		if (copy_len >= dst_size) {
			copy_len = dst_size - 1u;
		}
		memcpy(dst, src, copy_len);
		dst[copy_len] = '\0';
	}

	sdsfree(src);
	return true;
}

// Writes a crash dump and stack trace for the DTTR_Mods_API exception report entry.
bool dttr_sidecar_write_exception_report(
	const DTTR_Mods_ExceptionReportRequest *request,
	DTTR_Mods_ExceptionReport *report
) {
	if (!request || !report
		|| request->struct_size != sizeof(DTTR_Mods_ExceptionReportRequest)
		|| report->struct_size != sizeof(DTTR_Mods_ExceptionReport)
		|| request->exception_record.ExceptionCode == 0) {
		return false;
	}

	memset(report, 0, sizeof(*report));
	report->struct_size = sizeof(*report);

	EXCEPTION_RECORD exception_record = request->exception_record;
	CONTEXT context = request->context;
	EXCEPTION_POINTERS exception_pointers = {
		.ExceptionRecord = &exception_record,
		.ContextRecord = &context,
	};
	const DWORD current_thread_id = GetCurrentThreadId();
	const DWORD thread_id = request->thread_id ? request->thread_id : current_thread_id;

	sds dump_path = DTTR_CrashDump_Write(
		GetCurrentProcess(),
		GetCurrentProcessId(),
		thread_id,
		&exception_pointers
	);
	report->dump_written = dttr_sidecar_store_sds(
		report->dump_path,
		sizeof(report->dump_path),
		dump_path
	);
	if (!report->dump_written) {
		report->win32_error = GetLastError();
	}

	HANDLE thread = GetCurrentThread();
	bool close_thread = false;
	if (thread_id != current_thread_id) {
		thread = OpenThread(
			THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION,
			FALSE,
			thread_id
		);
		close_thread = thread != NULL;
		if (!thread && report->win32_error == ERROR_SUCCESS) {
			report->win32_error = GetLastError();
		}
	}

	sds stack_trace = DTTR_CrashDump_FormatStackTrace(
		GetCurrentProcess(),
		thread,
		&context
	);
	if (close_thread) {
		CloseHandle(thread);
	}

	report->stack_trace_written = dttr_sidecar_store_sds(
		report->stack_trace,
		sizeof(report->stack_trace),
		stack_trace
	);

	return report->dump_written || report->stack_trace_written;
}

static bool pcdogs_crash_symbol_should_add(
	bool resolved,
	uintptr_t address,
	const char *name,
	uintptr_t module_base,
	uint32_t image_size
) {
	if (!resolved || !name || !name[0] || image_size == 0) {
		return false;
	}

	const uintptr_t image_end = module_base + image_size;
	if (image_end < module_base) {
		return false;
	}

	return address >= module_base && address < image_end;
}

static bool pcdogs_module_image_size(HMODULE module, DWORD *out_image_size) {
	if (!module || !out_image_size) {
		return false;
	}

	const uint8_t *base = (const uint8_t *)module;
	const IMAGE_DOS_HEADER *dos = (const IMAGE_DOS_HEADER *)base;
	if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew <= 0) {
		return false;
	}

	const IMAGE_NT_HEADERS *nt = (const IMAGE_NT_HEADERS *)(base + dos->e_lfanew);
	if (nt->Signature != IMAGE_NT_SIGNATURE) {
		return false;
	}

	*out_image_size = nt->OptionalHeader.SizeOfImage;
	return *out_image_size != 0;
}

static bool ensure_pcdogs_dbghelp_module(HANDLE process, HMODULE module, DWORD image_size) {
	const DWORD base = (DWORD)(uintptr_t)module;
	IMAGEHLP_MODULE module_info = {.SizeOfStruct = sizeof(module_info)};
	if (SymGetModuleInfo(process, base, &module_info)) {
		return true;
	}

	char module_path[MAX_PATH] = {0};
	const char *image_name = NULL;
	if (GetModuleFileNameA(module, module_path, sizeof(module_path)) != 0) {
		image_name = module_path;
	}

	const DWORD loaded_base = SymLoadModuleEx(
		process,
		NULL,
		image_name,
		"pcdogs",
		base,
		image_size,
		NULL,
		SLMFLAG_VIRTUAL
	);
	if (loaded_base != 0) {
		return true;
	}

	const DWORD error = GetLastError();
	return error == ERROR_SUCCESS;
}

static bool dttr_pcdogs_crash_symbol_provider(HANDLE process, void *context) {
	const DTTR_Core_Context *runtime = (const DTTR_Core_Context *)context;
	if (!process || !runtime || !runtime->game_module) {
		return false;
	}

	const HMODULE module = runtime->game_module;
	DWORD image_size = 0;
	if (!pcdogs_module_image_size(module, &image_size)) {
		return false;
	}

	if (!ensure_pcdogs_dbghelp_module(process, module, image_size)) {
		return false;
	}

	const uintptr_t module_base = (uintptr_t)module;
	bool added_any = false;
	for (uint32_t i = 0; i < DTTR_PCDOGS_SymbolFunctionCount(); i++) {
		const DTTR_PCDOGS_T_Symbol_Function *fn = DTTR_PCDOGS_SymbolFunctionAt(i);
		const char *name = DTTR_PCDOGS_SymbolFunctionNameAt(i);
		if (!pcdogs_crash_symbol_should_add(
				fn->resolved,
				fn->address,
				name,
				module_base,
				image_size
			)) {
			continue;
		}

		if (SymAddSymbol(process, (DWORD)module_base, name, (DWORD)fn->address, 0, 0)) {
			added_any = true;
		}
	}

	return added_any;
}

void dttr_pcdogs_crash_symbols_register(const DTTR_Core_Context *runtime) {
	if (!runtime || !runtime->game_module) {
		dttr_pcdogs_crash_symbols_clear();
		return;
	}

	DTTR_CrashDump_SetSymbolProvider(dttr_pcdogs_crash_symbol_provider, (void *)runtime);
}

void dttr_pcdogs_crash_symbols_clear() {
	DTTR_CrashDump_ClearSymbolProvider();
}
