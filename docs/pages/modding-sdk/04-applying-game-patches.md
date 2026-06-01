# Applying Game Patches

A patch changes game code or data at runtime and can be reversed during cleanup. Use this page for byte patches, pointer patches, and other non-function-hook changes.

If you want to redirect a game function to your own code, see [Hooking Game Functions](05-hooking-game-functions.md).

Patch through a `DTTR_Core_PatchGroup` whenever possible. A patch group keeps related changes together, rolls back partial installs, and simplifies cleanup.

## Grouping related patches

Create one patch group for each related set of patches.

A simple mod usually does this during `DTTR_MODS_INIT`:

1. Build the patch specs.
2. Install the specs into a patch group.
3. Store the group pointer in mod-owned state.
4. Return `false` if a required patch fails.
5. Release the group during `DTTR_MODS_CLEANUP`.

Do not expose related behavior until its required patches have installed successfully; otherwise it can run against unpatched game code or data.

## Starting from generated patch specs

For known PCDogs globals and patch sites, use generated patch specs first. They already include the target and patch type.

Use lower-level target specs only when there is no SDK wrapper for the patch you need.

```c
static DTTR_Core_PatchGroup *patches;
static void *original_level_data;

DTTR_MODS_INIT {
    DTTR_PCDOGS_T_Patch_Report report = {0};
    const DTTR_PCDOGS_T_Patch_Spec specs[] = {
        DTTR_PCDOGS_D_CurrentLevelData->PatchSpec(
            true,
            replacement_level_data,
            &original_level_data
        ),
    };

    DTTR_Core_Result result = DTTR_PCDOGS_INSTALL_PATCHES(
        &ctx->runtime,
        specs,
        &patches,
        &report
    );
    if (!DTTR_Core_ResultOk(result)) {
        DTTR_MODS_LOG_ERROR(ctx, "patch install failed: %s", result.message);
        return false;
    }

    return true;
}

DTTR_MODS_CLEANUP {
    DTTR_Core_PatchGroupRelease(&patches);
    
    original_level_data = NULL;
}
```

## Marking patches as required or optional

A required patch is one the related behavior needs to work. If a required patch fails to install, stop instead of running against the wrong game code or data.

An optional patch is only for a target that legitimately exists in some supported game builds but not others, and does not ignore actual install errors.

When an optional patch is skipped, `DTTR_PCDOGS_T_Patch_Report::skipped_optional` increases.

## Disabling patched behavior temporarily

To turn off patched behavior without destroying the group, call:

```c
DTTR_Core_PatchGroupUninstall(patches);
```

That detaches active patch handles but keeps the group object available for reuse.

For normal mod shutdown, release the group instead:

```c
DTTR_Core_PatchGroupRelease(&patches);
```
