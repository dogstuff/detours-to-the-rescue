---
title: "Getting Started"
description: "Download DttR and get it running with your game disc, installed copy, or ISO."
seo_type: "article"
---

# Getting Started

This guide walks you through getting DttR running with 102 Dalmatians: Puppies to the Rescue.

## Downloading DttR

Download the build of DttR that matches your needs:

- [Vanilla](https://gitlab.com/dogstuff/detours-to-the-rescue/-/releases/permalink/latest/downloads/dttr-release.zip) is the normal build and does not allow the installation of game modifications. **This is the only build allowed in speedruns.**
- [Modding](https://gitlab.com/dogstuff/detours-to-the-rescue/-/releases/permalink/latest/downloads/dttr-modding-release.zip) is the build that's required to load and run third-party modifications.

Extract the archive wherever you'd like and then run `dttr.exe`.

## Loading the Game Files

DttR should open a prompt asking for the original game files:

![DttR prompt asking where to load game files from](../assets/load-prompt.png)

DttR can find the game files from a disc, an installed copy, or an ISO.
Choose the method that matches your setup.

=== "Original CD"

    !!! info ""

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

If everything worked, the game should pop up in a window.
If the game crashes or doesn't pop up, take a look at the [troubleshooting page](troubleshooting.md).

![102 Dalmatians title screen running through DttR](../assets/done.png)
