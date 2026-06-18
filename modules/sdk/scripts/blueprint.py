"""Small Python DSL for the PCDOGS SDK blueprint."""

from __future__ import annotations

from enum import StrEnum
from typing import Any, TypeAlias

from pydantic import (
    BaseModel,
    ConfigDict,
    Field,
    StrictBool,
    StrictInt,
    StrictStr,
    field_serializer,
    field_validator,
)
from pydantic.aliases import AliasChoices

__all__ = [
    "AbiStatus",
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
    "WritePolicy",
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
    EU = "eu"
    SC = "sc"
    EU_SC = "eu_sc"
    EN_EU = "en_eu"
    EN_SC = "en_sc"


class HookKind(StrEnum):
    REL32 = "rel32"
    HOTPATCH = "hotpatch"
    UNSUPPORTED = "unsupported"


class AbiStatus(StrEnum):
    VERIFIED = "verified"
    PLACEHOLDER = "placeholder"


class WritePolicy(StrEnum):
    READ_ONLY = "read_only"
    ENGINE_MANAGED = "engine_managed"
    RAW_MEMORY = "raw_memory"
    PATCH_ONLY = "patch_only"


class BlueprintModel(BaseModel):
    model_config = ConfigDict(extra="forbid", arbitrary_types_allowed=True)


class FrozenBlueprintModel(BlueprintModel):
    model_config = ConfigDict(frozen=True)


class Param(FrozenBlueprintModel):
    type: StrictStr
    name: StrictStr
    doc: StrictStr | None = None


UNKNOWN_PARAMS: list[Param] = [Param(type="uintptr_t", name=f"a{i}") for i in range(8)]


class Signature(FrozenBlueprintModel):
    pattern: StrictStr
    name: StrictStr | None = None
    required: Required = Required.ALL
    unstable: StrictBool = False
    doc: StrictStr | None = None


class TypeAlias(FrozenBlueprintModel):
    source_type: StrictStr
    name: StrictStr
    unstable: StrictBool = False
    doc: StrictStr | None = None


class FunctionTypeAlias(FrozenBlueprintModel):
    ret: StrictStr
    name: StrictStr
    params: list[Param]
    calling: CallingConvention = CallingConvention.CALLBACK
    unstable: StrictBool = False
    doc: StrictStr | None = None


class StructMember(FrozenBlueprintModel):
    type: StrictStr
    name: StrictStr
    offset: StrictInt | None = None
    doc: StrictStr | None = None


class Struct(FrozenBlueprintModel):
    name: StrictStr
    members: list[StructMember]
    size: StrictInt | None = None
    incomplete: StrictBool = True
    unstable: StrictBool = False
    doc: StrictStr | None = None


class EnumValue(FrozenBlueprintModel):
    name: StrictStr
    value: StrictInt
    doc: StrictStr | None = None


class Enum(FrozenBlueprintModel):
    name: StrictStr
    values: list[EnumValue]
    alias: StrictStr | None = None
    unstable: StrictBool = False
    doc: StrictStr | None = None


class Hook(FrozenBlueprintModel):
    patch_size: StrictInt
    kind: HookKind = HookKind.REL32
    entry_patch_size: StrictInt | None = None


class TypedFunction(FrozenBlueprintModel):
    ret: StrictStr
    params: list[Param]
    abi: CallingConvention | None = None
    args: list[StrictStr] | None = None
    try_params: list[Param] | None = None
    try_args: list[StrictStr] | None = None
    signature: StrictStr | None = None
    delta: StrictInt | None = None
    hook_kind: HookKind | None = None
    hook_prologue_size: StrictInt | None = None
    callable: StrictBool | None = None


FunctionRef: TypeAlias = "Function | str"
HookLike: TypeAlias = Hook | int


def _ref_name(ref: object) -> str:
    if isinstance(ref, str):
        return ref

    name = getattr(ref, "name", None)
    if isinstance(name, str) and name:
        return name

    raise ValueError(f"symbol reference must be a name or named Function: {ref!r}")


def _merged_optional(current: object, update: object, label: str, name: str) -> object:
    if current and update and current != update:
        raise ValueError(f"conflicting {label} for {name}")

    return current or update


class XRef(FrozenBlueprintModel):
    function: Any
    instr_off: StrictInt
    addr_off: StrictInt
    indirections: StrictInt = 0
    required: Required = Required.ALL
    access: StrictStr = "R/W"

    @field_validator("access")
    @classmethod
    def validate_access(cls, value: str) -> str:
        access_label = XREF_ACCESS_LABELS.get(value.strip().lower())
        if access_label is None:
            raise ValueError("xref access must be Read, Write, or R/W")

        return access_label

    @field_serializer("function")
    def serialize_function(self, value: object) -> str:
        return _ref_name(value)


XREF_ACCESS_LABELS = {
    "read": "Read",
    "write": "Write",
    "r/w": "R/W",
    "rw": "R/W",
}


class FunctionXRef(FrozenBlueprintModel):
    ref_function: Any
    instr_off: StrictInt
    addr_off: StrictInt
    indirections: StrictInt = 0
    required: Required = Required.ALL

    @field_serializer("ref_function")
    def serialize_ref_function(self, value: object) -> str:
        return _ref_name(value)


class Function(FrozenBlueprintModel):
    pattern: StrictStr
    name: StrictStr | None = None
    match_offset: StrictInt = 0
    cc: CallingConvention = CallingConvention.CDECL
    callable: StrictBool = True
    public: StrictBool = True
    hook: Hook = Field(default_factory=lambda: Hook(patch_size=5))
    required: Required = Required.ALL
    typed: TypedFunction | None = None
    xrefs: list[FunctionXRef] = Field(default_factory=list)
    unstable: StrictBool = False
    doc: StrictStr | None = None
    abi_status: AbiStatus = AbiStatus.VERIFIED
    stable_reason: StrictStr | None = None


class TypedData(FrozenBlueprintModel):
    type: StrictStr
    ref_function: Any
    instr_off: StrictInt
    addr_off: StrictInt
    resolver: Resolver = Resolver.XREF_U32
    indirections: StrictInt = 0
    required: Required = Required.ALL

    @field_serializer("ref_function")
    def serialize_ref_function(self, value: object) -> str:
        return _ref_name(value)


class Data(FrozenBlueprintModel):
    name: StrictStr | None = None
    typed: TypedData | None = None
    xrefs: list[XRef] = Field(default_factory=list)
    unstable: StrictBool = False
    doc: StrictStr | None = None
    stable_reason: StrictStr | None = None
    write_policy: WritePolicy | None = None


def hook(
    patch_size: int,
    kind: HookKind = HookKind.REL32,
    entry_patch_size: int | None = None,
) -> Hook:
    return Hook(patch_size=patch_size, kind=kind, entry_patch_size=entry_patch_size)


def typed(
    ret: str, params: list[Param] | None = None, **overrides: object
) -> TypedFunction:
    return TypedFunction(
        ret=ret,
        params=UNKNOWN_PARAMS if params is None else params,
        **overrides,
    )


def xref(
    function: FunctionRef,
    instr_off: int,
    addr_off: int,
    indirections: int = 0,
    *,
    required: Required = Required.ALL,
    access: str = "R/W",
) -> XRef:
    return XRef(
        function=function,
        instr_off=instr_off,
        addr_off=addr_off,
        indirections=indirections,
        required=required,
        access=access,
    )


def fx(
    ref_function: FunctionRef,
    instr_off: int,
    addr_off: int,
    indirections: int = 0,
    *,
    required: Required = Required.ALL,
) -> FunctionXRef:
    return FunctionXRef(
        ref_function=ref_function,
        instr_off=instr_off,
        addr_off=addr_off,
        indirections=indirections,
        required=required,
    )


def member(
    type: str, name: str, offset: int | None = None, *, doc: str | None = None
) -> StructMember:
    return StructMember(type=type, name=name, offset=offset, doc=doc)


def param(type: str, name: str, *, doc: str | None = None) -> Param:
    return Param(type=type, name=name, doc=doc)


def enum_value(name: str, value: int, *, doc: str | None = None) -> EnumValue:
    return EnumValue(name=name, value=value, doc=doc)


class Blueprint(BlueprintModel):
    name: StrictStr
    stable: StrictBool = False
    signatures: list[Signature] = Field(default_factory=list)
    functions: list[Function] = Field(default_factory=list)
    globals: list[Data] = Field(
        default_factory=list,
        validation_alias=AliasChoices("globals", "data"),
        serialization_alias="data",
    )
    types: list[TypeAlias | FunctionTypeAlias | Struct | Enum] = Field(
        default_factory=list
    )

    def __init__(self, name: str, stable: bool = False, **data: object) -> None:
        super().__init__(name=name, stable=stable, **data)

    def row_unstable(self, unstable: bool | None, stable: bool) -> bool:
        if stable and unstable is not None:
            raise ValueError("use stable=True or unstable=..., not both")

        if stable:
            return False

        return not self.stable if unstable is None else unstable

    def sig(
        self,
        name: str,
        pattern: str,
        *,
        required: Required = Required.ALL,
        stable: bool = False,
        unstable: bool | None = None,
        doc: str | None = None,
    ) -> Signature:
        row = Signature(
            pattern=pattern,
            name=name,
            required=required,
            unstable=self.row_unstable(unstable, stable),
            doc=doc,
        )
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
        stable: bool = False,
        unstable: bool | None = None,
        abi_status: AbiStatus = AbiStatus.VERIFIED,
        stable_reason: str | None = None,
        doc: str | None = None,
        **typed_overrides: object,
    ) -> Function:
        hook_row = Hook(patch_size=hook) if isinstance(hook, int) else hook
        typed_row = typed or TypedFunction(
            ret=ret,
            params=UNKNOWN_PARAMS if params is None else params,
            **typed_overrides,
        )

        row = Function(
            pattern=pattern,
            name=name,
            match_offset=match,
            cc=cc,
            callable=callable,
            public=public,
            hook=hook_row,
            required=required,
            typed=typed_row,
            xrefs=xrefs or [],
            unstable=self.row_unstable(unstable, stable),
            doc=doc,
            abi_status=abi_status,
            stable_reason=stable_reason,
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
        stable: bool = False,
        unstable: bool | None = None,
        stable_reason: str | None = None,
        doc: str | None = None,
        write_policy: WritePolicy | None = None,
    ) -> Data:
        typed = None

        if type is not None:
            ref = ref or (refs[0] if refs else None)
            if ref is None:
                raise ValueError(f"typed global {name} needs a resolver XRef")

            if ref.required != Required.ALL:
                raise ValueError(
                    f"typed global {name} canonical resolver must use Required.ALL; "
                    "add build-specific resolver candidates as positional xrefs"
                )

            typed = TypedData(
                type=type,
                ref_function=ref.function,
                instr_off=ref.instr_off,
                addr_off=ref.addr_off,
                resolver=resolver,
                indirections=indirections,
                required=ref.required,
            )

        row_unstable = self.row_unstable(unstable, stable)

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

            if typed is not None and typed != merged_typed:
                raise ValueError(f"conflicting typed global metadata for {name}")

            merged_doc = _merged_optional(
                existing.doc, doc, "global documentation", name
            )

            merged_stable_reason = _merged_optional(
                existing.stable_reason, stable_reason, "stable reason", name
            )

            merged_write_policy = _merged_optional(
                existing.write_policy, write_policy, "write policy", name
            )

            row = Data(
                name=name,
                typed=merged_typed,
                xrefs=merged_refs,
                unstable=existing.unstable,
                doc=merged_doc,
                stable_reason=merged_stable_reason,
                write_policy=merged_write_policy,
            )
            self.globals[i] = row
            return row

        row = Data(
            name=name,
            typed=typed,
            xrefs=list(refs),
            unstable=row_unstable,
            doc=doc,
            stable_reason=stable_reason,
            write_policy=write_policy,
        )
        self.globals.append(row)
        return row

    def type_alias(
        self,
        name: str,
        source_type: str,
        *,
        stable: bool = False,
        unstable: bool | None = None,
        doc: str | None = None,
    ) -> TypeAlias:
        row = TypeAlias(
            source_type=source_type,
            name=name,
            unstable=self.row_unstable(unstable, stable),
            doc=doc,
        )
        self.types.append(row)
        return row

    def callback_type(
        self,
        name: str,
        *,
        ret: str,
        params: list[Param],
        calling: CallingConvention = CallingConvention.CALLBACK,
        stable: bool = False,
        unstable: bool | None = None,
        doc: str | None = None,
    ) -> FunctionTypeAlias:
        row = FunctionTypeAlias(
            ret=ret,
            name=name,
            params=params,
            calling=calling,
            unstable=self.row_unstable(unstable, stable),
            doc=doc,
        )
        self.types.append(row)
        return row

    def struct(
        self,
        name: str,
        *members: StructMember,
        size: int | None = None,
        incomplete: bool = True,
        stable: bool = False,
        unstable: bool | None = None,
        doc: str | None = None,
    ) -> Struct:
        row = Struct(
            name=name,
            members=list(members),
            size=size,
            incomplete=incomplete,
            unstable=self.row_unstable(unstable, stable),
            doc=doc,
        )
        self.types.append(row)
        return row

    def enum(
        self,
        name: str,
        *values: EnumValue,
        alias: str | None = None,
        stable: bool = False,
        unstable: bool | None = None,
        doc: str | None = None,
    ) -> Enum:
        row = Enum(
            name=name,
            values=list(values),
            alias=alias,
            unstable=self.row_unstable(unstable, stable),
            doc=doc,
        )
        self.types.append(row)
        return row
