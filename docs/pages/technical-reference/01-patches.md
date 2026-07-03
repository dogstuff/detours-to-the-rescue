---
title: "Patches Reference"
description: "Technical reference for runtime patches that replace legacy dependencies and adjust game compatibility behavior."
seo_type: "article"
---

# Patches

This page lists the runtime hooks and byte patches DttR installs.

DttR resolves most sites against the loaded game module with `DTTR_Core_HookSigscan()`, which accepts signatures in space-separated hexadecimal byte format with `?` bytes as mask wildcards.

## Terminology

- IAT hooks replace an import-address-table slot. Future calls through that import go directly to DttR's replacement function; the call site itself is unchanged, and the hook may call the saved original import when needed.
- Jump hooks replace the first five bytes at the match with `E9 <rel32>`. Execution that reaches the patched address branches immediately to DttR's callback instead of continuing through the original function body.
- Trampoline hooks use the same jump patch and keep a callable copy of the original prologue. The callback can run custom code, call the trampoline to resume the displaced original instructions and continue into the game code, then return or adjust control flow as needed.

## Bootstrap

| Site | Signature | Target | Effect |
| --- | --- | --- | --- |
| `dttr_hook_win_main` | Signature `83 EC 40 53 8B 5C 24`, jump hook at the matched function entry | Game `Window_RunWinMain`-style entrypoint | Installs before sidecar setup. The callback initializes config, SDL, graphics, data pointers, hooks, movies, audio, and mods, then drives the original game loop. |

## Game Data and Process Fixes

| Site | Signature | Target | Effect |
| --- | --- | --- | --- |
| `DTTR_PCDOGS_F_File_OpenWithMode` | Signature `E8 ?? ?? ?? ?? 85 C0 75 ?? C3`, resolve the matched `CALL rel32` target | Game lower-level CRT-style open routine | Hands resolved paths back to the original file opener. |
| `dttr_crt_hook_open_file` | Signature `6A 40 FF 74 24 0C FF 74 24 0C E8`, jump hook at match | Game file-open wrapper | Routes file reads through DttR path resolution: case-insensitive lookup, ISO paths, and safe failure handling. |
| `dttr_hook_resolve_pcdogs_path` | Signature `51 8D 44 24 ?? 57`, optional jump hook at match | Game directory resolver | Fixes later releases that mis-detect the game directory when an earlier path segment contains `p`. |
| `dttr_hook_cleanup_title_resources` | Signature `6A 01 E8 ?? ?? ?? ?? E8 ?? ?? ?? ?? A1 ?? ?? ?? ?? 50 E8 ?? ?? ?? ?? 8B 0D ?? ?? ?? ?? 51 E8 ?? ?? ?? ?? 8B 15 ?? ?? ?? ?? 52 E8 ?? ?? ?? ?? A1 ?? ?? ?? ?? 50 E8 ?? ?? ?? ?? 8B 0D ?? ?? ?? ?? 51 E8 ?? ?? ?? ?? 83 ?? ??`, trampoline hook at match | Original cleanup-title-resources routine, captured as `dttr_hook_cleanup_title_resources_original` | Runs the original cleanup, then clears stale title resource pointers. |

## Graphics

### DirectDraw Import Hooks

| Site | Signature | Target | Effect |
| --- | --- | --- | --- |
| `dttr_hook_directdraw_create_ex` | `DDraw_CreateEx` import-thunk symbol, trampoline hook on import thunk | `ddraw!DirectDrawCreateEx` thunk body (`FF 25 <IAT slot>`) | Returns DttR's DirectDraw 7 translator and stores it in the game-side DirectDraw pointer. |
| `dttr_hook_directdraw_enumerate_ex_a` | `DDraw_EnumerateExA` import-thunk symbol, trampoline hook on import thunk | `ddraw!DirectDrawEnumerateExA` thunk body (`FF 25 <IAT slot>`) | Enumerates DttR's virtual display device. |

### Subpixel Vertex Precision Byte Patches

DttR installs these patches only when `vertex_precision` is `subpixel`. When
logical scaling is active, the sidecar also expands transformed opaque triangle
meshes slightly on the CPU before backend submission. That small overlap hides
subpixel cracks between adjacent mesh pieces.

