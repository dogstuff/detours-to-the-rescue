#include "imports_internal.h"

static const DTTR_ImportHookSpec s_mss32_hooks[] = {
	DTTR_IMPORT_SHOULD_NOT_CALL_SPEC("_AIL_open_stream@12"),
	DTTR_IMPORT_SHOULD_NOT_CALL_SPEC("_AIL_set_sample_playback_rate@8"),
	DTTR_IMPORT_SHOULD_NOT_CALL_SPEC("_AIL_set_sample_volume@8"),
	DTTR_IMPORT_SHOULD_NOT_CALL_SPEC("_AIL_close_stream@4"),
	DTTR_IMPORT_SHOULD_NOT_CALL_SPEC("_AIL_set_sample_pan@8"),
	DTTR_IMPORT_SHOULD_NOT_CALL_SPEC("_AIL_sample_status@4"),
	DTTR_IMPORT_SHOULD_NOT_CALL_SPEC("_AIL_init_sample@4"),
	DTTR_IMPORT_SHOULD_NOT_CALL_SPEC("_AIL_set_sample_file@12"),
	DTTR_IMPORT_SHOULD_NOT_CALL_SPEC("_AIL_set_sample_loop_count@8"),
	DTTR_IMPORT_SHOULD_NOT_CALL_SPEC("_AIL_start_stream@4"),
	DTTR_IMPORT_SHOULD_NOT_CALL_SPEC("_AIL_stream_status@4"),
	DTTR_IMPORT_SHOULD_NOT_CALL_SPEC("_AIL_stop_sample@4"),
	DTTR_IMPORT_SHOULD_NOT_CALL_SPEC("_AIL_end_sample@4"),
	DTTR_IMPORT_SHOULD_NOT_CALL_SPEC("_AIL_release_sample_handle@4"),
	DTTR_IMPORT_SHOULD_NOT_CALL_SPEC("_AIL_shutdown@0"),
	DTTR_IMPORT_SHOULD_NOT_CALL_SPEC("_AIL_startup@0"),
	DTTR_IMPORT_SHOULD_NOT_CALL_SPEC("_AIL_set_preference@8"),
	DTTR_IMPORT_SHOULD_NOT_CALL_SPEC("_AIL_waveOutOpen@16"),
	DTTR_IMPORT_SHOULD_NOT_CALL_SPEC("_AIL_waveOutClose@4"),
	DTTR_IMPORT_SHOULD_NOT_CALL_SPEC("_AIL_get_preference@4"),
	DTTR_IMPORT_SHOULD_NOT_CALL_SPEC("_AIL_allocate_sample_handle@4"),
	DTTR_IMPORT_SHOULD_NOT_CALL_SPEC("_AIL_set_stream_volume@8"),
	DTTR_IMPORT_SHOULD_NOT_CALL_SPEC("_AIL_start_sample@4"),
	DTTR_IMPORT_SHOULD_NOT_CALL_SPEC("_AIL_pause_stream@8"),
	DTTR_IMPORT_SHOULD_NOT_CALL_SPEC("_AIL_set_digital_master_volume@8"),
	DTTR_IMPORT_SHOULD_NOT_CALL_SPEC("_AIL_set_stream_loop_count@8"),
	DTTR_IMPORT_SHOULD_NOT_CALL_SPEC("_AIL_sample_playback_rate@4")
};

void dttr_platform_hooks_mss32_init(const DTTR_ComponentContext *ctx) {
	dttr_platform_hooks_install_module(
		ctx,
		"mss32.dll",
		s_mss32_hooks,
		sizeof(s_mss32_hooks) / sizeof(s_mss32_hooks[0])
	);
}

void dttr_platform_hooks_mss32_cleanup(const DTTR_ComponentContext *ctx) {
	dttr_platform_hooks_cleanup_module(
		ctx,
		s_mss32_hooks,
		sizeof(s_mss32_hooks) / sizeof(s_mss32_hooks[0])
	);
}
