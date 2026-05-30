# Initial Setup

This guide quickly overviews the process to get set up with DttR.

## Downloading DttR

1. Download the build of DttR you want to use:

    - [Vanilla](https://gitlab.com/dogstuff/detours-to-the-rescue/-/releases/permalink/latest/downloads/dttr-release.zip) - Does not contain modding support beyond the functionality built into DttR. Using a non-Vanilla build for speedruns will cause the run to be invalid.
    - [Modding](https://gitlab.com/dogstuff/detours-to-the-rescue/-/releases/permalink/latest/downloads/dttr-modding-release.zip) - Contains the DttR modding runtime and SDK.

2. Extract the archive to a writable directory.
3. Run `dttr.exe`.

## Loading the original game files

A window should now appear prompting you to point DttR at the original game files:

![DttR prompt asking where to load game files from](../assets/load-prompt.png)

There are a few different ways DttR can detect your game files depending on your situation.
Follow the instructions for whichever method is most convenient for you.

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

## You're done!

As soon DttR is provided the game files, you should be all set!

![102 Dalmatians title screen running through DttR](../assets/done.png)

If you're interested in further configuring DttR (e.g. controller mappings, graphics settings), take a look at [Configuration](01-configuration.md).