| Site | Signature | Bytes | Effect |
| --- | --- | --- | --- |
| `dttr_hook_precision_fast_path` | Signature `83 F8 ?? 7C ?? D9 43 ?? D8 1D ?? ?? ?? ?? DF E0 F6 C4 41 0F 85 ?? ?? ?? ??`, offset `+19` | `E9 BA 00 00 00 90` (`jmp +0xBA` plus `nop`) | Skips the original fast-path conditional branch that collapses vertex precision. |
| `dttr_hook_precision_batch_limit_a` | Signature `8B 08 EB ?? A1 ?? ?? ?? ?? 8B 0D ?? ?? ?? ?? 3B C1`, offset `+17` | `90 90` (two `nop`s) | Keeps the precision path from leaving through the original batch-limit check. |
| `dttr_hook_precision_batch_limit_b` | Signature `83 C1 14 4E 75 ?? A1 ?? ?? ?? ?? 8B 0D ?? ?? ?? ?? 3B C1`, offset `+19` | `90 90` (two `nop`s) | Applies the same batch-limit fix for the second matched loop. |
| `dttr_hook_precision_ftol_x` | Signature `DB 44 24 30 D9 1F`, offset `-15` | `D9 1F 90 90 90` (`fstp dword ptr [edi]` plus three `nop`s) | Stores the x coordinate directly as float, preserving subpixel precision. |
| `dttr_hook_precision_mov_x` | Signature `DB 44 24 30 D9 1F`, offset `-10` | `90 90 90 90` (`nop` x4) | Removes the integer move paired with the original x conversion. |
| `dttr_hook_precision_fstp2_x` | Signature `8D AE ?? ?? ?? ?? DB 44 24 30 D9 1F`, offset `+10` | `90 90` (two `nop`s) | Removes the second store from the original x conversion sequence. |
| `dttr_hook_precision_fild_x` | Signature `8D AE ?? ?? ?? ?? DB 44 24 30`, offset `+6` | `90 90 90 90` (`nop` x4) | Removes the integer reload for x. |
| `dttr_hook_precision_ftol_y` | Signature `8B 54 24 18 89 44 24 30`, offset `-5` | `D9 5D 00 90 90` (`fstp dword ptr [ebp+0]` plus two `nop`s) | Stores the y coordinate directly as float. |
| `dttr_hook_precision_mov_y` | Signature `8B 54 24 18 89 44 24 30`, offset `+4` | `90 90 90 90` (`nop` x4) | Removes the integer move paired with the original y conversion. |
| `dttr_hook_precision_fstp2_y` | Signature `83 C0 14 50 55 D9 5D 00`, offset `+5` | `90 90 90` (`nop` x3) | Removes the second store from the original y conversion sequence. |
| `dttr_hook_precision_fild_y` | Signature `52 DB 44 24 34`, offset `+1` | `90 90 90 90` (`nop` x4) | Removes the integer reload for y. |
| `dttr_hook_render_quad_snap` | Signature `53 8B 5C 24 14 55 33 C9 56 57 85 DB`, offset `+0` | `C3` (`ret`) | Optional compatibility patch for the subpixel path that stops the original render-quad snap helper. |

## Input

The `sidecar/inputs` patch group is assembled from direct input hooks, SDK `PatchSpec()`
rows, and sidecar-local byte patches expanded from `sidecar_inputs_byte_patches.def`.

### Input Hooks

| Site | Signature | Target | Effect |
| --- | --- | --- | --- |
| `dttr_inputs_hook_dinput_poll` | Signature `56 8B 74 24 ?? 56 8B 06`, jump hook at match | Game DirectInput joystick poll function | Maps SDL gamepad state into the game's joystick layout, with left-stick axes neutralized during analog remap. |
| `dttr_inputs_hook_get_async_key_state` | `Video_PlayMovieLoop_GetAsyncKeyStateThunk->PatchSpec()` in the `sidecar/inputs` patch group | IAT-style slot loaded by `mov ebx, [GetAsyncKeyStateSlot]` | Routes movie-loop keyboard polling through SDL scancode-backed key state. |
| `dttr_inputs_hook_is_key_pressed_async` | `Input_IsKeyPressedAsync->PatchSpec()` in the `sidecar/inputs` patch group | Game asynchronous key-state helper | Reads SDL keyboard state for Return and DttR scancode key codes, then falls back to the original helper for legacy virtual keys. |
| `dttr_inputs_hook_get_button_string` | `Input_GetButtonString->PatchSpec()` in the `sidecar/inputs` patch group | Game control-name lookup helper | Returns display names for DttR scancode key codes before falling back to the original button-name table. |
| `dttr_inputs_hook_format_button_name` | Optional `Input_FormatButtonName->PatchSpec()` in the `sidecar/inputs` patch group | Game formatted control-name helper | Writes custom DttR control names into the menu's formatted-name buffers when those buffers are available. |
| `dttr_inputs_hook_get_pressed_button` | `Input_GetPressedButton->PatchSpec()` in the `sidecar/inputs` patch group | Game pressed-button probe used by the controls menu | Lets the controls menu capture DttR scancode/remapped button codes while preserving the original pressed-button probe. |
| `dttr_inputs_hook_rumble` | `Input_TriggerRumbleIfAllowed->PatchSpec()` in the `sidecar/inputs` patch group; available on supported English builds | Game vibration wrapper | Forwards force-feedback to `SDL_RumbleGamepad` when an SDL gamepad is open; falls back to DirectInput otherwise. Respects the game's vibration option. |
| `dttr_inputs_hook_read_gamepad` | `Input_ReadGamepad->PatchSpec()` in the `sidecar/inputs` patch group; available on supported English builds | Game gamepad-to-input-state routine | Applies DttR left-stick analog axes and RZ/buttons while keeping the game's left-stick threshold buttons neutral during analog remap. |
| `dttr_inputs_hook_read_devices` | Optional `Input_ReadDevices->PatchSpec()` in the `sidecar/inputs` patch group | Game combined keyboard/gamepad input routine | Re-applies SDL-backed button bitmasks after the game's device read so direct overrides still work when native gamepad rows are unset. |

