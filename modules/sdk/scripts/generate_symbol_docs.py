#!/usr/bin/env python3
"""Generate user-facing Markdown reference pages from the PCDOGS SDK blueprints."""

from __future__ import annotations

import argparse
import difflib
import html
import re
import sys
from collections import defaultdict
from dataclasses import replace
from pathlib import Path
from typing import Callable

try:
    from mako.template import Template
except ImportError as exc:
    raise SystemExit(
        "generate_symbol_docs.py requires Mako; install it or enter the Nix dev shell."
    ) from exc

sys.dont_write_bytecode = True

from generate_headers import (  # noqa: E402
    CC_KEYWORD,
    SymbolDocKind,
    TypeRowKind,
    build_mask_bits,
    c_pascal_token,
    data_write_policy,
    header_context,
    load_blueprint,
    pcdogs_type_name,
    public_function_rows,
    row_doc,
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
    RelatedEntry,
    SignatureCard,
    SurfaceCards,
    SymbolFact,
    TypeCard,
    XRefItem,
)

DECIMAL_PLUS_OFFSET = re.compile(r"(?<![eE])\+([0-9]+)\b")

BUILD_NAMES = (
    ("EN", 0b001),
    ("EU", 0b010),
    ("SC", 0b100),
)

PREFIX_DISPLAY = {
    "bonetrail": "BoneTrail",
    "crt": "CRT",
    "d3d": "Direct3D",
    "ddraw": "DirectDraw",
    "demoreplay": "DemoReplay",
    "dinput": "DirectInput",
    "directx": "DirectX",
    "gamestate": "GameState",
    "joystate": "JoyState",
    "minigame": "MiniGame",
    "pkg": "Package",
    "savegame": "SaveGame",
    "scenenode": "SceneNode",
    "scriptcmd": "ScriptCmd",
    "scriptop": "ScriptOp",
    "titlescreen": "TitleScreen",
    "treemap": "TreeMap",
    "win32": "Win32",
    "winmain": "Misc",
}


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
    parser.add_argument(
        "--unstable-blueprint",
        type=Path,
        default=None,
        help="Unstable blueprint path. Defaults to modules/sdk/blueprints/dttr_pcdogs_unstable.py.",
    )
    parser.add_argument("--check", action="store_true", help="fail if output is stale")
    return parser.parse_args()


def sdk_root() -> Path:
    return Path(__file__).resolve().parent.parent


def default_blueprints(args: argparse.Namespace) -> tuple[Path, Path]:
    root = sdk_root()
    return (
        args.stable_blueprint or root / "blueprints/dttr_pcdogs.py",
        args.unstable_blueprint or root / "blueprints/dttr_pcdogs_unstable.py",
    )


def template(name: str) -> Template:
    path = Path(__file__).resolve().with_name(name)
    return Template(path.read_text(), filename=str(path), strict_undefined=True)


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

        items.append(
            MetadataItem(
                label=html.escape(title_case_metadata_label(label), quote=True),
                value=html.escape(value, quote=True),
            )
        )

    return items


def aob_target(kind: str, match_offset: object = 0) -> str:
    if kind == "function":
        offset = int(match_offset)
        if offset == 0:
            return "Function body"
        return f"Function body (entry = match {hex_signed(offset)})"
    if kind == "signature":
        return "Signature match"
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
    return text.split("_", 1)[0].lower()


def category_display(name: object) -> str:
    prefix = category_prefix(name)
    if prefix in PREFIX_DISPLAY:
        return PREFIX_DISPLAY[prefix]
    return prefix[:1].upper() + prefix[1:]


def category_slug(name: object) -> str:
    return slug(category_display(name))


def constrained_category_slug(name: object, domain_slugs: set[str]) -> str:
    candidate = category_slug(name)
    return candidate if candidate in domain_slugs else "misc"


def build_mask_label(value: object) -> str:
    bits = build_mask_bits(value)
    if bits == 0:
        return "None"

    return " ".join(name for name, bit in BUILD_NAMES if bits & bit)


