#!/usr/bin/env python3
"""Regenerate the PCDOGS SDK symbols definition from one Python blueprint."""

from __future__ import annotations

import argparse
import difflib
from collections import Counter
import re
import subprocess
import sys
from pathlib import Path

sys.dont_write_bytecode = True

from codegen import c_mask, c_sig, load_python_module

try:
    from mako.template import Template
except ImportError as exc:
    raise SystemExit(
        "generate_headers.py requires Mako; install it or enter the Nix dev shell."
    ) from exc

CC_ENUM = {
    "cdecl": "DTTR_PCDOGS_CC_CDECL",
    "stdcall": "DTTR_PCDOGS_CC_STDCALL",
    "fastcall": "DTTR_PCDOGS_CC_FASTCALL",
}
CC_KEYWORD = {
    "cdecl": "__cdecl",
    "stdcall": "__stdcall",
    "fastcall": "__fastcall",
}
HOOK_ENUM = {
    "rel32": "DTTR_PCDOGS_HOOK_REL32",
    "hotpatch": "DTTR_PCDOGS_HOOK_HOTPATCH",
    "unsupported": "DTTR_PCDOGS_HOOK_UNSUPPORTED",
}
DATA_RESOLVER = {
    "xref_u32": "DTTR_PCDOGS_DATA_RESOLVE_XREF_U32",
}
BUILD_MASK_ENUM = {
    "all": "DTTR_PCDOGS_BUILD_MASK_ALL",
    "en": "DTTR_PCDOGS_BUILD_MASK_EN",
    "eu": "DTTR_PCDOGS_BUILD_MASK_EU",
    "sc": "DTTR_PCDOGS_BUILD_MASK_SC",
    "eu_sc": "DTTR_PCDOGS_BUILD_MASK_EU_SC",
    "en_eu": "DTTR_PCDOGS_BUILD_MASK_EN_EU",
    "en_sc": "DTTR_PCDOGS_BUILD_MASK_EN_SC",
}
BUILD_MASK_BITS = {
    "all": 0b111,
    "en": 0b001,
    "eu": 0b010,
    "sc": 0b100,
    "eu_sc": 0b110,
    "en_eu": 0b011,
    "en_sc": 0b101,
}


def parse_args() -> argparse.Namespace:
    """Parse the blueprint selection and whether generated SDK outputs must already be current."""

    parser = argparse.ArgumentParser(
        description="Regenerate PCDOGS SDK symbol headers from Python blueprints.",
    )
    parser.add_argument(
        "blueprints",
        metavar="blueprint",
        nargs="?",
        default=None,
        help="blueprint Python path (default: stable and unstable blueprints)",
    )
    parser.add_argument(
        "--include-dir",
        type=Path,
        default=None,
        help="directory for generated public SDK headers (default: build/modules/sdk/generated/include)",
    )
    parser.add_argument(
        "--src-dir",
        type=Path,
        default=None,
        help="directory for generated SDK implementation outputs (default: build/modules/sdk/generated/src)",
    )
    parser.add_argument("--check", action="store_true", help="fail if output is stale")
    return parser.parse_args()


def param_decl(param: object) -> str:
    """Render blueprint parameter metadata as the C declaration used in generated prototypes."""

    if hasattr(param, "m_type") and hasattr(param, "m_name"):
        return f"{c_type(param.m_type)} {param.m_name}"
    if isinstance(param, tuple) and len(param) == 2:
        return f"{c_type(param[0])} {param[1]}"
    return str(param)


def arg_name(param: object) -> str:
    """Extract a call argument name from generated parameter metadata or raw C text."""

    if hasattr(param, "m_name"):
        return str(param.m_name)
    if isinstance(param, tuple) and len(param) == 2:
        return str(param[1])
    param = str(param)
    param = param.strip()
    while param.endswith("]"):
        param = param[: param.rfind("[")].rstrip()
    return param.replace("*", " ").replace("&", " ").split()[-1]


def param_args(params: list[object]) -> list[str]:
    """Return generated call argument names while treating void as an empty list."""

    if params == ["void"]:
        return []
    return [arg_name(param) for param in params]


