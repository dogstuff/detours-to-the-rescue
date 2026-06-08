#!/usr/bin/env python3
"""Generate user-facing Markdown reference pages from the PCDOGS SDK blueprints."""

from __future__ import annotations

import argparse
import difflib
import html
import json
import re
import sys
from collections import Counter, deque
from dataclasses import dataclass, field, replace
from functools import lru_cache
from pathlib import Path
from typing import Callable, Iterator

try:
    from mako.template import Template
except ImportError as exc:
    raise SystemExit(
        "generate_symbol_docs.py requires Mako; install it or enter the Nix dev shell."
    ) from exc

sys.dont_write_bytecode = True

from blueprint import Required  # noqa: E402
from generate_headers import (  # noqa: E402
    CC_KEYWORD,
    SymbolDocKind,
    TypeRowKind,
    build_mask_bits,
    c_array_type_parts,
    c_public_token,
    data_write_policy,
    header_context,
    load_blueprint,
    pcdogs_type_name,
    public_function_rows,
    row_doc,
    split_row_unstable_rows,
    symbol_doc,
    type_row_kind,
)
from symbol_docs_model import (  # noqa: E402
    Category,
    DataXRefCard,
    EnumValueCard,
    FunctionCard,
    FunctionXRefCard,
    GlobalCard,
    MemberCard,
    MetadataItem,
    OverviewRow,
    OverviewTotals,
    SignatureCard,
    SurfaceCards,
    SymbolFact,
    TypeCard,
    XRefItem,
)

SymbolPage = FunctionCard | GlobalCard | TypeCard | SignatureCard

DECIMAL_PLUS_OFFSET = re.compile(r"(?<![eE])\+([0-9]+)\b")
INTEGER_C_TYPES = re.compile(
    r"^(u?int(8|16|32|64)_t|DWORD|WORD|BYTE|BOOL|int|uint32_t|uintptr_t)$"
)

CODE_METADATA_LABELS = {
    "AOB Target",
    "AOB Pattern",
    "Hook Type",
    "Patch Size",
    "Entry Patch Size",
    "Match Offset",
    "Symbol ID",
    "SDK Accessor / ID",
    "Resolver",
    "C Name",
    "Type",
    "Write Policy",
}


BUILD_BITS = (
    ("EN", 0b001),
    ("EU", 0b010),
    ("SC", 0b100),
)
BUILD_BIT_BY_LABEL = dict(BUILD_BITS)


@dataclass(frozen=True, slots=True)
class FunctionCallKey:
    function: str
    ref_function: str
    instr_off: int
    addr_off: int
    indirections: int


@dataclass(frozen=True, slots=True)
class FunctionCallXRefRow:
    key: FunctionCallKey
    builds: str

    @classmethod
    def from_json(
        cls, data: object, *, default_build: str, path: Path
    ) -> FunctionCallXRefRow:
        if not isinstance(data, dict):
            raise ValueError(f"function call xref row must be an object: {path}")

        return cls(
            key=FunctionCallKey(
                function=str(data.get("function", "")),
                ref_function=str(data.get("ref_function", "")),
                instr_off=int(data.get("instr_off", 0)),
                addr_off=int(data.get("addr_off", 0)),
                indirections=int(data.get("indirections", 0)),
            ),
            builds=str(data.get("builds") or default_build).strip(),
        )


@dataclass(slots=True)
class FunctionCallXRefAccumulator:
    card: FunctionXRefCard
    builds: set[str] = field(default_factory=set)

    @classmethod
    def from_key(cls, key: FunctionCallKey) -> FunctionCallXRefAccumulator:
        return cls(
            card=FunctionXRefCard(
                category=category_slug(key.function),
                function=key.function,
                ref_function=key.ref_function,
                offsets=offset_pair(key.instr_off, key.addr_off),
                indirections=key.indirections,
                detail="Best-effort direct E8 scan",
            )
        )

    def add(self, row: FunctionCallXRefRow) -> None:
        if row.builds:
            self.builds.update(row.builds.split())

    def finish(self) -> FunctionXRefCard:
        self.card.builds = html.escape(merged_build_label(self.builds))
        return self.card


@dataclass(frozen=True, slots=True)
class FunctionCallXRefInput:
    build: str
    calls: list[object]

    @classmethod
    def from_path(cls, path: Path) -> FunctionCallXRefInput:
        payload = load_json_payload(path, "function call xref")
        require_schema_version(payload, path, "function call xref")

        calls = payload.get("calls", [])
        if not isinstance(calls, list):
            raise ValueError(f"function call xref calls must be a list: {path}")

        return cls(build=str(payload.get("build", "")).strip(), calls=calls)


@dataclass(frozen=True, slots=True)
class SymbolAddressInput:
    build: str
    image_base: object
    provenance: str
    functions: dict[str, object]
    data: dict[str, object]
    signatures: dict[str, object]

    @classmethod
    def from_path(cls, path: Path) -> SymbolAddressInput:
        payload = load_json_payload(path, "symbol address")
        require_schema_version(payload, path, "symbol address")

        build = str(payload.get("build", "")).strip()
        image_base = payload.get("image_base")
        if not build or image_base is None:
            raise ValueError(
                f"symbol address payload missing build or image_base: {path}"
            )

        return cls(
            build=build,
            image_base=image_base,
            provenance="",
            functions=symbol_address_map(payload, "functions", path),
            data=symbol_address_map(payload, "data", path),
            signatures=symbol_address_map(payload, "signatures", path),
        )