def function_prototype(fn: object, c_type_fn: Callable[[object], str]) -> str:
    typed = fn.typed
    if not typed:
        return "-"

    cc = CC_KEYWORD[str(typed.abi)]
    ret = c_type_fn(typed.return_type)
    if not typed.params:
        return f"{ret} ({cc}*) (void)"

    lines = [f"{ret} ({cc}*) ("]
    params = [f"{c_type_fn(param.type)} {param.name}" for param in typed.params]
    for index, param in enumerate(params):
        suffix = "," if index < len(params) - 1 else ""
        lines.append(f"    {param}{suffix}")
    lines.append(")")
    return "\n".join(lines)


def function_related_type_texts(
    fn: object, c_type_fn: Callable[[object], str]
) -> list[str]:
    if not fn.typed:
        return []
    texts = [str(fn.typed.return_type), c_type_fn(fn.typed.return_type)]
    for param in fn.typed.params:
        texts.extend([str(param.type), c_type_fn(param.type)])
    return texts


def c_array_type_parts_from_text(c_type: str) -> tuple[str, str] | None:
    match = re.match(r"^(?P<base>.+)\[(?P<count>[^\]]+)\]$", c_type.strip())
    if match is None:
        return None
    return match.group("base").strip(), match.group("count").strip()


def c_value_declaration(c_type: str, name: str, value: str) -> str:
    parts = c_array_type_parts_from_text(c_type)
    if parts is None:
        return f"{c_type} {name} = {value};"
    base, count = parts
    return f"{base} {name}[{count}] = {{0}};"


def data_default_value(c_type: str) -> str:
    normalized = c_type.strip()
    if c_array_type_parts_from_text(normalized) is not None:
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
    if re.match(
        r"^(u?int(8|16|32|64)_t|DWORD|WORD|BYTE|BOOL|int|uint32_t|uintptr_t)$",
        normalized,
    ):
        return f"({normalized})0"
    return "{0}"


def data_read_argument(c_type: str, name: str) -> str:
    return f"&{name}"


def data_write_argument(c_type: str, name: str) -> str:
    if c_array_type_parts_from_text(c_type) is not None:
        return f"&{name}"
    return name


def fallback_value(c_type: str) -> str:
    normalized = c_type.strip()
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
        lines.append(f"static {return_type} {cc} {name}(void) {{")

    if return_type == "void":
        lines.append(f"    {body_call};")
    else:
        lines.append(f"    return {body_call};")

    lines.append("}")
    return lines


def function_detour_boilerplate(
    fn: object, c_type_fn: Callable[[object], str]
) -> tuple[str, list[str]]:
    public = c_pascal_token(fn.name)
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
    if not fn.public:
        return "// Resolver-only symbol; no public SDK function helper is generated."
    if not typed:
        return "// No typed SDK call example is available yet."
    if not fn.callable:
        return "// This symbol is not marked safe to call through the SDK."

    params = [fallback_value(c_type_fn(param.type)) for param in typed.params]
    return_type = c_type_fn(typed.return_type)
    call_args = ["&ctx->runtime", *params]

    if return_type != "void":
        call_args.append(f"{fallback_value(return_type)} /* Default on failure */")

    lines = [f"{api}->Call("]

    for index, arg in enumerate(call_args):
        suffix = "," if index < len(call_args) - 1 else ""
        lines.append(f"    {arg}{suffix}")

    lines.append(");")
    call = "\n".join(lines)

    if return_type == "void":
        return call

    return f"{return_type} result = {call}"


def function_patch_spec_example(
    fn: object, c_type_fn: Callable[[object], str], api: str
) -> str:
    if not fn.public:
        return "// Resolver-only symbol; no public SDK patch spec helper is generated."
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


def function_status(fn: object) -> str:
    parts = ["callable" if fn.callable else "not callable", str(fn.hook.kind)]
    if fn.hook.patch_size:
        parts.append(f"patch {fn.hook.patch_size}")
    return ", ".join(parts)


def function_cards(
    functions: list[object],
    c_type_fn: Callable[[object], str],
    *,
    public_only: bool,
    unstable: bool,
) -> list[FunctionCard]:
    source = (
        public_function_rows(functions)
        if public_only
        else [fn for fn in functions if not fn.public]
    )
    cards = []

    for fn in source:
        public = c_pascal_token(fn.name)
        api = (
            f"DTTR_PCDOGS_F_{public}"
            if fn.public
            else f"DTTR_PCDOGS_SYMBOL_FUNCTION_ID_{fn.symbol_id}"
        )
        cards.append(
            FunctionCard(
                name=str(fn.name),
                heading=str(fn.name),
                category=category_slug(fn.name),
                anchor=anchor("unstable-function" if unstable else "function", fn.name),
                prototype=function_prototype(fn, c_type_fn),
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
                ),
                related_type_texts=function_related_type_texts(fn, c_type_fn),
            )
        )

    return cards


