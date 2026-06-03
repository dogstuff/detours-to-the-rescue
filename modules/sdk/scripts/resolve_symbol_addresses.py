#!/usr/bin/env python3

from __future__ import annotations

import argparse
import hashlib
import json
import struct
import sys
from collections.abc import Iterator
from dataclasses import asdict, dataclass
from enum import StrEnum
from pathlib import Path

import pefile

sys.dont_write_bytecode = True

from codegen import pattern_tokens  # noqa: E402
from generate_headers import build_mask_bits, load_blueprint  # noqa: E402


class Region(StrEnum):
    EN = "EN"
    EU = "EU"
    SC = "SC"


REGION_BITS = {Region.EN: 0b001, Region.EU: 0b010, Region.SC: 0b100}


@dataclass(frozen=True, slots=True)
class Section:
    va: int
    virtual_size: int
    raw_offset: int
    raw_size: int


@dataclass(frozen=True, slots=True)
class RefSite:
    name: str
    ref_function: str
    resolver: str
    instr_off: int
    addr_off: int
    indirections: int


@dataclass(frozen=True, slots=True)
class UnresolvedSymbols:
    signatures: list[dict[str, str]]
    functions: list[dict[str, str]]
    data: list[dict[str, str]]


@dataclass(frozen=True, slots=True)
class ResolvePayload:
    schema_version: int
    build: str
    source_game: str
    source_game_sha256: str
    image_base: str
    signatures: dict[str, str]
    functions: dict[str, str]
    data: dict[str, str]
    unresolved: UnresolvedSymbols


class Image:
    def __init__(self, data: bytes) -> None:
        self.data = data

        try:
            pe = pefile.PE(data=data, fast_load=True)
        except pefile.PEFormatError:
            self.image_base = 0
            self.sections = [Section(0, len(data), 0, len(data))]
            return

        self.image_base = pe.OPTIONAL_HEADER.ImageBase
        self.sections = [
            Section(
                section.VirtualAddress,
                section.Misc_VirtualSize,
                section.PointerToRawData,
                section.SizeOfRawData,
            )
            for section in pe.sections
            if section.SizeOfRawData and section.PointerToRawData < len(data)
        ]

    def offset_to_va(self, offset: int) -> int | None:
        for section in self.sections:
            start = section.raw_offset
            end = section.raw_offset + section.raw_size
            if not (start <= offset < end):
                continue

            return self.image_base + section.va + (offset - section.raw_offset)

        if self.image_base == 0 and 0 <= offset < len(self.data):
            return offset

        return None

    def va_to_offset(self, va: int) -> int | None:
        rva = va - self.image_base
        if rva < 0:
            return None

        for section in self.sections:
            if not (section.va <= rva < section.va + section.raw_size):
                continue

            offset = section.raw_offset + (rva - section.va)
            return offset if 0 <= offset < len(self.data) else None

        if self.image_base == 0 and 0 <= va < len(self.data):
            return va

        return None

    def read_u32_va(self, va: int) -> int | None:
        offset = self.va_to_offset(va)
        if offset is None or offset + 4 > len(self.data):
            return None

        return struct.unpack_from("<I", self.data, offset)[0]

    def scan(self, pattern: str) -> list[int]:
        pattern_bytes = [
            None if t == "??" else int(t, 16) for t in pattern_tokens(pattern)
        ]
        if not pattern_bytes:
            return []

        anchor_offset, anchor = longest_concrete_run(pattern_bytes)
        matches: list[int] = []
        for section in self.sections:
            start = section.raw_offset
            end = min(len(self.data), start + section.raw_size)
            section_data = self.data[start:end]
            if len(section_data) < len(pattern_bytes):
                continue

            for rel in candidate_offsets(
                section_data, pattern_bytes, anchor_offset, anchor
            ):
                va = self.offset_to_va(start + rel)
                if va is None:
                    continue

                matches.append(va)

        return matches