def try_params(ret: str, params: list[object]) -> list[object]:
    """Build try-call wrapper parameters, including context and out-return storage when needed."""

    if ret == "void":
        return [("const DTTR_Core_Context*", "ctx"), *params]
    return [
        ("const DTTR_Core_Context*", "ctx"),
        *params,
        (f"{c_type(ret)}*", "out_ret"),
    ]


def ref_name(ref: object, function_names: dict[int, str]) -> str:
    """Resolve object or string references so generated xrefs remain stable across blueprints."""

    if isinstance(ref, str):
        return ref
    name = function_names.get(id(ref))
    if name is None:
        raise ValueError(f"unbound function reference: {ref!r}")
    return name


def load_blueprint(path: Path) -> dict[str, object]:
    """Flatten one Python blueprint into dictionaries consumed by the C header template."""

    module = load_python_module(path, "blueprint_source")
    source = getattr(module, "BLUEPRINT", None)
    if source is None:
        raise ValueError(f"blueprint module does not define BLUEPRINT: {path}")

    blueprint = {
        "signatures": [],
        "functions": [],
        "function_xrefs": [],
        "globals": [],
        "xrefs": [],
        "structs": [],
    }
    signature_rows = list(source.m_signatures)
    function_rows = list(source.m_functions)
    global_rows = list(source.m_globals)
    type_rows = list(source.m_types)
    function_names = {id(row): row.m_name for row in function_rows}
    for row in signature_rows:
        name = row.m_name
        if name is None:
            raise ValueError(f"signature rows need explicit names: {row!r}")
        blueprint["signatures"].append(
            {
                "name": name,
                "pattern": row.m_pattern,
                "required": row.m_required,
                "unstable": row.m_unstable,
                "doc": row.m_doc,
            }
        )
    for row in function_rows:
        name = row.m_name
        if name is None:
            raise ValueError(f"function rows need explicit names: {row!r}")
        hook = {
            "kind": row.m_hook.m_kind,
            "patch_size": row.m_hook.m_patch_size,
            "entry_patch_size": row.m_hook.m_entry_patch_size
            or row.m_hook.m_patch_size,
        }
        fn = {
            "name": name,
            "calling_convention": row.m_cc,
            "callable": row.m_callable,
            "required": row.m_required,
            "match_offset": row.m_match_offset,
            "pattern": row.m_pattern,
            "hook": hook,
            "unstable": row.m_unstable,
            "doc": row.m_doc,
        }
        if row.m_typed is not None:
            ret = row.m_typed.m_ret
            params = row.m_typed.m_params
            args = row.m_typed.m_args or param_args(params)
            fn["typed"] = {
                "return": ret,
                "abi": row.m_typed.m_abi or row.m_cc,
                "params": params,
                "args": args,
                "try_params": row.m_typed.m_try_params or try_params(ret, params),
                "try_args": row.m_typed.m_try_args or args,
                "signature": row.m_typed.m_signature or name,
                "delta": (
                    row.m_typed.m_delta
                    if row.m_typed.m_delta is not None
                    else row.m_match_offset
                ),
                "hook_kind": row.m_typed.m_hook_kind or hook["kind"],
                "hook_prologue_size": (
                    row.m_typed.m_hook_prologue_size
                    if row.m_typed.m_hook_prologue_size is not None
                    else hook["entry_patch_size"]
                ),
                "callable": (
                    row.m_typed.m_callable
                    if row.m_typed.m_callable is not None
                    else row.m_callable
                ),
            }
        blueprint["functions"].append(fn)
        blueprint["function_xrefs"].extend(
            {
                "function": name,
                "ref_function": ref_name(ref.m_ref_function, function_names),
                "instr_off": ref.m_instr_off,
                "addr_off": ref.m_addr_off,
                "indirections": ref.m_indirections,
            }
            for ref in row.m_xrefs
        )
    for row in global_rows:
        name = row.m_name
        if name is None:
            raise ValueError(f"global rows need explicit names: {row!r}")
        glob = {"name": name, "unstable": row.m_unstable, "doc": row.m_doc}
        if row.m_typed is not None:
            glob["typed"] = {
                "type": row.m_typed.m_type,
                "resolver": row.m_typed.m_resolver,
                "ref_function": ref_name(row.m_typed.m_ref_function, function_names),
                "instr_off": row.m_typed.m_instr_off,
                "addr_off": row.m_typed.m_addr_off,
                "indirections": row.m_typed.m_indirections,
            }
        blueprint["globals"].append(glob)
        blueprint["xrefs"].extend(
            {
                "global": name,
                "function": ref_name(ref.m_function, function_names),
                "instr_off": ref.m_instr_off,
                "addr_off": ref.m_addr_off,
            }
            for ref in row.m_xrefs
        )
    for row in type_rows:
        blueprint["structs"].append(row)

    assign_symbol_ids(blueprint["functions"])
    assign_symbol_ids(blueprint["globals"])
    function_symbol_ids = {
        row["name"]: row["symbol_id"] for row in blueprint["functions"]
    }
    global_symbol_ids = {row["name"]: row["symbol_id"] for row in blueprint["globals"]}

    def function_symbol(name: str) -> str:
        return function_symbol_ids.get(name, c_symbol(name))

    for row in blueprint["function_xrefs"]:
        row["function_symbol"] = function_symbol(row["function"])
        row["ref_function_symbol"] = function_symbol(row["ref_function"])
    function_required = {row["name"]: row["required"] for row in blueprint["functions"]}
    xref_functions_by_global: dict[str, list[str]] = {}
    for row in blueprint["xrefs"]:
        row["global_symbol"] = global_symbol_ids[row["global"]]
        row["function_symbol"] = function_symbol(row["function"])
        xref_functions_by_global.setdefault(row["global"], []).append(row["function"])

    for row in blueprint["functions"]:
        row["supported_builds"] = row["required"]
    for row in blueprint["globals"]:
        refs = list(xref_functions_by_global.get(row["name"], []))
        if row.get("typed"):
            refs.append(row["typed"]["ref_function"])
        row["supported_builds"] = [
            function_required[ref] for ref in refs if ref in function_required
        ] or ["all"]

    validate_blueprint(blueprint)
    return blueprint