@dataclass(slots=True)
class SymbolRvaIndex:
    functions: dict[str, list[MetadataItem]]
    data: dict[str, list[MetadataItem]]
    signatures: dict[str, list[MetadataItem]]

    @classmethod
    def empty(cls) -> SymbolRvaIndex:
        return cls(functions={}, data={}, signatures={})

    def add_payload(self, payload: SymbolAddressInput) -> None:
        self._add_rows(self.functions, payload.functions, payload)
        self._add_rows(self.data, payload.data, payload)
        self._add_rows(self.signatures, payload.signatures, payload)

    def sort(self) -> None:
        for section in (self.functions, self.data, self.signatures):
            for rows in section.values():
                rows.sort(key=metadata_build_bits)

    def function_metadata(self, name: object) -> list[MetadataItem]:
        return self._metadata_for(self.functions, name)

    def data_metadata(self, name: object) -> list[MetadataItem]:
        return self._metadata_for(self.data, name)

    def signature_metadata(self, name: object) -> list[MetadataItem]:
        return self._metadata_for(self.signatures, name)

    def _metadata_for(
        self, section: dict[str, list[MetadataItem]], name: object
    ) -> list[MetadataItem]:
        return list(section.get(str(name), []))

    def _add_rows(
        self,
        target: dict[str, list[MetadataItem]],
        symbols: dict[str, object],
        payload: SymbolAddressInput,
    ) -> None:
        for name, address in symbols.items():
            target.setdefault(name, []).append(
                MetadataItem(
                    label=html.escape(f"Address ({payload.build})", quote=True),
                    value=html.escape(
                        address_detail(
                            address,
                            image_base=payload.image_base,
                            provenance=payload.provenance,
                        ),
                        quote=True,
                    ),
                )
            )


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Generate Markdown reference pages from PCDOGS SDK blueprints.",
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        required=True,
        help="Directory where generated Markdown pages are written.",
    )
    parser.add_argument(
        "--stable-blueprint",
        type=Path,
        default=None,
        help="Stable blueprint path. Defaults to modules/sdk/blueprints/dttr_pcdogs.py.",
    )
    parser.add_argument("--check", action="store_true", help="fail if output is stale")
    parser.add_argument(
        "--function-call-xrefs",
        action="append",
        type=Path,
        default=[],
        help="JSON call xrefs generated by generate_function_call_xrefs.py",
    )
    parser.add_argument(
        "--symbol-addresses",
        action="append",
        type=Path,
        default=[],
        help="JSON symbol addresses generated by resolve_symbol_addresses.py",
    )
    return parser.parse_args()


def sdk_root() -> Path:
    return Path(__file__).resolve().parent.parent


def default_blueprint(args: argparse.Namespace) -> Path:
    root = sdk_root()
    return args.stable_blueprint or root / "blueprints/dttr_pcdogs.py"


@lru_cache
def template(name: str) -> Template:
    path = Path(__file__).resolve().with_name(name)
    return Template(path.read_text(), filename=str(path), strict_undefined=True)


def render_template(name: str, **context: object) -> str:
    return template(name).render(**context).strip("\n")


def hex_offset(value: object) -> str:
    return f"+0x{int(value):X}"


def hex_signed(value: object) -> str:
    number = int(value)
    sign = "-" if number < 0 else ""
    return f"{sign}0x{abs(number):X}"


def offset_pair(instr_off: object, addr_off: object) -> str:
    return f"{hex_offset(instr_off)}/{hex_offset(addr_off)}"


def hex_memory_offsets(text: object) -> str:
    return DECIMAL_PLUS_OFFSET.sub(lambda match: hex_offset(match.group(1)), str(text))


def undocumented_text() -> str:
    return '<span class="pcdogs-undocumented">Not yet documented.</span>'


def markdown_text(value: object) -> str:
    if value is None:
        return undocumented_text()
    text = " ".join(str(value).strip().split())
    if not text or text in {"None", "Not yet documented."}:
        return undocumented_text()
    return hex_memory_offsets(text)


def escape_table_cell(value: object) -> str:
    return markdown_text(value).replace("|", "\\|")


def title_case_metadata_label(label: str) -> str:
    acronyms = {"aob": "AOB", "c": "C", "id": "ID", "sdk": "SDK"}

    def title_word(word: str) -> str:
        return acronyms.get(word.lower(), word[:1].upper() + word[1:].lower())

    return " / ".join(
        " ".join(title_word(word) for word in part.split())
        for part in label.split(" / ")
    )


def metadata_items(lines: list[object]) -> list[MetadataItem]:
    items = []

    for line in lines:
        text = str(line).strip()
        if not text:
            continue

        if ": " in text:
            label, value = text.split(": ", 1)
        else:
            label, value = "Metadata", text

        if value.strip() in {"", "-"}:
            continue

        label = title_case_metadata_label(label)
        value = html.escape(value, quote=True)
        if label in CODE_METADATA_LABELS:
            value = f"<code>{value}</code>"

        items.append(
            MetadataItem(
                label=html.escape(label, quote=True),
                value=value,
            )
        )

    return items


def load_json_payload(path: Path, label: str) -> dict[str, object]:
    payload = json.loads(path.read_text())
    if not isinstance(payload, dict):
        raise ValueError(f"{label} payload must be an object: {path}")

    return payload


def require_schema_version(payload: dict[str, object], path: Path, label: str) -> None:
    if int(payload.get("schema_version", 0)) != 1:
        raise ValueError(f"unsupported {label} schema: {path}")


def parse_hex_int(value: object) -> int:
    return int(str(value).strip(), 0)


def rva_value(address: object, image_base: object) -> str:
    rva = parse_hex_int(address) - parse_hex_int(image_base)
    return f"pcdogs.exe + 0x{rva:06X}"


def address_detail(address: object, *, image_base: object, provenance: str) -> str:
    detail = rva_value(address, image_base)
    return f"{detail} ({provenance})" if provenance else detail


def symbol_address_map(
    payload: dict[str, object], key: str, path: Path
) -> dict[str, object]:
    symbols = payload.get(key, {})
    if not isinstance(symbols, dict):
        raise ValueError(f"symbol address {key} must be an object: {path}")

    return {str(name): address for name, address in symbols.items()}


def metadata_build_bits(item: MetadataItem) -> int | None:
    label = html.unescape(item.label)
    build = label.removeprefix("Address (").removesuffix(")")
    return build_label_bits(build)


def symbol_rva_index(paths: list[Path]) -> SymbolRvaIndex:
    index = SymbolRvaIndex.empty()
    for path in paths:
        index.add_payload(SymbolAddressInput.from_path(path))

    index.sort()
    return index


def aob_target(kind: str, match_offset: object = 0) -> str:
    if kind == "function":
        offset = int(match_offset)
        if offset == 0:
            return "Function Prologue"

        return f"Function Body (entry = match {hex_signed(offset)})"

    if kind == "signature":
        return "Signature Match"

    return kind


def slug(value: object) -> str:
    text = str(value).strip().lower()
    text = re.sub(r"[^a-z0-9]+", "-", text).strip("-")
    return text or "misc"


def anchor(kind: str, name: object) -> str:
    return f"{kind}-{slug(name)}"


def category_prefix(name: object) -> str:
    text = str(name).strip()
    if "_" not in text:
        return "misc"
    return text.split("_", 1)[0]


def category_display(name: object) -> str:
    prefix = category_prefix(name)
    if prefix.islower():
        return prefix[:1].upper() + prefix[1:]

    return prefix


def category_slug(name: object) -> str:
    return slug(category_display(name))


def constrained_category_slug(name: object, domain_slugs: set[str]) -> str:
    candidate = category_slug(name)
    return candidate if candidate in domain_slugs else "misc"


def build_mask_label(value: object) -> str:
    bits = build_mask_bits(value)
    if bits == 0:
        return "None"

    return " ".join(label for label, bit in BUILD_BITS if bits & bit)


