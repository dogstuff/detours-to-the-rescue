# Working with Game Globals

## API Shape

PCDOGS globals expose typed helpers. Most use the same pattern: `DTTR_PCDOGS_D_SaveFilePlayerLives->Read()` to read and `DTTR_PCDOGS_D_SaveFilePlayerLives->Write()` to write.

Resolve symbols before using global helpers. For init-time resolving, raw addresses, and ID-based helpers, see [Manually Resolving Symbols](resolving-symbols.md).

## Reading Data

```c
static int32_t last_player_lives;
static bool has_player_lives;

DTTR_MODS_FRAME_BEGIN {
    int32_t lives = 0;
    has_player_lives = DTTR_PCDOGS_D_SaveFilePlayerLives->Read(&lives);

    if (has_player_lives) {
        last_player_lives = lives;
    }
}
```

A `false` return means the value was unavailable or the memory read failed.

## Writing Data

Typed writes check that the PCDOGS wrapper is available and that the target memory is writable. Check `WritePolicy` before exposing a feature that edits game memory:

| Policy | Contract |
| --- | --- |
| `DTTR_PCDOGS_DATA_WRITE_POLICY_RAW_MEMORY` | Plain generated data slot. `Write()` may update it after availability and memory-permission checks. |
| `DTTR_PCDOGS_DATA_WRITE_POLICY_READ_ONLY` | Dispatch, jump, lookup, opcode, or index table decoded as data. |
| `DTTR_PCDOGS_DATA_WRITE_POLICY_ENGINE_OWNED` | Live pointer or state owned by the game engine. |
| `DTTR_PCDOGS_DATA_WRITE_POLICY_PATCH_ONLY` | Symbol intended for explicit patch/hook flows. |
| `DTTR_PCDOGS_DATA_WRITE_POLICY_UNKNOWN` | Untyped or insufficiently classified global. |

```c
static bool write_save_file_player_lives(int32_t lives) {
    if (DTTR_PCDOGS_D_SaveFilePlayerLives->WritePolicy
        != DTTR_PCDOGS_DATA_WRITE_POLICY_RAW_MEMORY) {
        return false;
    }

    // Returns false when SaveFilePlayerLives is unavailable or not writable.
    return DTTR_PCDOGS_D_SaveFilePlayerLives->Write(lives);
}
```

`UnsafeWrite()` is intentionally named as an escape hatch. It bypasses `WritePolicy` but still requires writable process memory. Reserve it for one-off patching, reverse-engineering experiments, or SDK internals where the caller already accepts that risk.

For engine-owned pointers (e.g. `DTTR_PCDOGS_D_CurrentLevelData`) and read-only tables (e.g. `DTTR_PCDOGS_D_WindowLowMessageDispatchTable`), use documented helpers, hook APIs, or patch-group APIs for changes.
