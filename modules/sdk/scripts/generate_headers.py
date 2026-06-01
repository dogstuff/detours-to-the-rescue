#!/usr/bin/env python3
"""Regenerate the PCDOGS SDK symbols definition from one Python blueprint."""

from __future__ import annotations

import argparse
import difflib
import re
import subprocess
import sys
from collections import Counter
from collections.abc import Callable
from dataclasses import dataclass, field, fields, replace
from enum import StrEnum
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


class BlueprintTypeKind(StrEnum):
    TYPE_ALIAS = "TypeAlias"
    FUNCTION_TYPE_ALIAS = "FunctionTypeAlias"
    STRUCT = "Struct"
    ENUM = "Enum"


class TypeRowKind(StrEnum):
    TYPE_ALIAS = "type_alias"
    FUNCTION_TYPE_ALIAS = "function_type_alias"
    STRUCT = "struct"
    ENUM = "enum"


class SymbolDocKind(StrEnum):
    FUNCTION = "function"
    GLOBAL = "global"


class TypedField(StrEnum):
    NAME = "name"
    PUBLIC = "public"
    CC = "cc"
    RET = "ret"
    RETURN_KIND = "return_kind"
    PARAMS = "params"
    ARGS = "args"
    TRY_PARAMS = "try_params"
    TRY_ARGS = "try_args"
    SIGNATURE = "signature"
    DELTA = "delta"
    HOOK_KIND = "hook_kind"
    HOOK_PROLOGUE_SIZE = "hook_prologue_size"
    CALLABLE = "callable"


TYPED_FIELDS = (
    TypedField.NAME,
    TypedField.PUBLIC,
    TypedField.CC,
    TypedField.RET,
    TypedField.RETURN_KIND,
    TypedField.PARAMS,
    TypedField.ARGS,
    TypedField.TRY_PARAMS,
    TypedField.TRY_ARGS,
    TypedField.SIGNATURE,
    TypedField.DELTA,
    TypedField.HOOK_KIND,
    TypedField.HOOK_PROLOGUE_SIZE,
    TypedField.CALLABLE,
)

READ_ONLY_DATA_SUFFIXES = (
    "_dispatch_table",
    "_index_table",
    "_jump_table",
    "_lookup_table",
    "_opcode_table",
    "_op_table",
)

READ_ONLY_DATA_NAMES = {
    "script_command_table",
    "pkg_toc",
    "package_toc_file_sizes",
    "model_physics_callback_table",
    "movement_handler_table",
    "collision_state_handler_table",
}

ENGINE_MANAGED_DATA_NAMES = {
    "player_current_level_id",
    "menu_level_index",
    "player_actor",
    "active_entity_work_list",
    "current_entity_camera",
    "render_list_state",
    "current_level_data",
    "navigation_command_queue",
    "navigation_queue_head",
    "d3d_device7",
    "ddraw_back_buffer",
    "ddraw_primary_surface",
    "ddraw_z_buffer",
    "script_current_actor",
    "actor_default_update_callback_slot",
    "collision_dispatch_actor_func",
    "collision_process_func",
    "collision_response_actor_func",
    "powerup_collision_handler",
    "behavior_process_actor_func",
    "behavior_process_projectile_func",
    "behavior_process_snap_func",
    "behavior_target_actor",
    "behavior_param_0",
    "behavior_param_1",
    "behavior_param_2",
    "level_init_callback_1",
    "level_init_callback_2",
    "powerup_update_func",
    "player_movement_func",
    "projectile_logic_func",
}


@dataclass(frozen=True, slots=True)
class HeaderTypes:
    type_prefix_rows: list[object]
    packed_type_rows: list[object]
    external_type_rows: list[object]
    external_forward_names: list[str]
    forward_names: list[str]
    generated_type_names: dict[str, str]


@dataclass(frozen=True, slots=True)
class SignatureEntry:
    name: str
    sig: str
    mask: str
    required: str


@dataclass(slots=True)
class ParamRow:
    type: str
    name: str
    doc: str | None = None


@dataclass(slots=True)
class SignatureRow:
    name: str
    pattern: str
    required: object
    unstable: bool
    doc: str | None


@dataclass(slots=True)
class HookRow:
    kind: object
    patch_size: int
    entry_patch_size: int


@dataclass(slots=True)
class TypedFunction:
    return_type: str
    abi: object
    params: list[object]
    args: list[str]
    try_params: list[object]
    try_args: list[str]
    signature: str
    delta: int
    hook_kind: object
    hook_prologue_size: int
    callable: bool


@dataclass(slots=True)
class FunctionRow:
    name: str
    calling_convention: object
    callable: bool
    public: bool
    required: object
    match_offset: int
    pattern: str
    hook: HookRow
    unstable: bool
    doc: str | None
    typed: TypedFunction | None = None
    symbol_id: str = ""
    index: int = 0
    public_index: int = 0
    supported_builds: object = "all"


@dataclass(slots=True)
class FunctionXRefRow:
    function: str
    ref_function: str
    instr_off: int
    addr_off: int
    indirections: int = 0
    function_symbol: str = ""
    ref_function_symbol: str = ""


@dataclass(slots=True)
class TypedData:
    type: str
    resolver: object
    ref_function: str
    instr_off: int
    addr_off: int
    indirections: int


@dataclass(slots=True)
class GlobalRow:
    name: str
    unstable: bool
    doc: str | None
    typed: TypedData | None = None
    symbol_id: str = ""
    public_index: int = 0
    supported_builds: object = "all"


@dataclass(slots=True)
class XRefRow:
    global_name: str
    function: str
    instr_off: int
    addr_off: int
    global_symbol: str = ""
    function_symbol: str = ""


@dataclass(slots=True)
class TypeAliasRow:
    source_type: str
    name: str
    unstable: bool
    doc: str | None


@dataclass(slots=True)
class FunctionTypeAliasRow:
    ret: str
    name: str
    params: list[object]
    calling: object
    unstable: bool
    doc: str | None


@dataclass(slots=True)
class StructMemberRow:
    type: str
    name: str
    offset: int | None = None
    doc: str | None = None


@dataclass(slots=True)
class StructRow:
    name: str
    members: list[StructMemberRow]
    size: int | None
    incomplete: bool
    unstable: bool
    doc: str | None


@dataclass(slots=True)
class EnumValueRow:
    name: str
    value: int
    doc: str | None


@dataclass(slots=True)
class EnumRow:
    name: str
    values: list[EnumValueRow]
    alias: str | None
    unstable: bool
    doc: str | None


@dataclass(slots=True)
class BlueprintRows:
    signatures: list[SignatureRow] = field(default_factory=list)
    functions: list[FunctionRow] = field(default_factory=list)
    function_xrefs: list[FunctionXRefRow] = field(default_factory=list)
    globals: list[GlobalRow] = field(default_factory=list)
    xrefs: list[XRefRow] = field(default_factory=list)
    structs: list[object] = field(default_factory=list)
    external_structs: list[object] = field(default_factory=list)