def assign_symbol_ids(rows: list[dict[str, object]]) -> None:
    """Assign stable C enum tokens while preserving duplicate generated names through suffixes."""

    bases = [c_symbol(row["name"]) for row in rows]
    counts = Counter(bases)
    used: set[str] = set()
    for row, base in zip(rows, bases, strict=True):
        symbol_id = base
        if counts[base] > 1 and symbol_id in used:
            suffix = 2
            while f"{base}_{suffix}" in used:
                suffix += 1
            symbol_id = f"{base}_{suffix}"
        row["symbol_id"] = symbol_id
        used.add(symbol_id)


def check_unique(rows: list[dict[str, object]], label: str) -> None:
    """Reject duplicate generated symbol names before they become ambiguous C APIs."""

    seen = set()
    for row in rows:
        name = row["name"]
        if name in seen:
            raise ValueError(f"duplicate {label}: {name}")
        seen.add(name)


def validate_blueprint(blueprint: dict[str, object]) -> None:
    """Validate blueprint names and xrefs before emitting headers or implementation stubs."""

    for key, label in (
        ("signatures", "signature"),
        ("functions", "function"),
        ("globals", "global"),
    ):
        check_unique(blueprint[key], label)

    functions = {row["name"] for row in blueprint["functions"]}
    for row in blueprint["function_xrefs"]:
        if row["function"] not in functions:
            raise ValueError(f"function XRef has unknown target: {row['function']}")
        # The reference may point at the stable header when generating an
        # unstable extension header.


def write_or_check(path: Path, text: str, check: bool) -> bool:
    """Write generated SDK output, or print the stale diff used by CI check mode."""

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


def clang_format_header(path: Path, text: str) -> str:
    """Format generated C headers the same way checked-in SDK headers are stored."""

    try:
        result = subprocess.run(
            ["clang-format", f"--assume-filename={path}"],
            input=text,
            text=True,
            capture_output=True,
            check=True,
        )
    except (FileNotFoundError, subprocess.CalledProcessError):
        return text
    return result.stdout


def c_bool(value: object) -> str:
    """Render boolean metadata as the C literals expected by generated tables."""

    return "true" if value is True or value == "true" else "false"


def c_enum(value: object) -> str:
    """Render enum-like blueprint values as generated C token suffixes."""

    return str(value).upper()