def longest_concrete_run(pattern_bytes: list[int | None]) -> tuple[int, bytes]:
    best_start = start = 0
    best: list[int] = []
    current: list[int] = []

    for index, byte in enumerate(pattern_bytes):
        if byte is not None:
            current.append(byte)
            continue

        if len(current) > len(best):
            best_start, best = start, current

        start = index + 1
        current = []

    if len(current) > len(best):
        best_start, best = start, current

    return best_start, bytes(best)


def candidate_offsets(
    section_data: bytes,
    pattern_bytes: list[int | None],
    anchor_offset: int,
    anchor: bytes,
) -> Iterator[int]:
    starts: range | Iterator[int]
    if anchor:
        starts = anchored_offsets(
            section_data, len(pattern_bytes), anchor_offset, anchor
        )
    else:
        starts = range(len(section_data) - len(pattern_bytes) + 1)

    for offset in starts:
        if not all(
            byte is None or section_data[offset + i] == byte
            for i, byte in enumerate(pattern_bytes)
        ):
            continue

        yield offset


def anchored_offsets(
    section_data: bytes, pattern_length: int, anchor_offset: int, anchor: bytes
) -> Iterator[int]:
    rel = section_data.find(anchor)
    while rel != -1:
        offset = rel - anchor_offset
        if 0 <= offset <= len(section_data) - pattern_length:
            yield offset

        rel = section_data.find(anchor, rel + 1)


def supported(required: object, region: Region) -> bool:
    return bool(build_mask_bits(required) & REGION_BITS[region])


def hex_addr(value: int) -> str:
    return f"0x{value:X}"


def unresolved_rows(reasons: dict[str, str]) -> list[dict[str, str]]:
    return [
        {"name": name, "reason": reason} for name, reason in sorted(reasons.items())
    ]


def scan_one(
    image: Image, row: object, match_offset: int = 0
) -> tuple[int | None, str | None]:
    matches = image.scan(row.pattern)
    if not matches:
        return None, "pattern-not-found"

    if len(matches) > 1:
        return None, f"pattern-ambiguous:{len(matches)}"

    address = matches[0] + match_offset
    if image.va_to_offset(address) is None:
        return None, "invalid-match-offset"

    return address, None


def scan_rows(
    image: Image, rows: list[object], match_offset_attr: str | None = None
) -> tuple[dict[str, int], dict[str, str]]:
    resolved: dict[str, int] = {}
    unresolved: dict[str, str] = {}

    for row in rows:
        match_offset = (
            int(getattr(row, match_offset_attr, 0)) if match_offset_attr else 0
        )
        address, reason = scan_one(image, row, match_offset)
        if address is None:
            unresolved[row.name] = reason or "unresolved"
            continue

        resolved[row.name] = address

    return resolved, unresolved


def resolve_signatures(
    blueprint: object, image: Image, region: Region
) -> tuple[dict[str, int], list[dict[str, str]]]:
    rows = [row for row in blueprint.signatures if supported(row.required, region)]
    resolved, unresolved = scan_rows(image, rows)
    return resolved, unresolved_rows(unresolved)


def resolve_xref_u32(
    image: Image, base: int, instr_off: int, addr_off: int, indirections: int
) -> int | None:
    value = image.read_u32_va(base + instr_off + addr_off)
    while value and indirections:
        value = image.read_u32_va(value)
        indirections -= 1

    return value or None


def resolve_functions(
    blueprint: object, image: Image, region: Region
) -> tuple[dict[str, int], list[dict[str, str]]]:
    rows = [row for row in blueprint.functions if supported(row.required, region)]
    supported_names = {row.name for row in rows}
    xrefs = [
        row
        for row in blueprint.function_xrefs
        if row.function in supported_names
        and row.ref_function in supported_names
        and supported(row.required, region)
    ]

    resolved, unresolved = scan_rows(image, rows, "match_offset")

    made_progress = True
    while made_progress:
        made_progress = False
        for xref in xrefs:
            if xref.function in resolved or xref.ref_function not in resolved:
                continue

            value = resolve_xref_u32(
                image,
                resolved[xref.ref_function],
                xref.instr_off,
                xref.addr_off,
                xref.indirections,
            )
            if value is None:
                continue

            if image.va_to_offset(value) is None:
                unresolved[xref.function] = f"invalid-xref-target:{xref.ref_function}"
                continue

            resolved[xref.function] = value
            unresolved.pop(xref.function, None)
            made_progress = True

    return resolved, unresolved_rows(unresolved)


