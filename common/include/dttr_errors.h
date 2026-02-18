#ifndef DTTR_ERRORS_H
#define DTTR_ERRORS_H

#include <stdlib.h>

#include <SDL3/SDL.h>
#include <log.h>
#include <sds.h>
#include <windows.h>

#ifndef typeof
#define typeof __typeof__
#endif

#define DTTR_ERROR(error_message, ...) \
	do { \
		sds _err_msg = sdscatprintf(sdsempty(), error_message, ##__VA_ARGS__); \
		log_error("%s", _err_msg); \
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "DttR: Error", _err_msg, NULL); \
		sdsfree(_err_msg); \
	} while (0)

#define DTTR_REPORT_URL \
	"https://gitlab.com/dogstuff/detours-to-the-rescue/-/issues/new?issuable_template=Crash%20Report"

#define DTTR_REPORT_SUFFIX \
	"\n\nIf this error is unexpected, please report it at:\n" DTTR_REPORT_URL

#define DTTR_FATAL(error_message, ...) \
	do { \
		sds _err_msg = sdscatprintf(sdsempty(), error_message, ##__VA_ARGS__); \
		_err_msg = sdscat(_err_msg, DTTR_REPORT_SUFFIX); \
		log_error("%s", _err_msg); \
		SDL_ShowSimpleMessageBox( \
			SDL_MESSAGEBOX_ERROR, "DttR: Fatal Error", _err_msg, NULL \
		); \
		sdsfree(_err_msg); \
		exit(EXIT_FAILURE); \
	} while (0)

#define DTTR_UNWRAP_WINAPI() \
	do { \
		DWORD error_code = GetLastError(); \
		if (error_code == ERROR_SUCCESS) { \
			break; \
		} \
		LPSTR message = NULL; \
		FormatMessageA( \
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | \
				FORMAT_MESSAGE_IGNORE_INSERTS, \
			NULL, \
			error_code, \
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), \
			(LPSTR) & message, \
			0, \
			NULL \
		); \
		if (message == NULL) { \
			message = "unknown"; \
		} \
		DTTR_FATAL("Win32 API Error 0x%lX: %s", error_code, message); \
	} while (0)

#define DTTR_UNWRAP_WINAPI_IF(result, is_error) \
	__extension__({ \
		typeof(result) _r = (result); \
		if (is_error(_r)) { \
			DTTR_UNWRAP_WINAPI(); \
		} \
		_r; \
	})

// Returns true when the API result indicates failure
#define DTTR_UNWRAP_WINAPI_NONNEGATIVE_IS_ERROR(x) ((x) < 0)
#define DTTR_UNWRAP_WINAPI_NONNEGATIVE(result) \
	DTTR_UNWRAP_WINAPI_IF(result, DTTR_UNWRAP_WINAPI_NONNEGATIVE_IS_ERROR)

#define DTTR_UNWRAP_WINAPI_NONZERO_IS_ERROR(x) ((x) == FALSE)
#define DTTR_UNWRAP_WINAPI_NONZERO(result) \
	DTTR_UNWRAP_WINAPI_IF(result, DTTR_UNWRAP_WINAPI_NONZERO_IS_ERROR)

#define DTTR_UNWRAP_WINAPI_EXISTS_IS_ERROR(x) ((x) == NULL)
#define DTTR_UNWRAP_WINAPI_EXISTS(result) \
	DTTR_UNWRAP_WINAPI_IF(result, DTTR_UNWRAP_WINAPI_EXISTS_IS_ERROR)

#endif // DTTR_ERRORS_H
