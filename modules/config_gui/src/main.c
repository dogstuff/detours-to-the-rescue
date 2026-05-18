#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>

typedef int (*config_main_fn)(int argc, char **argv);

static const char *const CONFIG_DLL_RELATIVE_PATH = "modules\\libdttr_config.dll";

static bool get_exe_dir(char *buf, size_t buf_size) {
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

static bool config_dll_path(char *out, size_t out_size) {
	if (!get_exe_dir(out, out_size)) {
		return false;
	}

	const size_t len = strlen(out);
	const size_t remaining = out_size - len;
	const int written = snprintf(out + len, remaining, "%s", CONFIG_DLL_RELATIVE_PATH);
	return written > 0 && (size_t)written < remaining;
}

static void show_startup_error(const char *message, DWORD error) {
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

	MessageBoxA(NULL, text, "DttR Config: Error", MB_OK | MB_ICONERROR);
}

static config_main_fn resolve_config_main(HMODULE module) {
	return (config_main_fn)GetProcAddress(module, "dttr_config_main");
}

int main(int argc, char *argv[]) {
	char path[MAX_PATH];
	if (!config_dll_path(path, sizeof(path))) {
		show_startup_error("Could not resolve the DttR config module path.", 0);
		return 1;
	}

	HMODULE module = LoadLibraryExA(path, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
	if (!module) {
		show_startup_error("Could not load modules\\libdttr_config.dll.", GetLastError());
		return 1;
	}

	config_main_fn config_main = resolve_config_main(module);
	if (!config_main) {
		show_startup_error("Could not find the DttR config entry point.", GetLastError());
		FreeLibrary(module);
		return 1;
	}

	const int result = config_main(argc, argv);
	FreeLibrary(module);
	return result;
}