def iter_global_refs(
    blueprint: object, region: Region, supported_functions: set[str]
) -> Iterator[RefSite]:
    for row in blueprint.globals:
        typed = row.typed
        if not typed:
            continue

        if typed.ref_function not in supported_functions:
            continue

        if not supported(typed.required, region):
            continue

        yield RefSite(
            row.name,
            typed.ref_function,
            str(typed.resolver),
            typed.instr_off,
            typed.addr_off,
            typed.indirections,
        )

    for row in blueprint.xrefs:
        if row.function not in supported_functions:
            continue

        if not supported(row.required, region):
            continue

        yield RefSite(
            row.global_name,
            row.function,
            "xref_u32",
            row.instr_off,
            row.addr_off,
            row.indirections,
        )


def resolve_data(
    blueprint: object, image: Image, functions: dict[str, int], region: Region
) -> tuple[dict[str, int], list[dict[str, str]]]:
    supported_functions = {
        row.name for row in blueprint.functions if supported(row.required, region)
    }
    resolved: dict[str, int] = {}
    unresolved: dict[str, str] = {}

    for ref in iter_global_refs(blueprint, region, supported_functions):
        if ref.name in resolved:
            continue

        base = functions.get(ref.ref_function)
        if base is None:
            unresolved.setdefault(
                ref.name, f"unresolved-ref-function:{ref.ref_function}"
            )
            continue

        if ref.resolver != "xref_u32":
            unresolved.setdefault(ref.name, f"unsupported-resolver:{ref.resolver}")
            continue

        value = resolve_xref_u32(
            image, base, ref.instr_off, ref.addr_off, ref.indirections
        )
        if value is None:
            unresolved.setdefault(ref.name, f"xref-read-failed:{ref.ref_function}")
            continue

        resolved[ref.name] = value
        unresolved.pop(ref.name, None)

    return resolved, unresolved_rows(unresolved)


def resolve_payload(
    blueprint: object, image: Image, gamefile: Path, region: Region
) -> ResolvePayload:
    signatures, unresolved_signatures = resolve_signatures(blueprint, image, region)
    functions, unresolved_functions = resolve_functions(blueprint, image, region)
    data, unresolved_data = resolve_data(blueprint, image, functions, region)

    return ResolvePayload(
        schema_version=1,
        build=region.value,
        source_game=str(gamefile),
        source_game_sha256=hashlib.sha256(image.data).hexdigest(),
        image_base=hex_addr(image.image_base),
        signatures={name: hex_addr(addr) for name, addr in sorted(signatures.items())},
        functions={name: hex_addr(addr) for name, addr in sorted(functions.items())},
        data={name: hex_addr(addr) for name, addr in sorted(data.items())},
        unresolved=UnresolvedSymbols(
            signatures=unresolved_signatures,
            functions=unresolved_functions,
            data=unresolved_data,
        ),
    )


def region_arg(value: str) -> Region:
    try:
        return Region(value)
    except ValueError as exc:
        choices = ", ".join(region.value for region in Region)
        raise argparse.ArgumentTypeError(f"expected one of: {choices}") from exc


def existing_file(path: str) -> Path:
    resolved = Path(path)
    if not resolved.is_file():
        raise argparse.ArgumentTypeError(f"file not found: {path}")

    return resolved


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Resolve PCDOGS SDK symbol addresses.")
    parser.add_argument("--blueprint", type=existing_file, required=True)
    parser.add_argument("--gamefile", type=existing_file, required=True)
    parser.add_argument("--build", type=region_arg, required=True)
    parser.add_argument("--output", type=Path, required=True)
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)
    payload = resolve_payload(
        load_blueprint(args.blueprint),
        Image(args.gamefile.read_bytes()),
        args.gamefile,
        args.build,
    )
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(asdict(payload), indent=2, sort_keys=True) + "\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
