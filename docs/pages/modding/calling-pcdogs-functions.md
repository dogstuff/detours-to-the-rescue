# Calling Game Functions

Generated PCDOGS wrappers are the normal way to call known game functions. Use raw addresses only for unsupported reverse-engineering or patch work.

**NOTE:** PCDOGS SDK symbols have only been properly tested against the English game executable.

## Wrapper Rules

For a generated function like `Movie_PlayFile`, the wrapper provides:

- `IsCallable(&ctx->runtime)` to check direct-call support.
- `Try(&ctx->runtime, args..., out_ret)` to call only when available.
- `Call(&ctx->runtime, args..., fallback_ret)` to return a fallback value when unavailable.
- `HookKind()` and `HookPrologueSize()` for hook setup.
- `DTTR_PCDOGS_F_<Name>_proto` for typed original-function pointers.

Use `Try()` when a missing symbol should disable a feature or produce a log. Use `Call()` only when a simple fallback value is genuinely safe and game-owned output parameters do not need to change.

When a wrapper needs a game-owned pointer, use the generated SDK type. Do not guess struct layout from a raw address.

For raw address lookup, see [Manually Resolving Symbols](resolving-symbols.md). For hook chaining, see [Hooking Game Functions](hooking-functions.md).
