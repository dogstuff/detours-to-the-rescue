#include <dttr_log.h>

#include <stdarg.h>
#include <stdio.h>
#include <time.h>

#define S_MAX_CALLBACKS 32

typedef struct {
	log_LogFn fn;
	void *udata;
	int level;
} S_LogCallback;

static int s_level = LOG_TRACE;
static bool s_quiet;
static S_LogCallback s_callbacks[S_MAX_CALLBACKS];
static int s_callback_count;

static const char *s_log_level_string(int level) {
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

static void s_init_event(log_Event *ev, void *udata) {
	if (!ev->time) {
		time_t t = time(NULL);
		ev->time = localtime(&t);
	}
	ev->udata = udata;
}

static void s_stderr_callback(log_Event *ev) {
	char buf[16];
	buf[strftime(buf, sizeof(buf), "%H:%M:%S", ev->time)] = '\0';
	fprintf(
		ev->udata,
		"%s %-5s %s:%d: ",
		buf,
		s_log_level_string(ev->level),
		ev->file,
		ev->line
	);
	vfprintf(ev->udata, ev->fmt, ev->ap);
	fprintf(ev->udata, "\n");
	fflush(ev->udata);
}

static void s_file_callback(log_Event *ev) {
	char buf[64];
	buf[strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", ev->time)] = '\0';
	fprintf(
		ev->udata,
		"%s %-5s %s:%d: ",
		buf,
		s_log_level_string(ev->level),
		ev->file,
		ev->line
	);
	vfprintf(ev->udata, ev->fmt, ev->ap);
	fprintf(ev->udata, "\n");
	fflush(ev->udata);
}

bool dttr_log_is_enabled(int level) {
	if (!s_quiet && level >= s_level) {
		return true;
	}

	for (int i = 0; i < s_callback_count; i++) {
		if (level >= s_callbacks[i].level) {
			return true;
		}
	}

	return false;
}

static void s_dttr_vlog_unchecked(
	int level,
	const char *file,
	int line,
	const char *fmt,
	va_list args
) {
	log_Event ev = {
		.fmt = fmt,
		.file = file,
		.line = line,
		.level = level,
	};

	if (!s_quiet && level >= s_level) {
		s_init_event(&ev, stderr);
		va_copy(ev.ap, args);
		s_stderr_callback(&ev);
		va_end(ev.ap);
	}

	for (int i = 0; i < s_callback_count; i++) {
		S_LogCallback *cb = &s_callbacks[i];
		if (level >= cb->level) {
			s_init_event(&ev, cb->udata);
			va_copy(ev.ap, args);
			cb->fn(&ev);
			va_end(ev.ap);
		}
	}
}

void dttr_log(int level, const char *file, int line, const char *fmt, ...) {
	if (!dttr_log_is_enabled(level)) {
		return;
	}

	va_list args;
	va_start(args, fmt);
	s_dttr_vlog_unchecked(level, file, line, fmt, args);
	va_end(args);
}

void dttr_log_unchecked(int level, const char *file, int line, const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	s_dttr_vlog_unchecked(level, file, line, fmt, args);
	va_end(args);
}

void dttr_log_set_level(int level) { s_level = level; }

void dttr_log_set_quiet(bool enable) { s_quiet = enable; }

int dttr_log_add_callback(log_LogFn fn, void *udata, int level) {
	if (!fn || s_callback_count >= S_MAX_CALLBACKS) {
		return -1;
	}

	s_callbacks[s_callback_count++] = (S_LogCallback){
		.fn = fn,
		.udata = udata,
		.level = level,
	};
	return 0;
}

int dttr_log_add_fp(FILE *fp, int level) {
	return dttr_log_add_callback(s_file_callback, fp, level);
}
