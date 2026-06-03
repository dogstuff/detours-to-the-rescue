#!/usr/bin/env python3

from __future__ import annotations

import json
import os
import subprocess
import sys
from pathlib import Path
from tempfile import TemporaryDirectory


BUILD_BITS = {"EN": 0b001, "EU": 0b010, "SC": 0b100}
REPO_ROOT = Path(__file__).resolve().parents[3]
SCRIPTS = REPO_ROOT / "modules/sdk/scripts"

GAMEFILE_FIXTURES = {
    "EN": (
        "pcdogs_en.exe",
        "765f43e3853d4f8ad42164074ff20c6ff35099b1ecb2f89fe5981dfc2eb1131f",
    ),
    "EU": (
        "pcdogs_eu.exe",
        "c1aecfc41c2ecb8d9aab9592a113b3c672620509b86d91fbfd9ed2f67fa60a77",
    ),
    "SC": (
        "pcdogs_sc.exe",
        "4ad94984976bdab0fcd1b184911279f0ed76b87c3ddef55412d10b0ce46f4351",
    ),
}

EXPECTED_SYMBOLS = {
    "EN": {
        "signatures": {"Game_InitializeEngine": "0x42C810"},
        "functions": {"Actor_AddToCollisionList": "0x43AE00"},
        "data": {"ActiveEntityWorkList": "0x455D30"},
    },
    "EU": {
        "signatures": {"Game_InitializeEngine": "0x42D6C0"},
        "functions": {"Actor_AddToCollisionList": "0x43C1D0"},
        "data": {"ActiveEntityWorkList": "0x457DC0"},
    },
    "SC": {
        "signatures": {"Game_InitializeEngine": "0x42D6A0"},
        "functions": {"Actor_AddToCollisionList": "0x43C1B0"},
        "data": {"ActiveEntityWorkList": "0x457DB0"},
    },
}


def run_resolver(gamefile: Path, build: str, output: Path) -> dict:
    subprocess.run(
        [
            sys.executable,
            str(SCRIPTS / "resolve_symbol_addresses.py"),
            "--blueprint",
            str(REPO_ROOT / "modules/sdk/blueprints/dttr_pcdogs.py"),
            "--gamefile",
            str(gamefile),
            "--build",
            build,
            "--output",
            str(output),
        ],
        check=True,
    )
    return json.loads(output.read_text())


def supports(required: object, build: str, build_mask_bits) -> bool:
    return bool(build_mask_bits(required) & BUILD_BITS[build])


def supported_names(rows: list, build: str, build_mask_bits) -> set[str]:
    return {row.name for row in rows if supports(row.required, build, build_mask_bits)}


def supported_global_names(rows: list, build: str, build_mask_bits) -> set[str]:
    return {
        row.name
        for row in rows
        if any(
            supports(required, build, build_mask_bits)
            for required in row.supported_builds
        )
    }


def assert_payload(payload: dict, blueprint, expected: dict, build_mask_bits) -> None:
    build = expected["build"]

    assert payload["schema_version"] == 1
    assert payload["build"] == build
    assert payload["source_game_sha256"] == expected["sha256"]
    assert payload["unresolved"] == {"signatures": [], "functions": [], "data": []}

    assert set(payload["signatures"]) == supported_names(
        blueprint.signatures, build, build_mask_bits
    )

    assert set(payload["functions"]) == supported_names(
        blueprint.functions, build, build_mask_bits
    )

    assert set(payload["data"]) == supported_global_names(
        blueprint.globals, build, build_mask_bits
    )

    for category, symbols in EXPECTED_SYMBOLS[build].items():
        for name, address in symbols.items():
            assert payload[category][name] == address


def pcdogs_fixture_dir() -> Path:
    value = os.environ.get("DTTR_PCDOGS_FIXTURE_DIR")
    if value:
        return Path(value)

    return REPO_ROOT / "gamefiles"


def require_pcdogs_fixtures() -> bool:
    return os.environ.get("DTTR_REQUIRE_PCDOGS_FIXTURES") in {"1", "ON", "TRUE", "true"}


def has_all_gamefiles(fixture_dir: Path) -> bool:
    return all(
        (fixture_dir / gamefile).is_file() for gamefile, _ in GAMEFILE_FIXTURES.values()
    )


def assert_real_fixture_payloads() -> None:
    fixture_dir = pcdogs_fixture_dir()
    if not has_all_gamefiles(fixture_dir):
        if require_pcdogs_fixtures():
            missing = [
                str(fixture_dir / gamefile)
                for gamefile, _ in GAMEFILE_FIXTURES.values()
                if not (fixture_dir / gamefile).is_file()
            ]
            raise AssertionError(
                f"missing required PCDOGS fixtures: {', '.join(missing)}"
            )

        print(f"Skipping real PCDOGS fixture checks; missing files in {fixture_dir}")
        return

    add_scripts_to_path()

    from generate_headers import build_mask_bits, load_blueprint

    blueprint = load_blueprint(REPO_ROOT / "modules/sdk/blueprints/dttr_pcdogs.py")

    with TemporaryDirectory() as tmp:
        output_dir = Path(tmp)
        for build, (gamefile, sha256) in GAMEFILE_FIXTURES.items():
            expected = {"build": build, "sha256": sha256}

            payload = run_resolver(
                fixture_dir / gamefile,
                build,
                output_dir / f"{build.lower()}.json",
            )

            assert_payload(payload, blueprint, expected, build_mask_bits)


def add_scripts_to_path() -> None:
    script_path = str(SCRIPTS)
    if script_path not in sys.path:
        sys.path.insert(0, script_path)


def main() -> int:
    assert_real_fixture_payloads()

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