def build_mask_bits(value: object) -> int:
    """Collapse one or more blueprint build requirements into EN/EU/SC bits."""

    if isinstance(value, (list, tuple, set, frozenset)):
        bits = 0
        for item in value:
            bits |= build_mask_bits(item)
        return bits
    return BUILD_MASK_BITS[str(value)]


def c_build_mask(value: object) -> str:
    """Render blueprint build-availability metadata as public SDK mask constants."""

    bits = build_mask_bits(value)
    for key, key_bits in BUILD_MASK_BITS.items():
        if bits == key_bits:
            return BUILD_MASK_ENUM[key]
    parts = [
        BUILD_MASK_ENUM[key]
        for key in ("en", "eu", "sc")
        if bits & BUILD_MASK_BITS[key]
    ]
    return " | ".join(parts) if parts else "DTTR_PCDOGS_BUILD_MASK_NONE"


def c_symbol(value: object) -> str:
    """Convert generated symbol names to stable uppercase C enum tokens."""

    name = str(value)
    out = []
    for i, ch in enumerate(name):
        prev = name[i - 1] if i else ""
        nxt = name[i + 1] if i + 1 < len(name) else ""
        if (
            ch.isupper()
            and i
            and prev != "_"
            and (prev.islower() or prev.isdigit() or nxt.islower())
        ):
            out.append("_")
        out.append(ch)
    symbol = "".join(out).upper()
    return symbol.replace("D3" + "_D", "D3D").replace("D" + "_DRAW", "DDRAW")


def c_pascal_token(value: object) -> str:
    """Convert generated symbol names to a public Pascal-style C token."""

    return "".join(
        part[:1].upper() + part[1:] for part in str(value).split("_") if part
    )


def c_int(value: object) -> str:
    """Render signed numeric metadata without unsigned suffixes for generated offsets."""

    if isinstance(value, int):
        return str(value)
    return str(value).removesuffix("u")


def c_uint(value: object) -> str:
    """Render size and count metadata as unsigned C literals for generated tables."""

    if isinstance(value, int):
        return f"{value}u"
    text = str(value)
    return text if text.endswith("u") else f"{text}u"


def c_type(value: object) -> str:
    """Normalize generated C type spelling so generated declarations stay compact."""

    return re.sub(r"\s*([*&])\s*", r"\1", str(value).strip())


def pcdogs_type_name(name: str) -> str:
    """Return the public C name for a generated PCDOGS type."""

    if name.startswith("DTTR_PCDOGS_T_"):
        return name
    return f"DTTR_PCDOGS_T_{name}"


TYPE_NAME_RENDERERS = dict.fromkeys(
    ("TypeAlias", "FunctionTypeAlias", "Struct", "Enum"),
    pcdogs_type_name,
)


def pcdogs_type_names(rows: list[object]) -> dict[str, str]:
    """Map generated blueprint type identifiers to public PCDOGS names."""

    names: dict[str, str] = {}
    for row in rows:
        row_kind = type(row).__name__
        renderer = TYPE_NAME_RENDERERS.get(row_kind)
        if not renderer:
            continue

        names[row.m_name] = renderer(row.m_name)
        if row_kind == "Enum" and row.m_alias:
            names[row.m_alias] = renderer(row.m_alias)
    return names


def c_type_with_pcdogs_prefix(value: object, names: dict[str, str]) -> str:
    """Normalize a C type and prefix generated PCDOGS type identifiers."""

    text = c_type(value)
    for name, public_name in sorted(
        names.items(), key=lambda item: len(item[0]), reverse=True
    ):
        text = re.sub(
            rf"(?<!DTTR_PCDOGS_[EPSTP]_)\b{re.escape(name)}\b",
            public_name,
            text,
        )
    return text


def inner_parens(value: str) -> str:
    """Extract the inside of generated C parameter or argument parentheses."""

    value = value.strip()
    if value == "()":
        return ""
    if value.startswith("(") and value.endswith(")"):
        return value[1:-1].strip()
    return value


def c_list(value: object) -> list[str]:
    """Split generated C argument text while treating void-style lists as empty."""

    if isinstance(value, list):
        return [str(item) for item in value]
    inner = inner_parens(str(value))
    if not inner or inner == "void":
        return []
    return [part.strip() for part in inner.split(",")]