### Controls Menu Byte Patches

These byte patches support DttR's controls-menu remap flow where keyboard keys or
controller buttons can be bound to either input column. The byte patches keep
keyboard scancode values valid in the menu; the controls-menu hooks capture and
name both keyboard and controller inputs.

| Site | Signature | Bytes | Effect |
| --- | --- | --- | --- |
| `dttr_inputs_controls_enter_bind_branch` | Signature `E8 ?? ?? ?? ?? 8B F0 83 FE FF 0F 84 ?? ?? ?? ?? 83 FE 0D 0F 84 ?? ?? ?? ?? 83 FE 1B`, offset `+19` | `90 90 90 90 90 90` (`nop` x6) | Removes the native Enter-as-finish branch so Return can be bound like other keys. |
| `dttr_inputs_controls_keyboard_bind_limit` | Signature `3B C7 75 17 81 FE 00 01 00 00 7D 29 8B ?? ?? ?? ?? ?? 89 34`, offset `+4` | `81 FE 00 05 00 00` (`cmp esi, 0x500`) | Raises the keyboard binding limit so DttR keyboard and SDL controller codes fit. |
| `dttr_inputs_controls_keyboard_remap_done_limit` | Signature `3B C7 75 10 81 FE 00 01 00 00 7D 15 89 3D ?? ?? ?? ?? EB 3A 83 F8 01 75 F3 81 FE 00 01 00 00 7D EB`, offset `+4` | `81 FE 00 05 00 00` (`cmp esi, 0x500`) | Applies the raised keyboard limit to the remap-completion path. |

## Audio

### Audio Trampolines

These hooks keep game audio paths safe while DttR routes MSS through SDL.

| Site | Signature | Target | Effect |
| --- | --- | --- | --- |
| `DTTR_PCDOGS_F_Audio_InitializeSystem` | Signature `81 EC 90 ?? ?? ?? 55 56 57 FF 15`, `PatchSpec()` in the `sidecar/audio` patch group | Game audio system init, captured as `audio_init_system_original` | Skips MSS init when SDL reports no playback devices. |
| `DTTR_PCDOGS_F_Audio_StopAllSounds` | Signature `A1 ?? ?? ?? ?? 6A ?? 50 FF 15`, `PatchSpec()` in the `sidecar/audio` patch group | Game stop-all-sounds routine, captured as `audio_stop_all_sounds_original` | Stops DttR's SDL samples first, then calls the original only if a digital driver exists. |
| `DTTR_PCDOGS_F_Audio_InitializeLevelAudio` | Signature `A1 ?? ?? ?? ?? 6A 7F 50 FF 15`, `PatchSpec()` in the `sidecar/audio` patch group | Game level-audio init routine, captured as `audio_init_level_audio_original` | Guards level audio init when no driver is active. |
| `DTTR_PCDOGS_F_Audio_StopAllSamples` | Signature `56 57 8B 3D ?? ?? ?? ?? BE`, `PatchSpec()` in the `sidecar/audio` patch group | Game stop-all-samples routine, captured as `audio_stop_all_samples_original` | Stops DttR's SDL samples first, then calls the original only if a digital driver exists. |

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
| `dttr_movies_hook_movie_play_file` | `PatchSpec()` in the `sidecar/movie` patch group | Game `Video_PlayMovieFile` routine | Replaces MCI playback with DttR's FFmpeg/SDL-backed movie player. |
