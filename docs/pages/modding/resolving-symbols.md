# Manually Resolving Symbols

PCDOGS symbols describe known game functions, globals, patch sites, and types. Most mods should use typed helpers directly.

**NOTE:** PCDOGS SDK symbols have only been properly tested against the English game executable.

## Normal Path

DttR resolves required PCDOGS symbols before loading mods. Optional behavior should still guard the helper it needs with `IsCallable()`, `Try()`, `Read()`, or `Write()`.

Use `DTTR_PCDOGS_ResolveAll(&ctx->runtime)` for custom hosts and tests. A `false` return means at least one known symbol did not resolve; optional features can still check their own symbols.

## Raw Address Path

Generated wrappers expose availability and addresses:

- Function helpers: `IsResolved()`, `IsCallable(&ctx->runtime)`, and `Address()`.
- Data helpers: `IsResolved()`, `Read()`, `Write()`, and `Address()`.

Use `Address()` only when another SDK API requires a raw address. Use `DTTR_PCDOGS_FunctionResolve(...)` or `DTTR_PCDOGS_DataResolve(...)` only when no typed wrapper exists.
