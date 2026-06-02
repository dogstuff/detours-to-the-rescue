#include <dttr_config.h>
#include <dttr_crashdump.h>
#include <dttr_log.h>
#include <dttr_path.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>

#include <dttr_test_support.h>

static void assert_contains(const char *text, const char *needle) {
	assert_non_null(strstr(text, needle));
}

static void stack_trace_formatter_includes_marker(void **state) {
	sds stack_trace = DTTR_CrashDump_FormatStackTrace(NULL, NULL, NULL);

	assert_non_null(stack_trace);
	assert_contains(stack_trace, "<context unavailable>");
	assert_contains(stack_trace, "Stack trace:");
	assert_contains(stack_trace, "<unavailable>");

	sdsfree(stack_trace);
}

static char crash_report_log_message[256];

static void capture_error_log(log_Event *event) {
	vsnprintf(
		crash_report_log_message,
		sizeof(crash_report_log_message),
		event->fmt,
		event->ap
	);
}

static void crash_report_details_are_logged(void **state) {
	assert_non_null(state);
	crash_report_log_message[0] = '\0';
	assert_int_equal(DTTR_Log_AddCallback(capture_error_log, NULL, LOG_ERROR), 0);

	DTTR_CrashDump_LogAndTraceReport("Exception 0xC0000005\n\nStack trace:\n  pc=0x1");

	assert_string_equal(
		crash_report_log_message,
		"Exception 0xC0000005\n\nStack trace:\n  pc=0x1"
	);
}

static void set_level_filters_lower_priority_logs(void **state) {
	DTTR_Log_SetQuiet(false);
	DTTR_Log_SetLevel(LOG_ERROR);

	assert_false(DTTR_Log_IsEnabled(LOG_DEBUG));
	assert_true(DTTR_Log_IsEnabled(LOG_ERROR));
}

static void safe_relative_rejects_absolute_and_traversal_paths(void **state) {

	assert_true(DTTR_Path_IsSafeRelative("data/save.dat"));
	assert_true(DTTR_Path_IsSafeRelative("data\\save.dat"));
	assert_false(DTTR_Path_IsSafeRelative(NULL));
	assert_false(DTTR_Path_IsSafeRelative(""));
	assert_false(DTTR_Path_IsSafeRelative("/data/save.dat"));
	assert_false(DTTR_Path_IsSafeRelative("\\data\\save.dat"));
	assert_false(DTTR_Path_IsSafeRelative("C:\\data\\save.dat"));
	assert_false(DTTR_Path_IsSafeRelative("C:data\\save.dat"));
	assert_false(DTTR_Path_IsSafeRelative("data/../save.dat"));
	assert_false(DTTR_Path_IsSafeRelative("./save.dat"));
}

static void write_text_file(const char *path, const char *contents) {
	FILE *file = fopen(path, "wb");
	assert_non_null(file);
	assert_int_equal(fwrite(contents, 1, strlen(contents), file), strlen(contents));
	assert_int_equal(fclose(file), 0);
}

static void assert_config_rejected(const char *path, const char *contents) {
	write_text_file(path, contents);
	assert_false(DTTR_Config_Load(path));
	remove(path);
}

static void config_load_save_round_trips_show_crash_popup(void **state) {
	const char *path = "dttr-test-crash-popup-roundtrip.json";

	write_text_file(path, "{\"schema_major_version\":1}\n");
	assert_true(DTTR_Config_Load(path));
	assert_true(dttr_config.show_crash_popup);

	write_text_file(path, "{\"schema_major_version\":1,\"show_crash_popup\":false}\n");
	assert_true(DTTR_Config_Load(path));
	assert_false(dttr_config.show_crash_popup);
	assert_true(DTTR_Config_Save(path, &dttr_config));

	assert_true(DTTR_Config_Load(path));
	assert_false(dttr_config.show_crash_popup);
	remove(path);
}

static void crash_report_message_includes_stack_trace(void **state) {
	const char *summary = "Exception 0xDEADBEEF\n\nDump written to:\ndttr.dmp";
	const char *stack_trace = "\n\nStack trace:\n  game!crash+0x1";

	sds message = DTTR_CrashDump_AppendReportMessage(sdsnew(summary), stack_trace);

	assert_non_null(message);
	assert_contains(message, "Stack trace:");
	assert_contains(message, "Exception 0xDEADBEEF");
	assert_contains(message, "Feel free to report this error");

	sdsfree(message);
}

static void config_load_rejects_invalid_values(void **state) {

	assert_config_rejected(
		"dttr-test-int-overflow.json",
		"{\"schema_major_version\":9223372036854775807}\n"
	);
	assert_config_rejected(
		"dttr-test-float-overflow.json",
		"{\"schema_major_version\":1,\"audio\":{\"mss_sample_gain\":3.5e39}}\n"
	);

	const char *path = "dttr-test-overlong-string.json";
	FILE *file = fopen(path, "wb");
	assert_non_null(file);
	fputs("{\"schema_major_version\":1,\"log_file_path\":\"", file);
	for (int i = 0; i < MAX_PATH; i++) {
		fputc('a', file);
	}

	fputs("\"}\n", file);
	assert_int_equal(fclose(file), 0);

	assert_false(DTTR_Config_Load(path));
	remove(path);
}

static const DTTR_TestCase TEST_CASES[] = {
	{"stack-trace-marker", stack_trace_formatter_includes_marker},
	{"crash-report-details-are-logged", crash_report_details_are_logged},
	{"set-level-filters-lower-priority-logs", set_level_filters_lower_priority_logs},
	{"safe-relative-rejects-absolute-and-traversal-paths",
	 safe_relative_rejects_absolute_and_traversal_paths},
	{"config-load-rejects-invalid-values", config_load_rejects_invalid_values},
	{"config-load-save-round-trips-show-crash-popup",
	 config_load_save_round_trips_show_crash_popup},
	{"crash-report-message-includes-stack-trace",
	 crash_report_message_includes_stack_trace},
};

DTTR_TEST_MAIN(TEST_CASES)
