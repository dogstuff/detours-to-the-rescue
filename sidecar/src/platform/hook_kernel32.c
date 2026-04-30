#include "imports_internal.h"

DTTR_IMPORT_HOOK_DECL(
	dttr_import_kernel32,
	getcurrentdirectorya,
	"KERNEL32.dll!GetCurrentDirectoryA"
)
DTTR_IMPORT_HOOK_DECL(dttr_import_kernel32, gettickcount, "KERNEL32.dll!GetTickCount")
DTTR_IMPORT_HOOK_DECL(dttr_import_kernel32, getdrivetypea, "KERNEL32.dll!GetDriveTypeA")
DTTR_IMPORT_HOOK_DECL(dttr_import_kernel32, writefile, "KERNEL32.dll!WriteFile")
DTTR_IMPORT_HOOK_DECL(dttr_import_kernel32, closehandle, "KERNEL32.dll!CloseHandle")
DTTR_IMPORT_HOOK_DECL(dttr_import_kernel32, lstrcpyna, "KERNEL32.dll!lstrcpynA")
DTTR_IMPORT_HOOK_DECL(dttr_import_kernel32, localalloc, "KERNEL32.dll!LocalAlloc")
DTTR_IMPORT_HOOK_DECL(dttr_import_kernel32, createfilea, "KERNEL32.dll!CreateFileA")
DTTR_IMPORT_HOOK_DECL(dttr_import_kernel32, heapdestroy, "KERNEL32.dll!HeapDestroy")
DTTR_IMPORT_HOOK_DECL(dttr_import_kernel32, heapcreate, "KERNEL32.dll!HeapCreate")
DTTR_IMPORT_HOOK_DECL(
	dttr_import_kernel32,
	multibytetowidechar,
	"KERNEL32.dll!MultiByteToWideChar"
)
DTTR_IMPORT_HOOK_DECL(dttr_import_kernel32, virtualfree, "KERNEL32.dll!VirtualFree")
DTTR_IMPORT_HOOK_DECL(dttr_import_kernel32, getstringtypew, "KERNEL32.dll!GetStringTypeW")
DTTR_IMPORT_HOOK_DECL(dttr_import_kernel32, setstdhandle, "KERNEL32.dll!SetStdHandle")
DTTR_IMPORT_HOOK_DECL(dttr_import_kernel32, getstringtypea, "KERNEL32.dll!GetStringTypeA")
DTTR_IMPORT_HOOK_DECL(dttr_import_kernel32, getacp, "KERNEL32.dll!GetACP")
DTTR_IMPORT_HOOK_DECL(dttr_import_kernel32, getoemcp, "KERNEL32.dll!GetOEMCP")
DTTR_IMPORT_HOOK_DECL(dttr_import_kernel32, getcpinfo, "KERNEL32.dll!GetCPInfo")
DTTR_IMPORT_HOOK_DECL(dttr_import_kernel32, lcmapstringa, "KERNEL32.dll!LCMapStringA")
DTTR_IMPORT_HOOK_DECL(dttr_import_kernel32, lcmapstringw, "KERNEL32.dll!LCMapStringW")
DTTR_IMPORT_HOOK_DECL(dttr_import_kernel32, loadlibrarya, "KERNEL32.dll!LoadLibraryA")
DTTR_IMPORT_HOOK_DECL(dttr_import_kernel32, heaprealloc, "KERNEL32.dll!HeapReAlloc")
DTTR_IMPORT_HOOK_DECL(dttr_import_kernel32, heapalloc, "KERNEL32.dll!HeapAlloc")
DTTR_IMPORT_HOOK_DECL(
	dttr_import_kernel32,
	getenvironmentstrings,
	"KERNEL32.dll!GetEnvironmentStrings"
)
DTTR_IMPORT_HOOK_DECL(
	dttr_import_kernel32,
	flushfilebuffers,
	"KERNEL32.dll!FlushFileBuffers"
)
DTTR_IMPORT_HOOK_DECL(
	dttr_import_kernel32,
	getenvironmentstringsw,
	"KERNEL32.dll!GetEnvironmentStringsW"
)
DTTR_IMPORT_HOOK_DECL(
	dttr_import_kernel32,
	freeenvironmentstringsw,
	"KERNEL32.dll!FreeEnvironmentStringsW"
)
DTTR_IMPORT_HOOK_DECL(
	dttr_import_kernel32,
	freeenvironmentstringsa,
	"KERNEL32.dll!FreeEnvironmentStringsA"
)
DTTR_IMPORT_HOOK_DECL(
	dttr_import_kernel32,
	widechartomultibyte,
	"KERNEL32.dll!WideCharToMultiByte"
)
DTTR_IMPORT_HOOK_DECL(dttr_import_kernel32, getlasterror, "KERNEL32.dll!GetLastError")
DTTR_IMPORT_HOOK_DECL(dttr_import_kernel32, rtlunwind, "KERNEL32.dll!RtlUnwind")
DTTR_IMPORT_HOOK_DECL(dttr_import_kernel32, getprocaddress, "KERNEL32.dll!GetProcAddress")
DTTR_IMPORT_HOOK_DECL(dttr_import_kernel32, getversion, "KERNEL32.dll!GetVersion")
DTTR_IMPORT_HOOK_DECL(
	dttr_import_kernel32,
	getcommandlinea,
	"KERNEL32.dll!GetCommandLineA"
)
DTTR_IMPORT_HOOK_DECL(
	dttr_import_kernel32,
	getstartupinfoa,
	"KERNEL32.dll!GetStartupInfoA"
)
DTTR_IMPORT_HOOK_DECL(
	dttr_import_kernel32,
	getmodulehandlea,
	"KERNEL32.dll!GetModuleHandleA"
)
DTTR_IMPORT_HOOK_DECL(
	dttr_import_kernel32,
	getcurrentprocess,
	"KERNEL32.dll!GetCurrentProcess"
)
DTTR_IMPORT_HOOK_DECL(
	dttr_import_kernel32,
	terminateprocess,
	"KERNEL32.dll!TerminateProcess"
)
DTTR_IMPORT_HOOK_DECL(dttr_import_kernel32, exitprocess, "KERNEL32.dll!ExitProcess")
DTTR_IMPORT_HOOK_DECL(dttr_import_kernel32, heapfree, "KERNEL32.dll!HeapFree")
DTTR_IMPORT_HOOK_DECL(dttr_import_kernel32, sethandlecount, "KERNEL32.dll!SetHandleCount")
DTTR_IMPORT_HOOK_DECL(dttr_import_kernel32, heapsize, "KERNEL32.dll!HeapSize")
DTTR_IMPORT_HOOK_DECL(
	dttr_import_kernel32,
	getmodulefilenamea,
	"KERNEL32.dll!GetModuleFileNameA"
)
DTTR_IMPORT_HOOK_DECL(dttr_import_kernel32, setendoffile, "KERNEL32.dll!SetEndOfFile")
DTTR_IMPORT_HOOK_DECL(dttr_import_kernel32, virtualalloc, "KERNEL32.dll!VirtualAlloc")
DTTR_IMPORT_HOOK_DECL(
	dttr_import_kernel32,
	unhandledexceptionfilter,
	"KERNEL32.dll!UnhandledExceptionFilter"
)
DTTR_IMPORT_HOOK_DECL(dttr_import_kernel32, getstdhandle, "KERNEL32.dll!GetStdHandle")
DTTR_IMPORT_HOOK_DECL(dttr_import_kernel32, getfiletype, "KERNEL32.dll!GetFileType")
DTTR_IMPORT_HOOK_DECL(dttr_import_kernel32, setfilepointer, "KERNEL32.dll!SetFilePointer")
DTTR_IMPORT_HOOK_DECL(dttr_import_kernel32, readfile, "KERNEL32.dll!ReadFile")
DTTR_IMPORT_HOOK_DECL(dttr_import_kernel32, searchpatha, "KERNEL32.dll!SearchPathA")

