#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>

typedef int (*S_ConfigMainFn)(int argc, char **argv);

static const char *const S_CONFIG_DLL_RELATIVE_PATH = "modules\\libdttr_config.dll";

static void s_clear_path(char *buf) { buf[0] = '\0'; }

static bool s_get_exe_dir(char *buf, size_t buf_size) {
	if (buf_size == 0) {
		return false;
	}

	const DWORD len = GetModuleFileNameA(NULL, buf, (DWORD)buf_size);
	if (len == 0 || len >= buf_size) {
		s_clear_path(buf);
		return false;
	}

	char *last_sep = strrchr(buf, '\\');
	if (!last_sep) {
		s_clear_path(buf);
		return false;
	}

	last_sep[1] = '\0';
	return true;
}

static bool s_config_dll_path(char *out, size_t out_size) {
	if (!s_get_exe_dir(out, out_size)) {
		return false;
	}

	const size_t len = strlen(out);
	const size_t remaining = out_size - len;
	const int written = snprintf(out + len, remaining, "%s", S_CONFIG_DLL_RELATIVE_PATH);
	return written > 0 && (size_t)written < remaining;
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

	MessageBoxA(NULL, text, "DttR Config: Error", MB_OK | MB_ICONERROR);
}

static HMODULE s_load_config_module(char *path, size_t path_size) {
	if (!s_config_dll_path(path, path_size)) {
		s_show_startup_error("Could not resolve the DttR config module path.", 0);
		return NULL;
	}

	HMODULE module = LoadLibraryExA(path, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
	if (!module) {
		s_show_startup_error(
			"Could not load modules\\libdttr_config.dll.",
			GetLastError()
		);
		return NULL;
	}

	return module;
}

static S_ConfigMainFn s_load_config_main(HMODULE module) {
	S_ConfigMainFn config_main = (S_ConfigMainFn)
		GetProcAddress(module, "dttr_config_main");
	if (!config_main) {
		s_show_startup_error(
			"Could not find the DttR config entry point.",
			GetLastError()
		);
	}

	return config_main;
}

int main(int argc, char *argv[]) {
	char path[MAX_PATH];
	HMODULE module = s_load_config_module(path, sizeof(path));
	if (!module) {
		return 1;
	}

	S_ConfigMainFn config_main = s_load_config_main(module);
	if (!config_main) {
		FreeLibrary(module);
		return 1;
	}

	const int result = config_main(argc, argv);
	FreeLibrary(module);
	return result;
}
