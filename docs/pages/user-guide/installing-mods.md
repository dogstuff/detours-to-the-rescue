---
title: "Installing Mods"
description: "Install mod DLLs and assets in the mods directory, and use the Modding build when needed."
seo_type: "article"
---

# Installing Mods

!!! warning "Mods Are Pretty New"

    The DttR modding SDK and ecosystem is still very news and will likely have a limited
    number of stable mods available. This should improve with time!

DttR mods require the "Modding" build. The "Vanilla" build is intended for speedruns and does not load third-party mods.

[Modding (__DTTR_DOCS_STABLE_VERSION__)](__DTTR_DOCS_MODDING_STABLE_DOWNLOAD_URL__)

## Installing a Mod

1. Download and extract the "Modding" build of DttR.
2. Create a `mods` folder next to `dttr.exe` if it does not already exist.
3. Copy the mod `.dll` files into `mods`.
4. Run DttR.

A typical folder should look like this:

```text
dttr.exe
mods/
  some-mod.dll
```

## ABI Compatibility

Use mods built for your DttR SDK ABI. The top-left `Modding Build (ABI <version>)` label shows the running SDK's current ABI. The ABI version is separate from the DttR version.

Enable **Show Loaded Mods** in the **Modding** tab to list mods that actually initialized.

## Disabling a Mod

To disable a mod without deleting it, open `dttr-config.exe`, go to the `Modding` tab, uncheck the mod, then save.

You can also remove the mod DLL from the `mods` folder.

If DttR crashes only with a specific mod installed, report it to the mod's developer. Mod-specific crashes belong with the mod project, not the main DttR repository.