def build_label_bits(label: str) -> int | None:
    if label in {"All", str(Required.ALL)}:
        return build_mask_bits(Required.ALL)

    bits = 0
    for part in label.split():
        bit = BUILD_BIT_BY_LABEL.get(part)
        if bit is None:
            return None

        bits |= bit

    return bits


def xref_display_builds(value: object) -> str:
    label = value.strip() if isinstance(value, str) else build_mask_label(value)
    if not label:
        return ""

    bits = build_label_bits(label)
    return "" if bits == build_mask_bits(Required.ALL) else label


def function_pseudo_usage(fn: object, c_type_fn: Callable[[object], str]) -> str:
    typed = fn.typed
    if not typed:
        return "// No typed usage sketch is available yet."

    args = [sample_argument(param, c_type_fn(param.type)) for param in typed.params]
    if args:
        call = f"{fn.name}(\n" + ",\n".join(f"    {arg}" for arg in args) + "\n)"
    else:
        call = f"{fn.name}()"

    return_type = c_type_fn(typed.return_type).strip()
    if return_type == "void":
        return f"{call};"

    return f"{return_type} result = {call};"


def function_related_type_texts(
    fn: object, c_type_fn: Callable[[object], str]
) -> list[str]:
    if not fn.typed:
        return []
    texts = [str(fn.typed.return_type), c_type_fn(fn.typed.return_type)]
    for param in fn.typed.params:
        texts.extend([str(param.type), c_type_fn(param.type)])
    return texts


def call_signature_key(
    calling: object,
    return_type: object,
    params: object,
    c_type_fn: Callable[[object], str],
) -> tuple[str, ...]:
    return (
        str(calling),
        c_type_fn(return_type),
        *(c_type_fn(param.type) for param in params),
    )


def function_call_signature_key(
    fn: object, c_type_fn: Callable[[object], str]
) -> tuple[str, ...]:
    if not fn.typed:
        return ()

    return call_signature_key(
        fn.typed.abi,
        fn.typed.return_type,
        fn.typed.params,
        c_type_fn,
    )


def c_value_declaration(c_type: str, name: str, value: str) -> str:
    parts = c_array_type_parts(c_type)

    if parts is None:
        return f"{c_type} {name} = {value};"

    base, count = parts
    return f"{base} {name}[{count}] = {{0}};"


def data_default_value(c_type: str) -> str:
    normalized = c_type.strip()
    if c_array_type_parts(normalized) is not None:
        return "{0}"

    if "*" in normalized:
        return f"({normalized})NULL"

    if normalized == "bool":
        return "false"

    if normalized == "BOOL":
        return "FALSE"

    if normalized == "float":
        return "0.0f"

    if normalized == "double":
        return "0.0"

    if INTEGER_C_TYPES.match(normalized):
        return f"({normalized})0"

    return "{0}"


def data_read_argument(c_type: str, name: str) -> str:
    return f"&{name}"


def data_write_argument(c_type: str, name: str) -> str:
    if c_array_type_parts(c_type) is not None:
        return f"&{name}"
    return name


def sample_argument(param: object, c_type: str) -> str:
    normalized = c_type.strip()
    return f"({normalized}){sample_argument_name(param, normalized)}"


def sample_argument_name(param: object, c_type: str) -> str:
    handle_names = {
        "HANDLE": "handle",
        "HINSTANCE": "instance_handle",
        "HMODULE": "module_handle",
        "HWND": "window_handle",
    }
    if c_type in handle_names:
        return handle_names[c_type]

    name = getattr(param, "name", "") or ""
    name = re.sub(r"[^0-9A-Za-z_]+", "_", str(name)).strip("_")
    if name:
        if name[0].isdigit():
            return f"value_{name}"

        return name

    if "*" in c_type:
        return "value"

    if c_type == "bool":
        return "flag"

    if c_type == "BOOL":
        return "flag"

    if c_type == "float":
        return "value"

    if c_type == "double":
        return "value"

    return "value"


def sample_value(c_type: str) -> str:
    normalized = c_type.strip()
    if "*" in normalized:
        return f"({normalized})NULL"

    if normalized in {"HANDLE", "HINSTANCE", "HMODULE", "HWND"}:
        return f"({normalized})NULL"

    if normalized == "bool":
        return "false"

    if normalized == "BOOL":
        return "FALSE"

    if normalized == "float":
        return "0.0f"

    if normalized == "double":
        return "0.0"

    return f"({normalized})0"


def function_hook_possible(fn: object) -> bool:
    return bool(fn.public and fn.typed and fn.callable and str(fn.hook.kind) == "rel32")


def c_param_decls(params: object, c_type_fn: Callable[[object], str]) -> list[str]:
    return [f"{c_type_fn(param.type)} {param.name}" for param in params]


def c_detour_function(
    name: str,
    return_type: str,
    cc: str,
    param_decls: list[str],
    body_call: str,
) -> list[str]:
    lines = []

    if param_decls:
        lines.append(f"static {return_type} {cc} {name}(")

        for index, param in enumerate(param_decls):
            suffix = "," if index < len(param_decls) - 1 else ""
            lines.append(f"    {param}{suffix}")

        lines.append(") {")
    else:
        lines.append(f"static {return_type} {cc} {name}() {{")

    if return_type == "void":
        lines.append(f"    {body_call};")
    else:
        lines.append(f"    return {body_call};")

    lines.append("}")
    return lines


def function_detour_boilerplate(
    fn: object, c_type_fn: Callable[[object], str]
) -> tuple[str, list[str]]:
    public = c_public_token(fn.name)
    typed = fn.typed
    param_decls = c_param_decls(typed.params, c_type_fn)
    arg_text = ", ".join(str(param.name) for param in typed.params)
    original_call = f"original({arg_text})" if arg_text else "original()"
    lines = [f"static DTTR_PCDOGS_F_{public}_proto original;", ""]

    lines.extend(
        c_detour_function(
            f"{public}_hook",
            c_type_fn(typed.return_type),
            CC_KEYWORD[str(typed.abi)],
            param_decls,
            original_call,
        )
    )
    return f"{public}_hook", lines


def function_hook_example(
    fn: object, c_type_fn: Callable[[object], str], api: str
) -> str:
    if not function_hook_possible(fn):
        return ""

    hook_name, lines = function_detour_boilerplate(fn, c_type_fn)
    lines.extend(
        [
            "",
            f"{api}->Hook(",
            "    &ctx->runtime,",
            f"    {hook_name},",
            "    &original",
            ");",
        ]
    )
    return "\n".join(lines)