@dataclass(frozen=True, slots=True)
class TypedFunctionRow:
    name: str
    public: str
    display_name: str
    typedef_name: str
    accessor_struct_name: str
    cc: str
    ret: str
    return_kind: str
    params: str
    args: str
    try_params: str
    try_args: str
    signature: str
    delta: str
    hook_kind: str
    hook_prologue_size: str
    callable: str
    function_id: str
    symbol_id: str
    symbol_name: str
    doc: str
    group_doc: str
    param_docs: list[tuple[str, str]]
    try_param_docs: list[tuple[str, str]]
    is_callable_param_docs: list[tuple[str, str]]
    hook_param_docs: list[tuple[str, str]]
    unhook_param_docs: list[tuple[str, str]]

    def macro_values(self) -> tuple[str, ...]:
        return tuple(str(getattr(self, field)) for field in TYPED_FIELDS)


@dataclass(frozen=True, slots=True)
class HeaderContext:
    header_guard: str
    hook_macro_name: str
    type_prefix_rows: list[object]
    packed_type_rows: list[object]
    external_type_rows: list[object]
    external_forward_names: list[str]
    forward_names: list[str]
    signatures: list[SignatureRow]
    functions: list[FunctionRow]
    globals: list[GlobalRow]
    function_xrefs: list[FunctionXRefRow]
    xrefs: list[XRefRow]
    signature_entries: list[SignatureEntry]
    public_functions: list[FunctionRow]
    hidden_functions: list[FunctionRow]
    typed_function_rows: list[TypedFunctionRow]
    param_decl: Callable[[object], str]
    c_type: Callable[[object], str]
    c_data_ptr_decl: Callable[[object, str], str]
    c_data_read_param: Callable[[object, str], str]
    c_data_write_param: Callable[[object, str], str]
    c_data_ptr_cast: Callable[[object], str]
    c_data_write_source: Callable[[object, str], str]
    is_c_array_type: Callable[[object], bool]
    c_array_type_count: Callable[[object], str]
    c_params: Callable[[object], str]
    unstable: bool


_LEVEL_PACKAGE_USE = (
    "level package loading paths that locate material tables, scene graphs, "
    "mesh data, and collision sections"
)

_DOMAIN_USE = {
    "Actor": "spawned/template runtime actor paths for rendering, animation, movement, collision, camera targeting, and per-frame logic",
    "Animation": "animation playback paths for controller chains, keyframes, morph targets, visibility tracks, and mesh pose interpolation",
    "Audio": "Miles/sound playback paths for sample handles, sound descriptors, wave metadata, and runtime sound slots",
    "Camera": "camera update and rendering paths for frustum culling, entity visibility, and view-state management",
    "Checkers": "the in-game checkers/minigame board-state logic",
    "Collision": "level collision paths for geometry tests, hit events, responses, and movement blocking",
    "Component": "component attachment paths for spawned objects, collision boxes, mesh components, and trail effects",
    "D3D": "Direct3D device enumeration, capabilities, render-state, and graphics initialization paths",
    "DDraw": "DirectDraw surface, display-mode, pixel-format, and device enumeration paths",
    "DInput": "DirectInput joystick/device enumeration, data-format setup, and raw input polling paths",
    "Entity": "persistent spawn/script/defaults records that own or request active runtime actors",
    "File": "file and package access paths for asset loading, CRT-style file state, and sharing/access modes",
    "Input": "keyboard, joystick, and gamepad processing before movement and menu logic consume per-frame state",
    "Level": _LEVEL_PACKAGE_USE,
    "LevelBlob": _LEVEL_PACKAGE_USE,
    "Material": "material and texture-table paths for package loading, animation frames, and renderer state setup",
    "Menu": "menu and front-end/HUD paths that prepare progress summaries, option rows, prompts, and save/load screens",
    "Math": "geometry and transform paths for collision checks, culling, camera math, and mesh rendering",
    "Mesh": "mesh loading and rendering paths that read vertices, polygons, normals, material refs, and scene-node transforms",
    "Movie": "movie playback paths that open video streams, maintain playback buffers, and handle skip/close state",
    "Nav": "navigation graph and pathfinding paths for actor movement commands and neighbor/path state",
    "Physics": "physics and movement integration paths that carry per-object simulation state",
    "Powerup": "powerup spawn/update paths that walk per-level powerup entries and instantiate template actor records",
    "Pk": "package-resource parsing paths for level, geometry, material, texture, sprite, sound, script, and UI resources",
    "Pkg": "package table-of-contents and resource-record parsing paths used while loading game data archives",
    "Render": "renderer paths for polygon batches, clipping, sprite layers, colors, gradients, and draw work areas",
    "SaveGame": "save/load paths that store slots, current level progress, flags, and persisted game state",
    "Scene": "scene graph loading and traversal paths that link model/object nodes, local transforms, and resource references",
    "Script": "game scripting interpreter paths for opcode dispatch, script contexts, and script-bound entities",
    "Texture": "texture loading/rendering paths that track surface descriptors and package texture metadata",
    "Trail": "trail and bone-effect rendering paths for segmented trails attached to moving objects or bones",
    "UI": "front-end and HUD paths that draw text, lives icons, spots, and other interface resources",
}

_SPECIFIC_STRUCT_DOCS = {
    "D3D_DriverInfo": "DirectDraw/Direct3D driver enumeration record, covering display device selection, hardware acceleration, and display modes.",
    "File_Handle": "CRT-compatible file handle layout, used by package and asset loading streams.",
    "File_OpenMode": "Access/share-mode pair, passed through file-open wrappers.",
    "Input_Event": "Compact input event record; `type` selects the event kind and `value` carries the button/key payload.",
    "Input_State": "Per-frame input snapshot, shared by movement, menus, movie playback, and replay hooks.",
    "Input_JoystickState": "DirectInput-style joystick axis/POV snapshot, later folded into `DTTR_PCDOGS_Input_State`.",
    "DInput_JoystickState": "DirectInput DIJOYSTATE-compatible axis/button snapshot.",
    "DInput_DeviceEnumContext": "DirectInput joystick enumeration context for discovered device GUIDs.",
    "Movie_PlaybackBuffer": "Movie playback buffer state, covering frame reads, input, and close/skip handling.",
    "SaveGame_Data": "Save-file header plus 0x5c-byte save-slot payloads for game state, settings, and player-lives dwords.",
    "SaveGame_Slot": "0x5c-byte per-slot progress payload, used by save/load UI and completion calculations.",
    "Script_OpcodeTable": "Opcode dispatch table, routing bytecode operations to script handlers.",
    "Script_Context": "Script interpreter context for instruction state and game-script execution data.",
}

