#include "mss_internal.h"

#define DR_WAV_IMPLEMENTATION
#define DR_WAV_NO_STDIO
#define DR_WAV_NO_WCHAR
#include "dr_wav.h"

#include <limits.h>
#include <string.h>

uint16_t dttr_mss_wave_read_u16le(const uint8_t *p) {
	return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

uint32_t dttr_mss_wave_read_u32le(const uint8_t *p) {
	return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16)
		   | ((uint32_t)p[3] << 24);
}

size_t dttr_mss_wave_riff_size(const void *file_image) {
	const uint8_t *bytes = file_image;
	if (!bytes || memcmp(bytes, "RIFF", 4) != 0 || memcmp(bytes + 8, "WAVE", 4) != 0) {
		return 0;
	}

	return dttr_mss_wave_read_u32le(bytes + 4) + 8;
}

bool dttr_mss_wave_parse(const void *file_image, DTTR_MssWaveInfo *info) {
	if (!file_image || !info) {
		return false;
	}

	memset(info, 0, sizeof(*info));

	const size_t total = dttr_mss_wave_riff_size(file_image);
	if (total == 0) {
		return false;
	}

	drwav wav = {0};
	if (!drwav_init_memory(&wav, file_image, total, NULL)) {
		return false;
	}

	const bool chunk_info_fits = wav.dataChunkDataPos <= SIZE_MAX
								 && wav.dataChunkDataSize <= SIZE_MAX;
	if (!chunk_info_fits) {
		drwav_uninit(&wav);
		return false;
	}

	info->format_tag = wav.translatedFormatTag;
	info->channels = wav.channels;
	info->sample_rate = wav.sampleRate;
	info->block_align = wav.fmt.blockAlign;
	info->bits_per_sample = wav.bitsPerSample;
	info->data_offset = (size_t)wav.dataChunkDataPos;
	info->data_size = (size_t)wav.dataChunkDataSize;
	info->frame_count = wav.totalPCMFrameCount;
	info->has_fmt = true;
	info->has_data = wav.dataChunkDataSize > 0;

	drwav_uninit(&wav);
	return true;
}

int dttr_mss_wave_rate(const DTTR_MssWaveInfo *info) {
	if (!info || !info->has_fmt || info->sample_rate == 0
		|| info->sample_rate > INT_MAX) {
		return DTTR_MSS_DEFAULT_RATE;
	}

	return (int)info->sample_rate;
}

bool dttr_mss_wave_decode_f32(
	const void *file_image,
	size_t size,
	DTTR_MssWaveInfo *info,
	float **frames_out
) {
	if (!file_image || size == 0 || !frames_out) {
		return false;
	}

	unsigned int channels = 0;
	unsigned int sample_rate = 0;
	drwav_uint64 frame_count = 0;
	float *frames = drwav_open_memory_and_read_pcm_frames_f32(
		file_image,
		size,
		&channels,
		&sample_rate,
		&frame_count,
		NULL
	);
	if (!frames || channels == 0 || sample_rate == 0 || frame_count == 0) {
		drwav_free(frames, NULL);
		return false;
	}

	if (info) {
		info->channels = channels;
		info->sample_rate = sample_rate;
		info->frame_count = frame_count;
		info->has_fmt = true;
		info->has_data = true;
	}

	*frames_out = frames;
	return true;
}

void dttr_mss_wave_free(void *ptr) { drwav_free(ptr, NULL); }
