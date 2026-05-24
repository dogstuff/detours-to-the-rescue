# Patches

This page lists the runtime hooks and byte patches DttR installs.

DttR resolves most sites with `DTTR_Core_HookSigscan()` against the loaded game module. In signatures, `?` bytes are mask wildcards.

## Terms

- IAT hooks replace an import-address-table slot.
- Jump hooks replace the first five bytes at the match with `E9 <rel32>`.
- Trampoline hooks use the same jump patch and keep a callable copy of the original prologue.

## Bootstrap

| Site | Signature | Target | Effect |
| --- | --- | --- | --- |
| `dttr_hook_win_main` | Signature `83 EC 40 53 8B 5C 24`; jump hook at the matched function entry | Game `WinMain`-style entrypoint -> `DTTR_Hook_WinMainCallback` | Installs before sidecar setup. The callback initializes config, SDL, graphics, data pointers, hooks, movies, audio, and mods, then drives the original game loop. |

## Game Data and Process Fixes

| Site | Signature | Target | Effect |
| --- | --- | --- | --- |
| `DTTR_PCDOGS_F_FileOpenWithMode` | Signature `E8 ?? ?? ?? ?? 85 C0 75 ?? C3`; resolve the matched `CALL rel32` target | Game lower-level CRT-style open routine | Hands resolved paths back to the original file opener. |
| `dttr_crt_hook_open_file` | Signature `6A 40 FF 74 24 0C FF 74 24 0C E8`; jump hook at match | Game file-open wrapper -> `dttr_crt_hook_open_file_callback` | Routes file reads through DttR path resolution: case-insensitive lookup, ISO paths, and safe failure handling. |
| `dttr_hook_resolve_pcdogs_path` | Signature `51 8D 44 24 ?? 57`; optional jump hook at match | Game directory resolver -> `dttr_hook_resolve_pcdogs_path_callback` | Fixes later releases that mis-detect the game directory when an earlier path segment contains `p`. |
| `dttr_hook_cleanup_title_resources` | Signature `6A 01 E8 ?? ?? ?? ?? E8 ?? ?? ?? ?? A1 ?? ?? ?? ?? 50 E8 ?? ?? ?? ?? 8B 0D ?? ?? ?? ?? 51 E8 ?? ?? ?? ?? 8B 15 ?? ?? ?? ?? 52 E8 ?? ?? ?? ?? A1 ?? ?? ?? ?? 50 E8 ?? ?? ?? ?? 8B 0D ?? ?? ?? ?? 51 E8 ?? ?? ?? ?? 83 ?? ??`; trampoline hook at match | Original cleanup-title-resources routine, captured as `dttr_hook_cleanup_title_resources_original` -> `dttr_hook_cleanup_title_resources_callback` | Runs the original cleanup, then clears stale title resource pointers. |

## Graphics

### DirectDraw Import Hooks

| Site | Signature | Target | Effect |
| --- | --- | --- | --- |
| `dttr_hook_directdraw_create_ex` | Generated `DDraw_CreateEx` import-thunk symbol; trampoline hook on import thunk | `ddraw!DirectDrawCreateEx` thunk body (`FF 25 <IAT slot>`) -> `dttr_hook_directdraw_create_ex_callback` | Returns DttR's DirectDraw 7 translator and stores it in the game-side DirectDraw pointer. |
| `dttr_hook_directdraw_enumerate_ex_a` | Generated `DirectX_DirectDrawEnumerateExA` import-thunk symbol; trampoline hook on import thunk | `ddraw!DirectDrawEnumerateExA` thunk body (`FF 25 <IAT slot>`) -> `dttr_hook_directdraw_enumerate_ex_a_callback` | Enumerates DttR's virtual display device. |

### Subpixel Vertex Precision Byte Patches

DttR installs these patches only when `vertex_precision` is `subpixel`.