_KIND_USE = {
    "Header": "Carries offsets, counts, or version fields, and is read before payload parsing.",
    "Entry": "Represents one item, usually inside a subsystem array or table.",
    "Record": "Represents a serialized or runtime record, scoped to the surrounding subsystem.",
    "Descriptor": "Describes resource or runtime options, then feeds object creation.",
    "Definition": "Stores reusable definition data, which runtime objects reference later.",
    "Handle": "Lightweight reference to subsystem-owned runtime state.",
    "State": "Stores mutable runtime state, updated across frames or subsystem calls.",
    "Context": "Groups call-local or interpreter state for subsystem routines.",
    "Slot": "Represents one fixed slot, usually in a subsystem-managed table.",
    "Table": "Groups indexed lookup entries, used by loader, renderer, or scripting code.",
    "Node": "Participates in graph/tree traversal, or in hierarchy updates.",
    "Vertex": "Stores vertex data for mesh, collision, or renderer paths.",
    "Polygon": "Stores polygon/face data for mesh, collision, or renderer paths.",
    "Keyframe": "Stores animation values, sampled by frame interpolation code.",
    "Controller": "Carries animation control state, including blending inputs.",
    "Resource": "Reflects a package resource or metadata block from game data files.",
}

_DOMAIN_PREFIXES = (
    ("Pkg", "Pkg"),
    ("Pk", "Pk"),
    ("LevelBlob", "LevelBlob"),
    ("SceneNode", "Scene"),
    ("Texture", "Texture"),
)


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
        help="blueprint Python path (default: canonical PCDOGS blueprint)",
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


def param_type_name(param: object) -> tuple[object, object] | None:
    """Return the declared type/name pair for supported blueprint parameter shapes."""

    if hasattr(param, "type") and hasattr(param, "name"):
        return param.type, param.name

    if isinstance(param, tuple) and len(param) == 2:
        return param

    return None


def param_decl_with(param: object, c_type_fn: Callable[[object], str]) -> str:
    """Render parameter metadata as a C declaration using the selected type renderer."""

    if pair := param_type_name(param):
        type_, name = pair
        return f"{c_type_fn(type_)} {name}"

    return str(param)


def param_decl(param: object) -> str:
    """Render blueprint parameter metadata as the C declaration used in generated prototypes."""

    return param_decl_with(param, c_type)


def arg_name(param: object) -> str:
    """Extract a call argument name from generated parameter metadata or raw C text."""

    if pair := param_type_name(param):
        return str(pair[1])

    param = str(param)
    param = param.strip()
    while param.endswith("]"):
        param = param[: param.rfind("[")].rstrip()
    return param.replace("*", " ").replace("&", " ").split()[-1]


def param_args(params: list[object]) -> list[str]:
    """Return generated call argument names while handling void as an empty list."""

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


def row_name(row: object, label: str) -> str:
    """Read the required blueprint row name."""

    name = getattr(row, "name", None)
    if name is None:
        raise ValueError(f"{label} rows need explicit names: {row!r}")

    return name


def normalize_param(param: object) -> object:
    """Copy blueprint params into local rows with unprefixed fields."""

    if hasattr(param, "type") and hasattr(param, "name"):
        return ParamRow(param.type, param.name, getattr(param, "doc", None))

    return param


def normalize_params(params: list[object]) -> list[object]:
    """Copy blueprint parameter lists into local rows."""

    return [normalize_param(param) for param in params]


def normalize_type_row(row: object) -> object:
    """Copy blueprint type rows into local rows with unprefixed fields."""

    try:
        row_kind = BlueprintTypeKind(type(row).__name__)
    except ValueError:
        return row

    match row_kind:
        case BlueprintTypeKind.TYPE_ALIAS:
            return TypeAliasRow(
                row.source_type,
                row.name,
                row.unstable,
                row.doc,
            )
        case BlueprintTypeKind.FUNCTION_TYPE_ALIAS:
            return FunctionTypeAliasRow(
                row.ret,
                row.name,
                normalize_params(row.params),
                row.calling,
                row.unstable,
                row.doc,
            )
        case BlueprintTypeKind.STRUCT:
            return StructRow(
                row.name,
                [
                    StructMemberRow(
                        member.type,
                        member.name,
                        member.offset,
                        member.doc,
                    )
                    for member in row.members
                ],
                row.size,
                row.incomplete,
                row.unstable,
                row.doc,
            )
        case BlueprintTypeKind.ENUM:
            return EnumRow(
                row.name,
                [
                    EnumValueRow(value.name, value.value, value.doc)
                    for value in row.values
                ],
                row.alias,
                row.unstable,
                row.doc,
            )


def signature_entry(row: object) -> SignatureRow:
    """Flatten one signature row."""

    return SignatureRow(
        name=row_name(row, "signature"),
        pattern=row.pattern,
        required=row.required,
        unstable=row.unstable,
        doc=row.doc,
    )


def hook_entry(row: object) -> HookRow:
    """Flatten hook metadata used by generated call and hook helpers."""

    return HookRow(
        kind=row.hook.kind,
        patch_size=row.hook.patch_size,
        entry_patch_size=row.hook.entry_patch_size or row.hook.patch_size,
    )


def typed_function_entry(
    row: object,
    *,
    name: str,
    hook: HookRow,
) -> TypedFunction:
    """Flatten optional typed-function metadata."""

    typed = row.typed
    ret = typed.ret
    params = normalize_params(typed.params)
    args = typed.args or param_args(params)
    try_param_rows = normalize_params(typed.try_params or try_params(ret, params))
    return TypedFunction(
        return_type=ret,
        abi=typed.abi or row.cc,
        params=params,
        args=args,
        try_params=try_param_rows,
        try_args=typed.try_args or args,
        signature=typed.signature or name,
        delta=typed.delta if typed.delta is not None else row.match_offset,
        hook_kind=typed.hook_kind or hook.kind,
        hook_prologue_size=(
            typed.hook_prologue_size
            if typed.hook_prologue_size is not None
            else hook.entry_patch_size
        ),
        callable=typed.callable if typed.callable is not None else row.callable,
    )


def function_entry(
    row: object, function_names: dict[int, str]
) -> tuple[FunctionRow, list[FunctionXRefRow]]:
    """Flatten one function row and its xrefs."""

    name = row_name(row, "function")
    hook = hook_entry(row)
    function = FunctionRow(
        name=name,
        calling_convention=row.cc,
        callable=row.callable,
        public=row.public,
        required=row.required,
        match_offset=row.match_offset,
        pattern=row.pattern,
        hook=hook,
        unstable=row.unstable,
        doc=row.doc,
    )
    if row.typed is not None:
        function.typed = typed_function_entry(row, name=name, hook=hook)

    xrefs = [
        FunctionXRefRow(
            function=name,
            ref_function=ref_name(ref.ref_function, function_names),
            instr_off=ref.instr_off,
            addr_off=ref.addr_off,
            indirections=ref.indirections,
        )
        for ref in row.xrefs
    ]
    return function, xrefs