def global_type(glob: object, c_type_fn: Callable[[object], str]) -> str:
    return c_type_fn(glob.typed.type) if glob.typed else "-"


def global_accessor(surface_unstable: bool, glob: object) -> str:
    public = c_pascal_token(glob.name)
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

    lines = [
        c_value_declaration(c_type, "value", default_value),
        f"bool ok = {accessor}->Read({data_read_argument(c_type, 'value')});",
    ]
    return "\n".join(lines)


def global_write_example(glob: object, c_type_fn: Callable[[object], str]) -> str:
    if not glob.typed:
        return "// No typed data write example is available yet."

    policy = data_write_policy(glob).removeprefix("DTTR_PCDOGS_DATA_WRITE_POLICY_")
    accessor = global_accessor(glob.unstable, glob)
    c_type = c_type_fn(glob.typed.type)
    if policy != "RAW_MEMORY":
        return "\n".join(
            [
                f"// WritePolicy is {policy}; Write() returns false for this symbol.",
                "// Use Read() or Ptr() for inspection instead.",
            ]
        )

    default_value = data_default_value(c_type)

    lines = [
        c_value_declaration(c_type, "value", default_value),
        f"bool ok = {accessor}->Write({data_write_argument(c_type, 'value')});",
    ]
    return "\n".join(lines)


def global_cards(
    globals_: list[object],
    c_type_fn: Callable[[object], str],
    *,
    unstable: bool,
    domain_slugs: set[str],
) -> list[GlobalCard]:
    cards = []

    for glob in globals_:
        policy = data_write_policy(glob).removeprefix("DTTR_PCDOGS_DATA_WRITE_POLICY_")

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
                ),
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
        return f"alias of `{c_type_fn(row.source_type)}`"
    if kind == TypeRowKind.FUNCTION_TYPE_ALIAS:
        params = ", ".join(
            f"{c_type_fn(param.type)} {param.name}" for param in row.params
        )
        return f"callback `{c_type_fn(row.ret)} (*)({params or 'void'})`"
    if kind == TypeRowKind.STRUCT:
        size = f", size 0x{row.size:X}" if row.size is not None else ""
        completeness = "incomplete" if row.incomplete else "complete"
        return f"{len(row.members)} fields, {completeness}{size}"
    if kind == TypeRowKind.ENUM:
        return f"{len(row.values)} values"
    return "-"


def member_cards(row: object, c_type_fn: Callable[[object], str]) -> list[MemberCard]:
    members = []

    for member in getattr(row, "members", []) or []:
        members.append(
            MemberCard(
                name=str(member.name),
                type=c_type_fn(member.type),
                offset=f"0x{int(member.offset):X}",
                doc=escape_table_cell(member.doc),
            )
        )

    return members


def enum_values(row: object) -> list[EnumValueCard]:
    values = []

    for value in getattr(row, "values", []) or []:
        values.append(
            EnumValueCard(
                name=str(value.name),
                value=f"0x{int(value.value):X}",
                doc=markdown_text(getattr(value, "doc", None)),
                table_doc=escape_table_cell(getattr(value, "doc", None)),
            )
        )

    return values


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
        c_name = pcdogs_type_name(
            row.alias or row.name if kind == TypeRowKind.ENUM else row.name
        )
        cards.append(
            TypeCard(
                name=str(row.name),
                category=constrained_category_slug(row.name, domain_slugs),
                anchor=anchor("unstable-type" if unstable else "type", row.name),
                kind_label=title_case_metadata_label(kind.value.replace("_", " ")),
                c_name=c_name,
                shape=type_summary(row, c_type_fn),
                builds=build_mask_label("all"),
                summary=markdown_text(docs),
                members=member_cards(row, c_type_fn),
                enum_values=enum_values(row),
                metadata=metadata_items(
                    [
                        f"C name: {c_name}",
                        f"Shape: {type_summary(row, c_type_fn)}",
                    ]
                ),
                related_type_texts=type_related_texts(row, c_type_fn),
            )
        )

    return cards