def function_call_example(
    fn: object, c_type_fn: Callable[[object], str], api: str
) -> str:
    typed = fn.typed
    if not typed:
        return "// No typed SDK call example is available yet."
    if not fn.callable:
        return "// This symbol is not marked safe to call through the SDK."

    params = [sample_value(c_type_fn(param.type)) for param in typed.params]
    return_type = c_type_fn(typed.return_type)
    call_args = ["&ctx->runtime", *params]
    return_declaration = ""
    if return_type != "void":
        return_declaration = f"{return_type} result = {{0}};"
        call_args.append("&result")

    return render_template(
        "symbol_docs_function_call_example.c.mako",
        api=api,
        call_args=call_args,
        return_declaration=return_declaration,
    )


def function_patch_spec_example(
    fn: object, c_type_fn: Callable[[object], str], api: str
) -> str:
    if not fn.typed:
        return "// No typed SDK patch spec example is available yet."
    if str(fn.hook.kind) != "rel32":
        return "// PatchSpec() returns an unsupported spec for non-REL32 hook sites."

    hook_name, lines = function_detour_boilerplate(fn, c_type_fn)
    lines.extend(
        [
            "",
            f"{api}->PatchSpec(",
            "    true,",
            f"    {hook_name},",
            "    &original",
            ");",
        ]
    )
    return "\n".join(lines)


def function_cards(
    functions: list[object],
    c_type_fn: Callable[[object], str],
    *,
    unstable: bool,
    symbol_rvas: SymbolRvaIndex,
) -> list[FunctionCard]:
    cards = []

    for fn in functions:
        if not fn.public:
            continue

        public = c_public_token(fn.name)
        api = f"DTTR_PCDOGS_F_{public}"
        cards.append(
            FunctionCard(
                name=str(fn.name),
                heading=str(fn.name),
                category=category_slug(fn.name),
                anchor=anchor("unstable-function" if unstable else "function", fn.name),
                pseudo_usage=function_pseudo_usage(fn, c_type_fn),
                call_example=function_call_example(fn, c_type_fn, api),
                patch_spec_example=function_patch_spec_example(fn, c_type_fn, api),
                hook_example=function_hook_example(fn, c_type_fn, api),
                builds=build_mask_label(fn.supported_builds),
                summary=markdown_text(symbol_doc(SymbolDocKind.FUNCTION, fn)),
                metadata=metadata_items(
                    [
                        f"AOB target: {aob_target('function', fn.match_offset)}",
                        f"AOB pattern: {fn.pattern}",
                        f"Hook type: {fn.hook.kind}",
                        f"Patch size: {f'0x{int(fn.hook.patch_size):X}' if fn.hook.patch_size else '-'}",
                        (
                            f"Entry patch size: 0x{int(fn.hook.entry_patch_size):X}"
                            if fn.hook.entry_patch_size
                            and fn.hook.entry_patch_size != fn.hook.patch_size
                            else ""
                        ),
                        (
                            f"Match offset: {hex_signed(fn.match_offset)}"
                            if int(fn.match_offset) != 0
                            else ""
                        ),
                        f"Symbol ID: {fn.symbol_id}",
                    ]
                )
                + symbol_rvas.function_metadata(fn.name),
                related_type_texts=function_related_type_texts(fn, c_type_fn),
                call_signature_key=function_call_signature_key(fn, c_type_fn),
            )
        )

    return cards


def global_type(glob: object, c_type_fn: Callable[[object], str]) -> str:
    return c_type_fn(glob.typed.type) if glob.typed else "-"


def global_accessor(surface_unstable: bool, glob: object) -> str:
    public = c_public_token(glob.name)
    if glob.typed:
        return f"DTTR_PCDOGS_D_{public}"
    prefix = "DTTR_PCDOGS_SYMBOL_DATA_ID_" if surface_unstable else "DTTR_PCDOGS_DATA_"
    return prefix + glob.symbol_id


def global_resolver(glob: object) -> str:
    if not glob.typed:
        return "-"
    return f"{glob.typed.ref_function} {offset_pair(glob.typed.instr_off, glob.typed.addr_off)}"


def global_read_example(glob: object, c_type_fn: Callable[[object], str]) -> str:
    if not glob.typed:
        return "// No typed data read example is available yet."

    c_type = c_type_fn(glob.typed.type)
    accessor = global_accessor(glob.unstable, glob)
    default_value = data_default_value(c_type)

    return render_template(
        "symbol_docs_global_read_example.c.mako",
        accessor=accessor,
        read_argument=data_read_argument(c_type, "value"),
        value_declaration=c_value_declaration(c_type, "value", default_value),
    )


def global_write_example(glob: object, c_type_fn: Callable[[object], str]) -> str:
    if not glob.typed:
        return "// No typed data write example is available yet."

    policy = data_write_policy(glob).removeprefix("DTTR_PCDOGS_WRITE_POLICY_")
    accessor = global_accessor(glob.unstable, glob)
    c_type = c_type_fn(glob.typed.type)
    default_value = data_default_value(c_type)
    return render_template(
        "symbol_docs_global_write_example.c.mako",
        accessor=accessor,
        policy=policy,
        value_declaration=c_value_declaration(c_type, "value", default_value),
        write_allowed=policy == "RAW_MEMORY",
        write_argument=data_write_argument(c_type, "value"),
    )


def global_cards(
    globals_: list[object],
    c_type_fn: Callable[[object], str],
    *,
    unstable: bool,
    domain_slugs: set[str],
    symbol_rvas: SymbolRvaIndex,
) -> list[GlobalCard]:
    cards = []

    for glob in globals_:
        policy = data_write_policy(glob).removeprefix("DTTR_PCDOGS_WRITE_POLICY_")

        cards.append(
            GlobalCard(
                name=str(glob.name),
                category=constrained_category_slug(glob.name, domain_slugs),
                anchor=anchor("unstable-data" if unstable else "data", glob.name),
                type=global_type(glob, c_type_fn),
                builds=build_mask_label(glob.supported_builds),
                write_policy=policy,
                summary=markdown_text(symbol_doc(SymbolDocKind.GLOBAL, glob)),
                read_example=global_read_example(glob, c_type_fn),
                write_example=global_write_example(glob, c_type_fn),
                is_typed=bool(glob.typed),
                untyped_note="Type not recovered yet." if not glob.typed else "",
                metadata=metadata_items(
                    [
                        f"SDK accessor / ID: {global_accessor(unstable, glob)}",
                        f"Type: {global_type(glob, c_type_fn)}",
                        f"Resolver target: {'Data address xref' if glob.typed else 'Raw data symbol'}",
                        f"Resolver: {global_resolver(glob)}",
                        f"Write policy: {policy}",
                        f"Symbol ID: {glob.symbol_id}",
                    ]
                )
                + symbol_rvas.data_metadata(glob.name),
                related_type_texts=(
                    [str(glob.typed.type), global_type(glob, c_type_fn)]
                    if glob.typed
                    else []
                ),
            )
        )

    return cards


