# Applying Game Patches

Patches change bytes, pointer slots, or jump sites in game or runtime code. Keep each feature in one patch group so cleanup has one rollback path.

The usual flow is:

1. Create a patch group during init, or let `DTTR_PCDOGS_PatchGroup_Install()` create one for PCDOGS patch specs.
2. Install related patches into that group.
3. Release the group during cleanup.

## Patch Group Ownership

A `DTTR_Core_PatchGroup` lets a mod manage installed patches and hooks under one owner. If a required patch fails, the group can roll back anything it already installed.

```c
#include <dttr_sdk.h>

static const DTTR_Mods_Context *mod_ctx;
static DTTR_Core_PatchGroup *patch_group;
static DTTR_PCDOGS_F_PlayerSetLives_proto original_player_set_lives;

static bool install_player_lives_patch(const DTTR_Mods_Context *ctx);

DTTR_MODS_INFO("pcdogs-player-lives-patch", "0.1.0", "dogstuff")

DTTR_MODS_INIT {
    mod_ctx = ctx;
    return install_player_lives_patch(ctx);
}

DTTR_MODS_CLEANUP {
    DTTR_Core_PatchGroupRelease(&patch_group);
    original_player_set_lives = NULL;
    mod_ctx = NULL;
}
```

Create the patch group before exposing the feature. If an install API creates the group for you, keep the returned handle and release it during cleanup.

## PCDOGS Patch Specs

With `DTTR_PCDOGS_T_Patch_Spec`, one install call applies SDK-provided PCDOGS symbol patches.

```c
static int32_t __cdecl trace_player_set_lives(char lives) {
    DTTR_MODS_LOG_INFO(mod_ctx, "Player_SetLives(%d)", (int)lives);

    return original_player_set_lives(lives);
}

static bool install_player_lives_patch(const DTTR_Mods_Context *ctx) {
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
        &patch_group,
        &report
    );

    if (!DTTR_Core_ResultOk(result)) {
        DTTR_MODS_LOG_ERROR(ctx, "PCDOGS patch install failed: %s", result.message);
        return false;
    }

    DTTR_MODS_LOG_INFO(ctx, "installed %zu PCDOGS patch", report.installed);
    return true;
}
```

Use `true` for required specs when a missing target should abort the install.

## Target Specs

`DTTR_Core_TargetSpec` installs into an existing patch group. For manual patch-group management, create the group during init and check the result before adding targets:

```c
DTTR_Core_Result result = DTTR_Core_PatchGroupCreate(&ctx->runtime, &patch_group);
if (!DTTR_Core_ResultOk(result)) {
    DTTR_MODS_LOG_ERROR(ctx, "patch group setup failed: %s", result.message);
    return false;
}
```

```c
static bool install_player_lives_target_spec() {
    if (!DTTR_PCDOGS_F_PlayerSetLives->IsCallable(&mod_ctx->runtime)
        || DTTR_PCDOGS_F_PlayerSetLives->HookKind() != DTTR_PCDOGS_HOOK_REL32) {
        DTTR_MODS_LOG_WARN(mod_ctx, "Player_SetLives cannot be target-hooked");
        return false;
    }

    DTTR_Core_TargetSpec target = {
        .kind = DTTR_TARGET_ADDRESS_HOOK,
        .required = true,
        .address = DTTR_PCDOGS_F_PlayerSetLives->Address(),
        .detour = trace_player_set_lives,
        .out_original = (void **)&original_player_set_lives,
        .prologue_size = (int)DTTR_PCDOGS_F_PlayerSetLives->HookPrologueSize(),
    };
    DTTR_Core_TargetReport report = {0};

    DTTR_Core_Result result = DTTR_Core_PatchGroupInstallTargets(
        patch_group,
        &target,
        1,
        &report
    );

    if (!DTTR_Core_ResultOk(result)) {
        DTTR_MODS_LOG_ERROR(mod_ctx, "target spec failed: %s", result.message);
        return false;
    }

    DTTR_MODS_LOG_INFO(mod_ctx, "installed %zu target spec", report.installed);
    return true;
}
```

## Optional Targets

Use optional specs for targets that exist in some supported executables but not others.

```c
const DTTR_PCDOGS_T_Patch_Spec specs[] = {
    DTTR_PCDOGS_F_PlayerSetLives->PatchSpec(
        false,
        trace_player_set_lives,
        &original_player_set_lives
    ),
};
```

A skipped optional spec increments `DTTR_PCDOGS_T_Patch_Report::skipped_optional`; treat that as expected compatibility behavior.

## Cleanup

To disable a feature temporarily, call `DTTR_Core_PatchGroupUninstall(patch_group)`. It detaches active handles but keeps the group available for reuse.
