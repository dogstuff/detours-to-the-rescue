# Applying Game Patches

Patches change bytes, pointer slots, or jump sites. Keep each feature in one `DTTR_Core_PatchGroup` so cleanup has one rollback path.

## Rules

1. Create a patch group during `DTTR_MODS_INIT`, or let `DTTR_PCDOGS_PatchGroup_Install()` create one.
2. Install related patches into that group.
3. Store the group handle in mod-owned state.
4. Release it during `DTTR_MODS_CLEANUP`.

Do not expose the feature until required patches install successfully. If a required patch fails, the group can roll back anything it already installed.

Prefer generated `DTTR_PCDOGS_T_Patch_Spec` helpers for known symbols. Use `DTTR_Core_TargetSpec` only when generated patch specs do not fit the target.

Use optional specs only for targets that legitimately exist in some supported executables but not others. A skipped optional spec increments `DTTR_PCDOGS_T_Patch_Report::skipped_optional`; treat that as expected compatibility behavior.

To disable a feature temporarily, call `DTTR_Core_PatchGroupUninstall(patch_group)`. It detaches active handles but keeps the group available for reuse.
