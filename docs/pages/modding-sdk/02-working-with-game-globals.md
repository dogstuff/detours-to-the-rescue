# Working with Game Globals

PCDogs globals are known pieces of game data exposed through the SDK. When possible, use those helpers instead of raw addresses.

A global helper can tell you whether the symbol was found, read the value with the right type, and decide whether normal writes are allowed.

## Reading globals safely

`Read()` returns `DTTR_Result`. `DTTR_OK` means the value was read; failures such as unresolved symbol or read failure are reported in `result.status`.

```c
static int32_t last_lives;

DTTR_MODS_FRAME_BEGIN {
    int32_t lives = 0;
    if (DTTR_ResultOk(DTTR_PCDOGS_D_SaveFilePlayerLives->Read(&lives))) {
        last_lives = lives;
    }
}
```

## Checking write policies

Writing game memory is not always safe. Check `Policy()` before writing to a global.

- `RAW_MEMORY`: `Write()` may update the value after availability and memory-permission checks.
- `READ_ONLY`: Use for inspection only. These are usually decoded tables, dispatch slots, jumps, opcodes, or indexes.
- `ENGINE_MANAGED`: The game manages this pointer or state and may replace or overwrite it.
- `PATCH_ONLY`: Change this through patch or hook flows, not direct writes.
- `UNKNOWN`: The symbol has not been classified enough for normal writes.

`Write()` only succeeds for `RAW_MEMORY` globals. For every other policy, design the related functions around reading, patching, or hooking instead.


Read/write helpers return `DTTR_Result`, so callers keep the failure reason and may get an optional message:

- `Status()` reports whether the generated data descriptor is resolved.
- `Read(...)` distinguishes unresolved descriptors from read failures.
- `Write(...)` returns `DTTR_ERR_POLICY_MISMATCH` when the symbol is not `RAW_MEMORY`.
- `UnsafeWrite(...)` still bypasses policy, but it reports unresolved and write-failed states explicitly.


## Writing through policy-aware helpers

`UnsafeWrite()` bypasses `Policy()`. It still requires writable process memory but that does not imply the write is safe.

Using `UnsafeWrite()` in normal mod behavior bypasses the SDK's safety policy and can corrupt game-managed state. Reserve it for reverse-engineering experiments, explicit patching work, or SDK internals.


If another SDK API requires an address or if you are doing unsupported reverse-engineering work, see [Manually Resolving Symbols](07-resolving-symbols.md).