def type_summary(row: object, c_type_fn: Callable[[object], str]) -> str:
    kind = type_row_kind(row)
    if kind == TypeRowKind.TYPE_ALIAS:
        return f"alias of `{docs_type_label(c_type_fn(row.source_type))}`"

    if kind == TypeRowKind.FUNCTION_TYPE_ALIAS:
        params = ", ".join(
            f"{docs_type_label(c_type_fn(param.type))} {param.name}"
            for param in row.params
        )
        return f"callback `{docs_type_label(c_type_fn(row.ret))} (*)({params})`"

    if kind == TypeRowKind.STRUCT:
        size = f", size 0x{row.size:X}" if row.size is not None else ""
        completeness = "incomplete" if row.incomplete else "complete"
        return f"{len(row.members)} fields, {completeness}{size}"

    if kind == TypeRowKind.ENUM:
        return f"{len(row.values)} values"

    return "-"


def member_cards(row: object, c_type_fn: Callable[[object], str]) -> list[MemberCard]:
    return [
        MemberCard(
            name=str(member.name),
            type=c_type_fn(member.type),
            offset=f"0x{int(member.offset):X}",
            doc=escape_table_cell(member.doc),
        )
        for member in getattr(row, "members", []) or []
    ]


def enum_values(row: object) -> list[EnumValueCard]:
    return [
        EnumValueCard(
            name=str(value.name),
            value=f"0x{int(value.value):X}",
            doc=markdown_text(getattr(value, "doc", None)),
            table_doc=escape_table_cell(getattr(value, "doc", None)),
        )
        for value in getattr(row, "values", []) or []
    ]


def type_related_texts(row: object, c_type_fn: Callable[[object], str]) -> list[str]:
    kind = type_row_kind(row)
    if kind == TypeRowKind.TYPE_ALIAS:
        return [str(row.source_type), c_type_fn(row.source_type)]
    if kind == TypeRowKind.FUNCTION_TYPE_ALIAS:
        texts = [str(row.ret), c_type_fn(row.ret)]

        for param in row.params:
            texts.extend([str(param.type), c_type_fn(param.type)])

        return texts

    if kind == TypeRowKind.STRUCT:
        texts = []

        for member in row.members:
            texts.extend([str(member.type), c_type_fn(member.type)])

        return texts

    return []


def type_cards(
    types: list[object],
    c_type_fn: Callable[[object], str],
    *,
    domain_slugs: set[str],
    unstable: bool,
) -> list[TypeCard]:
    cards = []

    for row in types:
        kind = type_row_kind(row)
        docs = row_doc(row)
        c_name = pcdogs_type_name(row.name)
        shape = type_summary(row, c_type_fn)
        cards.append(
            TypeCard(
                name=str(row.name),
                category=constrained_category_slug(row.name, domain_slugs),
                anchor=anchor("unstable-type" if unstable else "type", row.name),
                kind_label=title_case_metadata_label(kind.value.replace("_", " ")),
                c_name=c_name,
                shape=shape,
                builds=build_mask_label(Required.ALL),
                summary=markdown_text(docs),
                members=member_cards(row, c_type_fn),
                enum_values=enum_values(row),
                metadata=metadata_items(
                    [
                        f"C name: {c_name}",
                        f"Shape: {shape}",
                    ]
                ),
                related_type_texts=type_related_texts(row, c_type_fn),
                call_signature_key=(
                    call_signature_key(row.calling, row.ret, row.params, c_type_fn)
                    if kind == TypeRowKind.FUNCTION_TYPE_ALIAS
                    else ()
                ),
            )
        )

    return cards


def signature_cards(
    signatures: list[object],
    *,
    domain_slugs: set[str],
    unstable: bool,
    symbol_rvas: SymbolRvaIndex,
) -> list[SignatureCard]:
    return [
        SignatureCard(
            name=str(sig.name),
            category=constrained_category_slug(sig.name, domain_slugs),
            anchor=anchor("unstable-signature" if unstable else "signature", sig.name),
            builds=build_mask_label(sig.required),
            summary=markdown_text(sig.doc),
            metadata=metadata_items(
                [
                    f"AOB target: {aob_target('signature')}",
                    f"AOB pattern: {sig.pattern}",
                ]
            )
            + symbol_rvas.signature_metadata(sig.name),
        )
        for sig in signatures
    ]


def merged_build_label(values: set[str]) -> str:
    ordered_labels = [label for label, _bit in BUILD_BITS if label in values]
    unknown_labels = sorted(values - set(ordered_labels))
    return xref_display_builds(" ".join([*ordered_labels, *unknown_labels]))


def function_call_xref_rows(path: Path) -> list[FunctionCallXRefRow]:
    payload = FunctionCallXRefInput.from_path(path)
    return [
        FunctionCallXRefRow.from_json(row, default_build=payload.build, path=path)
        for row in payload.calls
    ]


def function_call_xref_cards(paths: list[Path]) -> list[FunctionXRefCard]:
    calls_by_key: dict[FunctionCallKey, FunctionCallXRefAccumulator] = {}

    for path in paths:
        for row in function_call_xref_rows(path):
            calls_by_key.setdefault(
                row.key,
                FunctionCallXRefAccumulator.from_key(row.key),
            ).add(row)

    return sorted(
        (item.finish() for item in calls_by_key.values()),
        key=lambda card: (
            card.ref_function,
            card.function,
            card.offsets,
            card.indirections,
        ),
    )


def function_xref_cards(xrefs: list[object]) -> list[FunctionXRefCard]:
    return [
        FunctionXRefCard(
            category=category_slug(xref.function),
            function=str(xref.function),
            ref_function=str(xref.ref_function),
            offsets=offset_pair(xref.instr_off, xref.addr_off),
            indirections=int(xref.indirections),
            builds=xref_display_builds(getattr(xref, "required", Required.ALL)),
            detail="Resolver reference",
        )
        for xref in xrefs
    ]


def data_xref_cards(
    xrefs: list[object], *, domain_slugs: set[str]
) -> list[DataXRefCard]:
    return [
        DataXRefCard(
            category=constrained_category_slug(xref.global_name, domain_slugs),
            global_name=str(xref.global_name),
            function=str(xref.function),
            offsets=offset_pair(xref.instr_off, xref.addr_off),
            access=str(getattr(xref, "access", "R/W") or "R/W"),
            builds=xref_display_builds(getattr(xref, "required", Required.ALL)),
        )
        for xref in xrefs
    ]


def rendered_symbol_link(card: object) -> str:
    return f"../{card.anchor}/"


def xref_link(name: str, card: object | None) -> str:
    if card is None:
        return html.escape(name, quote=True)
    return f'<a href="{html.escape(rendered_symbol_link(card), quote=True)}">{html.escape(name)}</a>'