def c_params(value: object) -> str:
    """Render generated C prototype parameters from blueprint or raw parameter data."""

    items = (
        [param_decl(item) for item in value]
        if isinstance(value, list)
        else c_list(value)
    )
    if items == ["void"]:
        items = []
    return "()" if not items else "(" + ", ".join(items) + ")"


def c_args(value: object) -> str:
    """Render generated C call arguments from blueprint or raw parameter data."""

    items = (
        [arg_name(item) for item in value] if isinstance(value, list) else c_list(value)
    )
    return "(" + ", ".join(items) + ")"


def doxy_text(value: object) -> str:
    """Sanitize blueprint prose for generated Doxygen comments."""

    return str(value).strip()


def doxy_brief(value: object, *, indent: str = "") -> str:
    """Render a Doxygen brief block for generated declarations."""

    return doxy_comment(value, indent=indent)


def doxy_inline(value: object) -> str:
    """Render one safe inline Doxygen sentence."""

    text = " ".join(doxy_text(value).split())
    return text.rstrip(".") + "." if text else ""


def row_doc(row: object) -> str | None:
    """Read optional documentation from blueprint dataclasses or flattened rows."""

    if isinstance(row, dict):
        value = row.get("doc")
    else:
        value = getattr(row, "m_doc", None)
    return doxy_text(value) if value else None


def symbol_doc(kind: str, row: dict[str, object]) -> str:
    """Return explicit blueprint documentation or a useful generated fallback."""

    explicit = row_doc(row)
    if explicit:
        return explicit
    if kind == "function":
        return "Callable game function."
    if kind == "global":
        return "Game data symbol."
    return "PCDOGS symbol."


def fallback_param_doc(name: str) -> str:
    """Return a generic generated parameter explanation when blueprints have no prose."""

    if name == "ctx":
        return "Runtime context for resolution and calls."
    if name == "out_ret":
        return "Receives the return value on success."
    if name.startswith("out_"):
        return "Receives the value on success."
    return "Unnamed argument."


def param_doc_pairs(params: object, *, fallback: bool = False) -> list[tuple[str, str]]:
    """Extract parameter docs from blueprint Param rows for generated comments."""

    if not isinstance(params, list):
        return []
    pairs: list[tuple[str, str]] = []
    for param in params:
        name = arg_name(param)
        doc = getattr(param, "m_doc", None)
        if doc:
            pairs.append((name, doxy_text(doc)))
        elif fallback:
            pairs.append((name, fallback_param_doc(name)))
    return pairs


def doxy_comment(
    brief: object,
    *,
    params: list[tuple[str, str]] | None = None,
    returns: str | None = None,
    indent: str = "",
) -> str:
    """Render a Doxygen block with optional parameter and return details."""

    text = doxy_text(brief)
    if not text:
        return ""
    lines = [*([""] if not indent else [])]
    for i, line in enumerate(text.splitlines()):
        lines.append(f"{indent}/// {line}" if line else f"{indent}///")
    for name, doc in params or []:
        lines.append(f"{indent}/// @param {name} {doc}")
    if returns:
        lines.append(f"{indent}/// @return {doxy_text(returns)}")
    return "\n".join(lines)


def member_doc(row: object) -> str:
    """Render inline docs for generated struct members."""

    parts = []
    doc = row_doc(row)
    if doc:
        parts.append(doxy_inline(doc))
    offset = getattr(row, "m_offset", None)
    if offset is not None:
        parts.append(f"Offset 0x{offset:X}.")
    return " ".join(parts)


def auto_impl_params(row: dict[str, str], ctx_name: str) -> str:
    """Build auto-wrapper implementation parameters with the SDK context inserted first."""

    params = inner_parens(row["params"])
    if not params:
        return f"(const DTTR_Core_Context*{ctx_name})"
    return f"(const DTTR_Core_Context*{ctx_name}, {params})"


def auto_impl_args(row: dict[str, str], ctx_expr: str | None = None) -> str:
    """Build auto-wrapper call arguments that pass the chosen stored SDK context."""

    ctx_expr = ctx_expr or f"dttr_pcdogs_ctx_{row['name']}"
    args = inner_parens(row["args"])
    if not args:
        return f"({ctx_expr})"
    return f"({ctx_expr}, {args})"


