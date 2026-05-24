#include <dttr_log.h>

#include <stdarg.h>
#include <stdio.h>
#include <time.h>

#define MAX_CALLBACKS 32

typedef struct {
	log_LogFn fn;
	void *udata;
	int level;
} log_callback;

static int s_level = LOG_TRACE;
static bool quiet;
static log_callback callbacks[MAX_CALLBACKS];
static int callback_count;

static const char *level_to_string(int level) {
	switch (level) {
	case LOG_TRACE:
		return "TRACE";
	case LOG_DEBUG:
		return "DEBUG";
	case LOG_INFO:
		return "INFO";
	case LOG_WARN:
		return "WARN";
	case LOG_ERROR:
		return "ERROR";
	case LOG_FATAL:
		return "FATAL";
	default:
		return "UNKNOWN";
	}
}

static void init_event(log_Event *ev, void *udata) {
	if (!ev->time) {
		time_t t = time(NULL);
		ev->time = localtime(&t);
	}

	ev->udata = udata;
}

static void stderr_callback(log_Event *ev) {
	char buf[16];
	buf[strftime(buf, sizeof(buf), "%H:%M:%S", ev->time)] = '\0';
	fprintf(
		ev->udata,
		"%s %-5s %s:%d: ",
		buf,
		level_to_string(ev->level),
		ev->file,
		ev->line
	);
	vfprintf(ev->udata, ev->fmt, ev->ap);
	fprintf(ev->udata, "\n");
	fflush(ev->udata);
}

static void file_callback(log_Event *ev) {
	char buf[64];
	buf[strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", ev->time)] = '\0';
	fprintf(
		ev->udata,
		"%s %-5s %s:%d: ",
		buf,
		level_to_string(ev->level),
		ev->file,
		ev->line
	);
	vfprintf(ev->udata, ev->fmt, ev->ap);
	fprintf(ev->udata, "\n");
	fflush(ev->udata);
}

bool DTTR_Log_IsEnabled(int log_level) {
	if (!quiet && log_level >= s_level) {
		return true;
	}

	for (int i = 0; i < callback_count; i++) {
		if (log_level >= callbacks[i].level) {
			return true;
		}
	}

	return false;
}

static void dttr_vlog_unchecked(
	int log_level,
	const char *file,
	int line,
	const char *fmt,
	va_list args
) {
	log_Event ev = {
		.fmt = fmt,
		.file = file,
		.line = line,
		.level = log_level,
	};

	if (!quiet && log_level >= s_level) {
		init_event(&ev, stderr);
		va_copy(ev.ap, args);
		stderr_callback(&ev);
		va_end(ev.ap);
	}

	for (int i = 0; i < callback_count; i++) {
		log_callback *cb = &callbacks[i];
		if (log_level >= cb->level) {
			init_event(&ev, cb->udata);
			va_copy(ev.ap, args);
			cb->fn(&ev);
			va_end(ev.ap);
		}
	}
}

void DTTR_Log(int level, const char *file, int line, const char *fmt, ...) {
	if (!DTTR_Log_IsEnabled(level)) {
		return;
	}

	va_list args;
	va_start(args, fmt);
	dttr_vlog_unchecked(level, file, line, fmt, args);
	va_end(args);
}

void DTTR_Log_Unchecked(int level, const char *file, int line, const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	dttr_vlog_unchecked(level, file, line, fmt, args);
	va_end(args);
}

void DTTR_Log_SetLevel(int log_level) { s_level = log_level; }

void DTTR_Log_SetQuiet(bool enable) { quiet = enable; }

int DTTR_Log_AddCallback(log_LogFn fn, void *udata, int level) {
	if (!fn || callback_count >= MAX_CALLBACKS) {
		return -1;
	}

	callbacks[callback_count++] = (log_callback){
		.fn = fn,
		.udata = udata,
		.level = level,
	};

	return 0;
}

int DTTR_Log_AddFP(FILE *fp, int level) {
	return DTTR_Log_AddCallback(file_callback, fp, level);
}