def xref_items(rows: list[XRefItem]) -> list[XRefItem]:
    items = []

    for row in rows:
        value = str(row.value).strip()
        if not value:
            continue

        items.append(
            XRefItem(
                kind=html.escape(str(row.kind or "Reference"), quote=True),
                value=value,
                offsets=html.escape(str(row.offsets or "-"), quote=True),
                detail=html.escape(str(row.detail or "-"), quote=True),
                builds=html.escape(str(row.builds or ""), quote=True),
                provenance=html.escape(str(row.provenance or ""), quote=True),
            )
        )

    return items


def symbol_fact(label: str, value: object) -> SymbolFact:
    return SymbolFact(label=html.escape(label, quote=True), value=str(value))


def related_link(card: object) -> str:
    return xref_link(str(card.name), card)


def type_reference_matches(text: object, card: TypeCard) -> bool:
    reference_text = str(text)
    if not reference_text or reference_text == "-":
        return False

    names = [card.name, card.c_name]

    return any(
        name
        and re.search(
            rf"(?<![A-Za-z0-9_]){re.escape(name)}(?![A-Za-z0-9_])",
            reference_text,
        )
        for name in names
    )


def type_references(
    texts: list[object], types: list[TypeCard], *, exclude: str = ""
) -> list[TypeCard]:
    found = []

    for card in types:
        name = card.name
        if name == exclude:
            continue

        if any(type_reference_matches(text, card) for text in texts):
            found.append(card)

    return sorted(found, key=lambda card: card.name.casefold())


def docs_type_label(text: object) -> str:
    return re.sub(r"\bDTTR_PCDOGS_T_", "", str(text))


def linked_type_code(
    text: object, type_card: TypeCard, *, display_text: object | None = None
) -> str:
    value = str(docs_type_label(text) if display_text is None else display_text)
    return (
        f'<a href="{html.escape(rendered_symbol_link(type_card), quote=True)}">'
        f"<code>{html.escape(value)}</code></a>"
    )


def link_type_names(text: object, types: list[TypeCard], *, exclude: str = "") -> str:
    escaped = html.escape(str(text), quote=True)

    for card in sorted(types, key=lambda item: len(item.c_name), reverse=True):
        if card.name == exclude:
            continue

        for name in (card.c_name, card.name):
            if not name:
                continue

            escaped_name = html.escape(name, quote=True)
            escaped = re.sub(
                rf"(?<![A-Za-z0-9_]){re.escape(escaped_name)}(?![A-Za-z0-9_])",
                linked_type_code(name, card),
                escaped,
            )

    return escaped


SYMBOL_HREF_RE = re.compile(
    r'href="(?:(?:\.\./)?symbols/|\.\./)?(?P<anchor>[^"#/.]+)(?:/)?"'
)
REFERENCE_HIERARCHY_ENTRYPOINT_NAME = "Window_RunWinMain"
REFERENCE_HIERARCHY_MAX_PATHS_PER_SYMBOL = 128


def referenced_anchor(value: str) -> str:
    match = SYMBOL_HREF_RE.search(str(value))
    return match.group("anchor") if match else ""


def attach_reference_hierarchy_paths(cards: SurfaceCards) -> None:
    graph_cards = cards_with_xrefs(cards)
    by_anchor = {card.anchor: card for card in graph_cards}
    function_anchors = {card.anchor for card in cards.functions}
    function_graph: dict[str, set[str]] = {anchor: set() for anchor in function_anchors}
    function_references: dict[str, set[str]] = {
        anchor: set() for anchor in function_anchors
    }

    for card in cards.functions:
        for row in getattr(card, "references", []):
            target_anchor = referenced_anchor(row.value)
            if (
                not target_anchor
                or target_anchor == card.anchor
                or target_anchor not in by_anchor
            ):
                continue

            function_references[card.anchor].add(target_anchor)
            if target_anchor in function_anchors:
                function_graph[card.anchor].add(target_anchor)

    entrypoint = next(
        (
            card.anchor
            for card in cards.functions
            if card.name == REFERENCE_HIERARCHY_ENTRYPOINT_NAME
        ),
        "",
    )
    callback_signature_keys = {
        card.call_signature_key for card in cards.types if card.call_signature_key
    }

    incoming_function_anchors = {
        target for targets in function_graph.values() for target in targets
    }

    callback_roots = {
        anchor
        for anchor in function_anchors - incoming_function_anchors
        if getattr(by_anchor[anchor], "call_signature_key", ())
        in callback_signature_keys
        and (function_graph.get(anchor) or function_references.get(anchor))
    }

    blocked_roots = callback_roots - {entrypoint}

    def card_sort_key(anchor: str) -> tuple[str, str]:
        return str(by_anchor[anchor].name).casefold(), anchor

    sorted_function_graph = {
        anchor: tuple(sorted(targets, key=card_sort_key))
        for anchor, targets in function_graph.items()
    }

    sorted_function_references = {
        anchor: tuple(sorted(targets, key=card_sort_key))
        for anchor, targets in function_references.items()
    }

    found: dict[str, set[tuple[str, ...]]] = {anchor: set() for anchor in by_anchor}
    queue: deque[tuple[str, ...]] = deque()

    def add_path(anchor: str, path: tuple[str, ...]) -> bool:
        if not entrypoint or not path or path[0] != entrypoint:
            return False

        if any(item in blocked_roots for item in path[1:]):
            return False

        paths = found[anchor]
        if path in paths:
            return False

        if len(paths) >= REFERENCE_HIERARCHY_MAX_PATHS_PER_SYMBOL:
            return False

        paths.add(path)
        return True

    if entrypoint:
        root_path = (entrypoint,)
        if add_path(entrypoint, root_path):
            queue.append(root_path)

    while queue:
        path = queue.popleft()
        current = path[-1]

        for target in sorted_function_graph.get(current, ()):
            if target in path:
                continue

            if target in blocked_roots:
                continue

            next_path = (*path, target)
            if add_path(target, next_path):
                queue.append(next_path)

    for function_anchor, paths in list(found.items()):
        if function_anchor not in function_anchors or not paths:
            continue

        for target in sorted_function_references.get(function_anchor, ()):
            if target in function_anchors:
                continue

            for path in paths:
                add_path(target, (*path, target))

    for anchor, card in by_anchor.items():
        paths = sorted(
            found.get(anchor, []),
            key=lambda path: (
                len(path),
                [str(by_anchor[item].name).casefold() for item in path],
            ),
        )
        card.reference_hierarchy_paths = paths


def cards_with_xrefs(cards: SurfaceCards) -> list[FunctionCard | GlobalCard | TypeCard]:
    return [*cards.functions, *cards.globals, *cards.types]


