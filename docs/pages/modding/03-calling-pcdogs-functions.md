# Calling Game Functions

PCDOGS function helpers are the safe way to call known game functions. Use them instead of casting raw addresses.

Raw addresses are only for unsupported reverse-engineering or low-level patch work.

**Note:** PCDOGS SDK symbols have only been tested against the English game executable.

## Using the generated helpers

The SDK provides typed function wrappers for the majority of functions available in the game.

For example, `Movie_PlayFile` is exposed as `DTTR_PCDOGS_F_MoviePlayFile`.

Use the helper methods instead of calling the address yourself:

- `IsCallable(&ctx->runtime)`: Check whether the function is available and safe to call.
- `Try(&ctx->runtime, args..., out_ret)`: Call the function only when it is available.
- `Call(&ctx->runtime, args..., fallback_ret)`: Call the function when available, otherwise return a fallback value.

## Best Practices

Use `Try()` when a missing function should disable a feature, show a warning, or write a log message.

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

Your mod can decide what to do instead of pretending the game call worked.

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

Prefer `Try()` over `Call()` when the function writes to output parameters, changes game-owned state you depend on, or failing silently, as `Call()` doing so can cause undefined behavior.

## Check availability during setup

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

## Use SDK types

When a wrapper takes a game-owned pointer or struct, use the generated SDK type. Generally avoid guessing a struct layout unless you know what you're doing.
