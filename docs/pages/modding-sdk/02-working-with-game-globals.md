# Working with Game Globals

!!! warning "Cross-Region Compatibility Warning"

    The symbols and types exposed by the SDK have only been tested against the English game executable.


PCDOGS globals are known pieces of game data exposed through the SDK. When possible, use those helpers instead of raw addresses.

A global helper can tell you whether the symbol was found, read the value with the right type, and decide whether normal writes are allowed.

## Reading globals safely

Use `Read()` for normal access. It returns `false` if the symbol is unavailable or if the memory read fails.

```c
static int32_t last_lives;

DTTR_MODS_FRAME_BEGIN {
    int32_t lives = 0;
    if (DTTR_PCDOGS_D_SaveFilePlayerLives->Read(&lives)) {
        last_lives = lives;
    }
}
```

## Checking write policies

Writing game memory is not always safe. Check `WritePolicy` before exposing related behavior that edits a global.

- `RAW_MEMORY`: `Write()` may update the value after availability and memory-permission checks.
- `READ_ONLY`: Use for inspection only. These are usually decoded tables, dispatch slots, jumps, opcodes, or indexes.
- `ENGINE_OWNED`: The game owns this pointer or state and may replace or overwrite it.
- `PATCH_ONLY`: Change this through patch or hook flows, not direct writes.
- `UNKNOWN`: The symbol has not been classified enough for normal writes.

`Write()` only succeeds for `RAW_MEMORY` globals. For every other policy, design the related functions around reading, patching, or hooking instead.

## Writing through policy-aware helpers

`UnsafeWrite()` bypasses `WritePolicy`. It still requires writable process memory, but it does not mean the write is safe.

Using `UnsafeWrite()` in normal mod behavior bypasses the SDK's safety policy and can corrupt game-owned state. Reserve it for reverse-engineering experiments, explicit patching work, or SDK internals.

## Passing raw addresses at API boundaries

Most mods should not need a global's raw address. Use typed helpers first.

If another SDK API requires an address or if you are doing unsupported reverse-engineering work, see [Manually Resolving Symbols](07-resolving-symbols.md).