def signature_cards(
    signatures: list[object], *, domain_slugs: set[str], unstable: bool
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
            ),
        )
        for sig in signatures
    ]


def function_xref_cards(
    xrefs: list[object], *, unstable: bool
) -> list[FunctionXRefCard]:
    return [
        FunctionXRefCard(
            category=category_slug(xref.function),
            function=str(xref.function),
            ref_function=str(xref.ref_function),
            offsets=offset_pair(xref.instr_off, xref.addr_off),
            indirections=int(xref.indirections),
        )
        for xref in xrefs
    ]


def data_xref_cards(
    xrefs: list[object], *, domain_slugs: set[str], unstable: bool
) -> list[DataXRefCard]:
    return [
        DataXRefCard(
            category=constrained_category_slug(xref.global_name, domain_slugs),
            global_name=str(xref.global_name),
            function=str(xref.function),
            offsets=offset_pair(xref.instr_off, xref.addr_off),
        )
        for xref in xrefs
    ]


def rendered_symbol_link(card: object) -> str:
    return f"../{card.category}/#{card.anchor}"


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
            )
        )

    return items


def plain_xref_value(value: object) -> str:
    text = re.sub(r"<[^>]+>", "", str(value))
    return html.unescape(text).strip()


def xref_metadata_items(label: str, rows: list[XRefItem]) -> list[MetadataItem]:
    lines = []
    suffixes = len(rows) > 1

    for index, row in enumerate(rows, 1):
        name = plain_xref_value(row.value)
        if not name:
            continue

        kind = html.unescape(str(row.kind or "Reference"))
        offsets = html.unescape(str(row.offsets or "-"))
        detail = html.unescape(str(row.detail or "-"))
        row_label = f"{label} {index}" if suffixes else label
        lines.append(f"{row_label}: {kind} {name} @ {offsets} ({detail})")

    return metadata_items(lines)


def symbol_fact(label: str, value: object) -> SymbolFact:
    return SymbolFact(label=html.escape(label, quote=True), value=str(value))


def related_link(card: object) -> str:
    return f'<a href="{html.escape(rendered_symbol_link(card), quote=True)}">{html.escape(str(card.name))}</a>'


def related_entries(kind: str, values: list[str]) -> list[RelatedEntry]:
    seen: set[str] = set()
    entries = []

    for value in values:
        key = plain_xref_value(value)
        if not key or key in seen:
            continue

        seen.add(key)
        entries.append(RelatedEntry(kind=kind, value=value))

    return entries


def merge_related(*groups: list[RelatedEntry]) -> list[RelatedEntry]:
    seen: set[tuple[str, str]] = set()
    entries = []

    for group in groups:
        for item in group:
            key = (item.kind, plain_xref_value(item.value))
            if not key[1] or key in seen:
                continue

            seen.add(key)
            entries.append(item)

    return entries


