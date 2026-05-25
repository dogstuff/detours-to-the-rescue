#include <dttr_sdk.h>

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

static int stable_compile_check() {
	DTTR_Core_Context ctx = {0};
	DTTR_Core_Result result = {DTTR_OK};
	const DTTR_Mods_Context *mod_ctx = 0;
	DTTR_PCDOGS_T_Actor_State *actor = 0;
	runtime_storage_api_compile_check();
	return DTTR_MODS_API_VERSION + DTTR_RUNTIME_API_VERSION;
}
