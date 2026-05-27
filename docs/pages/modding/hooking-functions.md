# Hooking Game Functions

**Warning:** The hook signature and calling convention must match the target exactly. A mismatch can crash the game or corrupt its state.

## Shape and Lifetime

A function hook has a target address, a detour with the same signature, and usually an `original` pointer.

The SDK chains function hooks installed at the same address. The newest hook runs first. Its `original` pointer calls the next hook, ending at the real trampoline.

That pointer is valid only while the hook handle is installed. Copy any data needed after cleanup into mod-owned storage.

## Helper Choice

Use these in order:

1. Generated `PatchSpec()` helpers for hooks that should compose with other mods.
2. Generated `Hook()` / `Unhook()` helpers only when your mod owns that symbol's generated hook slot.
3. Raw core hook APIs only when no generated patch spec exists.

Other patch types still reject overlapping ranges. That includes byte patches, pointer hooks, raw rel32 jumps, and incompatible function hooks. Unsupported function-hook chaining reports `DTTR_ERR_HOOK_CHAIN_UNSUPPORTED`.

Before passing a generated address to a raw core hook API, check `IsCallable()` and `HookKind() == DTTR_PCDOGS_HOOK_REL32`.
