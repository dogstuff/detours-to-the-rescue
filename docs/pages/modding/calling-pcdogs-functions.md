# Calling Game Functions

The SDK gives you typed PCDOGS wrappers for known game functions. Use those wrappers unless you are working on an unsupported patch that needs a raw address.

## API Shape

For the generated function `Movie_PlayFile`, the PCDOGS wrapper exposes:

- `DTTR_PCDOGS_F_MoviePlayFile->IsCallable(&ctx->runtime)`
- `DTTR_PCDOGS_F_MoviePlayFile->Try(&ctx->runtime, args..., out_ret)`
- `DTTR_PCDOGS_F_MoviePlayFile->Call(&ctx->runtime, args..., fallback_ret)`
- `DTTR_PCDOGS_F_MoviePlayFile->HookKind()`
- `DTTR_PCDOGS_F_MoviePlayFile->HookPrologueSize()`
- `DTTR_PCDOGS_F_MoviePlayFile_proto`

For raw address lookup, see [Manually Resolving Symbols](resolving-symbols.md).

```c
#include <dttr_sdk.h>
```

## Callable Checks

Check callability before your mod depends on a direct call in the current executable.

```c
static bool can_call_movie_play_file;

DTTR_MODS_INIT {
    can_call_movie_play_file = DTTR_PCDOGS_F_MoviePlayFile->IsCallable(&ctx->runtime);

    DTTR_MODS_LOG_INFO(
        ctx,
        "Movie_PlayFile callable: %s",
        can_call_movie_play_file ? "yes" : "no"
    );

    return true;
}
```

## Fallback Calls

`DTTR_PCDOGS_*->Call()` calls the game function when available. If the symbol is not callable, it returns the fallback value and leaves game-owned output parameters untouched.

```c
// Returns 0 when Timer_GetRawTickCount is unavailable.
int32_t ticks = DTTR_PCDOGS_F_TimerGetRawTickCount->Call(&ctx->runtime, 0);
```

Use `Try()` when a missing symbol should disable a feature or produce a log instead of quietly taking a fallback value.

## Guarded Calls

`DTTR_PCDOGS_*->Try()` returns whether the SDK called the game function.

```c
static bool try_play_movie_path(
    const DTTR_Mods_Context *ctx,
    const char *movie_path,
    BOOL *out_played
) {
    if (!out_played) {
        return false;
    }

    *out_played = FALSE;
    return DTTR_PCDOGS_F_MoviePlayFile->Try(
        &ctx->runtime,
        movie_path,
        0,
        out_played
    );
}
```

## Game Pointers

When a wrapper needs a game-owned pointer, check the SDK declaration for the pointer type it expects.

```c
static BOOL actor_is_close(
    const DTTR_Mods_Context *ctx,
    DTTR_PCDOGS_T_Actor_State *actor
) {
    // Returns FALSE when Camera_CheckActorDistance is unavailable.
    return DTTR_PCDOGS_F_CameraCheckActorDistance->Call(
        &ctx->runtime,
        actor,
        FALSE
    );
}
```

## Hook Metadata

| Kind | Generated helper support | Prologue meaning |
| --- | --- | --- |
| `DTTR_PCDOGS_HOOK_REL32` | `Hook()` and `PatchSpec()` install a trampoline `E9 <rel32>` hook. | Decoded trampoline prologue size. |
| `DTTR_PCDOGS_HOOK_HOTPATCH` | Metadata only; generated helpers fail closed. | Entry-window size, not the five-byte pre-entry slot. |

For same-address hook chaining rules, see [Hooking Game Functions](hooking-functions.md). Before passing a generated address to a low-level core hook API, check `HookKind()`.
