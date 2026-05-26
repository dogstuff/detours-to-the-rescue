# Downloads the DttR modding build & SDK matching required version.

$ErrorActionPreference = "Stop"

Set-Location (Join-Path $PSScriptRoot "..")
$version = (Get-Content dttr-version.txt -Raw).Trim()

Remove-Item -Recurse -Force .dttr, .dttr-download -ErrorAction SilentlyContinue

New-Item -ItemType Directory .dttr-download | Out-Null

Invoke-WebRequest `
    -Uri "https://gitlab.com/dogstuff/detours-to-the-rescue/-/releases/$version/downloads/dttr-modding-$version-release.zip" `
    -OutFile .dttr-download/dttr.zip

Expand-Archive -Path .dttr-download/dttr.zip -DestinationPath .dttr-download

if (Test-Path .dttr-download/release-modding) {
    Move-Item .dttr-download/release-modding .dttr
} else {
    Move-Item .dttr-download/Release-modding .dttr
}

Remove-Item -Recurse -Force .dttr-download