def normalize_xrefs(cards: SurfaceCards) -> None:
    for card in cards_with_xrefs(cards):
        card.references = xref_items(card.references)
        card.referenced_by = xref_items(card.referenced_by)


def attach_xrefs(cards: SurfaceCards) -> None:
    functions = cards.functions
    function_by_name = {card.name: card for card in functions}
    global_by_name = {card.name: card for card in cards.globals}

    for card in cards_with_xrefs(cards):
        card.references = []
        card.referenced_by = []

    for xref in cards.function_xrefs:
        caller = function_by_name.get(xref.ref_function)
        callee = function_by_name.get(xref.function)
        detail = xref.detail or (
            f"{xref.indirections} indirections" if xref.indirections != 0 else "Direct"
        )

        if caller is not None:
            caller.references.append(
                XRefItem(
                    kind="Function",
                    value=xref_link(xref.function, callee),
                    offsets=xref.offsets,
                    detail=detail,
                    builds=xref.builds,
                    provenance=xref.provenance,
                )
            )

        if callee is not None:
            callee.referenced_by.append(
                XRefItem(
                    kind="Function",
                    value=xref_link(xref.ref_function, caller),
                    offsets=xref.offsets,
                    detail=detail,
                    builds=xref.builds,
                    provenance=xref.provenance,
                )
            )

    for xref in cards.data_xrefs:
        source = function_by_name.get(xref.function)
        target = global_by_name.get(xref.global_name)

        if source is not None:
            source.references.append(
                XRefItem(
                    kind="Data",
                    value=xref_link(xref.global_name, target),
                    offsets=xref.offsets,
                    detail=xref.access,
                    builds=xref.builds,
                )
            )

        if target is not None:
            target.referenced_by.append(
                XRefItem(
                    kind="Function",
                    value=xref_link(xref.function, source),
                    offsets=xref.offsets,
                    detail=xref.access,
                    builds=xref.builds,
                )
            )


def add_type_usage_xref(
    source: FunctionCard | GlobalCard | TypeCard,
    target: TypeCard,
    *,
    source_kind: str,
) -> None:
    builds = xref_display_builds(getattr(source, "builds", ""))
    source.references.append(
        XRefItem(
            kind="Type",
            value=related_link(target),
            offsets="-",
            detail="type usage",
            builds=builds,
        )
    )
    target.referenced_by.append(
        XRefItem(
            kind=source_kind,
            value=related_link(source),
            offsets="-",
            detail="type usage",
            builds=builds,
        )
    )


def attach_related(cards: SurfaceCards) -> None:
    functions = cards.functions
    globals_ = cards.globals
    types = cards.types
    type_refs = {
        id(card): type_references(
            getattr(card, "related_type_texts", []),
            types,
            exclude=card.name if isinstance(card, TypeCard) else "",
        )
        for card in [*functions, *globals_, *types]
    }

    for card in functions:
        for type_card in type_refs[id(card)]:
            add_type_usage_xref(card, type_card, source_kind="Function")

        card.facts = [symbol_fact("Kind", "Function")]

    for card in globals_:
        for type_card in type_refs[id(card)]:
            add_type_usage_xref(card, type_card, source_kind="Data")

        card.facts = [
            symbol_fact("Kind", "Data"),
            symbol_fact("Policy", card.write_policy),
        ]

        if card.is_typed:
            card.facts.append(symbol_fact("Type", card.type))

    for card in types:
        for type_card in type_refs[id(card)]:
            add_type_usage_xref(card, type_card, source_kind="Type")

        for item in card.metadata:
            if item.label == "Shape":
                item.value = link_type_names(
                    html.unescape(item.value),
                    types,
                    exclude=card.name,
                )

        for member in card.members:
            member_refs = type_references([member.type], types)
            member_display_type = docs_type_label(member.type)
            member.type_link = (
                linked_type_code(
                    member.type, member_refs[0], display_text=member_display_type
                )
                if member_refs
                else f"<code>{html.escape(member_display_type)}</code>"
            )

        card.facts = [symbol_fact("Kind", card.kind_label)]

    normalize_xrefs(cards)

    for card in cards.signatures:
        card.facts = [symbol_fact("Kind", "Signature")]


CATEGORY_DISPLAY_OVERRIDES = {
    "d3d": "D3D",
    "ddraw": "DDraw",
    "dinput": "DInput",
    "pkg": "PKG",
    "ui": "UI",
    "win32": "Win32",
}


def category_display_from_slug(category_id: str) -> str:
    normalized = category_id or "misc"

    if normalized in CATEGORY_DISPLAY_OVERRIDES:
        return CATEGORY_DISPLAY_OVERRIDES[normalized]

    return (
        " ".join(part[:1].upper() + part[1:] for part in normalized.split("-"))
        or "Misc"
    )


def category_from_slug(category_id: str) -> Category:
    category_id = category_id or "misc"

    return Category(
        display=category_display_from_slug(category_id),
        slug=category_id,
    )


def category_for(categories: dict[str, Category], item: object) -> Category:
    cat_slug = str(item.category)
    return categories.setdefault(cat_slug, category_from_slug(cat_slug))


def category_sort_key(category: Category) -> tuple[int, str]:
    display = category.display
    return (1 if display == "Misc" else 0, display.casefold())


def build_categories(cards: SurfaceCards) -> list[Category]:
    categories: dict[str, Category] = {}
    groups = (
        (cards.functions, "functions"),
        (cards.globals, "globals"),
        (cards.types, "types"),
        (cards.signatures, "signatures"),
    )

    for items, attr in groups:
        for item in items:
            getattr(category_for(categories, item), attr).append(item)

    return sorted(categories.values(), key=category_sort_key)


def surface_cards(
    blueprint: object,
    *,
    unstable: bool,
    stable_type_rows: list[object],
    symbol_rvas: SymbolRvaIndex,
) -> SurfaceCards:
    context_blueprint = (
        replace(blueprint, external_structs=stable_type_rows) if unstable else blueprint
    )
    context = header_context(context_blueprint, unstable=unstable)
    c_type_fn = context.c_type

    functions = function_cards(
        blueprint.functions,
        c_type_fn,
        unstable=unstable,
        symbol_rvas=symbol_rvas,
    )

    domain_slugs = {fn.category for fn in functions}
    domain_slugs.add("misc")

    return SurfaceCards(
        functions=functions,
        globals=global_cards(
            blueprint.globals,
            c_type_fn,
            unstable=unstable,
            domain_slugs=domain_slugs,
            symbol_rvas=symbol_rvas,
        ),
        types=type_cards(
            blueprint.structs,
            c_type_fn,
            domain_slugs=domain_slugs,
            unstable=unstable,
        ),
        signatures=signature_cards(
            blueprint.signatures,
            domain_slugs=domain_slugs,
            unstable=unstable,
            symbol_rvas=symbol_rvas,
        ),
        function_xrefs=function_xref_cards(blueprint.function_xrefs),
        data_xrefs=data_xref_cards(
            blueprint.xrefs,
            domain_slugs=domain_slugs,
        ),
    )