def auto_return(
    row: dict[str, str],
    impl_name: str | None = None,
    ctx_expr: str | None = None,
) -> str:
    """Render a generated auto-wrapper body that handles void and value returns."""

    impl_name = impl_name or f"dttr_pcdogs_impl_{row['name']}"
    call = f"{impl_name}{auto_impl_args(row, ctx_expr)}"
    if row["ret"] == "void":
        return f"\t{call};"
    return f"\treturn {call};"


def split_type_rows(rows: list[object]) -> tuple[list[object], list[object]]:
    """Separate prefix rows from packed structs for safe header ordering."""

    for index, row in enumerate(rows):
        if type(row).__name__ == "Struct":
            return rows[:index], rows[index:]
    return rows, []


def base_type_name(type_: str) -> str:
    """Return the base type name used to derive generated forward declarations."""

    base = type_.split("[", 1)[0]
    base = base.replace("*", " ").replace("const", " ")
    base = base.replace("volatile", " ").strip()
    return base.split()[-1] if base.split() else ""


def sort_struct_rows_by_value_dependencies(rows: list[object]) -> list[object]:
    """Order struct rows so embedded struct fields are defined before their users."""

    struct_by_name = {row.m_name: row for row in rows if type(row).__name__ == "Struct"}
    if not struct_by_name:
        return rows

    def value_dependencies(row: object) -> set[str]:
        deps: set[str] = set()
        for member in row.m_members:
            member_type = str(member.m_type)
            if "*" in member_type:
                continue
            dep = base_type_name(member_type)
            if dep in struct_by_name and dep != row.m_name:
                deps.add(dep)
        return deps

    dependencies = {
        name: value_dependencies(row) for name, row in struct_by_name.items()
    }
    pending = dict(struct_by_name)
    ordered_names: list[str] = []
    while pending:
        ready = [name for name in pending if not (dependencies[name] & pending.keys())]
        if not ready:
            ordered_names.extend(pending)
            break
        ordered_names.extend(ready)
        for name in ready:
            del pending[name]

    ordered_structs = [struct_by_name[name] for name in ordered_names]
    non_structs = [row for row in rows if type(row).__name__ != "Struct"]
    return non_structs + ordered_structs


def is_struct_ref(name: str, excluded_names: set[str]) -> bool:
    """Identify struct-like types that need forward declarations in generated headers."""

    if not name or name in excluded_names or name.startswith("DTTR_"):
        return False
    return name[0].isupper() and any(ch.islower() for ch in name)


def declared_type_names(rows: list[object]) -> set[str]:
    """Collect aliases and enums that already declare names used by generated type rows."""

    names = set()
    for row in rows:
        row_kind = type(row).__name__
        if row_kind in {"TypeAlias", "FunctionTypeAlias"}:
            names.add(row.m_name)
        elif row_kind == "Enum":
            names.add(row.m_name)
            names.add(row.m_alias or row.m_name)
    return names


def param_base_type(param: object) -> str:
    """Return the base C type named by one generated parameter row."""

    if hasattr(param, "m_type"):
        return base_type_name(str(param.m_type))
    if isinstance(param, tuple) and param:
        return base_type_name(str(param[0]))
    return base_type_name(str(param))


def add_param_type_refs(params: object, add: object) -> None:
    """Collect base type references from generated parameters for forward declaration derivation."""

    if not isinstance(params, list):
        return
    for param in params:
        add(param_base_type(param))


def derived_forward_names(
    blueprint: dict[str, object], rows: list[object]
) -> list[str]:
    """Derive struct forward declarations required by generated types and signatures."""

    names = []
    seen = set()
    excluded_names = declared_type_names(rows)

    def add(name: str) -> None:
        """Append one forward type when it has not already been declared."""

        if name and name not in seen and is_struct_ref(name, excluded_names):
            seen.add(name)
            names.append(name)

    for row in rows:
        if type(row).__name__ != "Struct":
            continue
        add(row.m_name)
        for member in row.m_members:
            add(base_type_name(member.m_type))
    for row in rows:
        if type(row).__name__ != "FunctionTypeAlias":
            continue
        add(base_type_name(row.m_ret))
        for param in row.m_params:
            add(param_base_type(param))
    for fn in blueprint["functions"]:
        typed = fn.get("typed")
        if not typed:
            continue
        add(base_type_name(str(typed["return"])))
        add_param_type_refs(typed["params"], add)
        add_param_type_refs(typed["try_params"], add)
    for glob in blueprint["globals"]:
        typed = glob.get("typed")
        if typed:
            add(base_type_name(str(typed["type"])))
    return names


