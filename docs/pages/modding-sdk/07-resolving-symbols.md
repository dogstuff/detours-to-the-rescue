# Manually Resolving Symbols (Advanced)

Most mods should not need to manually resolve PCDogs symbols. Prefer SDK wrappers when possible.

## SDK Wrapper Checks

DttR resolves normal PCDogs symbols before loading mods. In most mod code, check the helper you plan to use:

- Function helpers: `Status(&ctx->runtime)`, `IsCallable(&ctx->runtime)`, `Call(...)`.
- Global helpers: `Read(...)`, `Write(...)`, or `IsResolved()`.
- Patch helpers: `PatchSpec(...)`.

An SDK wrapper may say a symbol is unavailable, in which case it cannot be used.

## Raw Addresses

SDK wrappers expose `Address()` for cases where another SDK API needs a raw address.

This is sometimes necessary, but the rest of your mod should use typed helpers wherever possible.

## Untyped Helpers

Use these SDK helpers only when no SDK wrapper exists for the game symbol you need:

- `DTTR_PCDOGS_FunctionResolve(...)`
- `DTTR_PCDOGS_DataResolve(...)`
