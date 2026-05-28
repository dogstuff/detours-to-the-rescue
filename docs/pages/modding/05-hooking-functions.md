# Hooking Game Functions

A function hook redirects a game function to your detour. Your detour can run custom code, call the original function, change arguments, change the return value, or block the original call.

**Warning:** The detour must use the exact same signature and calling convention as the game function. A mismatch can crash the game or corrupt memory.

## Using the SDK wrappers

Generated PCDOGS helpers include a function-pointer type for each known function, suffixed with `_proto`. This can be used to store your "original" game function pointer.

```c
static DTTR_PCDOGS_F_PlayerSetLives_proto original_player_set_lives;
```

Hook signatures must match the game perfectly and will likely cause the game to crash if there are any divergences. If the generated type does not exist, hooking the function will require additional reverse engineering work.

## Writing a detour

A detour has the same signature as the game function.

```c
static int32_t __cdecl player_set_lives_detour(char lives) {
    if (lives > 99) {
        lives = 99;
    }

    return original_player_set_lives(lives);
}
```

In this case we call `original_player_set_lives(...)` to make normal game behavior to continue. If we didn't call it, the original function's behavior will be fully overridden.

The `original` pointer is only valid while the hook is installed.

## Prefer patch groups for hooks

When applying multiple related hooks and patches, install the generated `PatchSpec()` through a patch group to simplify rollbacks and clean up.

```c
static DTTR_Core_PatchGroup *patches;
static DTTR_PCDOGS_F_PlayerSetLives_proto original_player_set_lives;

DTTR_MODS_INIT {
    DTTR_PCDOGS_T_Patch_Report report = {0};
    const DTTR_PCDOGS_T_Patch_Spec specs[] = {
        DTTR_PCDOGS_F_PlayerSetLives->PatchSpec(
            true,
            player_set_lives_detour,
            &original_player_set_lives
        ),
    };

    DTTR_Core_Result result = DTTR_PCDOGS_INSTALL_PATCHES(
        &ctx->runtime,
        specs,
        &patches,
        &report
    );
    return DTTR_Core_ResultOk(result);
}

DTTR_MODS_CLEANUP {
    DTTR_Core_PatchGroupRelease(&patches);
    original_player_set_lives = NULL;
}
```

## Use `Hook()` only for simple owned hooks

Generated helpers also expose `Hook()` and `Unhook()`:

```c
DTTR_PCDOGS_F_PlayerSetLives->Hook(
    &ctx->runtime,
    player_set_lives_detour,
    &original_player_set_lives
);

DTTR_PCDOGS_F_PlayerSetLives->Unhook(&ctx->runtime);
```

Use this only when your mod fully owns that generated hook slot and the hook is simple to clean up. This should generally be avoided in favor of patch groups.

## Using the raw hook API

The SDK offers an API for hooking functions when no generated helper exists.

The raw hook APIs require reverse engineering work to use and should be avoided in most casts.

Before passing a generated address to a raw hook API, check both conditions:

```c
if (!DTTR_PCDOGS_F_PlayerSetLives->IsCallable(&ctx->runtime)) {
    return false;
}
if (DTTR_PCDOGS_F_PlayerSetLives->HookKind() != DTTR_PCDOGS_HOOK_REL32) {
    return false;
}
```

Other patch types still reject overlapping ranges, including byte patches, pointer hooks, raw rel32 jumps, and incompatible function hooks. Unsupported function-hook chaining returns `DTTR_ERR_HOOK_CHAIN_UNSUPPORTED`.