static const DTTR_ImportHookSpec s_kernel32_hooks[] = {
	DTTR_IMPORT_HOOK_SPEC(
		dttr_import_kernel32,
		getcurrentdirectorya,
		"GetCurrentDirectoryA"
	),
	DTTR_IMPORT_HOOK_SPEC(dttr_import_kernel32, gettickcount, "GetTickCount"),
	DTTR_IMPORT_HOOK_SPEC(dttr_import_kernel32, getdrivetypea, "GetDriveTypeA"),
	DTTR_IMPORT_HOOK_SPEC(dttr_import_kernel32, writefile, "WriteFile"),
	DTTR_IMPORT_HOOK_SPEC(dttr_import_kernel32, closehandle, "CloseHandle"),
	DTTR_IMPORT_HOOK_SPEC(dttr_import_kernel32, lstrcpyna, "lstrcpynA"),
	DTTR_IMPORT_HOOK_SPEC(dttr_import_kernel32, localalloc, "LocalAlloc"),
	DTTR_IMPORT_HOOK_SPEC(dttr_import_kernel32, createfilea, "CreateFileA"),
	DTTR_IMPORT_HOOK_SPEC(dttr_import_kernel32, heapdestroy, "HeapDestroy"),
	DTTR_IMPORT_HOOK_SPEC(dttr_import_kernel32, heapcreate, "HeapCreate"),
	DTTR_IMPORT_HOOK_SPEC(
		dttr_import_kernel32,
		multibytetowidechar,
		"MultiByteToWideChar"
	),
	DTTR_IMPORT_HOOK_SPEC(dttr_import_kernel32, virtualfree, "VirtualFree"),
	DTTR_IMPORT_HOOK_SPEC(dttr_import_kernel32, getstringtypew, "GetStringTypeW"),
	DTTR_IMPORT_HOOK_SPEC(dttr_import_kernel32, setstdhandle, "SetStdHandle"),
	DTTR_IMPORT_HOOK_SPEC(dttr_import_kernel32, getstringtypea, "GetStringTypeA"),
	DTTR_IMPORT_HOOK_SPEC(dttr_import_kernel32, getacp, "GetACP"),
	DTTR_IMPORT_HOOK_SPEC(dttr_import_kernel32, getoemcp, "GetOEMCP"),
	DTTR_IMPORT_HOOK_SPEC(dttr_import_kernel32, getcpinfo, "GetCPInfo"),
	DTTR_IMPORT_HOOK_SPEC(dttr_import_kernel32, lcmapstringa, "LCMapStringA"),
	DTTR_IMPORT_HOOK_SPEC(dttr_import_kernel32, lcmapstringw, "LCMapStringW"),
	DTTR_IMPORT_HOOK_SPEC(dttr_import_kernel32, loadlibrarya, "LoadLibraryA"),
	DTTR_IMPORT_HOOK_SPEC(dttr_import_kernel32, heaprealloc, "HeapReAlloc"),
	DTTR_IMPORT_HOOK_SPEC(dttr_import_kernel32, heapalloc, "HeapAlloc"),
	DTTR_IMPORT_HOOK_SPEC(
		dttr_import_kernel32,
		getenvironmentstrings,
		"GetEnvironmentStrings"
	),
	DTTR_IMPORT_HOOK_SPEC(dttr_import_kernel32, flushfilebuffers, "FlushFileBuffers"),
	DTTR_IMPORT_HOOK_SPEC(
		dttr_import_kernel32,
		getenvironmentstringsw,
		"GetEnvironmentStringsW"
	),
	DTTR_IMPORT_HOOK_SPEC(
		dttr_import_kernel32,
		freeenvironmentstringsw,
		"FreeEnvironmentStringsW"
	),
	DTTR_IMPORT_HOOK_SPEC(
		dttr_import_kernel32,
		freeenvironmentstringsa,
		"FreeEnvironmentStringsA"
	),
	DTTR_IMPORT_HOOK_SPEC(
		dttr_import_kernel32,
		widechartomultibyte,
		"WideCharToMultiByte"
	),
	DTTR_IMPORT_HOOK_SPEC(dttr_import_kernel32, getlasterror, "GetLastError"),
	DTTR_IMPORT_HOOK_SPEC(dttr_import_kernel32, rtlunwind, "RtlUnwind"),
	DTTR_IMPORT_HOOK_SPEC(dttr_import_kernel32, getprocaddress, "GetProcAddress"),
	DTTR_IMPORT_HOOK_SPEC(dttr_import_kernel32, getversion, "GetVersion"),
	DTTR_IMPORT_HOOK_SPEC(dttr_import_kernel32, getcommandlinea, "GetCommandLineA"),
	DTTR_IMPORT_HOOK_SPEC(dttr_import_kernel32, getstartupinfoa, "GetStartupInfoA"),
	DTTR_IMPORT_HOOK_SPEC(dttr_import_kernel32, getmodulehandlea, "GetModuleHandleA"),
	DTTR_IMPORT_HOOK_SPEC(dttr_import_kernel32, getcurrentprocess, "GetCurrentProcess"),
	DTTR_IMPORT_HOOK_SPEC(dttr_import_kernel32, terminateprocess, "TerminateProcess"),
	DTTR_IMPORT_HOOK_SPEC(dttr_import_kernel32, exitprocess, "ExitProcess"),
	DTTR_IMPORT_HOOK_SPEC(dttr_import_kernel32, heapfree, "HeapFree"),
	DTTR_IMPORT_HOOK_SPEC(dttr_import_kernel32, sethandlecount, "SetHandleCount"),
	DTTR_IMPORT_HOOK_SPEC(dttr_import_kernel32, heapsize, "HeapSize"),
	DTTR_IMPORT_HOOK_SPEC(dttr_import_kernel32, getmodulefilenamea, "GetModuleFileNameA"),
	DTTR_IMPORT_HOOK_SPEC(dttr_import_kernel32, setendoffile, "SetEndOfFile"),
	DTTR_IMPORT_HOOK_SPEC(dttr_import_kernel32, virtualalloc, "VirtualAlloc"),
	DTTR_IMPORT_HOOK_SPEC(
		dttr_import_kernel32,
		unhandledexceptionfilter,
		"UnhandledExceptionFilter"
	),
	DTTR_IMPORT_HOOK_SPEC(dttr_import_kernel32, getstdhandle, "GetStdHandle"),
	DTTR_IMPORT_HOOK_SPEC(dttr_import_kernel32, getfiletype, "GetFileType"),
	DTTR_IMPORT_HOOK_SPEC(dttr_import_kernel32, setfilepointer, "SetFilePointer"),
	DTTR_IMPORT_HOOK_SPEC(dttr_import_kernel32, readfile, "ReadFile"),
	DTTR_IMPORT_HOOK_SPEC(dttr_import_kernel32, searchpatha, "SearchPathA")
};

void dttr_platform_hooks_kernel32_init(const DTTR_ComponentContext *ctx) {
	dttr_platform_hooks_install_module(
		ctx,
		"KERNEL32.dll",
		s_kernel32_hooks,
		sizeof(s_kernel32_hooks) / sizeof(s_kernel32_hooks[0])
	);
}

void dttr_platform_hooks_kernel32_cleanup(const DTTR_ComponentContext *ctx) {
	dttr_platform_hooks_cleanup_module(
		ctx,
		s_kernel32_hooks,
		sizeof(s_kernel32_hooks) / sizeof(s_kernel32_hooks[0])
	);
}
