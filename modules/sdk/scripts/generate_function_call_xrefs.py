#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import struct
import subprocess
import sys
from dataclasses import asdict, dataclass
from pathlib import Path
from tempfile import TemporaryDirectory

sys.dont_write_bytecode = True

from resolve_symbol_addresses import (
    Image,
    Region,
    existing_file,
    region_arg,
)


@dataclass(frozen=True, slots=True)
class FunctionCallXRef:
    function: str
    ref_function: str
    instr_off: int
    addr_off: int
    indirections: int
    builds: str


@dataclass(frozen=True, slots=True)
class FunctionCallXRefPayload:
    schema_version: int
    build: str
    source_game: str
    source_game_sha256: str
    unresolved: object
    calls: list[FunctionCallXRef]


@dataclass(frozen=True, slots=True)
class ResolvedSymbols:
    functions: dict[str, int]
    source_game_sha256: str
    unresolved: object

    @classmethod
    def from_json(cls, data: object) -> ResolvedSymbols:
        if not isinstance(data, dict):
            raise ValueError("resolved symbol payload must be an object")

        functions = data.get("functions", {})
        if not isinstance(functions, dict):
            raise ValueError("resolved symbol functions must be an object")

        return cls(
            functions={name: int(address, 16) for name, address in functions.items()},
            source_game_sha256=str(data.get("source_game_sha256", "")),
            unresolved=data.get("unresolved", {}),
        )


def function_ranges(functions: dict[str, int]) -> list[tuple[str, int, int]]:
    ordered = sorted(functions.items(), key=lambda item: item[1])
    ranges = []

    for index, (name, start) in enumerate(ordered):
        end = ordered[index + 1][1] if index + 1 < len(ordered) else start + 1
        if end > start:
            ranges.append((name, start, end))

    return ranges


def resolve_symbols(
    *, resolver: Path, blueprint: Path, gamefile: Path, build: Region
) -> ResolvedSymbols:
    with TemporaryDirectory() as tmpdir:
        output = Path(tmpdir) / "resolved.json"
        subprocess.run(
            [
                sys.executable,
                str(resolver),
                "--blueprint",
                str(blueprint),
                "--gamefile",
                str(gamefile),
                "--build",
                build.value,
                "--output",
                str(output),
            ],
            check=True,
        )

        return ResolvedSymbols.from_json(json.loads(output.read_text()))


def direct_call_target(image: Image, call_va: int) -> int | None:
    offset = image.va_to_offset(call_va)
    if offset is None or offset + 5 > len(image.data):
        return None

    if image.data[offset] != 0xE8:
        return None

    rel = struct.unpack_from("<i", image.data, offset + 1)[0]
    return call_va + 5 + rel


def section_end_offset(image: Image, va: int) -> int | None:
    rva = va - image.image_base
    if rva < 0:
        return None

    for section in image.sections:
        if section.va <= rva < section.va + section.raw_size:
            return min(len(image.data), section.raw_offset + section.raw_size)

    if image.image_base == 0 and 0 <= va < len(image.data):
        return len(image.data)

    return None


def scan_direct_calls(
    image: Image, resolved: ResolvedSymbols, build: Region
) -> list[FunctionCallXRef]:
    address_to_name = {address: name for name, address in resolved.functions.items()}
    calls = []

    for caller, start, end in function_ranges(resolved.functions):
        start_offset = image.va_to_offset(start)
        end_offset = image.va_to_offset(end)
        if start_offset is None:
            continue
        if end_offset is None or end_offset <= start_offset:
            end_offset = section_end_offset(image, start) or start_offset
        if end_offset <= start_offset:
            continue

        body = image.data[start_offset:end_offset]
        for rel_offset, byte in enumerate(body[:-4]):
            if byte != 0xE8:
                continue

            call_va = start + rel_offset
            target = direct_call_target(image, call_va)
            if target is None:
                continue

            callee = address_to_name.get(target)
            if callee is None or callee == caller:
                continue

            calls.append(
                FunctionCallXRef(
                    function=callee,
                    ref_function=caller,
                    instr_off=rel_offset,
                    addr_off=1,
                    indirections=0,
                    builds=build.value,
                )
            )

    return sorted(
        calls,
        key=lambda row: (row.ref_function, row.instr_off, row.function),
    )


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)

    parser.add_argument("--blueprint", type=existing_file, required=True)
    parser.add_argument("--gamefile", type=existing_file, required=True)
    parser.add_argument("--build", type=region_arg, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument(
        "--resolver",
        type=existing_file,
        default=Path(__file__).resolve().with_name("resolve_symbol_addresses.py"),
        help="symbol resolver script used to locate function entrypoints",
    )

    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)

    resolved = resolve_symbols(
        resolver=args.resolver,
        blueprint=args.blueprint,
        gamefile=args.gamefile,
        build=args.build,
    )

    payload = FunctionCallXRefPayload(
        schema_version=1,
        build=args.build.value,
        source_game=str(args.gamefile),
        source_game_sha256=resolved.source_game_sha256,
        unresolved=resolved.unresolved,
        calls=scan_direct_calls(
            Image(args.gamefile.read_bytes()),
            resolved,
            args.build,
        ),
    )

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(asdict(payload), indent=2, sort_keys=True) + "\n")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
