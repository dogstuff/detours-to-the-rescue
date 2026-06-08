#include "mss_private.h"
#include "sidecar_private.h"

#include <dttr_core.h>
#include <dttr_log.h>

#include <SDL3/SDL.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <windows.h>

typedef struct {
	const char *hook_name;
	const char *import_name;
	void *callback;
	DTTR_Core_Hook *handle;
} mss_import_hook;

static const char *dttr_mss_result_detail(DTTR_Result result) {
	return result.message ? result.message : DTTR_StatusName(result.status);
}

static mss_import_hook mss_import_hooks[] = {
	{"dttr_hook_mss_ail_allocate_sample_handle",
	 "_AIL_allocate_sample_handle@4",
	 dttr_mss_ail_allocate_sample_handle},
	{"dttr_hook_mss_ail_close_stream", "_AIL_close_stream@4", dttr_mss_ail_close_stream},
	{"dttr_hook_mss_ail_end_sample", "_AIL_end_sample@4", dttr_mss_ail_end_sample},
	{"dttr_hook_mss_ail_get_preference",
	 "_AIL_get_preference@4",
	 dttr_mss_ail_get_preference},
	{"dttr_hook_mss_ail_init_sample", "_AIL_init_sample@4", dttr_mss_ail_init_sample},
	{"dttr_hook_mss_ail_open_stream", "_AIL_open_stream@12", dttr_mss_ail_open_stream},
	{"dttr_hook_mss_ail_pause_stream", "_AIL_pause_stream@8", dttr_mss_ail_pause_stream},
	{"dttr_hook_mss_ail_release_sample_handle",
	 "_AIL_release_sample_handle@4",
	 dttr_mss_ail_release_sample_handle},
	{"dttr_hook_mss_ail_sample_playback_rate",
	 "_AIL_sample_playback_rate@4",
	 dttr_mss_ail_sample_playback_rate},
	{"dttr_hook_mss_ail_sample_status",
	 "_AIL_sample_status@4",
	 dttr_mss_ail_sample_status},
	{"dttr_hook_mss_ail_set_digital_master_volume",
	 "_AIL_set_digital_master_volume@8",
	 dttr_mss_ail_set_digital_master_volume},
	{"dttr_hook_mss_ail_set_preference",
	 "_AIL_set_preference@8",
	 dttr_mss_ail_set_preference},
	{"dttr_hook_mss_ail_set_sample_file",
	 "_AIL_set_sample_file@12",
	 dttr_mss_ail_set_sample_file},
	{"dttr_hook_mss_ail_set_sample_loop_count",
	 "_AIL_set_sample_loop_count@8",
	 dttr_mss_ail_set_sample_loop_count},
	{"dttr_hook_mss_ail_set_sample_pan",
	 "_AIL_set_sample_pan@8",
	 dttr_mss_ail_set_sample_pan},
	{"dttr_hook_mss_ail_set_sample_playback_rate",
	 "_AIL_set_sample_playback_rate@8",
	 dttr_mss_ail_set_sample_playback_rate},
	{"dttr_hook_mss_ail_set_sample_volume",
	 "_AIL_set_sample_volume@8",
	 dttr_mss_ail_set_sample_volume},
	{"dttr_hook_mss_ail_set_stream_loop_count",
	 "_AIL_set_stream_loop_count@8",
	 dttr_mss_ail_set_stream_loop_count},
	{"dttr_hook_mss_ail_set_stream_volume",
	 "_AIL_set_stream_volume@8",
	 dttr_mss_ail_set_stream_volume},
	{"dttr_hook_mss_ail_shutdown", "_AIL_shutdown@0", dttr_mss_ail_shutdown},
	{"dttr_hook_mss_ail_start_sample", "_AIL_start_sample@4", dttr_mss_ail_start_sample},
	{"dttr_hook_mss_ail_start_stream", "_AIL_start_stream@4", dttr_mss_ail_start_stream},
	{"dttr_hook_mss_ail_startup", "_AIL_startup@0", dttr_mss_ail_startup},
	{"dttr_hook_mss_ail_stop_sample", "_AIL_stop_sample@4", dttr_mss_ail_stop_sample},
	{"dttr_hook_mss_ail_stream_status",
	 "_AIL_stream_status@4",
	 dttr_mss_ail_stream_status},
	{"dttr_hook_mss_ail_waveOutClose", "_AIL_waveOutClose@4", dttr_mss_ail_waveOutClose},
	{"dttr_hook_mss_ail_waveOutOpen", "_AIL_waveOutOpen@16", dttr_mss_ail_waveOutOpen},
};

