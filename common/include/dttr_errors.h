#ifndef DTTR_ERRORS_H
#define DTTR_ERRORS_H

#include <stdarg.h>
#include <stdlib.h>

#include <log.h>
#include <sds.h>
#include <windows.h>

#ifndef typeof
#define typeof __typeof__
#endif

static inline void s_raise_error(const char *error_message, ...) {
	va_list args;
	va_start(args, error_message);

	sds message = sdscatvprintf(sdsempty(), error_message, args);
	log_error("%s", message);
	sdsfree(message);

	va_end(args);
}

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
		s_raise_error("Win32 API Error 0x%X: %s", error_code, message); \
		LocalFree(message); \
		exit(error_code); \
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
