#include "imports_internal.h"

#define S_MSS32_IMPORTS(X)                                                               \
	X(SKIP,                                                                              \
	  dttr_import_mss32,                                                                 \
	  ail_open_stream,                                                                   \
	  "_AIL_open_stream@12",                                                             \
	  "_AIL_open_stream@12")                                                             \
	X(SKIP,                                                                              \
	  dttr_import_mss32,                                                                 \
	  ail_set_sample_playback_rate,                                                      \
	  "_AIL_set_sample_playback_rate@8",                                                 \
	  "_AIL_set_sample_playback_rate@8")                                                 \
	X(SKIP,                                                                              \
	  dttr_import_mss32,                                                                 \
	  ail_set_sample_volume,                                                             \
	  "_AIL_set_sample_volume@8",                                                        \
	  "_AIL_set_sample_volume@8")                                                        \
	X(SKIP,                                                                              \
	  dttr_import_mss32,                                                                 \
	  ail_close_stream,                                                                  \
	  "_AIL_close_stream@4",                                                             \
	  "_AIL_close_stream@4")                                                             \
	X(SKIP,                                                                              \
	  dttr_import_mss32,                                                                 \
	  ail_set_sample_pan,                                                                \
	  "_AIL_set_sample_pan@8",                                                           \
	  "_AIL_set_sample_pan@8")                                                           \
	X(SKIP,                                                                              \
	  dttr_import_mss32,                                                                 \
	  ail_sample_status,                                                                 \
	  "_AIL_sample_status@4",                                                            \
	  "_AIL_sample_status@4")                                                            \
	X(SKIP,                                                                              \
	  dttr_import_mss32,                                                                 \
	  ail_init_sample,                                                                   \
	  "_AIL_init_sample@4",                                                              \
	  "_AIL_init_sample@4")                                                              \
	X(SKIP,                                                                              \
	  dttr_import_mss32,                                                                 \
	  ail_set_sample_file,                                                               \
	  "_AIL_set_sample_file@12",                                                         \
	  "_AIL_set_sample_file@12")                                                         \
	X(SKIP,                                                                              \
	  dttr_import_mss32,                                                                 \
	  ail_set_sample_loop_count,                                                         \
	  "_AIL_set_sample_loop_count@8",                                                    \
	  "_AIL_set_sample_loop_count@8")                                                    \
	X(SKIP,                                                                              \
	  dttr_import_mss32,                                                                 \
	  ail_start_stream,                                                                  \
	  "_AIL_start_stream@4",                                                             \
	  "_AIL_start_stream@4")                                                             \
	X(SKIP,                                                                              \
	  dttr_import_mss32,                                                                 \
	  ail_stream_status,                                                                 \
	  "_AIL_stream_status@4",                                                            \
	  "_AIL_stream_status@4")                                                            \
	X(SKIP,                                                                              \
	  dttr_import_mss32,                                                                 \
	  ail_stop_sample,                                                                   \
	  "_AIL_stop_sample@4",                                                              \
	  "_AIL_stop_sample@4")                                                              \
	X(SKIP, dttr_import_mss32, ail_end_sample, "_AIL_end_sample@4", "_AIL_end_sample@4") \
	X(SKIP,                                                                              \
	  dttr_import_mss32,                                                                 \
	  ail_release_sample_handle,                                                         \
	  "_AIL_release_sample_handle@4",                                                    \
	  "_AIL_release_sample_handle@4")                                                    \
	X(SKIP, dttr_import_mss32, ail_shutdown, "_AIL_shutdown@0", "_AIL_shutdown@0")       \
	X(SKIP, dttr_import_mss32, ail_startup, "_AIL_startup@0", "_AIL_startup@0")          \
	X(SKIP,                                                                              \
	  dttr_import_mss32,                                                                 \
	  ail_set_preference,                                                                \
	  "_AIL_set_preference@8",                                                           \
	  "_AIL_set_preference@8")                                                           \
	X(SKIP,                                                                              \
	  dttr_import_mss32,                                                                 \
	  ail_waveoutopen,                                                                   \
	  "_AIL_waveOutOpen@16",                                                             \
	  "_AIL_waveOutOpen@16")                                                             \
	X(SKIP,                                                                              \
	  dttr_import_mss32,                                                                 \
	  ail_waveoutclose,                                                                  \
	  "_AIL_waveOutClose@4",                                                             \
	  "_AIL_waveOutClose@4")                                                             \
	X(SKIP,                                                                              \
	  dttr_import_mss32,                                                                 \
	  ail_get_preference,                                                                \
	  "_AIL_get_preference@4",                                                           \
	  "_AIL_get_preference@4")                                                           \
	X(SKIP,                                                                              \
	  dttr_import_mss32,                                                                 \
	  ail_allocate_sample_handle,                                                        \
	  "_AIL_allocate_sample_handle@4",                                                   \
	  "_AIL_allocate_sample_handle@4")                                                   \
	X(SKIP,                                                                              \
	  dttr_import_mss32,                                                                 \
	  ail_set_stream_volume,                                                             \
	  "_AIL_set_stream_volume@8",                                                        \
	  "_AIL_set_stream_volume@8")                                                        \
	X(SKIP,                                                                              \
	  dttr_import_mss32,                                                                 \
	  ail_start_sample,                                                                  \
	  "_AIL_start_sample@4",                                                             \
	  "_AIL_start_sample@4")                                                             \
	X(SKIP,                                                                              \
	  dttr_import_mss32,                                                                 \
	  ail_pause_stream,                                                                  \
	  "_AIL_pause_stream@8",                                                             \
	  "_AIL_pause_stream@8")                                                             \
	X(SKIP,                                                                              \
	  dttr_import_mss32,                                                                 \
	  ail_set_digital_master_volume,                                                     \
	  "_AIL_set_digital_master_volume@8",                                                \
	  "_AIL_set_digital_master_volume@8")                                                \
	X(SKIP,                                                                              \
	  dttr_import_mss32,                                                                 \
	  ail_set_stream_loop_count,                                                         \
	  "_AIL_set_stream_loop_count@8",                                                    \
	  "_AIL_set_stream_loop_count@8")                                                    \
	X(SKIP,                                                                              \
	  dttr_import_mss32,                                                                 \
	  ail_sample_playback_rate,                                                          \
	  "_AIL_sample_playback_rate@4",                                                     \
	  "_AIL_sample_playback_rate@4")

S_MSS32_IMPORTS(DTTR_IMPORT_ENTRY_DECL)

static const DTTR_ImportHookSpec s_mss32_hooks[] = {
	S_MSS32_IMPORTS(DTTR_IMPORT_ENTRY_SPEC)
};

void dttr_platform_hooks_mss32_init(const DTTR_ComponentContext *ctx) {
	dttr_platform_hooks_install_module(
		ctx,
		"mss32.dll",
		s_mss32_hooks,
		DTTR_IMPORT_ARRAY_COUNT(s_mss32_hooks)
	);
}

void dttr_platform_hooks_mss32_cleanup(const DTTR_ComponentContext *ctx) {
	dttr_platform_hooks_cleanup_module(
		ctx,
		s_mss32_hooks,
		DTTR_IMPORT_ARRAY_COUNT(s_mss32_hooks)
	);
}

#undef S_MSS32_IMPORTS