def global_entry(
    row: object, function_names: dict[int, str]
) -> tuple[GlobalRow, list[XRefRow]]:
    """Flatten one global data row and its xrefs."""

    name = row_name(row, "global")
    typed = None
    if row.typed is not None:
        typed = TypedData(
            type=row.typed.type,
            resolver=row.typed.resolver,
            ref_function=ref_name(row.typed.ref_function, function_names),
            instr_off=row.typed.instr_off,
            addr_off=row.typed.addr_off,
            indirections=row.typed.indirections,
        )

    xrefs = [
        XRefRow(
            global_name=name,
            function=ref_name(ref.function, function_names),
            instr_off=ref.instr_off,
            addr_off=ref.addr_off,
        )
        for ref in row.xrefs
    ]
    return GlobalRow(name, row.unstable, row.doc, typed), xrefs


def attach_symbol_metadata(blueprint: BlueprintRows) -> None:
    """Fill derived symbol names, indexes, and build masks in a flattened blueprint."""

    assign_symbol_ids(blueprint.functions)
    assign_symbol_ids(blueprint.globals)
    function_symbol_ids = {row.name: row.symbol_id for row in blueprint.functions}
    global_symbol_ids = {row.name: row.symbol_id for row in blueprint.globals}

    def function_symbol(name: str) -> str:
        return function_symbol_ids.get(name, c_symbol(name))

    for row in blueprint.function_xrefs:
        row.function_symbol = function_symbol(row.function)
        row.ref_function_symbol = function_symbol(row.ref_function)

    function_required = {row.name: row.required for row in blueprint.functions}
    xref_functions_by_global: dict[str, list[str]] = {}
    for row in blueprint.xrefs:
        row.global_symbol = global_symbol_ids[row.global_name]
        row.function_symbol = function_symbol(row.function)
        xref_functions_by_global.setdefault(row.global_name, []).append(row.function)

    public_function_index = 0
    for index, row in enumerate(blueprint.functions):
        row.index = index
        if row.public:
            row.public_index = public_function_index
            public_function_index += 1

        row.supported_builds = row.required

    for public_data_index, row in enumerate(blueprint.globals):
        row.public_index = public_data_index
        refs = list(xref_functions_by_global.get(row.name, []))
        if row.typed:
            refs.append(row.typed.ref_function)

        row.supported_builds = [
            function_required[ref] for ref in refs if ref in function_required
        ] or ["all"]


def load_blueprint(path: Path) -> BlueprintRows:
    """Flatten one Python blueprint into rows consumed by the C header template."""

    module = load_python_module(path, "blueprint_source")
    source = getattr(module, "BLUEPRINT", None)
    if source is None:
        raise ValueError(f"blueprint module does not define BLUEPRINT: {path}")

    blueprint = BlueprintRows()
    signature_rows = list(source.signatures)
    function_rows = list(source.functions)
    global_rows = list(source.globals)
    function_names = {id(row): row.name for row in function_rows}

    for row in signature_rows:
        blueprint.signatures.append(signature_entry(row))

    for row in function_rows:
        function, xrefs = function_entry(row, function_names)
        blueprint.functions.append(function)
        blueprint.function_xrefs.extend(xrefs)

    for row in global_rows:
        glob, xrefs = global_entry(row, function_names)
        blueprint.globals.append(glob)
        blueprint.xrefs.extend(xrefs)

    blueprint.structs.extend(normalize_type_row(row) for row in source.types)

    attach_symbol_metadata(blueprint)
    validate_blueprint(blueprint)
    return blueprint


def assign_symbol_ids(rows: list[object]) -> None:
    """Assign stable C enum tokens while preserving duplicate generated names through suffixes."""

    bases = [c_symbol(row.name) for row in rows]
    counts = Counter(bases)
    used: set[str] = set()
    for row, base in zip(rows, bases, strict=True):
        symbol_id = base
        if counts[base] > 1 and symbol_id in used:
            suffix = 2
            while f"{base}_{suffix}" in used:
                suffix += 1
            symbol_id = f"{base}_{suffix}"
        row.symbol_id = symbol_id
        used.add(symbol_id)


def check_unique(
    rows: list[object], label: str, *, allow_stable_unstable_pair: bool = False
) -> None:
    """Reject duplicate generated symbol names before they become ambiguous C APIs."""

    seen = set()
    for row in rows:
        key = (row.name, bool(row.unstable)) if allow_stable_unstable_pair else row.name
        if key in seen:
            raise ValueError(f"duplicate {label}: {row.name}")

        seen.add(key)


def validate_blueprint(blueprint: BlueprintRows) -> None:
    """Validate blueprint names and xrefs before emitting headers or implementation stubs."""

    check_unique(blueprint.signatures, "signature")
    check_unique(blueprint.functions, "function", allow_stable_unstable_pair=True)
    check_unique(blueprint.globals, "global")

    functions = {row.name for row in blueprint.functions}
    for row in blueprint.function_xrefs:
        if row.function not in functions:
            raise ValueError(f"function XRef has unknown target: {row.function}")

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


_PASCAL_ACRONYMS = {
    "ail": "AIL",
    "api": "API",
    "argb": "ARGB",
    "bgra": "BGRA",
    "cpu": "CPU",
    "d3d": "D3D",
    "ddraw": "DDraw",
    "dds": "DDS",
    "dinput": "DInput",
    "gpu": "GPU",
    "guid": "GUID",
    "io": "IO",
    "lod": "LOD",
    "obb": "OBB",
    "rgb": "RGB",
    "rgba": "RGBA",
    "rhw": "RHW",
    "sdl": "SDL",
    "toc": "TOC",
    "ui": "UI",
    "uv": "UV",
    "xzy": "XZY",
    "xyz": "XYZ",
}


def c_pascal_part(part: str) -> str:
    """Convert one snake-case part while preserving known SDK acronyms."""

    lower = part.lower()
    if lower in _PASCAL_ACRONYMS:
        return _PASCAL_ACRONYMS[lower]

    for acronym, rendered in _PASCAL_ACRONYMS.items():
        if lower.startswith(acronym) and lower[len(acronym) :].isdigit():
            return rendered + part[len(acronym) :]

    return part[:1].upper() + part[1:]


def c_pascal_token(value: object) -> str:
    """Convert generated symbol names to a public Pascal-style C token."""

    return "".join(c_pascal_part(part) for part in str(value).split("_") if part)


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


ARRAY_TYPE_RE = re.compile(r"^(?P<base>.+)\[(?P<count>[^\]]+)\]$")


def c_array_type_parts(value: object) -> tuple[str, str] | None:
    """Return the base type and element count for blueprint C arrays like char[N]."""

    text = c_type(value)
    match = ARRAY_TYPE_RE.match(text)
    if match is None:
        return None

    return match.group("base").strip(), match.group("count").strip()


def is_c_array_type(value: object) -> bool:
    """Return whether a blueprint type is a C array type."""

    return c_array_type_parts(value) is not None


def c_array_type_count(value: object) -> str:
    """Return a C literal for a blueprint array element count."""

    parts = c_array_type_parts(value)
    if parts is None:
        raise ValueError(f"not a C array type: {value}")

    return c_uint(parts[1])


