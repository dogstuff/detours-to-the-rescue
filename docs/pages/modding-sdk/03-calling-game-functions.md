# Calling Game Functions

Symbol wrapper function helpers are the safest way to call known game functions. Use them instead of casting raw addresses.

Raw addresses are only for unsupported reverse-engineering or low-level patch work.


## Calling game functions through SDK wrappers

The SDK provides typed wrappers for most known game functions.

For example, `Movie_PlayFile` is exposed as `DTTR_PCDOGS_F_MoviePlayFile`.

Use the SDK wrapper instead of casting the address yourself:

- `IsCallable(&ctx->runtime)`: Check whether the function is available and safe to call.
- `Call(&ctx->runtime, args..., out_ret)`: Call the function and return `DTTR_Result`.

## Handling unavailable functions

Check `Call()` when a missing function should disable related behavior, show a warning, or write a log message.

```c
BOOL played = FALSE;
if (!DTTR_ResultOK(DTTR_PCDOGS_F_MoviePlayFile->Call(
        &ctx->runtime,
        movie_path,
        0,
        &played
    ))) {
    DTTR_MODS_LOG_WARN(ctx, "Movie_PlayFile is unavailable");
    return;
}
```

Your mod can then fail closed, warn, or skip the feature instead of pretending the game call worked.

## Checking availability during setup

If your mod requires a game function to be available, check it during `DTTR_MODS_INIT`:

```c
DTTR_MODS_INIT {
    if (!DTTR_PCDOGS_F_MoviePlayFile->IsCallable(&ctx->runtime)) {
        DTTR_MODS_LOG_ERROR(ctx, "Movie_PlayFile is required");
        return false;
    }

    return true;
}
```

Fail init for required functions and check `Call()` results for optional functions.

## Passing game-managed types

When a wrapper takes a game-managed pointer or struct, use the generated SDK type. Guessing a struct layout can cause incorrect reads, corrupt writes, or crashes.
