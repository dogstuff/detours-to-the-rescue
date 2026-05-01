#include "dttr_hooks_audio.h"
#include "mss_private.h"

#include <dttr_log.h>

#include <SDL3/SDL.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <windows.h>

static bool s_wave_format_spec(const void *format, SDL_AudioSpec *spec) {
	if (!format || !spec) {
		return false;
	}

	const uint8_t *bytes = format;
	const uint16_t format_tag = dttr_mss_wave_read_u16le(bytes);
	const uint16_t channels = dttr_mss_wave_read_u16le(bytes + 2);
	const uint32_t sample_rate = dttr_mss_wave_read_u32le(bytes + 4);
	const uint16_t bits_per_sample = dttr_mss_wave_read_u16le(bytes + 14);

	if (format_tag != DTTR_MSS_WAVE_FORMAT_PCM || channels == 0 || sample_rate == 0) {
		return false;
	}

	if (!dttr_mss_wave_bits_supported(bits_per_sample)) {
		return false;
	}

	spec->format = DTTR_MSS_MIXER_FORMAT;
	spec->channels = DTTR_MSS_MIXER_CHANNELS;
	spec->freq = (int)sample_rate;
	return true;
}

bool dttr_mss_sdl_original_mode_enabled(void) {
	return dttr_mss_core_original_mode_enabled();
}

bool dttr_mss_sdl_has_driver(void) { return dttr_mss_core_has_driver(); }

void dttr_mss_sdl_shutdown(void) {
	dttr_mss_stream_shutdown_all();
	dttr_mss_sample_shutdown_all();
	dttr_mss_core_destroy_mixer();
	dttr_mss_core_reset_driver_open_count();
	dttr_mss_core_set_master_gain(DTTR_MSS_DEFAULT_MASTER_GAIN);
}

static bool s_install_mss_import_hook(
	const DTTR_ComponentContext *ctx,
	const char *name,
	uintptr_t site
) {
#define S_TRY_MSS_IMPORT_HOOK(hook_name, import_name, callback)                          \
	if (strcmp(name, import_name) == 0) {                                                \
		DTTR_INSTALL_POINTER_AT(hook_name, ctx, site, callback);                         \
		return true;                                                                     \
	}

	DTTR_MSS_IMPORT_HOOKS(S_TRY_MSS_IMPORT_HOOK)
#undef S_TRY_MSS_IMPORT_HOOK
	return false;
}

static void s_install_mss_import_descriptor(
	const DTTR_ComponentContext *ctx,
	uint8_t *base,
	IMAGE_IMPORT_DESCRIPTOR *desc
) {
	IMAGE_THUNK_DATA *name_thunk = (IMAGE_THUNK_DATA *)(base + desc->OriginalFirstThunk);
	IMAGE_THUNK_DATA *addr_thunk = (IMAGE_THUNK_DATA *)(base + desc->FirstThunk);
	for (; name_thunk->u1.AddressOfData; name_thunk++, addr_thunk++) {
		if (IMAGE_SNAP_BY_ORDINAL(name_thunk->u1.Ordinal)) {
			continue;
		}

		IMAGE_IMPORT_BY_NAME *import_name = (IMAGE_IMPORT_BY_NAME
												 *)(base + name_thunk->u1.AddressOfData);
		if (s_install_mss_import_hook(
				ctx,
				(const char *)import_name->Name,
				(uintptr_t)&addr_thunk->u1.Function
			)) {
			continue;
		}

		DTTR_LOG_ERROR("Unhandled MSS32 import: %s", import_name->Name);
	}
}

void dttr_mss_sdl_release_hooks(const DTTR_ComponentContext *ctx) {
#define S_RELEASE_MSS_IMPORT_HOOK(name, import_name, callback) DTTR_UNINSTALL(name, ctx);
	DTTR_MSS_IMPORT_HOOKS(S_RELEASE_MSS_IMPORT_HOOK)
#undef S_RELEASE_MSS_IMPORT_HOOK
}

