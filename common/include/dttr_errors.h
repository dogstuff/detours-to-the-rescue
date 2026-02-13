#ifndef DTTR_ERRORS_H
#define DTTR_ERRORS_H

#include <stdlib.h>

#include <log.h>
#include <sds.h>
#include <windows.h>

#ifndef typeof
#define typeof __typeof__
#endif

#define DTTR_FATAL(error_message, ...) \
	do { \
		sds _err_msg = sdscatprintf(sdsempty(), error_message, ##__VA_ARGS__); \
		log_error("%s", _err_msg); \
		MessageBoxA(NULL, _err_msg, "DttR Error", MB_OK | MB_ICONERROR); \
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
