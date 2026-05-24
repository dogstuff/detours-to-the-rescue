# Hooking Game Functions

**Warning:** The hook signature and calling convention must match the target exactly. A mismatch can crash the game or corrupt its state.

## Hook Shape

A function hook has three parts:

- A target function address
- A detour with the same signature and calling convention as the target
- An optional `original` pointer that calls the next hook in the chain, or the original function when no earlier hook remains

## Patch Spec Hook

Prefer generated PCDOGS patch specs for most mods. They keep hook metadata with the generated symbol, install through a `DTTR_Core_PatchGroup`, and give cleanup one owner.

```c
#include <dttr_sdk.h>

static const DTTR_Mods_Context *mod_ctx;
static DTTR_Core_PatchGroup *patches;
static DTTR_PCDOGS_F_PlayerSetLives_proto original_player_set_lives;

static int32_t __cdecl trace_player_set_lives(char lives) {
    DTTR_MODS_LOG_INFO(mod_ctx, "Player_SetLives(%d)", (int)lives);

    return original_player_set_lives(lives);
}

DTTR_MODS_INFO("pcdogs-player-lives-hook", "0.1.0", "dogstuff")

DTTR_MODS_INIT {
    mod_ctx = ctx;

    const DTTR_PCDOGS_T_Patch_Spec specs[] = {
        DTTR_PCDOGS_F_PlayerSetLives->PatchSpec(
            true,
            trace_player_set_lives,
            &original_player_set_lives
        ),
    };
    DTTR_PCDOGS_T_Patch_Report report = {0};

    DTTR_Core_Result result = DTTR_PCDOGS_PatchGroup_Install(
        &ctx->runtime,
        specs,
        sizeof(specs) / sizeof(specs[0]),
        &patches,
        &report
    );

    if (!DTTR_Core_ResultOk(result)) {
        DTTR_MODS_LOG_ERROR(ctx, "Player_SetLives hook failed: %s", result.message);
        mod_ctx = NULL;
        return false;
    }

    return true;
}

DTTR_MODS_CLEANUP {
    DTTR_Core_PatchGroupRelease(&patches);
    original_player_set_lives = NULL;
    mod_ctx = NULL;
}
```

The SDK chains function hooks installed at the same address. The newest hook runs first. The `original` pointer returned to that hook calls the next installed hook, ending at the real trampoline.

That pointer is valid only while the hook handle is installed. Copy any data needed after cleanup into mod-owned storage.

Other patch types still reject overlaps: byte patches, pointer hooks, raw rel32 jumps, and incompatible function-hook ranges. Result-returning hook APIs report unsupported function-hook chaining as `DTTR_ERR_HOOK_CHAIN_UNSUPPORTED`.

## Single-Owner Convenience Hook

The generated `Hook()` and `Unhook()` helpers are shorter, but they have one generated hook slot per symbol. Use them only when your mod is the only owner of that helper. Prefer patch specs when hooks should compose with other mods.

```c
#include <dttr_sdk.h>

static const DTTR_Mods_Context *mod_ctx;
static DTTR_PCDOGS_F_PlayerSetLives_proto original_player_set_lives;

static int32_t __cdecl trace_player_set_lives(char lives) {
    DTTR_MODS_LOG_INFO(mod_ctx, "Player_SetLives(%d)", (int)lives);

    return original_player_set_lives(lives);
}

DTTR_MODS_INFO("pcdogs-player-lives-helper-hook", "0.1.0", "dogstuff")

DTTR_MODS_INIT {
    mod_ctx = ctx;

    if (!DTTR_PCDOGS_F_PlayerSetLives->Hook(
            &ctx->runtime,
            trace_player_set_lives,
            &original_player_set_lives
        )) {
        DTTR_MODS_LOG_WARN(ctx, "Player_SetLives unavailable or already hooked");
        mod_ctx = NULL;
        return false;
    }

    return true;
}

DTTR_MODS_CLEANUP {
    if (mod_ctx) {
        DTTR_PCDOGS_F_PlayerSetLives->Unhook(&mod_ctx->runtime);
    }

    original_player_set_lives = NULL;
    mod_ctx = NULL;
}
```

## Advanced Raw Hook

Use raw core hook APIs only when you need a lower-level target that has no generated patch spec. When the target comes from generated PCDOGS metadata, guard the hook shape first:

```c
static bool install_raw_player_lives_hook() {
    if (!DTTR_PCDOGS_F_PlayerSetLives->IsCallable(&mod_ctx->runtime)
        || DTTR_PCDOGS_F_PlayerSetLives->HookKind() != DTTR_PCDOGS_HOOK_REL32) {
        DTTR_MODS_LOG_WARN(mod_ctx, "Player_SetLives cannot be raw-hooked");
        return false;
    }

    void *original = NULL;
    DTTR_Core_Result result = DTTR_Core_PatchGroupHookFunction(
        patches,
        DTTR_PCDOGS_F_PlayerSetLives->Address(),
        (int)DTTR_PCDOGS_F_PlayerSetLives->HookPrologueSize(),
        (void *)trace_player_set_lives,
        &original,
        NULL
    );

    if (!DTTR_Core_ResultOk(result)) {
        DTTR_MODS_LOG_ERROR(mod_ctx, "Player_SetLives hook failed: %s", result.message);
        return false;
    }

    original_player_set_lives = (DTTR_PCDOGS_F_PlayerSetLives_proto)original;
    return true;
}
```
