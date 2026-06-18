#!/usr/bin/env python3
"""Basic sanity checks for the generated PCDOGS symbol manifest."""

from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path
from typing import Any

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "scripts"))

from symbol_manifest import (  # noqa: E402
    SUPPORTED_BUILDS,
    SymbolManifest,
    symbol_manifest_schema,
)

JsonObject = dict[str, Any]

HEADER_COUNTS = {
    "functions": "DTTR_PCDOGS_FUNCTION_COUNT_VALUE",
    "data": "DTTR_PCDOGS_DATA_COUNT_VALUE",
    "symbol_functions": "DTTR_PCDOGS_SYMBOL_FUNCTION_COUNT_VALUE",
    "symbol_data": "DTTR_PCDOGS_SYMBOL_DATA_COUNT_VALUE",
}

KIND_COUNT_KEYS = {
    "function": "functions",
    "data": "data_symbols",
    "type": "types",
    "signature": "signatures",
}


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Check generated PCDOGS symbol manifest files."
    )

    parser.add_argument("--manifest", type=Path, required=True)
    parser.add_argument("--schema", type=Path, required=True)
    parser.add_argument("--header", type=Path, required=True)

    return parser.parse_args()


def expect(condition: bool, message: str) -> None:
    if not condition:
        raise ValueError(message)


def load_json_object(path: Path, label: str) -> JsonObject:
    value = json.loads(path.read_text())
    expect(isinstance(value, dict), f"{label} JSON root must be object: {path}")
    return value


def check_schema(manifest: JsonObject, schema: JsonObject) -> None:
    expect(manifest.get("schema_version") == 1, "manifest schema_version must be 1")
    expect(schema == symbol_manifest_schema(), "schema drift from SymbolManifest model")


def check_supported_builds(manifest: JsonObject) -> None:
    supported_builds = manifest.get("supported_builds")
    expect(
        isinstance(supported_builds, dict),
        "top-level supported_builds must be an object keyed by build id",
    )
    expect(
        set(supported_builds) == set(SUPPORTED_BUILDS),
        "top-level supported_builds must contain every supported build",
    )

    for build, entry in supported_builds.items():
        expect(isinstance(build, str) and build, "supported build keys must be strings")
        expect(isinstance(entry, dict), f"supported_builds.{build} must be an object")


def check_symbol_shape(manifest: JsonObject) -> dict[str, int]:
    symbols = manifest.get("symbols")
    expect(isinstance(symbols, dict), "top-level symbols must be an object")
    expect(symbols, "top-level symbols must not be empty")

    counts = {kind: 0 for kind in KIND_COUNT_KEYS}
    for name, entry in symbols.items():
        expect(isinstance(name, str) and name, "symbol names must be non-empty strings")
        expect(isinstance(entry, dict), f"symbols.{name} must be an object")
        expect(entry, f"symbols.{name} must contain at least one kind")

        for kind, kind_entry in entry.items():
            expect(kind in KIND_COUNT_KEYS, f"symbols.{name} has unknown kind: {kind}")
            expect(
                isinstance(kind_entry, dict),
                f"symbols.{name}.{kind} must be an object",
            )
            counts[kind] += 1

    return counts


def check_manifest_counts(manifest: JsonObject, actual_counts: dict[str, int]) -> None:
    counts = manifest.get("counts")
    expect(isinstance(counts, dict), "top-level counts must be an object")

    for kind, count_key in KIND_COUNT_KEYS.items():
        expect(
            counts.get(count_key) == actual_counts[kind],
            f"counts.{count_key} drift: {counts.get(count_key)} != {actual_counts[kind]}",
        )


def header_counts(header: str) -> dict[str, int]:
    counts: dict[str, int] = {}
    for key, token in HEADER_COUNTS.items():
        match = re.search(rf"\b{re.escape(token)}\s*=\s*(\d+)", header)
        expect(match is not None, f"header missing {token}")
        counts[key] = int(match.group(1))
    return counts


def check_header_counts(actual_counts: dict[str, int], header: str) -> None:
    counts = header_counts(header)
    expected = {
        "functions": actual_counts["function"],
        "data": actual_counts["data"],
        "symbol_functions": actual_counts["function"],
        "symbol_data": actual_counts["data"],
    }

    for key, expected_count in expected.items():
        token = HEADER_COUNTS[key]
        expect(
            counts[key] == expected_count,
            f"header {token} drift: {counts[key]} != {expected_count}",
        )


def main() -> int:
    args = parse_args()
    manifest = load_json_object(args.manifest, "manifest")
    schema = load_json_object(args.schema, "schema")
    header = args.header.read_text()

    check_schema(manifest, schema)
    check_supported_builds(manifest)
    actual_counts = check_symbol_shape(manifest)
    check_manifest_counts(manifest, actual_counts)
    check_header_counts(actual_counts, header)
    SymbolManifest.model_validate(manifest)

    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as exc:  # noqa: BLE001 - CTest should print one concise reason.
        print(f"symbol manifest check failed: {exc}", file=sys.stderr)
        raise SystemExit(1)
