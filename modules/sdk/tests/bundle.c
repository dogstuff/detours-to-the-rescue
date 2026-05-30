#include <dttr_sdk.h>
#include <stddef.h>

#ifndef DTTR_MODS_API_VERSION
#error "stable mod API missing"
#endif
#ifndef DTTR_RUNTIME_API_VERSION
#error "runtime API missing"
#endif
#ifdef DTTR_PCDOGS_UNSTABLE_H
#error "unstable declarations should require DTTR_SDK_ENABLE_UNSTABLE"
#endif

DTTR_DECLARE_HOOK_STORAGE(dttr_test_storage_declared)
DTTR_DECLARE_STORAGE(int, dttr_test_storage_declared_value)
DTTR_DECLARE_HOOK_STORAGE(dttr_test_storage_slot_alpha)
DTTR_DECLARE_HOOK_STORAGE(dttr_test_storage_slot_beta)
DTTR_DECLARE_STORAGE(int, dttr_test_storage_slot_value)

DTTR_DEFINE_HOOK_STORAGE(dttr_test_storage_defined)
DTTR_DEFINE_STORAGE(int, dttr_test_storage_defined_value)
DTTR_DEFINE_HOOK_STORAGE(dttr_test_storage_slot_alpha)
DTTR_DEFINE_HOOK_STORAGE(dttr_test_storage_slot_beta)
DTTR_DEFINE_STORAGE(int, dttr_test_storage_slot_value)

static void runtime_storage_api_compile_check() {
	dttr_test_storage_defined_site = 1;
	dttr_test_storage_defined_handle = 0;
	dttr_test_storage_defined_value = (int)dttr_test_storage_defined_site;

	dttr_test_storage_slot_alpha_site = 1;
	dttr_test_storage_slot_alpha_handle = 0;
	dttr_test_storage_slot_beta_site = 2;
	dttr_test_storage_slot_beta_handle = 0;
	dttr_test_storage_slot_value = (int)(dttr_test_storage_slot_alpha_site
										 + dttr_test_storage_slot_beta_site);
}

#define SDK_READ_GROUPED_DATA(symbol, value)                                             \
	do {                                                                                 \
		(symbol)->Ptr();                                                                 \
		(symbol)->Read(&(value));                                                        \
	} while (0)

static void stable_grouped_data_compile_check() {
	int32_t input_map[DTTR_PCDOGS_D_INPUT_BUTTON_MAP_COUNT] = {0};
	int32_t input_map_alt[DTTR_PCDOGS_D_INPUT_BUTTON_MAP_ALT_COUNT] = {0};
	int32_t hammerhead[DTTR_PCDOGS_D_CONTROLLER_HAMMERHEAD_BUTTONS_COUNT] = {0};
	int32_t sidewinder[DTTR_PCDOGS_D_CONTROLLER_SIDEWINDER_BUTTONS_COUNT] = {0};
	int32_t gravis[DTTR_PCDOGS_D_CONTROLLER_GRAVIS_BUTTONS_COUNT] = {0};

	SDK_READ_GROUPED_DATA(DTTR_PCDOGS_D_InputButtonMap, input_map);
	DTTR_PCDOGS_D_InputButtonMap->Write(&input_map);
	DTTR_PCDOGS_D_InputButtonMap->UnsafeWrite(&input_map);

	SDK_READ_GROUPED_DATA(DTTR_PCDOGS_D_InputButtonMapAlt, input_map_alt);
	SDK_READ_GROUPED_DATA(DTTR_PCDOGS_D_ControllerHammerheadButtons, hammerhead);
	SDK_READ_GROUPED_DATA(DTTR_PCDOGS_D_ControllerSidewinderButtons, sidewinder);
	SDK_READ_GROUPED_DATA(DTTR_PCDOGS_D_ControllerGravisButtons, gravis);
}

#undef SDK_READ_GROUPED_DATA

static bool exception_report_compile_check(
	const DTTR_Mods_ExceptionReportRequest *request,
	DTTR_Mods_ExceptionReport *report
) {
	return request && report && request->struct_size == sizeof(*request)
		   && report->struct_size == sizeof(*report);
}

static int stable_compile_check() {
	DTTR_Core_Context ctx = {0};
	DTTR_Core_Result result = {DTTR_OK};
	const DTTR_Mods_Context *mod_ctx = 0;
	DTTR_PCDOGS_T_Actor_State *actor = 0;
	DTTR_Mods_ExceptionReportRequest exception_request = {
		.struct_size = sizeof(DTTR_Mods_ExceptionReportRequest),
	};
	DTTR_Mods_ExceptionReport exception_report = {
		.struct_size = sizeof(DTTR_Mods_ExceptionReport),
	};
	DTTR_Mods_API api = {
		.struct_size = sizeof(DTTR_Mods_API),
		.api_version = DTTR_MODS_API_VERSION,
		.write_exception_report = exception_report_compile_check,
	};
	DTTR_Mods_API older_api = api;
	older_api.api_version = DTTR_MODS_API_VERSION - 1u;

	DTTR_Mods_API future_api = api;
	future_api.api_version = DTTR_MODS_API_VERSION + 1u;
	future_api.struct_size = sizeof(DTTR_Mods_API) + 1u;

	DTTR_Mods_Context compatible_context = {
		.api_version = DTTR_MODS_API_VERSION,
		.struct_size = sizeof(DTTR_Mods_Context),
	};

	DTTR_Mods_Context older_context = compatible_context;
	older_context.api_version = DTTR_MODS_API_VERSION - 1u;

	DTTR_Mods_Context future_context = compatible_context;
	future_context.api_version = DTTR_MODS_API_VERSION + 1u;
	future_context.struct_size = sizeof(DTTR_Mods_Context) + 1u;

	DTTR_Mods_WriteExceptionReportFn exception_reporter = DTTR_Mods_GetWriteExceptionReportFn(
		&api
	);

	runtime_storage_api_compile_check();
	stable_grouped_data_compile_check();

	return DTTR_MODS_API_VERSION + DTTR_RUNTIME_API_VERSION
		   + (int)exception_request.struct_size + (int)exception_report.struct_size
		   + (DTTR_Mods_GetWriteExceptionReportFn(&older_api) == NULL ? 1 : 0)
		   + (DTTR_Mods_GetWriteExceptionReportFn(&future_api) != NULL ? 1 : 0)
		   + (DTTR_Mods_ContextIsCompatible(&compatible_context) ? 1 : 0)
		   + (!DTTR_Mods_ContextIsCompatible(&older_context) ? 1 : 0)
		   + (DTTR_Mods_ContextIsCompatible(&future_context) ? 1 : 0)
		   + (exception_reporter(&exception_request, &exception_report) ? 1 : 0);
}

_Static_assert(
	DTTR_MODS_API_VERSION >= 12,
	"strict exception report ABI requires mod API v12"
);
_Static_assert(
	DTTR_MODS_EXCEPTION_REPORT_STACK_TRACE_CAPACITY == 16384,
	"exception report stack trace capacity is part of the SDK contract"
);
_Static_assert(
	offsetof(DTTR_Mods_API, write_exception_report) < sizeof(DTTR_Mods_API),
	"exception reporter is a named DTTR_Mods_API field"
);
