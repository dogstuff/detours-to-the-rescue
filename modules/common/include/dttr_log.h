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

bool DTTR_Log_IsEnabled(int level);
void DTTR_Log(int level, const char *file, int line, const char *fmt, ...);
void DTTR_Log_Unchecked(int level, const char *file, int line, const char *fmt, ...);
void DTTR_Log_SetLevel(int level);
int DTTR_Log_AddFP(FILE *fp, int level);

#define DTTR_LOG_AT(level, ...)                                                          \
	do {                                                                                 \
		if (DTTR_Log_IsEnabled(level)) {                                                 \
			DTTR_Log_Unchecked(level, __FILE__, __LINE__, __VA_ARGS__);                  \
		}                                                                                \
	} while (0)

#define DTTR_LOG_TRACE(...) DTTR_LOG_AT(LOG_TRACE, __VA_ARGS__)
#define DTTR_LOG_DEBUG(...) DTTR_LOG_AT(LOG_DEBUG, __VA_ARGS__)
#define DTTR_LOG_INFO(...) DTTR_LOG_AT(LOG_INFO, __VA_ARGS__)
#define DTTR_LOG_WARN(...) DTTR_LOG_AT(LOG_WARN, __VA_ARGS__)
#define DTTR_LOG_ERROR(...) DTTR_LOG_AT(LOG_ERROR, __VA_ARGS__)
#define DTTR_LOG_FATAL(...) DTTR_LOG_AT(LOG_FATAL, __VA_ARGS__)

#endif // DTTR_LOG_H