def header_context(
    blueprint: dict[str, object], *, unstable: bool
) -> dict[str, object]:
    """Build the stable or unstable template context used for generated PCDOGS headers."""

    guard = "DTTR_PCDOGS_UNSTABLE_H" if unstable else "DTTR_PCDOGS_H"
    external_type_rows = sort_struct_rows_by_value_dependencies(
        list(blueprint.get("external_structs", []))
    )
    type_prefix_rows, packed_type_rows = split_type_rows(blueprint["structs"])
    packed_type_rows = sort_struct_rows_by_value_dependencies(packed_type_rows)
    external_type_names = pcdogs_type_names(external_type_rows)
    external_forward_names = (
        derived_forward_names(
            {"functions": [], "globals": [], "structs": external_type_rows},
            external_type_rows,
        )
        if unstable and external_type_rows
        else []
    )
    forward_names = [
        name
        for name in derived_forward_names(blueprint, blueprint["structs"])
        if unstable or name not in external_type_names.keys()
    ]
    generated_type_names = {
        **pcdogs_type_names(blueprint["structs"]),
        **external_type_names,
        **{name: pcdogs_type_name(name) for name in forward_names},
        **{name: pcdogs_type_name(name) for name in external_forward_names},
    }

    def local_c_type(value: object) -> str:
        return c_type_with_pcdogs_prefix(value, generated_type_names)

    def local_param_decl(param: object) -> str:
        if hasattr(param, "m_type") and hasattr(param, "m_name"):
            return f"{local_c_type(param.m_type)} {param.m_name}"
        if isinstance(param, tuple) and len(param) == 2:
            return f"{local_c_type(param[0])} {param[1]}"
        return str(param)

    def local_c_params(value: object) -> str:
        items = (
            [local_param_decl(item) for item in value]
            if isinstance(value, list)
            else c_list(value)
        )
        return "()" if not items else "(" + ", ".join(items) + ")"

    return {
        "header_guard": guard,
        "hook_macro_name": (
            "DTTR_PCDOGS_UNSTABLE_HOOK" if unstable else "DTTR_PCDOGS_HOOK"
        ),
        "type_prefix_rows": type_prefix_rows,
        "packed_type_rows": packed_type_rows,
        "external_type_rows": external_type_rows,
        "external_forward_names": external_forward_names,
        "forward_names": forward_names,
        "signatures": blueprint["signatures"],
        "functions": blueprint["functions"],
        "globals": blueprint["globals"],
        "function_xrefs": blueprint["function_xrefs"],
        "xrefs": blueprint["xrefs"],
        "param_decl": local_param_decl,
        "c_type": local_c_type,
        "alias_name": pcdogs_type_name,
        "function_type_name": pcdogs_type_name,
        "struct_name": pcdogs_type_name,
        "enum_name": pcdogs_type_name,
        "c_symbol": c_symbol,
        "c_pascal_token": c_pascal_token,
        "c_sig": c_sig,
        "c_mask": c_mask,
        "c_enum": c_enum,
        "c_build_mask": c_build_mask,
        "c_int": c_int,
        "c_uint": c_uint,
        "c_bool": c_bool,
        "c_params": local_c_params,
        "c_args": c_args,
        "doxy_brief": doxy_brief,
        "doxy_comment": doxy_comment,
        "doxy_inline": doxy_inline,
        "member_doc": member_doc,
        "param_doc_pairs": param_doc_pairs,
        "row_doc": row_doc,
        "symbol_doc": symbol_doc,
        "CC_KEYWORD": CC_KEYWORD,
        "CC_ENUM": CC_ENUM,
        "HOOK_ENUM": HOOK_ENUM,
        "DATA_RESOLVER": DATA_RESOLVER,
        "auto_impl_params": auto_impl_params,
        "auto_return": auto_return,
        "unstable": unstable,
    }


