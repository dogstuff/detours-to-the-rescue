# Working with Game Globals

PCDOGS globals are known pieces of game data exposed through the SDK. When possible, use those helpers instead of raw addresses.

A global helper can tell you whether the symbol was found, read the value with the right type, and decide whether normal writes are allowed.

## Read a global

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

## Write only when the policy allows it

Writing game memory is not always safe. Check `WritePolicy` before exposing a feature that edits a global.

- `RAW_MEMORY`: `Write()` may update the value after availability and memory-permission checks.
- `READ_ONLY`: Use for inspection only. These are usually decoded tables, dispatch slots, jumps, opcodes, or indexes.
- `ENGINE_OWNED`: The game owns this pointer or state and may replace or overwrite it.
- `PATCH_ONLY`: Change this through patch or hook flows, not direct writes.
- `UNKNOWN`: The symbol has not been classified enough for normal writes.

`Write()` only succeeds for `RAW_MEMORY` globals. For every other policy, design the feature around reading, patching, or hooking instead.

## Avoid `UnsafeWrite()` in normal mods

`UnsafeWrite()` bypasses `WritePolicy`. It still requires writable process memory, but it does not mean the write is safe.

Use it only for reverse-engineering experiments, explicit patching work, or SDK internals. Most mods generally should not need to use this.

## Use raw addresses only at API boundaries

Most mods should not need a global's raw address. Use typed helpers first.

If another SDK API requires an address, or if you are doing unsupported reverse-engineering work, see [Manually Resolving Symbols](resolving-symbols.md).