| Site | Signature | Bytes | Effect |
| --- | --- | --- | --- |
| `dttr_hook_precision_fast_path` | Signature `83 F8 ?? 7C ?? D9 43 ?? D8 1D ?? ?? ?? ?? DF E0 F6 C4 41 0F 85 ?? ?? ?? ??`, offset `+19` | `E9 BA 00 00 00 90` (`jmp +0xBA; nop`) | Skips the original fast-path conditional branch that collapses vertex precision. |
| `dttr_hook_precision_batch_limit_a` | Signature `8B 08 EB ?? A1 ?? ?? ?? ?? 8B 0D ?? ?? ?? ?? 3B C1`, offset `+17` | `90 90` (`nop; nop`) | Keeps the precision path from leaving through the original batch-limit check. |
| `dttr_hook_precision_batch_limit_b` | Signature `83 C1 14 4E 75 ?? A1 ?? ?? ?? ?? 8B 0D ?? ?? ?? ?? 3B C1`, offset `+19` | `90 90` (`nop; nop`) | Applies the same batch-limit fix for the second matched loop. |
| `dttr_hook_precision_ftol_x` | Signature `DB 44 24 30 D9 1F`, offset `-15` | `D9 1F 90 90 90` (`fstp dword ptr [edi]; nop; nop; nop`) | Stores the x coordinate directly as float, preserving subpixel precision. |
| `dttr_hook_precision_mov_x` | Signature `DB 44 24 30 D9 1F`, offset `-10` | `90 90 90 90` (`nop` x4) | Removes the integer move paired with the original x conversion. |
| `dttr_hook_precision_fstp2_x` | Signature `8D AE ?? ?? ?? ?? DB 44 24 30 D9 1F`, offset `+10` | `90 90` (`nop; nop`) | Removes the second store from the original x conversion sequence. |
| `dttr_hook_precision_fild_x` | Signature `8D AE ?? ?? ?? ?? DB 44 24 30`, offset `+6` | `90 90 90 90` (`nop` x4) | Removes the integer reload for x. |
| `dttr_hook_precision_ftol_y` | Signature `8B 54 24 18 89 44 24 30`, offset `-5` | `D9 5D 00 90 90` (`fstp dword ptr [ebp+0]; nop; nop`) | Stores the y coordinate directly as float. |
| `dttr_hook_precision_mov_y` | Signature `8B 54 24 18 89 44 24 30`, offset `+4` | `90 90 90 90` (`nop` x4) | Removes the integer move paired with the original y conversion. |
| `dttr_hook_precision_fstp2_y` | Signature `83 C0 14 50 55 D9 5D 00`, offset `+5` | `90 90 90` (`nop` x3) | Removes the second store from the original y conversion sequence. |
| `dttr_hook_precision_fild_y` | Signature `52 DB 44 24 34`, offset `+1` | `90 90 90 90` (`nop` x4) | Removes the integer reload for y. |
| `dttr_hook_render_quad_snap` | Signature `53 8B 5C 24 14 55 33 C9 56 57 85 DB`, offset `+0` | `C3` (`ret`) | Optional compatibility patch for the subpixel path; stops the original render-quad snap helper. |

## Input

| Site | Signature | Target | Effect |
| --- | --- | --- | --- |
| `dttr_inputs_hook_dinput_poll` | Signature `56 8B 74 24 ?? 56 8B 06`; jump hook at match | Game DirectInput joystick poll function -> `dttr_inputs_hook_dinput_poll_callback` | Maps SDL gamepad state into the game's joystick layout. |
| `dttr_inputs_hook_get_async_key_state` | Finds `8B 1D ?? ?? ?? ?? 56 33 F6`, then patches `*(uint32_t *)(match_ + 2)` as an IAT hook | IAT-style slot loaded by `mov ebx, [GetAsyncKeyStateSlot]` -> `dttr_inputs_hook_get_async_key_state_callback` | Routes keyboard state through SDL and limits input to the SDL window. |

## Audio

### Audio Trampolines

These hooks keep game audio paths safe while DttR routes MSS through SDL.

| Site | Signature | Target | Effect |
| --- | --- | --- | --- |
| `DTTR_PCDOGS_F_AudioInitializeSystem` | Signature `81 EC 90 ?? ?? ?? 55 56 57 FF 15`; generated `Hook()` helper | Game audio system init, captured as `audio_init_system_original` -> `audio_init_system_detour` | Skips MSS init when SDL reports no playback devices. |
| `DTTR_PCDOGS_F_AudioStopAllSounds` | Signature `A1 ?? ?? ?? ?? 6A ?? 50 FF 15`; generated `Hook()` helper | Game stop-all-sounds routine, captured as `audio_stop_all_sounds_original` -> `audio_stop_all_sounds_detour` | Stops DttR's SDL samples first, then calls the original only if a digital driver exists. |
| `DTTR_PCDOGS_F_AudioInitializeLevelAudio` | Signature `A1 ?? ?? ?? ?? 6A 7F 50 FF 15`; generated `Hook()` helper | Game level-audio init routine, captured as `audio_init_level_audio_original` -> `audio_init_level_audio_detour` | Guards level audio init when no driver is active. |
| `DTTR_PCDOGS_F_AudioStopAllSamples` | Signature `56 57 8B 3D ?? ?? ?? ?? BE`; generated `Hook()` helper | Game stop-all-samples routine, captured as `audio_stop_all_samples_original` -> `audio_stop_all_samples_detour` | Stops DttR's SDL samples first, then calls the original only if a digital driver exists. |

