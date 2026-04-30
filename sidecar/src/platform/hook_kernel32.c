#include "imports_internal.h"

#define S_KERNEL32_IMPORTS(X)                                                            \
	X(TRACE,                                                                             \
	  dttr_import_kernel32,                                                              \
	  getcurrentdirectorya,                                                              \
	  "GetCurrentDirectoryA",                                                            \
	  "KERNEL32.dll!GetCurrentDirectoryA")                                               \
	X(TRACE,                                                                             \
	  dttr_import_kernel32,                                                              \
	  gettickcount,                                                                      \
	  "GetTickCount",                                                                    \
	  "KERNEL32.dll!GetTickCount")                                                       \
	X(TRACE,                                                                             \
	  dttr_import_kernel32,                                                              \
	  getdrivetypea,                                                                     \
	  "GetDriveTypeA",                                                                   \
	  "KERNEL32.dll!GetDriveTypeA")                                                      \
	X(TRACE, dttr_import_kernel32, writefile, "WriteFile", "KERNEL32.dll!WriteFile")     \
	X(TRACE,                                                                             \
	  dttr_import_kernel32,                                                              \
	  closehandle,                                                                       \
	  "CloseHandle",                                                                     \
	  "KERNEL32.dll!CloseHandle")                                                        \
	X(TRACE, dttr_import_kernel32, lstrcpyna, "lstrcpynA", "KERNEL32.dll!lstrcpynA")     \
	X(TRACE, dttr_import_kernel32, localalloc, "LocalAlloc", "KERNEL32.dll!LocalAlloc")  \
	X(TRACE,                                                                             \
	  dttr_import_kernel32,                                                              \
	  createfilea,                                                                       \
	  "CreateFileA",                                                                     \
	  "KERNEL32.dll!CreateFileA")                                                        \
	X(TRACE,                                                                             \
	  dttr_import_kernel32,                                                              \
	  heapdestroy,                                                                       \
	  "HeapDestroy",                                                                     \
	  "KERNEL32.dll!HeapDestroy")                                                        \
	X(TRACE, dttr_import_kernel32, heapcreate, "HeapCreate", "KERNEL32.dll!HeapCreate")  \
	X(TRACE,                                                                             \
	  dttr_import_kernel32,                                                              \
	  multibytetowidechar,                                                               \
	  "MultiByteToWideChar",                                                             \
	  "KERNEL32.dll!MultiByteToWideChar")                                                \
	X(TRACE,                                                                             \
	  dttr_import_kernel32,                                                              \
	  virtualfree,                                                                       \
	  "VirtualFree",                                                                     \
	  "KERNEL32.dll!VirtualFree")                                                        \
	X(TRACE,                                                                             \
	  dttr_import_kernel32,                                                              \
	  getstringtypew,                                                                    \
	  "GetStringTypeW",                                                                  \
	  "KERNEL32.dll!GetStringTypeW")                                                     \
	X(TRACE,                                                                             \
	  dttr_import_kernel32,                                                              \
	  setstdhandle,                                                                      \
	  "SetStdHandle",                                                                    \
	  "KERNEL32.dll!SetStdHandle")                                                       \
	X(TRACE,                                                                             \
	  dttr_import_kernel32,                                                              \
	  getstringtypea,                                                                    \
	  "GetStringTypeA",                                                                  \
	  "KERNEL32.dll!GetStringTypeA")                                                     \
	X(TRACE, dttr_import_kernel32, getacp, "GetACP", "KERNEL32.dll!GetACP")              \
	X(TRACE, dttr_import_kernel32, getoemcp, "GetOEMCP", "KERNEL32.dll!GetOEMCP")        \
	X(TRACE, dttr_import_kernel32, getcpinfo, "GetCPInfo", "KERNEL32.dll!GetCPInfo")     \
	X(TRACE,                                                                             \
	  dttr_import_kernel32,                                                              \
	  lcmapstringa,                                                                      \
	  "LCMapStringA",                                                                    \
	  "KERNEL32.dll!LCMapStringA")                                                       \
	X(TRACE,                                                                             \
	  dttr_import_kernel32,                                                              \
	  lcmapstringw,                                                                      \
	  "LCMapStringW",                                                                    \
	  "KERNEL32.dll!LCMapStringW")                                                       \
	X(TRACE,                                                                             \
	  dttr_import_kernel32,                                                              \
	  loadlibrarya,                                                                      \
	  "LoadLibraryA",                                                                    \
	  "KERNEL32.dll!LoadLibraryA")                                                       \
	X(TRACE,                                                                             \
	  dttr_import_kernel32,                                                              \
	  heaprealloc,                                                                       \
	  "HeapReAlloc",                                                                     \
	  "KERNEL32.dll!HeapReAlloc")                                                        \
	X(TRACE, dttr_import_kernel32, heapalloc, "HeapAlloc", "KERNEL32.dll!HeapAlloc")     \
	X(TRACE,                                                                             \
	  dttr_import_kernel32,                                                              \
	  getenvironmentstrings,                                                             \
	  "GetEnvironmentStrings",                                                           \
	  "KERNEL32.dll!GetEnvironmentStrings")                                              \
	X(TRACE,                                                                             \
	  dttr_import_kernel32,                                                              \
	  flushfilebuffers,                                                                  \
	  "FlushFileBuffers",                                                                \
	  "KERNEL32.dll!FlushFileBuffers")                                                   \
	X(TRACE,                                                                             \
	  dttr_import_kernel32,                                                              \
	  getenvironmentstringsw,                                                            \
	  "GetEnvironmentStringsW",                                                          \
	  "KERNEL32.dll!GetEnvironmentStringsW")                                             \
	X(TRACE,                                                                             \
	  dttr_import_kernel32,                                                              \
	  freeenvironmentstringsw,                                                           \
	  "FreeEnvironmentStringsW",                                                         \
	  "KERNEL32.dll!FreeEnvironmentStringsW")                                            \
	X(TRACE,                                                                             \
	  dttr_import_kernel32,                                                              \
	  freeenvironmentstringsa,                                                           \
	  "FreeEnvironmentStringsA",                                                         \
	  "KERNEL32.dll!FreeEnvironmentStringsA")                                            \
	X(TRACE,                                                                             \
	  dttr_import_kernel32,                                                              \
	  widechartomultibyte,                                                               \
	  "WideCharToMultiByte",                                                             \
	  "KERNEL32.dll!WideCharToMultiByte")                                                \
	X(TRACE,                                                                             \
	  dttr_import_kernel32,                                                              \
	  getlasterror,                                                                      \
	  "GetLastError",                                                                    \
	  "KERNEL32.dll!GetLastError")                                                       \
	X(TRACE, dttr_import_kernel32, rtlunwind, "RtlUnwind", "KERNEL32.dll!RtlUnwind")     \
	X(TRACE,                                                                             \
	  dttr_import_kernel32,                                                              \
	  getprocaddress,                                                                    \
	  "GetProcAddress",                                                                  \
	  "KERNEL32.dll!GetProcAddress")                                                     \
	X(TRACE, dttr_import_kernel32, getversion, "GetVersion", "KERNEL32.dll!GetVersion")  \
	X(TRACE,                                                                             \
	  dttr_import_kernel32,                                                              \
	  getcommandlinea,                                                                   \
	  "GetCommandLineA",                                                                 \
	  "KERNEL32.dll!GetCommandLineA")                                                    \
	X(TRACE,                                                                             \
	  dttr_import_kernel32,                                                              \
	  getstartupinfoa,                                                                   \
	  "GetStartupInfoA",                                                                 \
	  "KERNEL32.dll!GetStartupInfoA")                                                    \
	X(TRACE,                                                                             \
	  dttr_import_kernel32,                                                              \
	  getmodulehandlea,                                                                  \
	  "GetModuleHandleA",                                                                \
	  "KERNEL32.dll!GetModuleHandleA")                                                   \
	X(TRACE,                                                                             \
	  dttr_import_kernel32,                                                              \
	  getcurrentprocess,                                                                 \
	  "GetCurrentProcess",                                                               \
	  "KERNEL32.dll!GetCurrentProcess")                                                  \
	X(TRACE,                                                                             \
	  dttr_import_kernel32,                                                              \
	  terminateprocess,                                                                  \
	  "TerminateProcess",                                                                \
	  "KERNEL32.dll!TerminateProcess")                                                   \
	X(TRACE,                                                                             \
	  dttr_import_kernel32,                                                              \
	  exitprocess,                                                                       \
	  "ExitProcess",                                                                     \
	  "KERNEL32.dll!ExitProcess")                                                        \
	X(TRACE, dttr_import_kernel32, heapfree, "HeapFree", "KERNEL32.dll!HeapFree")        \
	X(TRACE,                                                                             \
	  dttr_import_kernel32,                                                              \
	  sethandlecount,                                                                    \
	  "SetHandleCount",                                                                  \
	  "KERNEL32.dll!SetHandleCount")                                                     \
	X(TRACE, dttr_import_kernel32, heapsize, "HeapSize", "KERNEL32.dll!HeapSize")        \
	X(TRACE,                                                                             \
	  dttr_import_kernel32,                                                              \
	  getmodulefilenamea,                                                                \
	  "GetModuleFileNameA",                                                              \
	  "KERNEL32.dll!GetModuleFileNameA")                                                 \
	X(TRACE,                                                                             \
	  dttr_import_kernel32,                                                              \
	  setendoffile,                                                                      \
	  "SetEndOfFile",                                                                    \
	  "KERNEL32.dll!SetEndOfFile")                                                       \
	X(TRACE,                                                                             \
	  dttr_import_kernel32,                                                              \
	  virtualalloc,                                                                      \
	  "VirtualAlloc",                                                                    \
	  "KERNEL32.dll!VirtualAlloc")                                                       \
	X(TRACE,                                                                             \
	  dttr_import_kernel32,                                                              \
	  unhandledexceptionfilter,                                                          \
	  "UnhandledExceptionFilter",                                                        \
	  "KERNEL32.dll!UnhandledExceptionFilter")                                           \
	X(TRACE,                                                                             \
	  dttr_import_kernel32,                                                              \
	  getstdhandle,                                                                      \
	  "GetStdHandle",                                                                    \
	  "KERNEL32.dll!GetStdHandle")                                                       \
	X(TRACE,                                                                             \
	  dttr_import_kernel32,                                                              \
	  getfiletype,                                                                       \
	  "GetFileType",                                                                     \
	  "KERNEL32.dll!GetFileType")                                                        \
	X(TRACE,                                                                             \
	  dttr_import_kernel32,                                                              \
	  setfilepointer,                                                                    \
	  "SetFilePointer",                                                                  \
	  "KERNEL32.dll!SetFilePointer")                                                     \
	X(TRACE, dttr_import_kernel32, readfile, "ReadFile", "KERNEL32.dll!ReadFile")        \
	X(TRACE, dttr_import_kernel32, searchpatha, "SearchPathA", "KERNEL32.dll!SearchPathA")

S_KERNEL32_IMPORTS(DTTR_IMPORT_ENTRY_DECL)

static const DTTR_ImportHookSpec s_kernel32_hooks[] = {
	S_KERNEL32_IMPORTS(DTTR_IMPORT_ENTRY_SPEC)
};

void dttr_platform_hooks_kernel32_init(const DTTR_ComponentContext *ctx) {
	dttr_platform_hooks_install_module(
		ctx,
		"KERNEL32.dll",
		s_kernel32_hooks,
		DTTR_IMPORT_ARRAY_COUNT(s_kernel32_hooks)
	);
}

void dttr_platform_hooks_kernel32_cleanup(const DTTR_ComponentContext *ctx) {
	dttr_platform_hooks_cleanup_module(
		ctx,
		s_kernel32_hooks,
		DTTR_IMPORT_ARRAY_COUNT(s_kernel32_hooks)
	);
}

#undef S_KERNEL32_IMPORTS
