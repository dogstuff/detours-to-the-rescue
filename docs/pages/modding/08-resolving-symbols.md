# Manually Resolving Symbols (Advanced)

Most mods should not need to manually resolve PCDOGS symbols. Prefer the SDK-provided wrappers when possible.

## Using SDK wrappers

DttR resolves normal PCDOGS symbols before loading mods. In most mod code, check the helper you plan to use:

- Function helpers: `IsCallable(&ctx->runtime)`, `Try(...)`, or `Call(...)`.
- Global helpers: `Read(...)`, `Write(...)`, or `IsResolved()`.
- Patch helpers: `PatchSpec(...)`.

An SDK wrapper may say a symbol is unavailable, in which case it cannot be used.

## Use `Address()` only when an API needs it

Generated helpers expose `Address()` for cases where another SDK API needs a raw address.

This is sometimes necessary, but the rest of your mod should use typed helpers wherever possible.

## Resolve manually without a typed helper

The following SDK helpers are exposed for situations where no generated wrapper exists for something you need to resolve in the game:

- `DTTR_PCDOGS_FunctionResolve(...)`
- `DTTR_PCDOGS_DataResolve(...)`
