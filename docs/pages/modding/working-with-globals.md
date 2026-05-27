# Working with Game Globals

PCDOGS globals expose typed helpers for known game data. Prefer them over raw addresses.

## Read and Write

Use `Read()` for normal access. A `false` return means the symbol was unavailable or the memory read failed.

Check `WritePolicy` before exposing a feature that edits game memory:

- `RAW_MEMORY`: `Write()` may update the slot after availability and memory-permission checks.
- `READ_ONLY`: The symbol is a decoded table, dispatch slot, jump, opcode, or index.
- `ENGINE_OWNED`: The game owns the live pointer or state.
- `PATCH_ONLY`: Change it through patch or hook flows.
- `UNKNOWN`: The symbol is not classified enough for ordinary writes.

Use `UnsafeWrite()` only for one-off patching, reverse-engineering experiments, or SDK internals where the caller accepts the risk. It bypasses `WritePolicy` but still requires writable process memory.

For raw addresses or ID-based lookup, see [Manually Resolving Symbols](resolving-symbols.md).