// Converts a Miles WAVEFORMAT block into the SDL mixer spec.
static bool wave_format_spec(const void *format, SDL_AudioSpec *spec) {
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

// Tears down the SDL-backed MSS mixer and resets driver state.
void dttr_mss_sdl_shutdown() {
	dttr_mss_stream_shutdown_all();
	dttr_mss_sample_shutdown_all();
	dttr_mss_core_destroy_mixer();
	dttr_mss_core_reset_driver_open_count();
	dttr_mss_core_set_master_gain(DTTR_MSS_DEFAULT_MASTER_GAIN);
}

// Installs one pointer hook for a Miles Sound System import.
static bool install_pointer_hook(
	const DTTR_Mods_Context *ctx,
	const char *name,
	DTTR_Core_Hook **handle,
	uintptr_t addr,
	void *callback
) {
	if (*handle) {
		return true;
	}

	DTTR_Result
		result = DTTR_Core_HookPointer(&ctx->runtime, addr, callback, NULL, handle);

	if (!DTTR_ResultOK(result)) {
		DTTR_MODS_LOG_ERROR(
			ctx,
			"%s: pointer hook failed: %s",
			name,
			dttr_mss_result_detail(result)
		);
		return false;
	}

	DTTR_MODS_LOG_DEBUG(ctx, "Installed %s at 0x%08X", name, (unsigned)addr);
	return true;
}

// Installs the SDL replacement for one named Miles Sound System import.
static bool install_mss_import_hook(
	const DTTR_Mods_Context *ctx,
	const char *name,
	uintptr_t site
) {
	for (size_t i = 0; i < DTTR_ARRAY_COUNT(mss_import_hooks); i++) {
		mss_import_hook *hook = &mss_import_hooks[i];

		if (strcmp(name, hook->import_name) != 0) {
			continue;
		}

		return install_pointer_hook(
			ctx,
			hook->hook_name,
			&hook->handle,
			site,
			hook->callback
		);
	}

	return false;
}

// Installs SDL replacements for the imports in one Miles descriptor.
static bool install_mss_import_descriptor(
	const DTTR_Mods_Context *ctx,
	uint8_t *base,
	IMAGE_IMPORT_DESCRIPTOR *desc
) {
	bool ok = true;
	IMAGE_THUNK_DATA *name_thunk = (IMAGE_THUNK_DATA *)(base + desc->OriginalFirstThunk);
	IMAGE_THUNK_DATA *addr_thunk = (IMAGE_THUNK_DATA *)(base + desc->FirstThunk);

	for (; name_thunk->u1.AddressOfData; name_thunk++, addr_thunk++) {
		if (IMAGE_SNAP_BY_ORDINAL(name_thunk->u1.Ordinal)) {
			continue;
		}

		IMAGE_IMPORT_BY_NAME
		*import_name = (IMAGE_IMPORT_BY_NAME *)(base + name_thunk->u1.AddressOfData);
		if (!install_mss_import_hook(
				ctx,
				(const char *)import_name->Name,
				(uintptr_t)&addr_thunk->u1.Function
			)) {
			DTTR_MODS_LOG_ERROR(
				ctx,
				"Unhandled or unhooked MSS32 import: %s",
				import_name->Name
			);
			ok = false;
		}
	}

	return ok;
}

// Releases Miles import hooks owned by the SDL-backed MSS shim.
void dttr_mss_sdl_release_hooks() {
	for (size_t i = 0; i < DTTR_ARRAY_COUNT(mss_import_hooks); i++) {
		mss_import_hook *hook = &mss_import_hooks[i];

		if (!hook->handle) {
			continue;
		}

		DTTR_Result result = DTTR_Core_Unhook(hook->handle);
		if (!DTTR_ResultOK(result)) {
			DTTR_LOG_ERROR(
				"%s: pointer unhook failed: %s",
				hook->hook_name,
				dttr_mss_result_detail(result)
			);
			continue;
		}

		hook->handle = NULL;
	}
}

// Installs SDL-backed replacements for the game's mss32.dll imports.
bool dttr_mss_sdl_install_hooks(const DTTR_Mods_Context *ctx) {
	HMODULE module = ctx->runtime.game_module;
	uint8_t *base = (uint8_t *)module;
	IMAGE_DOS_HEADER *dos = (IMAGE_DOS_HEADER *)base;
	IMAGE_NT_HEADERS *nt = (IMAGE_NT_HEADERS *)(base + dos->e_lfanew);
	IMAGE_DATA_DIRECTORY imports_dir = nt->OptionalHeader
										   .DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
	IMAGE_IMPORT_DESCRIPTOR
	*desc = (IMAGE_IMPORT_DESCRIPTOR *)(base + imports_dir.VirtualAddress);

	for (; desc && desc->Name; desc++) {
		if (_stricmp((const char *)(base + desc->Name), "mss32.dll") != 0) {
			continue;
		}

		return install_mss_import_descriptor(ctx, base, desc);
	}

	DTTR_MODS_LOG_ERROR(ctx, "mss32.dll import descriptor not found");
	return false;
}

// Converts Miles master volume into SDL mixer gain.
static float master_gain_for_volume(int volume) {
	if (volume <= 0) {
		return 0.0f;
	}

	if (volume >= DTTR_MSS_DEFAULT_VOLUME) {
		return 1.0f;
	}

	return (float)volume / DTTR_MSS_MAX_VOLUME;
}

// Starts the SDL-backed MSS shim for Miles AIL startup.
int __stdcall dttr_mss_ail_startup() {
	dttr_mss_core_ensure_preferences();
	return dttr_mss_core_ensure_mix_initialized() ? 1 : 0;
}

// Shuts down the SDL-backed MSS shim for Miles AIL shutdown.
void __stdcall dttr_mss_ail_shutdown() {
	dttr_mss_sdl_shutdown();
}

// Stores one Miles AIL preference in the SDL-backed MSS shim.
int __stdcall dttr_mss_ail_set_preference(unsigned int preference, int value) {
	return dttr_mss_core_set_preference(preference, value);
}

// Reads one Miles AIL preference from the SDL-backed MSS shim.
int __stdcall dttr_mss_ail_get_preference(unsigned int preference) {
	return dttr_mss_core_get_preference(preference);
}

// Opens the SDL-backed mixer for Miles AIL waveOut.
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

	if (wave_format_spec(format, &desired_spec)) {
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

// Closes one Miles AIL waveOut driver reference.
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

// Applies Miles digital master volume to all SDL-backed playback.
void __stdcall dttr_mss_ail_set_digital_master_volume(void *driver, int volume) {
	dttr_mss_core_set_master_gain(master_gain_for_volume(volume));
	dttr_mss_sample_apply_master_gain();
	dttr_mss_stream_apply_master_gain();
}