def c_data_ptr_decl(value: object, declarator: str) -> str:
    """Declare a pointer-returning data accessor while preserving array types."""

    parts = c_array_type_parts(value)
    if parts is None:
        return f"{c_type(value)}* {declarator}"

    base, count = parts
    return f"{base} (*{declarator})[{count}]"


def c_data_read_param(value: object, name: str) -> str:
    """Declare the output parameter for a generated data Read accessor."""

    parts = c_array_type_parts(value)
    if parts is None:
        return f"{c_type(value)}* {name}"

    base, count = parts
    return f"{base} (*{name})[{count}]"


def c_data_write_param(value: object, name: str) -> str:
    """Declare the input parameter for generated data Write/UnsafeWrite accessors."""

    parts = c_array_type_parts(value)
    if parts is None:
        return f"{c_type(value)} {name}"

    base, count = parts
    return f"const {base} (*{name})[{count}]"


def c_data_ptr_cast(value: object) -> str:
    """Return a cast expression prefix for a generated data Ptr accessor."""

    parts = c_array_type_parts(value)
    if parts is None:
        return f"({c_type(value)}*)"

    base, count = parts
    return f"({base} (*)[{count}])"


def c_data_write_source(value: object, name: str) -> str:
    """Return the source pointer used when copying a Write accessor value."""

    return name if is_c_array_type(value) else f"&{name}"


def pcdogs_type_name(name: str) -> str:
    """Return the public C name for a generated PCDOGS type."""

    if name.startswith("DTTR_PCDOGS_T_"):
        return name

    return f"DTTR_PCDOGS_T_{name}"


def type_row_kind(row: object) -> TypeRowKind:
    """Classify flattened type rows without exposing implementation class names."""

    if isinstance(row, TypeAliasRow):
        return TypeRowKind.TYPE_ALIAS

    if isinstance(row, FunctionTypeAliasRow):
        return TypeRowKind.FUNCTION_TYPE_ALIAS

    if isinstance(row, StructRow):
        return TypeRowKind.STRUCT

    if isinstance(row, EnumRow):
        return TypeRowKind.ENUM

    raise ValueError(f"unknown type row: {row!r}")


TYPE_NAME_RENDERERS = dict.fromkeys(
    (
        TypeRowKind.TYPE_ALIAS,
        TypeRowKind.FUNCTION_TYPE_ALIAS,
        TypeRowKind.STRUCT,
        TypeRowKind.ENUM,
    ),
    pcdogs_type_name,
)


def pcdogs_type_names(rows: list[object]) -> dict[str, str]:
    """Map generated blueprint type identifiers to public PCDOGS names."""

    names: dict[str, str] = {}
    for row in rows:
        row_kind = type_row_kind(row)
        renderer = TYPE_NAME_RENDERERS.get(row_kind)
        if not renderer:
            continue

        names[row.name] = renderer(row.name)
        if row_kind == TypeRowKind.ENUM and row.alias:
            names[row.alias] = renderer(row.alias)
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
    """Split generated C argument text while handling void-style lists as empty."""

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


def struct_doc(name: str) -> str:
    """Return a concise, usage-oriented description for a generated structure."""

    if name in _SPECIFIC_STRUCT_DOCS:
        return _SPECIFIC_STRUCT_DOCS[name]

    for prefix, domain in _DOMAIN_PREFIXES:
        if name.startswith(prefix):
            return f"Used in {_DOMAIN_USE[domain]}. {_kind_sentence(name)}"

    if "_" in name:
        domain = name.split("_", 1)[0]
    else:
        domain = next(
            (
                candidate
                for candidate in sorted(_DOMAIN_USE, key=len, reverse=True)
                if name.startswith(candidate)
            ),
            "PCDOGS",
        )

    if domain == "PCDOGS" or domain not in _DOMAIN_USE:
        return f"Used by game code. {_kind_sentence(name)}"

    return f"Used in {_DOMAIN_USE[domain]}. {_kind_sentence(name)}"


def _kind_sentence(name: str) -> str:
    for suffix, sentence in _KIND_USE.items():
        if name.endswith(suffix) or f"_{suffix}" in name:
            return sentence

    return "Keeps the field layout that subsystem callers read or write directly."


def row_doc(row: object) -> str | None:
    """Read optional documentation from blueprint dataclasses or flattened rows."""

    value = getattr(row, "doc", None)
    if value:
        return doxy_text(value)

    if isinstance(row, StructRow):
        return doxy_text(struct_doc(row.name))

    if type(row).__name__ == BlueprintTypeKind.STRUCT:
        return doxy_text(struct_doc(getattr(row, "name", "")))

    return None


def symbol_doc(kind: SymbolDocKind | str, row: object) -> str:
    """Return explicit blueprint documentation or generated text."""

    explicit = row_doc(row)
    if explicit:
        return explicit

    SymbolDocKind(kind)
    return "Not yet documented."


def inferred_param_doc(name: str) -> str:
    """Return a generic generated parameter explanation when blueprints have no prose."""

    if name == "ctx":
        return "Runtime context for resolution and calls."

    if name == "out_ret":
        return "Receives the return value on success."

    if name.startswith("out_"):
        return "Receives the value on success."

    return "Unnamed argument."


def param_doc_pairs(
    params: object, *, infer_missing: bool = False
) -> list[tuple[str, str]]:
    """Extract parameter docs from blueprint Param rows for generated comments."""

    if not isinstance(params, list):
        return []

    pairs: list[tuple[str, str]] = []
    for param in params:
        name = arg_name(param)
        doc = getattr(param, "doc", None)
        if doc:
            pairs.append((name, doxy_text(doc)))
        elif infer_missing:
            pairs.append((name, inferred_param_doc(name)))
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
    offset = getattr(row, "offset", None)
    if offset is not None:
        parts.append(f"Offset 0x{offset:X}.")
    return " ".join(parts)


def auto_impl_params(row: TypedFunctionRow, ctx_name: str) -> str:
    """Build auto-wrapper implementation parameters with the SDK context inserted first."""

    params = inner_parens(row.params)
    if not params:
        return f"(const DTTR_Core_Context*{ctx_name})"

    return f"(const DTTR_Core_Context*{ctx_name}, {params})"


def auto_impl_args(row: TypedFunctionRow, ctx_expr: str | None = None) -> str:
    """Build auto-wrapper call arguments that pass the chosen stored SDK context."""

    ctx_expr = ctx_expr or f"dttr_pcdogs_ctx_{row.name}"
    args = inner_parens(row.args)
    if not args:
        return f"({ctx_expr})"

    return f"({ctx_expr}, {args})"


def auto_return(
    row: TypedFunctionRow,
    impl_name: str | None = None,
    ctx_expr: str | None = None,
) -> str:
    """Render a generated auto-wrapper body that handles void and value returns."""

    impl_name = impl_name or f"dttr_pcdogs_impl_{row.name}"
    call = f"{impl_name}{auto_impl_args(row, ctx_expr)}"
    if row.ret == "void":
        return f"\t{call};"

    return f"\treturn {call};"


