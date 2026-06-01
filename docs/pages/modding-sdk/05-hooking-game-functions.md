# Hooking Game Functions

A function hook redirects a game function to your detour. Your detour can run custom code, call the original function, change arguments, change the return value, or block the original call.

**Warning:** A detour signature or calling-convention mismatch can cause crashes or memory corruption.

## Storing the original game function

SDK wrappers include a function-pointer type for each known function, suffixed with `_proto`. Use it to store your "original" game function pointer.

```c
static DTTR_PCDOGS_F_PlayerSetLives_proto original_player_set_lives;
```

Hook signatures that diverge from the game function can cause crashes or memory corruption. If the generated type does not exist, hooking the function requires additional reverse engineering work.

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

This detour calls `original_player_set_lives(...)` so normal game behavior continues. If it did not call the original function, the detour would fully override that behavior.

The `original` pointer is only valid while the hook is installed.

## Installing hooks through patch groups

When applying related hooks and patches, install the generated `PatchSpec()` through a patch group to simplify rollbacks and cleanup.

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

    DTTR_Result result = DTTR_PCDOGS_INSTALL_PATCHES(
        &ctx->runtime,
        specs,
        &patches,
        &report
    );
    return DTTR_ResultOk(result);
}

DTTR_MODS_CLEANUP {
    DTTR_Core_PatchGroupRelease(&patches);
    original_player_set_lives = NULL;
}
```

## Reserving direct hooks for simple cases

SDK wrappers also expose `Hook()` and `Unhook()`:

```c
DTTR_PCDOGS_F_PlayerSetLives->Hook(
    &ctx->runtime,
    player_set_lives_detour,
    &original_player_set_lives
);

DTTR_PCDOGS_F_PlayerSetLives->Unhook(&ctx->runtime);
```

Using `Hook()` while another mod or patch group owns the same hook slot can cause hook conflicts. Use it for simple hooks your mod fully owns, and use patch groups for most hooks.

## Hooking functions without SDK wrappers

The SDK also has raw APIs for hooking functions when no SDK wrapper exists.

The raw hook APIs require reverse engineering work. Prefer SDK wrappers or patch groups in most cases.

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
