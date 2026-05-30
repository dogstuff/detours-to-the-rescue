# Manually Resolving Symbols (Advanced)

!!! warning "Cross-Region Compatibility Warning"

    The symbols and types exposed by the SDK have currently only been tested against the English game executable.


Most mods should not need to manually resolve PCDOGS symbols. Prefer SDK wrappers when possible.

## Checking symbols through SDK wrappers

DttR resolves normal PCDOGS symbols before loading mods. In most mod code, check the helper you plan to use:

- Function helpers: `IsCallable(&ctx->runtime)`, `Try(...)`, or `Call(...)`.
- Global helpers: `Read(...)`, `Write(...)`, or `IsResolved()`.
- Patch helpers: `PatchSpec(...)`.

An SDK wrapper may say a symbol is unavailable, in which case it cannot be used.

## Passing raw addresses between SDK APIs

SDK wrappers expose `Address()` for cases where another SDK API needs a raw address.

This is sometimes necessary, but the rest of your mod should use typed helpers wherever possible.

## Resolving symbols without a typed helper

The following SDK helpers are exposed for situations where no SDK wrapper exists for something you need to resolve in the game:

- `DTTR_PCDOGS_FunctionResolve(...)`
- `DTTR_PCDOGS_DataResolve(...)`
