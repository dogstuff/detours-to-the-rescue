# Patches

This page tracks the runtime hooks and byte patches installed by the sidecar. It is meant as a map back to the code, not as a patching tutorial.

Most sites come from `dttr_hook_sigscan()` against the loaded game module. In signatures, `?` bytes are wildcards from the paired mask.

## Terms

- IAT hooks replace an import-address-table slot.
- Jump hooks replace the first five bytes of the matched assembly with `E9 <rel32>`.
- Trampoline hooks use the same jump patch, but keep a callable copy of the original prologue.

## Bootstrap

| Site | Signature / site | Patch | Original target | Replacement target | Notes |
| --- | --- | --- | --- | --- | --- |
| `dttr_hook_win_main` | `83 EC 40 53 8B 5C 24` | `E9 <rel32>` at the matched function entry | Game `WinMain`-style entrypoint | `dttr_hook_win_main_callback` | Installed before normal sidecar runtime setup. The callback initializes config, SDL, graphics, data pointers, hooks, movies, audio, and components, then drives the original game loop. |

## Game data and process fixes

| Site | Signature / site | Patch | Original target | Replacement target | Notes |
| --- | --- | --- | --- | --- | --- |
| `dttr_crt_open_file_with_mode` | `E8 ?? ?? ?? ?? 85 C0 75 ?? C3` | Resolve only | Target of the matched `CALL rel32` | — | Finds the game's lower-level CRT-style open routine so DttR can still delegate successful resolved paths back to the original file opener. |
| `dttr_crt_hook_open_file` | `6A 40 FF 74 24 0C FF 74 24 0C E8` | `E9 <rel32>` at match | Game file-open wrapper | `dttr_crt_hook_open_file_callback` | Routes file reads through DttR path resolution, case-insensitive lookup, ISO paths, and safe failure handling. |
| `dttr_hook_resolve_pcdogs_path` | `51 8D 44 24 ?? 57` | Optional `E9 <rel32>` at match | Game directory resolver | `dttr_hook_resolve_pcdogs_path_callback` | Fixes later releases that mis-detect the game directory when an earlier path segment contains `p`. |
| `dttr_hook_cleanup_level_assets` | `6A 01 E8 ?? ?? ?? ?? E8 ?? ?? ?? ?? A1 ?? ?? ?? ?? 50 E8 ?? ?? ?? ?? 8B 0D ?? ?? ?? ?? 51 E8 ?? ?? ?? ?? 8B 15 ?? ?? ?? ?? 52 E8 ?? ?? ?? ?? A1 ?? ?? ?? ?? 50 E8 ?? ?? ?? ?? 8B 0D ?? ?? ?? ?? 51 E8 ?? ?? ?? ?? 83 ?? ??` | Trampoline `E9 <rel32>` at match | Original cleanup-level-assets routine, available through `dttr_hook_cleanup_level_assets_trampoline` | `dttr_hook_cleanup_level_assets_callback` | Runs the original cleanup, then clears stale level asset pointers. |

## Graphics

### DirectDraw import hooks

| Site | Signature / site | Patch | Original IAT target | Replacement target | Notes |
| --- | --- | --- | --- | --- | --- |
| `dttr_hook_directdraw_create_ex` | Finds `E8 ?? ?? ?? ?? 85 C0 7D ?? 68 ?? ?? ?? ?? 6A 00 50 E8`, then patches `DTTR_FF25_ADDR(DTTR_E8_TARGET(match_))` | IAT hook | IAT/thunk target for `DirectDrawCreateEx` | `dttr_hook_directdraw_create_ex_callback` | Returns DttR's DirectDraw 7 translator and stores it in the game-side DirectDraw pointer. |
| `dttr_hook_directdraw_enumerate_ex_a` | Finds `E8 ?? ?? ?? ?? 8B F0 A1`, then patches `DTTR_FF25_ADDR(DTTR_E8_TARGET(match_))` | IAT hook | IAT/thunk target for `DirectDrawEnumerateExA` | `dttr_hook_directdraw_enumerate_ex_a_callback` | Enumerates DttR's virtual display device. |

### Subpixel vertex precision byte patches

These patches are installed only when `vertex_precision` is set to `subpixel`.