def signature_symbols(signatures: list[SignatureRow]) -> set[str]:
    """Return signature enum names declared directly by blueprint signatures."""

    return {c_symbol(row.name) for row in signatures}


def template_signature_entries(
    signatures: list[SignatureRow],
    functions: list[FunctionRow],
    explicit_names: set[str],
) -> list[SignatureEntry]:
    """Build signature macro rows emitted by the private generated header."""

    entries = [
        SignatureEntry(
            name=c_symbol(row.name),
            sig=c_sig(row.pattern),
            mask=c_mask(row.pattern),
            required=c_enum(row.required),
        )
        for row in signatures
    ]
    entries.extend(
        SignatureEntry(
            name="FN_" + c_symbol(row.name),
            sig=c_sig(row.pattern),
            mask=c_mask(row.pattern),
            required=c_enum(row.required),
        )
        for row in functions
        if "FN_" + c_symbol(row.name) not in explicit_names
    )
    return entries


def public_function_rows(functions: list[FunctionRow]) -> list[FunctionRow]:
    """Return functions exposed through the public generated facade."""

    return [row for row in functions if row.public]


def hidden_function_rows(functions: list[FunctionRow]) -> list[FunctionRow]:
    """Return private function symbols kept out of the public facade."""

    return [row for row in functions if not row.public]


def data_write_policy(row: GlobalRow) -> str:
    """Classify how generated data helpers may write a symbol."""

    name = row.name.lower()
    if name.endswith(READ_ONLY_DATA_SUFFIXES):
        return "DTTR_PCDOGS_WRITE_POLICY_READ_ONLY"

    if name in READ_ONLY_DATA_NAMES:
        return "DTTR_PCDOGS_WRITE_POLICY_READ_ONLY"

    if name in ENGINE_MANAGED_DATA_NAMES:
        return "DTTR_PCDOGS_WRITE_POLICY_ENGINE_MANAGED"

    if row.typed:
        return "DTTR_PCDOGS_WRITE_POLICY_RAW_MEMORY"

    return "DTTR_PCDOGS_WRITE_POLICY_UNKNOWN"


def typed_data_is_pointer(row: GlobalRow) -> bool:
    """Return whether a typed data symbol stores a pointer value."""

    return bool(row.typed and str(row.typed.type).strip().endswith("*"))


def data_patch_spec_kind(row: GlobalRow) -> str:
    """Return the generated patch-spec kind available for a data symbol."""

    if typed_data_is_pointer(row):
        return "DTTR_PCDOGS_PATCH_DATA_POINTER_HOOK"

    return "DTTR_PCDOGS_PATCH_UNSUPPORTED"


def typed_function_template_row(
    fn: FunctionRow,
    *,
    explicit_signature_names: set[str],
    c_type_fn: Callable[[object], str],
    c_params_fn: Callable[[object], str],
) -> TypedFunctionRow:
    """Build one template row for typed function helpers."""

    typed = fn.typed
    if typed is None:
        raise ValueError(f"function is not typed: {fn.name}")

    ident = fn.name.lower()
    public = c_pascal_token(fn.name)
    display_name = fn.name
    typedef_name = f"DTTR_PCDOGS_F_{public}_proto"
    ret = c_type_fn(typed.return_type)
    signature = c_symbol(typed.signature)
    if signature not in explicit_signature_names:
        signature = "FN_" + c_symbol(fn.name)

    param_docs = param_doc_pairs(typed.params, infer_missing=True)
    return TypedFunctionRow(
        name=ident,
        public=public,
        display_name=display_name,
        typedef_name=typedef_name,
        accessor_struct_name=f"dttr_pcdogs_function_accessor_{public}",
        cc=CC_KEYWORD[str(typed.abi)],
        ret=ret,
        return_kind=(
            "DTTR_PCDOGS_RETURN_VOID" if ret == "void" else "DTTR_PCDOGS_RETURN_VALUE"
        ),
        params=c_params_fn(typed.params),
        args=c_args(typed.args),
        try_params=c_params_fn(typed.try_params),
        try_args=c_args(typed.try_args),
        signature=signature,
        delta=c_int(typed.delta),
        hook_kind=HOOK_ENUM[str(typed.hook_kind)],
        hook_prologue_size=c_uint(typed.hook_prologue_size),
        callable=c_bool(typed.callable),
        function_id=f"DTTR_PCDOGS_FUNCTION_{fn.symbol_id}",
        symbol_id=f"DTTR_PCDOGS_SYMBOL_FUNCTION_ID_{fn.symbol_id}",
        symbol_name=fn.name,
        doc=symbol_doc(SymbolDocKind.FUNCTION, fn),
        group_doc=f"Helpers for `{display_name}`.",
        param_docs=param_docs,
        try_param_docs=param_doc_pairs(typed.try_params, infer_missing=True),
        is_callable_param_docs=[("ctx", "Runtime context for this check.")],
        hook_param_docs=[
            ("ctx", "Runtime context for hook install."),
            ("detour", f"Replacement function with the `{typedef_name}` signature."),
            ("out_original", "Receives the trampoline when requested."),
        ],
        unhook_param_docs=[("ctx", "Runtime context for hook detach.")],
    )


def typed_function_template_rows(
    functions: list[FunctionRow],
    *,
    explicit_signature_names: set[str],
    c_type_fn: Callable[[object], str],
    c_params_fn: Callable[[object], str],
) -> list[TypedFunctionRow]:
    """Build typed function helper rows for the Mako template."""

    return [
        typed_function_template_row(
            fn,
            explicit_signature_names=explicit_signature_names,
            c_type_fn=c_type_fn,
            c_params_fn=c_params_fn,
        )
        for fn in functions
        if fn.typed
    ]


def split_type_rows(rows: list[object]) -> tuple[list[object], list[object]]:
    """Separate prefix rows from packed structs for safe header ordering."""

    for index, row in enumerate(rows):
        if type_row_kind(row) == TypeRowKind.STRUCT:
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

    structs = {
        row.name: row for row in rows if type_row_kind(row) == TypeRowKind.STRUCT
    }
    if not structs:
        return rows

    def value_dependencies(row: StructRow) -> set[str]:
        deps: set[str] = set()
        for member in row.members:
            member_type = str(member.type)
            if "*" in member_type:
                continue

            dep = base_type_name(member_type)
            if dep in structs and dep != row.name:
                deps.add(dep)
        return deps

    dependencies = {name: value_dependencies(row) for name, row in structs.items()}
    pending = dict(structs)
    ordered_names: list[str] = []
    while pending:
        ready = [name for name in pending if not (dependencies[name] & pending.keys())]
        if not ready:
            ordered_names.extend(pending)
            break

        ordered_names.extend(ready)
        for name in ready:
            del pending[name]

    ordered_structs = [structs[name] for name in ordered_names]
    non_structs = [row for row in rows if type_row_kind(row) != TypeRowKind.STRUCT]
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
        row_kind = type_row_kind(row)
        if row_kind in {TypeRowKind.TYPE_ALIAS, TypeRowKind.FUNCTION_TYPE_ALIAS}:
            names.add(row.name)
        elif row_kind == TypeRowKind.ENUM:
            names.add(row.name)
            names.add(row.alias or row.name)
    return names


