#ifndef DTTR_HOOKS_AUDIO_H
#define DTTR_HOOKS_AUDIO_H

#include <dttr_components.h>

/// Skip MSS init when no SDL audio devices are available.
#define DTTR_DECLARE_AUDIO_TRAMPOLINE_HOOK(name)                                         \
	void __cdecl name##_callback(void);                                                  \
	DTTR_TRAMPOLINE_HOOK(name)

DTTR_DECLARE_AUDIO_TRAMPOLINE_HOOK(dttr_hook_audio_init_system);

/// Guard audio stop/reinit entry points when no driver is active.
DTTR_DECLARE_AUDIO_TRAMPOLINE_HOOK(dttr_hook_audio_stop_all_sounds);
DTTR_DECLARE_AUDIO_TRAMPOLINE_HOOK(dttr_hook_audio_init_level_audio);
DTTR_DECLARE_AUDIO_TRAMPOLINE_HOOK(dttr_hook_audio_stop_all_samples);

#undef DTTR_DECLARE_AUDIO_TRAMPOLINE_HOOK

#define DTTR_MSS_IMPORT_HOOKS(X)                                                         \
	X(dttr_hook_mss_ail_allocate_sample_handle,                                          \
	  "_AIL_allocate_sample_handle@4",                                                   \
	  dttr_mss_ail_allocate_sample_handle)                                               \
	X(dttr_hook_mss_ail_close_stream, "_AIL_close_stream@4", dttr_mss_ail_close_stream)  \
	X(dttr_hook_mss_ail_end_sample, "_AIL_end_sample@4", dttr_mss_ail_end_sample)        \
	X(dttr_hook_mss_ail_get_preference,                                                  \
	  "_AIL_get_preference@4",                                                           \
	  dttr_mss_ail_get_preference)                                                       \
	X(dttr_hook_mss_ail_init_sample, "_AIL_init_sample@4", dttr_mss_ail_init_sample)     \
	X(dttr_hook_mss_ail_open_stream, "_AIL_open_stream@12", dttr_mss_ail_open_stream)    \
	X(dttr_hook_mss_ail_pause_stream, "_AIL_pause_stream@8", dttr_mss_ail_pause_stream)  \
	X(dttr_hook_mss_ail_release_sample_handle,                                           \
	  "_AIL_release_sample_handle@4",                                                    \
	  dttr_mss_ail_release_sample_handle)                                                \
	X(dttr_hook_mss_ail_sample_playback_rate,                                            \
	  "_AIL_sample_playback_rate@4",                                                     \
	  dttr_mss_ail_sample_playback_rate)                                                 \
	X(dttr_hook_mss_ail_sample_status,                                                   \
	  "_AIL_sample_status@4",                                                            \
	  dttr_mss_ail_sample_status)                                                        \
	X(dttr_hook_mss_ail_set_digital_master_volume,                                       \
	  "_AIL_set_digital_master_volume@8",                                                \
	  dttr_mss_ail_set_digital_master_volume)                                            \
	X(dttr_hook_mss_ail_set_preference,                                                  \
	  "_AIL_set_preference@8",                                                           \
	  dttr_mss_ail_set_preference)                                                       \
	X(dttr_hook_mss_ail_set_sample_file,                                                 \
	  "_AIL_set_sample_file@12",                                                         \
	  dttr_mss_ail_set_sample_file)                                                      \
	X(dttr_hook_mss_ail_set_sample_loop_count,                                           \
	  "_AIL_set_sample_loop_count@8",                                                    \
	  dttr_mss_ail_set_sample_loop_count)                                                \
	X(dttr_hook_mss_ail_set_sample_pan,                                                  \
	  "_AIL_set_sample_pan@8",                                                           \
	  dttr_mss_ail_set_sample_pan)                                                       \
	X(dttr_hook_mss_ail_set_sample_playback_rate,                                        \
	  "_AIL_set_sample_playback_rate@8",                                                 \
	  dttr_mss_ail_set_sample_playback_rate)                                             \
	X(dttr_hook_mss_ail_set_sample_volume,                                               \
	  "_AIL_set_sample_volume@8",                                                        \
	  dttr_mss_ail_set_sample_volume)                                                    \
	X(dttr_hook_mss_ail_set_stream_loop_count,                                           \
	  "_AIL_set_stream_loop_count@8",                                                    \
	  dttr_mss_ail_set_stream_loop_count)                                                \
	X(dttr_hook_mss_ail_set_stream_volume,                                               \
	  "_AIL_set_stream_volume@8",                                                        \
	  dttr_mss_ail_set_stream_volume)                                                    \
	X(dttr_hook_mss_ail_shutdown, "_AIL_shutdown@0", dttr_mss_ail_shutdown)              \
	X(dttr_hook_mss_ail_start_sample, "_AIL_start_sample@4", dttr_mss_ail_start_sample)  \
	X(dttr_hook_mss_ail_start_stream, "_AIL_start_stream@4", dttr_mss_ail_start_stream)  \
	X(dttr_hook_mss_ail_startup, "_AIL_startup@0", dttr_mss_ail_startup)                 \
	X(dttr_hook_mss_ail_stop_sample, "_AIL_stop_sample@4", dttr_mss_ail_stop_sample)     \
	X(dttr_hook_mss_ail_stream_status,                                                   \
	  "_AIL_stream_status@4",                                                            \
	  dttr_mss_ail_stream_status)                                                        \
	X(dttr_hook_mss_ail_waveOutClose, "_AIL_waveOutClose@4", dttr_mss_ail_waveOutClose)  \
	X(dttr_hook_mss_ail_waveOutOpen, "_AIL_waveOutOpen@16", dttr_mss_ail_waveOutOpen)

#define DTTR_DECLARE_MSS_IMPORT_HOOK(name, import_name, callback) DTTR_HOOK(name)
DTTR_MSS_IMPORT_HOOKS(DTTR_DECLARE_MSS_IMPORT_HOOK)
#undef DTTR_DECLARE_MSS_IMPORT_HOOK

/// Install audio crash-prevention hooks.
void dttr_audio_init(const DTTR_ComponentContext *ctx);

/// Remove audio hooks and free trampoline memory.
void dttr_audio_cleanup(const DTTR_ComponentContext *ctx);

/// Handle SDL audio hot-plug events.
void dttr_audio_handle_device_event(const SDL_Event *event);

#endif /* DTTR_HOOKS_AUDIO_H */
