# Manually Resolving Symbols

PCDOGS symbols describe known game functions, globals, patch sites, and types. Resolve them against the current executable before a feature uses typed wrappers or raw addresses.

## Resolution in DttR

DttR resolves the required PCDOGS symbols before loading mods. Most mods can use typed helpers directly and guard optional behavior with `IsCallable()`, `Try()`, `Read()`, or `Write()`.

Custom SDK hosts and tests can resolve all known symbols explicitly:

```c
#include <dttr_sdk.h>

DTTR_MODS_INIT {
    if (!DTTR_PCDOGS_ResolveAll(&ctx->runtime)) {
        DTTR_MODS_LOG_WARN(ctx, "PCDOGS symbols unavailable");
    }

    return true;
}
```

A `false` return from `DTTR_PCDOGS_ResolveAll()` means at least one known symbol did not resolve. Optional features can still check the symbol they need.

## Typed Wrapper Checks

Generated wrappers expose availability checks and resolved addresses:

- `DTTR_PCDOGS_F_MoviePlayFile->IsResolved()`
- `DTTR_PCDOGS_F_MoviePlayFile->IsCallable(&ctx->runtime)`
- `DTTR_PCDOGS_F_MoviePlayFile->Address()`
- `DTTR_PCDOGS_D_SaveFilePlayerLives->Address()`

Use `IsCallable()` before direct function calls or function hooks. Use `Address()` only when another SDK API needs a raw address.

```c
// Returns 0 when Movie_PlayFile cannot be called directly.
uintptr_t movie_play_file_address =
    DTTR_PCDOGS_F_MoviePlayFile->IsCallable(&ctx->runtime)
        ? DTTR_PCDOGS_F_MoviePlayFile->Address()
        : 0;
```

```c
uintptr_t address = DTTR_PCDOGS_D_SaveFilePlayerLives->Address();

// Returns NULL when SaveFilePlayerLives is unresolved.
int32_t *save_file_player_lives = address ? (int32_t *)address : NULL;
```

Keep copied values in mod state. Do not assume a game-memory pointer stays valid across executable changes or reload paths.

## ID-Based Resolution

Use ID-based helpers when no typed wrapper exists, or when a lower-level SDK call needs a raw address selected by enum.

```c
uintptr_t address = 0;
DTTR_Core_Result result = DTTR_PCDOGS_FunctionResolve(
    &ctx->runtime,
    DTTR_PCDOGS_FUNCTION_MOVIE_PLAY_FILE,
    &address
);

// Returns 0 when Movie_PlayFile cannot be resolved by ID.
uintptr_t movie_play_file_address = DTTR_Core_ResultOk(result) ? address : 0;
```

```c
uintptr_t address = 0;
DTTR_Core_Result result = DTTR_PCDOGS_DataResolve(
    &ctx->runtime,
    DTTR_PCDOGS_DATA_PLAYER_LIVES,
    &address
);

// Returns 0 when PlayerLives cannot be resolved by ID.
uintptr_t player_lives_address = DTTR_Core_ResultOk(result) ? address : 0;
```

Prefer typed wrappers for ordinary reads, writes, calls, and hooks. Use ID-based resolution only for raw-address APIs or reverse-engineering work with no generated helper yet.
