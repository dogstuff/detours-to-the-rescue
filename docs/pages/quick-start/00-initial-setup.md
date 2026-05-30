# Initial Setup

This guide gets DttR running with your original game files.

## Downloading DttR

1. Download the build of DttR you want to use:

    - [Vanilla](https://gitlab.com/dogstuff/detours-to-the-rescue/-/releases/permalink/latest/downloads/dttr-release.zip) - Does not load third-party mods. Use this build for speedruns.
    - [Modding](https://gitlab.com/dogstuff/detours-to-the-rescue/-/releases/permalink/latest/downloads/dttr-modding-release.zip) - Includes the DttR modding runtime and SDK.

2. Extract the archive to a writable directory.
3. Run `dttr.exe`.

## Loading the original game files

DttR should open a prompt asking for the original game files:

![DttR prompt asking where to load game files from](../assets/load-prompt.png)

DttR can find the game files from a disc, an installed copy, or an ISO.
Choose the method that matches your setup.

=== "Original CD"

    !!! note ""

        The `Open Disc ...` buttons will only show if DttR detects an inserted/mounted disc containing the original game files.
        This includes official retail copies and mounted disc images.

    Insert the original *102 Dalmatians: Puppies to the Rescue* CD, then select the disc.

    ![DttR loader showing an inserted game disc](../assets/load-disc.png)

=== "Installed Copy"

    Choose `Open Directory` and select the installed game directory containing `pcdogs.exe`.

    ![Windows directory picker selecting an installed 102 Dalmatians directory](../assets/load-install.png)

=== "ISO"

    Choose `Open ISO`, select the game ISO file, and click `Open`.

    ![Windows file picker selecting a 102 Puppies ISO image](../assets/load-iso.png)

## You're done

Once DttR has the game files, setup is complete.

![102 Dalmatians title screen running through DttR](../assets/done.png)

For controller mappings, graphics settings, and other options, see [Configuration](01-configuration.md).