def param_base_type(param: object) -> str:
    """Return the base C type named by one generated parameter row."""

    if pair := param_type_name(param):
        return base_type_name(str(pair[0]))

    return base_type_name(str(param))


def add_param_type_refs(params: object, add: object) -> None:
    """Collect base type references from generated parameters for forward declaration derivation."""

    if not isinstance(params, list):
        return

    for param in params:
        add(param_base_type(param))


def derived_forward_names(blueprint: BlueprintRows, rows: list[object]) -> list[str]:
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
        if type_row_kind(row) != TypeRowKind.STRUCT:
            continue

        add(row.name)
        for member in row.members:
            add(base_type_name(member.type))
    for row in rows:
        if type_row_kind(row) != TypeRowKind.FUNCTION_TYPE_ALIAS:
            continue

        add(base_type_name(row.ret))
        for param in row.params:
            add(param_base_type(param))
    for fn in blueprint.functions:
        typed = fn.typed
        if not typed:
            continue

        add(base_type_name(str(typed.return_type)))
        add_param_type_refs(typed.params, add)
        add_param_type_refs(typed.try_params, add)
    for glob in blueprint.globals:
        typed = glob.typed
        if typed:
            add(base_type_name(str(typed.type)))
    return names


def header_type_context(blueprint: BlueprintRows, *, unstable: bool) -> HeaderTypes:
    """Prepare type rows and public generated names for header rendering."""

    external_type_rows = sort_struct_rows_by_value_dependencies(
        list(blueprint.external_structs)
    )
    type_prefix_rows, packed_type_rows = split_type_rows(blueprint.structs)
    packed_type_rows = sort_struct_rows_by_value_dependencies(packed_type_rows)
    external_type_names = pcdogs_type_names(external_type_rows)
    external_forward_names: list[str] = []
    if unstable and external_type_rows:
        external_forward_names = derived_forward_names(
            BlueprintRows(structs=external_type_rows),
            external_type_rows,
        )

    forward_names = [
        name
        for name in derived_forward_names(blueprint, blueprint.structs)
        if name not in external_type_names
    ]
    generated_type_names = {
        **pcdogs_type_names(blueprint.structs),
        **external_type_names,
        **{name: pcdogs_type_name(name) for name in forward_names},
        **{name: pcdogs_type_name(name) for name in external_forward_names},
    }
    return HeaderTypes(
        type_prefix_rows=type_prefix_rows,
        packed_type_rows=packed_type_rows,
        external_type_rows=external_type_rows,
        external_forward_names=external_forward_names,
        forward_names=forward_names,
        generated_type_names=generated_type_names,
    )


STATIC_TEMPLATE_CONTEXT = {
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
    "data_patch_spec_kind": data_patch_spec_kind,
    "data_write_policy": data_write_policy,
    "type_row_kind": type_row_kind,
    "TYPE_ROW": TypeRowKind,
    "DOC_KIND": SymbolDocKind,
}


def header_context(blueprint: BlueprintRows, *, unstable: bool) -> HeaderContext:
    """Build the stable or unstable template context used for generated PCDOGS headers."""

    guard = "DTTR_PCDOGS_UNSTABLE_H" if unstable else "DTTR_PCDOGS_H"
    header_types = header_type_context(blueprint, unstable=unstable)

    def local_c_type(value: object) -> str:
        return c_type_with_pcdogs_prefix(value, header_types.generated_type_names)

    def local_param_decl(param: object) -> str:
        return param_decl_with(param, local_c_type)

    def local_c_params(value: object) -> str:
        items = (
            [local_param_decl(item) for item in value]
            if isinstance(value, list)
            else c_list(value)
        )
        return "()" if not items else "(" + ", ".join(items) + ")"

    public_functions = public_function_rows(blueprint.functions)
    explicit_names = signature_symbols(blueprint.signatures)

    return HeaderContext(
        header_guard=guard,
        hook_macro_name=(
            "DTTR_PCDOGS_UNSTABLE_HOOK" if unstable else "DTTR_PCDOGS_HOOK"
        ),
        type_prefix_rows=header_types.type_prefix_rows,
        packed_type_rows=header_types.packed_type_rows,
        external_type_rows=header_types.external_type_rows,
        external_forward_names=header_types.external_forward_names,
        forward_names=header_types.forward_names,
        signatures=blueprint.signatures,
        functions=blueprint.functions,
        globals=blueprint.globals,
        function_xrefs=blueprint.function_xrefs,
        xrefs=blueprint.xrefs,
        signature_entries=template_signature_entries(
            blueprint.signatures,
            blueprint.functions,
            explicit_names,
        ),
        public_functions=public_functions,
        hidden_functions=hidden_function_rows(blueprint.functions),
        typed_function_rows=typed_function_template_rows(
            public_functions,
            explicit_signature_names=explicit_names,
            c_type_fn=local_c_type,
            c_params_fn=local_c_params,
        ),
        param_decl=local_param_decl,
        c_type=local_c_type,
        c_data_ptr_decl=lambda value, declarator: c_data_ptr_decl(
            local_c_type(value), declarator
        ),
        c_data_read_param=lambda value, name: c_data_read_param(
            local_c_type(value), name
        ),
        c_data_write_param=lambda value, name: c_data_write_param(
            local_c_type(value), name
        ),
        c_data_ptr_cast=lambda value: c_data_ptr_cast(local_c_type(value)),
        c_data_write_source=c_data_write_source,
        is_c_array_type=lambda value: is_c_array_type(local_c_type(value)),
        c_array_type_count=lambda value: c_array_type_count(local_c_type(value)),
        c_params=local_c_params,
        unstable=unstable,
    )


def template_context(context: HeaderContext) -> dict[str, object]:
    """Return a Mako context mapping without requiring dataclass instances to have __dict__."""

    return {
        **STATIC_TEMPLATE_CONTEXT,
        **{field.name: getattr(context, field.name) for field in fields(context)},
    }


def header_h(blueprint: BlueprintRows, *, unstable: bool) -> str:
    """Render a full generated PCDOGS header before stripping private sections."""

    template_path = Path(__file__).resolve().with_name("header_template.h.mako")
    template_text = template_path.read_text()
    literal_backslash = "__DTTR_PCDOGS_MAKO_LITERAL_BACKSLASH__"
    template_text = template_text.replace("\\\n", f"{literal_backslash}\n")
    text = Template(
        template_text,
        filename=str(template_path),
        strict_undefined=True,
    ).render_unicode(**template_context(header_context(blueprint, unstable=unstable)))
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