def type_reference_matches(text: object, card: TypeCard) -> bool:
    haystack = str(text)
    if not haystack or haystack == "-":
        return False
        
    names = [card.name, card.c_name]
    
    return any(
        name
        and re.search(
            rf"(?<![A-Za-z0-9_]){re.escape(name)}(?![A-Za-z0-9_])",
            haystack,
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


def linked_type_code(text: object, type_card: TypeCard) -> str:
    value = str(text)
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


def xref_related_values(card: FunctionCard | GlobalCard, kind: str) -> list[str]:
    values = []

    for row in [*card.references, *card.referenced_by]:
        if row.kind == kind:
            values.append(row.value)

    return values


def attach_xrefs(cards: SurfaceCards) -> None:
    functions = [*cards.functions, *cards.resolver_functions]
    function_by_name = {card.name: card for card in functions}
    global_by_name = {card.name: card for card in cards.globals}

    for card in [*functions, *cards.globals]:
        card.references = []
        card.referenced_by = []
        card.xref_count = 0

    for xref in cards.function_xrefs:
        source = function_by_name.get(xref.function)
        target = function_by_name.get(xref.ref_function)
        detail = (
            f"{xref.indirections} indirections" if xref.indirections != 0 else "Direct"
        )

        if source is not None:
            source.references.append(
                XRefItem(
                    kind="Function",
                    value=xref_link(xref.ref_function, target),
                    offsets=xref.offsets,
                    detail=detail,
                )
            )

        if target is not None:
            target.referenced_by.append(
                XRefItem(
                    kind="Function",
                    value=xref_link(xref.function, source),
                    offsets=xref.offsets,
                    detail=detail,
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
                    detail="data xref",
                )
            )

        if target is not None:
            target.referenced_by.append(
                XRefItem(
                    kind="Function",
                    value=xref_link(xref.function, source),
                    offsets=xref.offsets,
                    detail="data xref",
                )
            )

    for card in [*functions, *cards.globals]:
        card.references = xref_items(card.references)
        card.referenced_by = xref_items(card.referenced_by)
        card.xref_count = len(card.references) + len(card.referenced_by)

        card.metadata.extend(xref_metadata_items("Resolver reference", card.references))
        card.metadata.extend(
            xref_metadata_items("Resolver referenced by", card.referenced_by)
        )


def attach_related(cards: SurfaceCards) -> None:
    functions = [*cards.functions, *cards.resolver_functions]
    globals_ = cards.globals
    types = cards.types
    type_refs = {
        id(card): type_references(
            getattr(card, "related_type_texts", []),
            types,
            exclude=card.name if card in types else "",
        )
        for card in [*functions, *globals_, *types]
    }
    type_related_functions: dict[str, list[str]] = defaultdict(list)
    type_related_data: dict[str, list[str]] = defaultdict(list)

    for card in [*functions, *globals_, *types, *cards.signatures]:
        card.related = []

    for card in functions:
        related_types = type_refs[id(card)]

        for type_card in related_types:
            type_related_functions[type_card.name].append(related_link(card))

        related = merge_related(
            related_entries(
                "Type",
                [related_link(type_card) for type_card in related_types],
            ),
            related_entries("Function", xref_related_values(card, "Function")),
            related_entries("Data", xref_related_values(card, "Data")),
        )

        card.related = related
        card.facts = [symbol_fact("Versions", card.builds)]

    for card in globals_:
        related_types = type_refs[id(card)]

        for type_card in related_types:
            type_related_data[type_card.name].append(related_link(card))

        related = merge_related(
            related_entries(
                "Type",
                [related_link(type_card) for type_card in related_types],
            ),
            related_entries("Function", xref_related_values(card, "Function")),
            related_entries("Data", xref_related_values(card, "Data")),
        )

        card.related = related
        card.facts = []

        if card.is_typed:
            card.facts.append(symbol_fact("Type", card.type))

        card.facts.append(symbol_fact("Versions", card.builds))

    for card in types:
        related_types = type_refs[id(card)]

        for item in card.metadata:
            if item.label == "Shape":
                item.value = link_type_names(
                    html.unescape(item.value),
                    types,
                    exclude=card.name,
                )

        for member in card.members:
            member_refs = type_references([member.type], types)
            member.type_link = (
                linked_type_code(member.type, member_refs[0])
                if member_refs
                else f"<code>{html.escape(member.type)}</code>"
            )

        related = merge_related(
            related_entries(
                "Type",
                [related_link(type_card) for type_card in related_types],
            ),
            related_entries("Function", type_related_functions[card.name]),
            related_entries("Data", type_related_data[card.name]),
        )

        card.related = related
        card.facts = [
            symbol_fact("Kind", card.kind_label),
            symbol_fact("Versions", card.builds),
        ]

    for card in cards.signatures:
        card.facts = [symbol_fact("Versions", card.builds)]


def empty_category(display: str) -> Category:
    category_id = slug(display)
    return Category(
        display=display,
        slug=category_id,
        filename=f"{category_id}.md",
    )


def add_by_category(
    categories: dict[str, Category], key: str, items: list[object]
) -> None:
    for item in items:
        cat_slug = str(item.category)
        categories.setdefault(
            cat_slug,
            empty_category(category_display(getattr(item, "name", item.category))),
        )
        getattr(categories[cat_slug], key).append(item)


def category_sort_key(category: Category) -> tuple[int, str]:
    display = category.display
    return (1 if display == "Misc" else 0, display.casefold())


def build_categories(cards: SurfaceCards) -> list[Category]:
    categories: dict[str, Category] = {}

    add_by_category(categories, "functions", cards.functions)
    add_by_category(categories, "resolver_functions", cards.resolver_functions)
    add_by_category(categories, "globals", cards.globals)
    add_by_category(categories, "types", cards.types)
    add_by_category(categories, "signatures", cards.signatures)
    add_by_category(categories, "function_xrefs", cards.function_xrefs)
    add_by_category(categories, "data_xrefs", cards.data_xrefs)

    return sorted(categories.values(), key=category_sort_key)


def surface_cards(
    blueprint: object,
    *,
    unstable: bool,
    stable_type_rows: list[object],
) -> SurfaceCards:
    context_blueprint = (
        replace(blueprint, external_structs=stable_type_rows) if unstable else blueprint
    )
    context = header_context(context_blueprint, unstable=unstable)
    c_type_fn = context.c_type

    functions = function_cards(
        blueprint.functions,
        c_type_fn,
        public_only=True,
        unstable=unstable,
    )
    resolver_functions = function_cards(
        blueprint.functions,
        c_type_fn,
        public_only=False,
        unstable=unstable,
    )

    domain_slugs = {fn.category for fn in [*functions, *resolver_functions]}
    domain_slugs.update(slug(display) for display in PREFIX_DISPLAY.values())
    domain_slugs.add("misc")

    return SurfaceCards(
        functions=functions,
        resolver_functions=resolver_functions,
        globals=global_cards(
            blueprint.globals,
            c_type_fn,
            unstable=unstable,
            domain_slugs=domain_slugs,
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
        ),
        function_xrefs=function_xref_cards(
            blueprint.function_xrefs,
            unstable=unstable,
        ),
        data_xrefs=data_xref_cards(
            blueprint.xrefs,
            domain_slugs=domain_slugs,
            unstable=unstable,
        ),
    )


def merge_surfaces(stable: SurfaceCards, unstable: SurfaceCards) -> SurfaceCards:
    return SurfaceCards(
        functions=[*stable.functions, *unstable.functions],
        resolver_functions=[*stable.resolver_functions, *unstable.resolver_functions],
        globals=[*stable.globals, *unstable.globals],
        types=[*stable.types, *unstable.types],
        signatures=[*stable.signatures, *unstable.signatures],
        function_xrefs=[*stable.function_xrefs, *unstable.function_xrefs],
        data_xrefs=[*stable.data_xrefs, *unstable.data_xrefs],
    )


def symbol_doc_cards(stable: object, unstable: object) -> SurfaceCards:
    stable_cards = surface_cards(stable, unstable=False, stable_type_rows=[])
    unstable_cards = surface_cards(
        unstable,
        unstable=True,
        stable_type_rows=list(stable.structs),
    )

    cards = merge_surfaces(stable_cards, unstable_cards)

    attach_xrefs(cards)
    attach_related(cards)

    return cards


def render_outputs(categories: list[Category]) -> dict[str, str]:
    caveat = (
        "Stable symbols are the supported modder-facing PCDOGS surface. "
        "Unstable symbols are included beside them for discovery and marked inline."
    )
    index_template = template("symbol_docs_index_template.md.mako")
    category_template = template("symbol_docs_category_template.md.mako")

    outputs = {
        "pcdogs/index.md": index_template.render_unicode(
            title="PCDOGS Symbols",
            caveat=caveat,
            categories=categories,
        )
    }

    for category in categories:
        outputs[f"pcdogs/{category.filename}"] = normalize_rendered_markdown(
            category_template.render_unicode(
                category=category,
                functions=category.functions,
                resolver_functions=category.resolver_functions,
                globals=category.globals,
                types=category.types,
                signatures=category.signatures,
            )
        )

    return outputs


def normalize_rendered_markdown(text: str) -> str:
    return re.sub(r"\n---\n{3,}(?=### `)", "\n---\n\n", text)


def combined_outputs(stable: object, unstable: object) -> dict[str, str]:
    return render_outputs(build_categories(symbol_doc_cards(stable, unstable)))


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
    stable_path, unstable_path = default_blueprints(args)
    stable = load_blueprint(stable_path)
    unstable = load_blueprint(unstable_path)
    outputs = combined_outputs(stable, unstable)
    ok = True

    for name, text in outputs.items():
        ok = write_or_check(args.output_dir / name, text, args.check) and ok

    if not ok:
        print("PCDOGS symbol docs are stale", file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
