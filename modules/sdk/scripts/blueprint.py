"""Small Python DSL for the PCDOGS SDK blueprint."""

from __future__ import annotations

from dataclasses import dataclass, field as dc_field
from enum import StrEnum
from typing import TypeAlias

__all__ = [
    "Blueprint",
    "CallingConvention",
    "Enum",
    "EnumValue",
    "Function",
    "FunctionTypeAlias",
    "FunctionXRef",
    "Data",
    "Hook",
    "HookKind",
    "Param",
    "Required",
    "Resolver",
    "Signature",
    "Struct",
    "StructMember",
    "TypeAlias",
    "TypedFunction",
    "TypedData",
    "UNKNOWN_PARAMS",
    "enum_value",
    "fx",
    "hook",
    "member",
    "param",
    "typed",
    "xref",
]


class CallingConvention(StrEnum):
    CDECL = "cdecl"
    STDCALL = "stdcall"
    FASTCALL = "fastcall"
    CALLBACK = "CALLBACK"


class Resolver(StrEnum):
    XREF_U32 = "xref_u32"


class Required(StrEnum):
    ALL = "all"
    EN = "en"
    EU_SC = "eu_sc"
    EN_EU = "en_eu"
    EN_SC = "en_sc"


class HookKind(StrEnum):
    REL32 = "rel32"
    HOTPATCH = "hotpatch"
    UNSUPPORTED = "unsupported"


@dataclass(frozen=True)
class Param:
    type: str
    name: str
    doc: str | None = None


UNKNOWN_PARAMS: list[Param] = [Param("uintptr_t", f"a{i}") for i in range(8)]


@dataclass(frozen=True)
class Signature:
    pattern: str
    name: str | None = None
    required: Required = Required.ALL
    unstable: bool = False
    doc: str | None = None


@dataclass(frozen=True)
class TypeAlias:
    source_type: str
    name: str
    unstable: bool = False
    doc: str | None = None


@dataclass(frozen=True)
class FunctionTypeAlias:
    ret: str
    name: str
    params: list[Param]
    calling: CallingConvention = CallingConvention.CALLBACK
    unstable: bool = False
    doc: str | None = None


@dataclass(frozen=True)
class StructMember:
    type: str
    name: str
    offset: int | None = None
    doc: str | None = None


@dataclass(frozen=True)
class Struct:
    name: str
    members: list[StructMember]
    size: int | None = None
    incomplete: bool = True
    unstable: bool = False
    doc: str | None = None


@dataclass(frozen=True)
class EnumValue:
    name: str
    value: int
    doc: str | None = None


@dataclass(frozen=True)
class Enum:
    name: str
    values: list[EnumValue]
    alias: str | None = None
    unstable: bool = False
    doc: str | None = None


@dataclass(frozen=True)
class Hook:
    patch_size: int
    kind: HookKind = HookKind.REL32
    entry_patch_size: int | None = None


@dataclass(frozen=True)
class TypedFunction:
    ret: str
    params: list[Param]
    abi: CallingConvention | None = None
    args: list[str] | None = None
    try_params: list[Param] | None = None
    try_args: list[str] | None = None
    signature: str | None = None
    delta: int | None = None
    hook_kind: HookKind | None = None
    hook_prologue_size: int | None = None
    callable: bool | None = None


@dataclass(frozen=True)
class XRef:
    function: object
    instr_off: int
    addr_off: int
    indirections: int = 0


@dataclass(frozen=True)
class FunctionXRef:
    ref_function: object
    instr_off: int
    addr_off: int
    indirections: int = 0


@dataclass(frozen=True)
class Function:
    pattern: str
    name: str | None = None
    match_offset: int = 0
    cc: CallingConvention = CallingConvention.CDECL
    callable: bool = True
    public: bool = True
    hook: Hook = dc_field(default_factory=lambda: Hook(5))
    required: Required = Required.ALL
    typed: TypedFunction | None = None
    xrefs: list[FunctionXRef] = dc_field(default_factory=list)
    unstable: bool = False
    doc: str | None = None


@dataclass(frozen=True)
class TypedData:
    type: str
    ref_function: object
    instr_off: int
    addr_off: int
    resolver: Resolver = Resolver.XREF_U32
    indirections: int = 0


@dataclass(frozen=True)
class Data:
    name: str | None = None
    typed: TypedData | None = None
    xrefs: list[XRef] = dc_field(default_factory=list)
    unstable: bool = False
    doc: str | None = None


FunctionRef: TypeAlias = Function | str
HookLike: TypeAlias = Hook | int


def hook(
    patch_size: int,
    kind: HookKind = HookKind.REL32,
    entry_patch_size: int | None = None,
) -> Hook:
    """Describe how much code a generated hook may patch at a resolved game function."""

    return Hook(patch_size, kind, entry_patch_size)


def typed(
    ret: str, params: list[Param] | None = None, **overrides: object
) -> TypedFunction:
    """Describe a callable game function signature, preserving placeholders for unknown rows."""

    return TypedFunction(
        ret,
        UNKNOWN_PARAMS if params is None else params,
        **overrides,
    )


def xref(
    function: FunctionRef, instr_off: int, addr_off: int, indirections: int = 0
) -> XRef:
    """Record where a generated global resolver should read an address from function code."""

    return XRef(function, instr_off, addr_off, indirections)


def fx(
    ref_function: FunctionRef, instr_off: int, addr_off: int, indirections: int = 0
) -> FunctionXRef:
    """Record a function xref used when one generated symbol is discovered through another."""

    return FunctionXRef(ref_function, instr_off, addr_off, indirections)


def member(
    type: str, name: str, offset: int | None = None, *, doc: str | None = None
) -> StructMember:
    """Define one reverse-engineered structure member with any fixed offset."""

    return StructMember(type, name, offset, doc)