def merge_surfaces(stable: SurfaceCards, unstable: SurfaceCards) -> SurfaceCards:
    return SurfaceCards(
        functions=[*stable.functions, *unstable.functions],
        globals=[*stable.globals, *unstable.globals],
        types=[*stable.types, *unstable.types],
        signatures=[*stable.signatures, *unstable.signatures],
        function_xrefs=[*stable.function_xrefs, *unstable.function_xrefs],
        data_xrefs=[*stable.data_xrefs, *unstable.data_xrefs],
    )


def symbol_doc_cards(
    stable: object,
    unstable: object,
    *,
    function_call_xrefs: list[Path] | None = None,
    symbol_addresses: list[Path] | None = None,
) -> SurfaceCards:
    symbol_rvas = symbol_rva_index(symbol_addresses or [])
    stable_cards = surface_cards(
        stable,
        unstable=False,
        stable_type_rows=[],
        symbol_rvas=symbol_rvas,
    )
    unstable_cards = surface_cards(
        unstable,
        unstable=True,
        stable_type_rows=list(stable.structs),
        symbol_rvas=symbol_rvas,
    )

    cards = merge_surfaces(stable_cards, unstable_cards)
    cards.function_xrefs.extend(function_call_xref_cards(function_call_xrefs or []))

    attach_xrefs(cards)
    attach_related(cards)
    attach_reference_hierarchy_paths(cards)

    return cards


def overview_totals(rows: list[OverviewRow]) -> OverviewTotals:
    kind_counts = Counter(row.kind for row in rows)
    stability_counts = Counter(row.stability_slug for row in rows)
    build_counts = Counter(
        build
        for row in rows
        for build in row.builds.split()
        if build in BUILD_BIT_BY_LABEL
    )

    return OverviewTotals(
        symbols=sum(kind_counts.values()),
        functions=kind_counts["function"],
        globals=kind_counts["data"],
        types=kind_counts["type"],
        signatures=kind_counts["signature"],
        stable=stability_counts["stable"],
        unstable=stability_counts["unstable"],
        build_en=build_counts["EN"],
        build_eu=build_counts["EU"],
        build_sc=build_counts["SC"],
    )


def iter_category_cards(category: Category) -> Iterator[tuple[str, SymbolPage]]:
    for kind, cards in (
        ("function", category.functions),
        ("data", category.globals),
        ("type", category.types),
        ("signature", category.signatures),
    ):
        for card in cards:
            yield kind, card


def overview_rows(categories: list[Category]) -> list[OverviewRow]:
    rows: list[OverviewRow] = []

    for category in categories:
        for kind, card in iter_category_cards(category):
            unstable = str(card.anchor).startswith("unstable-")
            rows.append(
                OverviewRow(
                    name=str(card.name),
                    kind=kind,
                    category=category.display,
                    stability="Unstable" if unstable else "Stable",
                    stability_slug="unstable" if unstable else "stable",
                    href=f"symbols/{card.anchor}",
                    builds=str(getattr(card, "builds", "")),
                    refs_in=len(getattr(card, "referenced_by", [])),
                    refs_out=len(getattr(card, "references", [])),
                    summary=str(getattr(card, "summary", "")),
                )
            )

    rows.sort(key=lambda row: (row.name.casefold(), row.category.casefold()))
    return rows


def iter_symbol_cards(
    categories: list[Category],
) -> Iterator[tuple[str, Category, SymbolPage]]:
    for category in categories:
        for kind, card in iter_category_cards(category):
            yield kind, category, card


def symbol_doc_categories(
    stable: object,
    unstable: object,
    *,
    function_call_xrefs: list[Path] | None = None,
    symbol_addresses: list[Path] | None = None,
) -> list[Category]:
    return build_categories(
        symbol_doc_cards(
            stable,
            unstable,
            function_call_xrefs=function_call_xrefs,
            symbol_addresses=symbol_addresses,
        )
    )


def symbol_overview_output_path() -> str:
    return "pcdogs/index.md"


def symbol_detail_output_path(page: SymbolPage) -> str:
    return f"pcdogs/symbols/{page.anchor}.md"


def render_outputs(categories: list[Category]) -> dict[str, str]:
    detail_template = template("symbol_docs_detail_template.md.mako")
    overview_template = template("symbol_docs_overview_template.md.mako")
    rows = overview_rows(categories)
    cards_by_anchor = {
        card.anchor: card for _, _, card in iter_symbol_cards(categories)
    }
    outputs = {
        symbol_overview_output_path(): normalize_rendered_markdown(
            overview_template.render_unicode(
                categories=categories,
                rows=rows,
                totals=overview_totals(rows),
            ).strip("\n")
        )
    }

    for kind, category, card in iter_symbol_cards(categories):
        outputs[symbol_detail_output_path(card)] = normalize_rendered_markdown(
            detail_template.render_unicode(
                category=category,
                kind=kind,
                card=card,
                cards_by_anchor=cards_by_anchor,
            ).strip("\n")
        )

    return outputs


def normalize_rendered_markdown(text: str) -> str:
    return re.sub(r"\n{3,}", "\n\n", text)


def combined_outputs(
    stable: object,
    unstable: object,
    *,
    function_call_xrefs: list[Path] | None = None,
    symbol_addresses: list[Path] | None = None,
) -> dict[str, str]:
    return render_outputs(
        symbol_doc_categories(
            stable,
            unstable,
            function_call_xrefs=function_call_xrefs,
            symbol_addresses=symbol_addresses,
        )
    )


def write_or_check(path: Path, text: str, check: bool) -> bool:
    old = path.read_text() if path.exists() else ""

    if old == text:
        return True

    if check:
        diff = difflib.unified_diff(
            old.splitlines(),
            text.splitlines(),
            fromfile=str(path),
            tofile=f"{path} (generated)",
            lineterm="",
        )
        print("\n".join(diff), file=sys.stderr)
        return False

    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text)
    return True


def main() -> int:
    args = parse_args()

    blueprint = load_blueprint(default_blueprint(args))
    stable, unstable = split_row_unstable_rows(blueprint)
    outputs = combined_outputs(
        stable,
        unstable,
        function_call_xrefs=args.function_call_xrefs,
        symbol_addresses=args.symbol_addresses,
    )
    ok = True

    for name, text in outputs.items():
        ok = write_or_check(args.output_dir / name, text, args.check) and ok

    if not ok:
        print("PCDOGS symbol docs are stale", file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
