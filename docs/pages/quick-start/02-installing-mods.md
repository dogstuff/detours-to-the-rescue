# Installing Mods

DttR mods require the "Modding" build of DttR. Because it is intended for speedruns, the "Vanilla" build does not load third-party mods.

[Download Modding (latest)](https://gitlab.com/dogstuff/detours-to-the-rescue/-/releases/permalink/latest/downloads/dttr-modding-release.zip)

## Installing a mod

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

## Disabling a mod

To disable a mod without deleting it, open `dttr-config.exe`, go to the `Modding` tab, uncheck the mod, then save.

You can also remove the mod DLL from the `mods` folder.

If DttR crashes only with a specific mod installed, report it to the mod's developer. Reporting mod-specific crashes to the main DttR repository will send the issue to the wrong project.
