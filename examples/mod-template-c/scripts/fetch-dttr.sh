#!/usr/bin/env bash
# Downloads the DttR modding build & SDK matching required version.

set -euo pipefail

cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.."

version=$(cat dttr-version.txt)

rm -rf .dttr .dttr-download
mkdir .dttr-download

curl --fail -L -o .dttr-download/dttr.zip \
  "https://gitlab.com/dogstuff/detours-to-the-rescue/-/releases/$version/downloads/dttr-modding-release.zip"

unzip -q .dttr-download/dttr.zip -d .dttr-download
if [ -d .dttr-download/release-modding ]; then
  mv .dttr-download/release-modding .dttr
else
  mv .dttr-download/Release-modding .dttr
fi

rm -rf .dttr-download