| Site | Signature / site | Offset | Patch bytes / assembly | Original target | Notes |
| --- | --- | ---: | --- | --- | --- |
| `dttr_hook_precision_fast_path` | `83 F8 ?? 7C ?? D9 43 ?? D8 1D ?? ?? ?? ?? DF E0 F6 C4 41 0F 85 ?? ?? ?? ??` | `+19` | `E9 BA 00 00 00 90` (`jmp +0xBA; nop`) | Original fast-path conditional branch | Skips the integer-conversion fast path that collapses vertex precision. |
| `dttr_hook_precision_batch_limit_a` | `8B 08 EB ?? A1 ?? ?? ?? ?? 8B 0D ?? ?? ?? ?? 3B C1` | `+17` | `90 90` (`nop; nop`) | Original batch-limit branch/check tail | Keeps the precision path from bailing out through the old batch limit check. |
| `dttr_hook_precision_batch_limit_b` | `83 C1 14 4E 75 ?? A1 ?? ?? ?? ?? 8B 0D ?? ?? ?? ?? 3B C1` | `+19` | `90 90` (`nop; nop`) | Original batch-limit branch/check tail | Same batch-limit fix for the second matched loop. |
| `dttr_hook_precision_ftol_x` | `DB 44 24 30 D9 1F` | `-15` | `D9 1F 90 90 90` (`fstp dword ptr [edi]; nop; nop; nop`) | Original x-coordinate float-to-int conversion sequence | Stores the x coordinate directly as float instead of converting through integer precision. |
| `dttr_hook_precision_mov_x` | `DB 44 24 30 D9 1F` | `-10` | `90 90 90 90` (`nop` x4) | Original x-coordinate integer move | Removes the integer move paired with the old x conversion. |
| `dttr_hook_precision_fstp2_x` | `8D AE ?? ?? ?? ?? DB 44 24 30 D9 1F` | `+10` | `90 90` (`nop; nop`) | Original second x-coordinate store | Removes the second store from the old x conversion sequence. |
| `dttr_hook_precision_fild_x` | `8D AE ?? ?? ?? ?? DB 44 24 30` | `+6` | `90 90 90 90` (`nop` x4) | Original x-coordinate `fild` reload | Removes the integer reload for x. |
| `dttr_hook_precision_ftol_y` | `8B 54 24 18 89 44 24 30` | `-5` | `D9 5D 00 90 90` (`fstp dword ptr [ebp+0]; nop; nop`) | Original y-coordinate float-to-int conversion sequence | Stores the y coordinate directly as float. |
| `dttr_hook_precision_mov_y` | `8B 54 24 18 89 44 24 30` | `+4` | `90 90 90 90` (`nop` x4) | Original y-coordinate integer move | Removes the integer move paired with the old y conversion. |
| `dttr_hook_precision_fstp2_y` | `83 C0 14 50 55 D9 5D 00` | `+5` | `90 90 90` (`nop` x3) | Original second y-coordinate store | Removes the second store from the old y conversion sequence. |
| `dttr_hook_precision_fild_y` | `52 DB 44 24 34` | `+1` | `90 90 90 90` (`nop` x4) | Original y-coordinate `fild` reload | Removes the integer reload for y. |
| `dttr_hook_render_quad_snap` | `53 8B 5C 24 14 55 33 C9 56 57 85 DB` | `+0` | `C3` (`ret`) | Original render-quad snap helper | Optional compatibility patch used with the subpixel path to stop render-quad snapping. |

## Input

| Site | Signature / site | Patch | Original IAT target | Replacement target | Notes |
| --- | --- | --- | --- | --- | --- |
| `dttr_inputs_hook_dinput_poll` | `56 8B 74 24 ?? 56 8B 06` | `E9 <rel32>` at match | Game DirectInput joystick poll function | `dttr_inputs_hook_dinput_poll_callback` | Maps SDL gamepad state into the game's joystick layout. |
| `dttr_inputs_hook_get_async_key_state` | Finds `8B 1D ?? ?? ?? ?? 56 33 F6`, then patches `*(uint32_t *)(match_ + 2)` | IAT hook | IAT-style slot loaded by `mov ebx, [GetAsyncKeyStateSlot]` | `dttr_inputs_hook_get_async_key_state_callback` | Routes keyboard state through SDL and limits input to the SDL window. |

## Audio

### Audio trampolines

These guard game audio paths around the SDL-backed MSS shim.

| Site | Signature / site | Patch | Original target | Replacement target | Notes |
| --- | --- | --- | --- | --- | --- |
| `dttr_hook_audio_init_system` | `81 EC 90 ?? ?? ?? 55 56 57 FF 15` | Trampoline `E9 <rel32>` at match | Game audio system init, available through `dttr_hook_audio_init_system_trampoline` | `dttr_hook_audio_init_system_callback` | Skips MSS init when SDL reports no playback devices. |
| `dttr_hook_audio_stop_all_sounds` | `A1 ?? ?? ?? ?? 6A ?? 50 FF 15` | Trampoline `E9 <rel32>` at match | Game stop-all-sounds routine, available through `dttr_hook_audio_stop_all_sounds_trampoline` | `dttr_hook_audio_stop_all_sounds_callback` | Stops DttR's SDL samples first, then calls the original only if a digital driver exists. |
| `dttr_hook_audio_init_level_audio` | `A1 ?? ?? ?? ?? 6A 7F 50 FF 15` | Trampoline `E9 <rel32>` at match | Game level-audio init routine, available through `dttr_hook_audio_init_level_audio_trampoline` | `dttr_hook_audio_init_level_audio_callback` | Guards level audio init when no driver is active. |
| `dttr_hook_audio_stop_all_samples` | `56 57 8B 3D ?? ?? ?? ?? BE` | Trampoline `E9 <rel32>` at match | Game stop-all-samples routine, available through `dttr_hook_audio_stop_all_samples_trampoline` | `dttr_hook_audio_stop_all_samples_callback` | Stops DttR's SDL samples first, then calls the original only if a digital driver exists. |

### Miles Sound System import hooks

DttR patches the `mss32.dll` import address table by name. The original target is the imported MSS32 function in the game's IAT slot; DttR keeps it only so the hook can be restored.

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

| Site | Signature / site | Patch | Original target | Replacement target | Notes |
| --- | --- | --- | --- | --- | --- |
| `dttr_movies_hook_movie_play_file` | `8B 44 24 08 8B 0D ?? ?? ?? ?? 8B 54 24 04 56 50` | `E9 <rel32>` at match | Game `Movie_PlayFile` routine | `dttr_movies_hook_movie_play_file_callback` | Replaces MCI playback with DttR movie playback through FFmpeg and SDL audio/video output. |
