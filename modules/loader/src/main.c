#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>

typedef int (*S_LauncherMainFn)(int argc, char **argv);

static const char *const S_LAUNCHER_DLL_RELATIVE_PATH = "modules\\libdttr_launcher.dll";

static bool s_get_exe_dir(char *buf, size_t buf_size) {
	if (buf_size == 0) {
		return false;
	}

	const DWORD len = GetModuleFileNameA(NULL, buf, (DWORD)buf_size);
	if (len == 0 || len >= buf_size) {
		buf[0] = '\0';
		return false;
	}

	char *last_sep = strrchr(buf, '\\');
	if (!last_sep) {
		buf[0] = '\0';
		return false;
	}

	last_sep[1] = '\0';
	return true;
}

static bool s_launcher_dll_path(char *out, size_t out_size) {
	if (!s_get_exe_dir(out, out_size)) {
		return false;
	}

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

static S_LauncherMainFn s_resolve_launcher_main(HMODULE module) {
	return (S_LauncherMainFn)GetProcAddress(module, "dttr_launcher_main");
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

	S_LauncherMainFn launcher_main = s_resolve_launcher_main(module);
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