def default_blueprint_path(args: argparse.Namespace, sdk_root: Path) -> Path:
    return (
        Path(args.blueprints)
        if args.blueprints
        else sdk_root / "blueprints/dttr_pcdogs.py"
    )


def output_dirs(args: argparse.Namespace, repo_root: Path) -> tuple[Path, Path]:
    """Resolve public include and generated source output directories."""

    include_dir = args.include_dir or repo_root / "build/modules/sdk/generated/include"
    src_dir = args.src_dir or repo_root / "build/modules/sdk/generated/src"
    return include_dir, src_dir


def header_blueprint(
    blueprint: BlueprintRows,
    *,
    is_unstable: bool,
    stable_type_rows: list[object],
) -> BlueprintRows:
    """Add stable type rows when rendering the unstable extension header."""

    if not is_unstable or not stable_type_rows:
        return blueprint

    # Unstable is an extension surface. Do not re-emit stable type rows;
    # only make their names visible while rendering unstable-only rows.
    return replace(blueprint, external_structs=stable_type_rows)


def header_names(is_unstable: bool) -> tuple[str, str]:
    """Return the public and private generated header names."""

    if is_unstable:
        return "dttr_pcdogs_unstable.h", "dttr_pcdogs_unstable_full.h"

    return "dttr_pcdogs.h", "dttr_pcdogs_full.h"


def write_blueprint_outputs(
    blueprint: BlueprintRows,
    *,
    is_unstable: bool,
    stable_type_rows: list[object],
    include_dir: Path,
    src_dir: Path,
    check: bool,
) -> bool:
    """Render and write the generated outputs for one blueprint."""

    public_header_name, private_header_name = header_names(is_unstable)

    full_header = header_h(
        header_blueprint(
            blueprint,
            is_unstable=is_unstable,
            stable_type_rows=stable_type_rows,
        ),
        unstable=is_unstable,
    )

    public_header_path = include_dir / public_header_name
    private_header_path = src_dir / "generated" / private_header_name
    public_header = clang_format_header(
        public_header_path,
        public_header_from_full(full_header),
    )
    private_header = clang_format_header(private_header_path, full_header)
    ok = write_or_check(public_header_path, public_header, check)
    ok = write_or_check(private_header_path, private_header, check) and ok

    if is_unstable:
        return ok

    return write_or_check(src_dir / "pcdogs.c", implementation_source_c(), check) and ok


def _row_names(rows: list[object]) -> set[str]:
    return {row.name for row in rows}


def _split_stability(rows: list[object]) -> tuple[list[object], list[object]]:
    return (
        [row for row in rows if not row.unstable],
        [row for row in rows if row.unstable],
    )


def _xrefs_for_surface(
    rows: list[object], global_names: set[str], function_names: set[str]
) -> list[object]:
    return [
        row
        for row in rows
        if row.global_name in global_names and row.function in function_names
    ]


def _function_xrefs_for_surface(
    rows: list[object], function_names: set[str]
) -> list[object]:
    return [
        row
        for row in rows
        if row.function in function_names and row.ref_function in function_names
    ]


def split_row_unstable_rows(
    blueprint: BlueprintRows,
) -> tuple[BlueprintRows, BlueprintRows]:
    stable_signatures, unstable_signatures = _split_stability(blueprint.signatures)
    stable_functions, unstable_functions = _split_stability(blueprint.functions)
    stable_globals, unstable_globals = _split_stability(blueprint.globals)
    stable_structs, unstable_structs = _split_stability(blueprint.structs)

    unstable_function_names = _row_names(unstable_functions)
    unstable_global_names = _row_names(unstable_globals)
    stable_function_names = _row_names(stable_functions)
    stable_global_names = _row_names(stable_globals)
    stable = replace(
        blueprint,
        signatures=stable_signatures,
        functions=stable_functions,
        function_xrefs=_function_xrefs_for_surface(
            blueprint.function_xrefs, stable_function_names
        ),
        globals=stable_globals,
        xrefs=_xrefs_for_surface(
            blueprint.xrefs, stable_global_names, stable_function_names
        ),
        structs=stable_structs,
    )

    unstable_symbol_function_names = unstable_function_names | {
        row.function
        for row in blueprint.xrefs
        if row.global_name in unstable_global_names
    }

    xref_support_functions = [
        replace(row, public=False, unstable=True)
        for row in stable_functions
        if row.name in unstable_symbol_function_names
        and row.name not in unstable_function_names
    ]

    promoted = replace(
        blueprint,
        signatures=unstable_signatures,
        functions=[*unstable_functions, *xref_support_functions],
        function_xrefs=_function_xrefs_for_surface(
            blueprint.function_xrefs, unstable_symbol_function_names
        ),
        globals=unstable_globals,
        xrefs=_xrefs_for_surface(
            blueprint.xrefs, unstable_global_names, unstable_symbol_function_names
        ),
        structs=unstable_structs,
        external_structs=[],
    )

    attach_symbol_metadata(stable)
    attach_symbol_metadata(promoted)
    validate_blueprint(stable)
    validate_blueprint(promoted)
    validate_split_blueprints(stable, promoted)

    return stable, promoted


def validate_split_blueprints(stable: BlueprintRows, unstable: BlueprintRows) -> None:
    stable_public_functions = {row.name for row in stable.functions if row.public}
    unstable_public_functions = {row.name for row in unstable.functions if row.public}

    overlap = stable_public_functions & unstable_public_functions
    if overlap:
        raise ValueError(
            "public function exported by both stable and unstable surfaces: "
            + ", ".join(sorted(overlap))
        )

    unstable_function_names = {row.name for row in unstable.functions}
    unstable_data_xrefs = {row.global_name for row in unstable.xrefs}

    for row in unstable.globals:
        if row.typed and row.name not in unstable_data_xrefs:
            raise ValueError(f"unstable typed global has no data xref: {row.name}")

        if row.typed and row.typed.ref_function not in unstable_function_names:
            raise ValueError(
                f"unstable typed global xref function missing from output: {row.name}"
            )


def main() -> int:
    """Regenerate stable and unstable SDK outputs from the canonical blueprint."""

    args = parse_args()
    sdk_root = Path(__file__).resolve().parent.parent
    repo_root = sdk_root.parent.parent
    blueprint_path = default_blueprint_path(args, sdk_root)
    include_dir, src_dir = output_dirs(args, repo_root)

    stable_blueprint, unstable_blueprint = split_row_unstable_rows(
        load_blueprint(blueprint_path)
    )

    stable_type_rows = list(stable_blueprint.structs)
    ok = True

    for is_unstable, output_blueprint in (
        (False, stable_blueprint),
        (True, unstable_blueprint),
    ):
        ok = (
            write_blueprint_outputs(
                output_blueprint,
                is_unstable=is_unstable,
                stable_type_rows=stable_type_rows,
                include_dir=include_dir,
                src_dir=src_dir,
                check=args.check,
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
