#ifndef MSS_PRIVATE_H
#define MSS_PRIVATE_H

#include <dttr_mods.h>

#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <windows.h>

#define DTTR_MSS_STATUS_DONE 2
#define DTTR_MSS_STATUS_PLAYING 4
#define DTTR_MSS_STATUS_STOPPED 8

#define DTTR_MSS_PREF_DIG_MIXER_CHANNELS 1
#define DTTR_MSS_PREF_DIG_DEFAULT_VOLUME 2
#define DTTR_MSS_DEFAULT_MIXER_CHANNELS 64
#define DTTR_MSS_DEFAULT_LOOP_COUNT 1
#define DTTR_MSS_DEFAULT_MASTER_GAIN 1.0f
#define DTTR_MSS_DEFAULT_PAN 64
#define DTTR_MSS_DEFAULT_VOLUME 127

#define DTTR_MSS_WAVE_FORMAT_PCM 1

#define DTTR_MSS_DEFAULT_RATE 22050
#define DTTR_MSS_MIXER_CHANNELS 2
#define DTTR_MSS_MIXER_FORMAT SDL_AUDIO_F32
#define DTTR_MSS_MIXER_RATE DTTR_MSS_DEFAULT_RATE
#define DTTR_MSS_MAX_VOLUME 127.0f
#define DTTR_MSS_PREFERENCES_CAPACITY 64
#define DTTR_MSS_LOG_TAG "MSS"
#define DTTR_MSS_STREAM_HEADROOM_GAIN 1.0f

typedef struct {
	uint16_t format_tag;
	uint16_t channels;
	uint32_t sample_rate;
	uint16_t block_align;
	uint16_t bits_per_sample;
	size_t data_offset;
	size_t data_size;
	uint64_t frame_count;
	bool has_fmt;
	bool has_data;
} mss_wave_info;

static inline void dttr_mss_reset_preferences(int *preferences, size_t count) {
	if (!preferences) {
		return;
	}

	memset(preferences, 0, sizeof(*preferences) * count);
	if (count > DTTR_MSS_PREF_DIG_MIXER_CHANNELS) {
		preferences[DTTR_MSS_PREF_DIG_MIXER_CHANNELS] = DTTR_MSS_DEFAULT_MIXER_CHANNELS;
	}

	if (count > DTTR_MSS_PREF_DIG_DEFAULT_VOLUME) {
		preferences[DTTR_MSS_PREF_DIG_DEFAULT_VOLUME] = DTTR_MSS_DEFAULT_VOLUME;
	}
}

static inline int dttr_mss_loops_to_sdl(int mss_loop_count) {
	if (mss_loop_count <= 0) {
		return mss_loop_count == 0 ? -1 : 0;
	}

	return mss_loop_count - 1;
}

bool dttr_mss_core_has_driver();
void dttr_mss_core_pump_silent_mixer();
void dttr_mss_core_reset_preferences();
void dttr_mss_core_ensure_preferences();
int dttr_mss_core_get_preference(unsigned int preference);
int dttr_mss_core_set_preference(unsigned int preference, int value);
bool dttr_mss_core_ensure_mix_initialized();
bool dttr_mss_core_ensure_mixer();
void dttr_mss_core_destroy_mixer();
MIX_Mixer *dttr_mss_core_mixer();
void dttr_mss_core_set_desired_spec(const SDL_AudioSpec *spec);
int dttr_mss_core_driver_open_count();
void dttr_mss_core_increment_driver_open_count();
void dttr_mss_core_decrement_driver_open_count();
void dttr_mss_core_reset_driver_open_count();
float dttr_mss_core_master_gain();
void dttr_mss_core_set_master_gain(float gain);
float dttr_mss_core_sample_headroom_gain();

void dttr_mss_sample_shutdown_all();
void dttr_mss_sample_destroy_sync();
void dttr_mss_sample_stop_all();
void dttr_mss_sample_set_master_gain(float gain);
void dttr_mss_sample_mix_into(
	const SDL_AudioSpec *spec,
	float *pcm,
	int values,
	float *sfx_bus
);

void dttr_mss_stream_shutdown_all();
void dttr_mss_stream_apply_master_gain();

float dttr_mss_track_gain(int volume, float master_gain, float headroom);
void dttr_mss_track_play(MIX_Track *track, int sdl_loops);
int dttr_mss_track_status(MIX_Track *track, int previous_status);

uint16_t dttr_mss_wave_read_u16le(const uint8_t *p);
uint32_t dttr_mss_wave_read_u32le(const uint8_t *p);
size_t dttr_mss_wave_riff_size(const void *file_image);
bool dttr_mss_wave_parse(const void *file_image, mss_wave_info *info);
int dttr_mss_wave_rate(const mss_wave_info *info);
bool dttr_mss_wave_decode_f32(
	const void *file_image,
	size_t size,
	mss_wave_info *info,
	float **frames_out
);
void dttr_mss_wave_free(void *ptr);

// These declarations manage SDL-backed MSS hook installation and cleanup.
void dttr_mss_sdl_shutdown();
void dttr_mss_sdl_release_hooks();
bool dttr_mss_sdl_install_hooks(const DTTR_Mods_Context *ctx);

int __stdcall dttr_mss_ail_startup();
void __stdcall dttr_mss_ail_shutdown();
int __stdcall dttr_mss_ail_set_preference(unsigned int preference, int value);
int __stdcall dttr_mss_ail_get_preference(unsigned int preference);
int __stdcall dttr_mss_ail_waveOutOpen(
	void **driver_out,
	void *wave_out,
	int device_id,
	const void *format
);
void __stdcall dttr_mss_ail_waveOutClose(void *driver);
void *__stdcall dttr_mss_ail_allocate_sample_handle(void *driver);
void __stdcall dttr_mss_ail_release_sample_handle(void *sample);
void __stdcall dttr_mss_ail_init_sample(void *sample);
int __stdcall dttr_mss_ail_set_sample_file(
	void *sample,
	const void *file_image,
	int block
);
void __stdcall dttr_mss_ail_start_sample(void *sample);
void __stdcall dttr_mss_ail_stop_sample(void *sample);
void __stdcall dttr_mss_ail_end_sample(void *sample);
int __stdcall dttr_mss_ail_sample_status(void *sample);
void __stdcall dttr_mss_ail_set_sample_loop_count(void *sample, int loops);
void __stdcall dttr_mss_ail_set_sample_volume(void *sample, int volume);
void __stdcall dttr_mss_ail_set_sample_pan(void *sample, int pan);
void __stdcall dttr_mss_ail_set_sample_playback_rate(void *sample, int rate);
int __stdcall dttr_mss_ail_sample_playback_rate(void *sample);
void *__stdcall dttr_mss_ail_open_stream(void *driver, const char *path, int stream_mem);
void __stdcall dttr_mss_ail_close_stream(void *stream);
void __stdcall dttr_mss_ail_start_stream(void *stream);
int __stdcall dttr_mss_ail_stream_status(void *stream);
void __stdcall dttr_mss_ail_pause_stream(void *stream, int pause);
void __stdcall dttr_mss_ail_set_stream_volume(void *stream, int volume);
void __stdcall dttr_mss_ail_set_stream_loop_count(void *stream, int loops);
void __stdcall dttr_mss_ail_set_digital_master_volume(void *driver, int volume);

#endif // MSS_PRIVATE_H
