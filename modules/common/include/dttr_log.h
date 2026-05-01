#ifndef DTTR_LOG_H
#define DTTR_LOG_H

#include <log.h>
#include <stdbool.h>

#undef log_trace
#undef log_debug
#undef log_info
#undef log_warn
#undef log_error
#undef log_fatal

bool dttr_log_is_enabled(int level);
void dttr_log(int level, const char *file, int line, const char *fmt, ...);
void dttr_log_unchecked(int level, const char *file, int line, const char *fmt, ...);
void dttr_log_set_level(int level);
void dttr_log_set_quiet(bool enable);
int dttr_log_add_callback(log_LogFn fn, void *udata, int level);
int dttr_log_add_fp(FILE *fp, int level);

#define DTTR_LOG_AT(level, ...)                                                          \
	do {                                                                                 \
		if (dttr_log_is_enabled(level)) {                                                \
			dttr_log_unchecked(level, __FILE__, __LINE__, __VA_ARGS__);                  \
		}                                                                                \
	} while (0)

#define DTTR_LOG_TRACE(...) DTTR_LOG_AT(LOG_TRACE, __VA_ARGS__)
#define DTTR_LOG_DEBUG(...) DTTR_LOG_AT(LOG_DEBUG, __VA_ARGS__)
#define DTTR_LOG_INFO(...) DTTR_LOG_AT(LOG_INFO, __VA_ARGS__)
#define DTTR_LOG_WARN(...) DTTR_LOG_AT(LOG_WARN, __VA_ARGS__)
#define DTTR_LOG_ERROR(...) DTTR_LOG_AT(LOG_ERROR, __VA_ARGS__)
#define DTTR_LOG_FATAL(...) DTTR_LOG_AT(LOG_FATAL, __VA_ARGS__)

#endif // DTTR_LOG_H