### Miles Sound System Import Hooks

DttR patches the `mss32.dll` import address table by name. It keeps the original MSS32 function from each game IAT slot so the hook can be restored.

| Hook | Import name / site | Replacement target |
| --- | --- | --- |
| `dttr_hook_mss_ail_allocate_sample_handle` | `_AIL_allocate_sample_handle@4` | `dttr_mss_ail_allocate_sample_handle` |
| `dttr_hook_mss_ail_close_stream` | `_AIL_close_stream@4` | `dttr_mss_ail_close_stream` |
| `dttr_hook_mss_ail_end_sample` | `_AIL_end_sample@4` | `dttr_mss_ail_end_sample` |
| `dttr_hook_mss_ail_get_preference` | `_AIL_get_preference@4` | `dttr_mss_ail_get_preference` |
| `dttr_hook_mss_ail_init_sample` | `_AIL_init_sample@4` | `dttr_mss_ail_init_sample` |
| `dttr_hook_mss_ail_open_stream` | `_AIL_open_stream@12` | `dttr_mss_ail_open_stream` |
| `dttr_hook_mss_ail_pause_stream` | `_AIL_pause_stream@8` | `dttr_mss_ail_pause_stream` |
| `dttr_hook_mss_ail_release_sample_handle` | `_AIL_release_sample_handle@4` | `dttr_mss_ail_release_sample_handle` |
| `dttr_hook_mss_ail_sample_playback_rate` | `_AIL_sample_playback_rate@4` | `dttr_mss_ail_sample_playback_rate` |
| `dttr_hook_mss_ail_sample_status` | `_AIL_sample_status@4` | `dttr_mss_ail_sample_status` |
| `dttr_hook_mss_ail_set_digital_master_volume` | `_AIL_set_digital_master_volume@8` | `dttr_mss_ail_set_digital_master_volume` |
| `dttr_hook_mss_ail_set_preference` | `_AIL_set_preference@8` | `dttr_mss_ail_set_preference` |
| `dttr_hook_mss_ail_set_sample_file` | `_AIL_set_sample_file@12` | `dttr_mss_ail_set_sample_file` |
| `dttr_hook_mss_ail_set_sample_loop_count` | `_AIL_set_sample_loop_count@8` | `dttr_mss_ail_set_sample_loop_count` |
| `dttr_hook_mss_ail_set_sample_pan` | `_AIL_set_sample_pan@8` | `dttr_mss_ail_set_sample_pan` |
| `dttr_hook_mss_ail_set_sample_playback_rate` | `_AIL_set_sample_playback_rate@8` | `dttr_mss_ail_set_sample_playback_rate` |
| `dttr_hook_mss_ail_set_sample_volume` | `_AIL_set_sample_volume@8` | `dttr_mss_ail_set_sample_volume` |
| `dttr_hook_mss_ail_set_stream_loop_count` | `_AIL_set_stream_loop_count@8` | `dttr_mss_ail_set_stream_loop_count` |
| `dttr_hook_mss_ail_set_stream_volume` | `_AIL_set_stream_volume@8` | `dttr_mss_ail_set_stream_volume` |
| `dttr_hook_mss_ail_shutdown` | `_AIL_shutdown@0` | `dttr_mss_ail_shutdown` |
| `dttr_hook_mss_ail_start_sample` | `_AIL_start_sample@4` | `dttr_mss_ail_start_sample` |
| `dttr_hook_mss_ail_start_stream` | `_AIL_start_stream@4` | `dttr_mss_ail_start_stream` |
| `dttr_hook_mss_ail_startup` | `_AIL_startup@0` | `dttr_mss_ail_startup` |
| `dttr_hook_mss_ail_stop_sample` | `_AIL_stop_sample@4` | `dttr_mss_ail_stop_sample` |
| `dttr_hook_mss_ail_stream_status` | `_AIL_stream_status@4` | `dttr_mss_ail_stream_status` |
| `dttr_hook_mss_ail_waveOutClose` | `_AIL_waveOutClose@4` | `dttr_mss_ail_waveOutClose` |
| `dttr_hook_mss_ail_waveOutOpen` | `_AIL_waveOutOpen@16` | `dttr_mss_ail_waveOutOpen` |

## Movies

| Site | Signature | Target | Effect |
| --- | --- | --- | --- |
| `dttr_movies_hook_movie_play_file` | Signature `8B 44 24 08 8B 0D ?? ?? ?? ?? 8B 54 24 04 56 50`; jump hook at match | Game `Movie_PlayFile` routine -> `dttr_movies_hook_movie_play_file_callback` | Replaces MCI playback with DttR's FFmpeg/SDL-backed movie player. |
