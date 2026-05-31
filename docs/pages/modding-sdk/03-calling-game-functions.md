# Calling Game Functions

!!! warning "Cross-Region Compatibility Warning"

    The symbols and types exposed by the SDK have only been tested against the English game executable.


Symbol wrapper function helpers are the safest way to call known game functions. Use them instead of casting raw addresses.

Raw addresses are only for unsupported reverse-engineering or low-level patch work.


## Calling game functions through SDK wrappers

The SDK provides typed wrappers for most known game functions.

For example, `Movie_PlayFile` is exposed as `DTTR_PCDOGS_F_MoviePlayFile`.

Use the SDK wrapper methods instead of calling the address yourself:

- `IsCallable(&ctx->runtime)`: Check whether the function is available and safe to call.
- `Try(&ctx->runtime, args..., out_ret)`: Call the function only when it is available.
- `Call(&ctx->runtime, args..., fallback_ret)`: Call the function when available, otherwise return a fallback value.

## Handling unavailable functions

Use `Try()` when a missing function should disable related behavior, show a warning, or write a log message.

```c
BOOL played = FALSE;
if (!DTTR_PCDOGS_F_MoviePlayFile->Try(
        &ctx->runtime,
        movie_path,
        0,
        &played
    )) {
    DTTR_MODS_LOG_WARN(ctx, "Movie_PlayFile is unavailable");
    return;
}
```

Your mod can then decide what to do instead of pretending the game call worked.

`Call()` is typically shorter but can hide why the game function did not run.

```c
BOOL played = DTTR_PCDOGS_F_MoviePlayFile->Call(
    &ctx->runtime,
    movie_path,
    0,
    FALSE
);
```

This example returns `FALSE` if `Movie_PlayFile` is unavailable.

Using `Call()` when a failed game function writes output parameters or changes game-owned state can cause undefined behavior. Use `Try()` when the caller needs to handle failure explicitly.

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

Fail init for required functions and use `Try()` for optional functions.

## Passing game-owned types

When a wrapper takes a game-owned pointer or struct, use the generated SDK type. Guessing a struct layout can cause incorrect reads, corrupt writes, or crashes.