def param(type: str, name: str, *, doc: str | None = None) -> Param:
    """Define one typed C parameter for generated callbacks and try-call wrappers."""

    return Param(type, name, doc)


def enum_value(name: str, value: int, *, doc: str | None = None) -> EnumValue:
    """Define one named C enum constant from game values."""

    return EnumValue(name, value, doc)


@dataclass(init=False)
class Blueprint:
    name: str
    unstable: bool = False
    signatures: list[Signature] = dc_field(default_factory=list)
    functions: list[Function] = dc_field(default_factory=list)
    globals: list[Data] = dc_field(default_factory=list)
    types: list[object] = dc_field(default_factory=list)

    def __init__(self, name: str, unstable: bool = False) -> None:
        """Start a symbol blueprint whose rows inherit the stable or unstable surface by default."""

        self.name = name
        self.unstable = unstable
        self.signatures = []
        self.functions = []
        self.globals = []
        self.types = []

    def row_unstable(self, unstable: bool | None) -> bool:
        """Apply per-row stability overrides without repeating the blueprint-wide default."""

        return self.unstable if unstable is None else unstable

    def sig(
        self,
        name: str,
        pattern: str,
        *,
        required: Required = Required.ALL,
        unstable: bool | None = None,
        doc: str | None = None,
    ) -> Signature:
        """Add a raw pattern signature that can later back generated symbol lookup."""

        row = Signature(pattern, name, required, self.row_unstable(unstable), doc)
        self.signatures.append(row)
        return row

    def fn(
        self,
        name: str,
        pattern: str,
        *,
        match: int = 0,
        cc: CallingConvention = CallingConvention.CDECL,
        callable: bool = True,
        public: bool = True,
        hook: HookLike = 5,
        required: Required = Required.ALL,
        ret: str = "uintptr_t",
        params: list[Param] | None = None,
        typed: TypedFunction | None = None,
        xrefs: list[FunctionXRef] | None = None,
        unstable: bool | None = None,
        doc: str | None = None,
        **typed_overrides: object,
    ) -> Function:
        """Add a generated function symbol with its scan pattern, ABI, and hook metadata."""

        hook_row = Hook(hook) if isinstance(hook, int) else hook
        typed_row = typed or TypedFunction(
            ret,
            UNKNOWN_PARAMS if params is None else params,
            **typed_overrides,
        )
        row = Function(
            pattern,
            name,
            match,
            cc,
            callable,
            public,
            hook_row,
            required,
            typed_row,
            xrefs or [],
            self.row_unstable(unstable),
            doc,
        )
        self.functions.append(row)
        return row

    def data(
        self,
        name: str,
        *refs: XRef,
        type: str | None = None,
        ref: XRef | None = None,
        resolver: Resolver = Resolver.XREF_U32,
        indirections: int = 0,
        unstable: bool | None = None,
        doc: str | None = None,
    ) -> Data:
        """Add global symbol metadata and merge refs from reverse-engineered sites."""

        typed = None
        if type is not None:
            ref = ref or (refs[0] if refs else None)
            if ref is None:
                raise ValueError(f"typed global {name} needs a resolver XRef")

            typed = TypedData(
                type,
                ref.function,
                ref.instr_off,
                ref.addr_off,
                resolver,
                indirections,
            )
        row_unstable = self.row_unstable(unstable)
        for i, existing in enumerate(self.globals):
            if existing.name != name:
                continue

            merged_refs = list(existing.xrefs)
            for data_ref in refs:
                if data_ref not in merged_refs:
                    merged_refs.append(data_ref)

            merged_typed = existing.typed
            if merged_typed is None:
                merged_typed = typed
            elif typed is not None and typed != merged_typed:
                raise ValueError(f"conflicting typed global metadata for {name}")

            merged_doc = existing.doc or doc
            if existing.doc and doc and existing.doc != doc:
                raise ValueError(f"conflicting global documentation for {name}")

            row = Data(
                name,
                merged_typed,
                merged_refs,
                existing.unstable or row_unstable,
                merged_doc,
            )
            self.globals[i] = row
            return row

        row = Data(name, typed, list(refs), row_unstable, doc)
        self.globals.append(row)
        return row

    def type_alias(
        self,
        name: str,
        source_type: str,
        *,
        unstable: bool | None = None,
        doc: str | None = None,
    ) -> TypeAlias:
        """Add a generated C alias for game-facing types."""

        row = TypeAlias(source_type, name, self.row_unstable(unstable), doc)
        self.types.append(row)
        return row

    def callback_type(
        self,
        name: str,
        *,
        ret: str,
        params: list[Param],
        calling: CallingConvention = CallingConvention.CALLBACK,
        unstable: bool | None = None,
        doc: str | None = None,
    ) -> FunctionTypeAlias:
        """Add a generated callback typedef used by hook helpers and detours."""

        row = FunctionTypeAlias(
            ret, name, params, calling, self.row_unstable(unstable), doc
        )
        self.types.append(row)
        return row

    def struct(
        self,
        name: str,
        *members: StructMember,
        size: int | None = None,
        incomplete: bool = True,
        unstable: bool | None = None,
        doc: str | None = None,
    ) -> Struct:
        """Add structure layout metadata for generated public type declarations."""

        row = Struct(
            name, list(members), size, incomplete, self.row_unstable(unstable), doc
        )
        self.types.append(row)
        return row

    def enum(
        self,
        name: str,
        *values: EnumValue,
        alias: str | None = None,
        unstable: bool | None = None,
        doc: str | None = None,
    ) -> Enum:
        """Add enum metadata and the public alias exposed in generated headers."""

        row = Enum(name, list(values), alias, self.row_unstable(unstable), doc)
        self.types.append(row)
        return row