def header_h(blueprint: dict[str, object], *, unstable: bool) -> str:
    """Render a full generated PCDOGS header before stripping private sections."""

    template_path = Path(__file__).resolve().with_name("header_template.h.mako")
    template_text = template_path.read_text()
    literal_backslash = "__DTTR_PCDOGS_MAKO_LITERAL_BACKSLASH__"
    template_text = template_text.replace("\\\n", f"{literal_backslash}\n")
    text = Template(
        template_text,
        filename=str(template_path),
        strict_undefined=True,
    ).render_unicode(**header_context(blueprint, unstable=unstable))
    text = text.replace(literal_backslash, "\\")
    return "\n".join(line.rstrip() for line in text.splitlines()) + (
        "\n" if text.endswith("\n") else ""
    )


def public_header_from_full(full: str) -> str:
    """Strip implementation-only sections so the public header stays safe for SDK consumers."""

    out: list[str] = []
    lines = full.splitlines(keepends=True)
    i = 0
    while i < len(lines):
        if lines[i].strip() != "#ifdef DTTR_PCDOGS_IMPLEMENTATION":
            out.append(lines[i])
            i += 1
            continue

        depth = 0
        i += 1
        while i < len(lines):
            stripped = lines[i].strip()
            if stripped.startswith("#if"):
                depth += 1
            elif stripped.startswith("#endif"):
                if depth == 0:
                    i += 1
                    break
                depth -= 1
            i += 1

    return "".join(out)


def implementation_source_c() -> str:
    """Return the stable C shim that owns generated implementation symbols in one object."""

    return """#define DTTR_PCDOGS_IMPLEMENTATION
#include "generated/dttr_pcdogs_full.h"
"""


def main() -> int:
    """Regenerate all PCDOGS SDK outputs from stable and unstable blueprints."""

    args = parse_args()
    sdk_root = Path(__file__).resolve().parent.parent
    repo_root = sdk_root.parent.parent
    blueprint_paths = (
        [Path(args.blueprints)]
        if args.blueprints is not None
        else [
            sdk_root / "blueprints/dttr_pcdogs.py",
            sdk_root / "blueprints/dttr_pcdogs_unstable.py",
        ]
    )
    ok = True
    include_dir = args.include_dir or repo_root / "build/modules/sdk/generated/include"
    src_dir = args.src_dir or repo_root / "build/modules/sdk/generated/src"
    generated_src_dir = src_dir / "generated"
    stable_type_rows: list[object] = []
    for blueprint_path in blueprint_paths:
        blueprint = load_blueprint(blueprint_path)
        is_unstable = blueprint_path.stem.endswith("_unstable")
        header_blueprint = blueprint
        if is_unstable and stable_type_rows:
            header_blueprint = dict(blueprint)
            # Unstable is an extension surface. Do not re-emit stable type rows;
            # only make their names visible while rendering unstable-only rows.
            header_blueprint["external_structs"] = stable_type_rows
        elif not is_unstable:
            stable_type_rows = list(blueprint["structs"])
        header_name = "dttr_pcdogs_unstable.h" if is_unstable else "dttr_pcdogs.h"
        full_header = header_h(header_blueprint, unstable=is_unstable)
        public_header_path = include_dir / header_name
        public_header = clang_format_header(
            public_header_path,
            public_header_from_full(full_header),
        )
        ok = (
            write_or_check(
                public_header_path,
                public_header,
                args.check,
            )
            and ok
        )
        private_header_name = (
            "dttr_pcdogs_unstable_full.h" if is_unstable else "dttr_pcdogs_full.h"
        )
        private_header_path = generated_src_dir / private_header_name
        private_header = clang_format_header(private_header_path, full_header)
        ok = (
            write_or_check(
                private_header_path,
                private_header,
                args.check,
            )
            and ok
        )
        if not is_unstable:
            ok = (
                write_or_check(
                    src_dir / "pcdogs.c",
                    implementation_source_c(),
                    args.check,
                )
                and ok
            )
    if not ok:
        print(
            "PCDOGS symbols header is stale; run modules/sdk/scripts/generate_headers.py",
            file=sys.stderr,
        )
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
