#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>

typedef int (*S_LauncherMain)(int argc, char **argv);

static const char *const S_LAUNCHER_DLL_RELATIVE_PATH = "modules\\libdttr_launcher.dll";

static void s_get_exe_dir(char *buf, size_t buf_size) {
	GetModuleFileNameA(NULL, buf, (DWORD)buf_size);
	char *last_sep = strrchr(buf, '\\');
	if (last_sep) {
		last_sep[1] = '\0';
	}
}

static bool s_launcher_dll_path(char *out, size_t out_size) {
	s_get_exe_dir(out, out_size);
	const size_t len = strlen(out);
	if (len >= out_size) {
		return false;
	}

	const int written = snprintf(
		out + len,
		out_size - len,
		"%s",
		S_LAUNCHER_DLL_RELATIVE_PATH
	);
	return written > 0 && (size_t)written < out_size - len;
}

static void s_show_startup_error(const char *message, DWORD error) {
	char detail[512];
	char text[768];

	if (error
		&& FormatMessageA(
			FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			error,
			0,
			detail,
			sizeof(detail),
			NULL
		)
		&& detail[0]) {
		snprintf(text, sizeof(text), "%s\n\nError %lu: %s", message, error, detail);
	} else {
		snprintf(text, sizeof(text), "%s", message);
	}

	MessageBoxA(NULL, text, "DttR: Error", MB_OK | MB_ICONERROR);
}

int main(int argc, char *argv[]) {
	char path[MAX_PATH];
	if (!s_launcher_dll_path(path, sizeof(path))) {
		s_show_startup_error("Could not resolve the DttR launcher module path.", 0);
		return 1;
	}

	HMODULE module = LoadLibraryExA(path, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
	if (!module) {
		s_show_startup_error(
			"Could not load modules\\libdttr_launcher.dll.",
			GetLastError()
		);
		return 1;
	}

	S_LauncherMain launcher_main = (S_LauncherMain)
		GetProcAddress(module, "dttr_launcher_main");
	if (!launcher_main) {
		s_show_startup_error(
			"Could not find the DttR launcher entry point.",
			GetLastError()
		);
		FreeLibrary(module);
		return 1;
	}

	return launcher_main(argc, argv);
}