void dttr_mss_sdl_install_hooks(const DTTR_ComponentContext *ctx) {
	if (dttr_mss_sdl_original_mode_enabled()) {
		DTTR_LOG_INFO("MSS SDL import shim disabled");
		return;
	}

	HMODULE module = ctx->m_game_module;
	uint8_t *base = (uint8_t *)module;
	IMAGE_DOS_HEADER *dos = (IMAGE_DOS_HEADER *)base;
	IMAGE_NT_HEADERS *nt = (IMAGE_NT_HEADERS *)(base + dos->e_lfanew);
	IMAGE_DATA_DIRECTORY imports_dir = nt->OptionalHeader
										   .DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
	IMAGE_IMPORT_DESCRIPTOR *desc = (IMAGE_IMPORT_DESCRIPTOR
										 *)(base + imports_dir.VirtualAddress);

	for (; desc && desc->Name; desc++) {
		if (_stricmp((const char *)(base + desc->Name), "mss32.dll") != 0) {
			continue;
		}

		s_install_mss_import_descriptor(ctx, base, desc);
	}
}

static float s_master_gain_for_volume(int volume) {
	if (volume <= 0) {
		return 0.0f;
	}

	if (volume >= DTTR_MSS_DEFAULT_VOLUME) {
		return 1.0f;
	}

	return (float)volume / DTTR_MSS_MAX_VOLUME;
}

int __stdcall dttr_mss_ail_startup(void) {
	dttr_mss_core_ensure_preferences();
	return dttr_mss_core_ensure_mix_initialized() ? 1 : 0;
}

void __stdcall dttr_mss_ail_shutdown(void) { dttr_mss_sdl_shutdown(); }

int __stdcall dttr_mss_ail_set_preference(unsigned int preference, int value) {
	return dttr_mss_core_set_preference(preference, value);
}

int __stdcall dttr_mss_ail_get_preference(unsigned int preference) {
	return dttr_mss_core_get_preference(preference);
}

int __stdcall dttr_mss_ail_waveOutOpen(
	void **driver_out,
	void *wave_out,
	int device_id,
	const void *format
) {
	DTTR_LOG_TRACE(
		"MSS AIL_waveOutOpen(driver_out=%p, wave_out=%p, device_id=%d, format=%p)",
		driver_out,
		wave_out,
		device_id,
		format
	);
	if (driver_out) {
		*driver_out = NULL;
	}

	SDL_AudioSpec desired_spec = {0};
	if (s_wave_format_spec(format, &desired_spec)) {
		dttr_mss_core_set_desired_spec(&desired_spec);
		DTTR_LOG_TRACE(
			"MSS AIL_waveOutOpen desired spec: format=%u channels=%d freq=%d",
			(unsigned)desired_spec.format,
			desired_spec.channels,
			desired_spec.freq
		);
	}

	if (!dttr_mss_core_ensure_mixer()) {
		DTTR_LOG_TRACE("MSS AIL_waveOutOpen -> -1 (mixer unavailable)");
		return -1;
	}

	dttr_mss_core_increment_driver_open_count();
	if (driver_out) {
		*driver_out = dttr_mss_core_mixer();
	}

	DTTR_LOG_TRACE(
		"MSS AIL_waveOutOpen -> 0 driver=%p open_count=%d",
		dttr_mss_core_mixer(),
		dttr_mss_core_driver_open_count()
	);
	return 0;
}

void __stdcall dttr_mss_ail_waveOutClose(void *driver) {
	DTTR_LOG_TRACE(
		"MSS AIL_waveOutClose(driver=%p, mixer=%p, open_count=%d)",
		driver,
		dttr_mss_core_mixer(),
		dttr_mss_core_driver_open_count()
	);
	if (driver && driver != dttr_mss_core_mixer()) {
		DTTR_LOG_ERROR("Ignoring AIL_waveOutClose for unknown driver %p", driver);
		return;
	}

	if (dttr_mss_core_driver_open_count() > 0) {
		dttr_mss_core_decrement_driver_open_count();
	}

	DTTR_LOG_TRACE(
		"MSS AIL_waveOutClose open_count -> %d",
		dttr_mss_core_driver_open_count()
	);
	if (dttr_mss_core_driver_open_count() > 0) {
		return;
	}

	dttr_mss_sdl_shutdown();
}

void __stdcall dttr_mss_ail_set_digital_master_volume(void *driver, int volume) {
	dttr_mss_core_set_master_gain(s_master_gain_for_volume(volume));
	dttr_mss_sample_apply_master_gain();
	dttr_mss_stream_apply_master_gain();
}
