"""Pydantic models for the versioned PCDOGS SDK symbol manifest."""

from __future__ import annotations

from typing import Literal

from pydantic import (
    BaseModel,
    ConfigDict,
    Field,
    StrictBool,
    StrictInt,
    StrictStr,
    model_validator,
)

from blueprint import AbiStatus, CallingConvention, HookKind, Resolver, WritePolicy

SCHEMA_VERSION = 1
SUPPORTED_BUILDS = ("en", "eu", "sc")
MANIFEST_FILENAME = "pcdogs.symbols.v1.json"
SCHEMA_FILENAME = "pcdogs.symbols.schema.json"
SCHEMA_DRAFT_URI = "https://json-schema.org/draft/2020-12/schema"
SCHEMA_ID = "https://dttr.102.dog/sdk/metadata/pcdogs.symbols.schema.v1.json"

Build = Literal["en", "eu", "sc"]

AnalysisKind = Literal["function", "data", "signature"]
SymbolKind = Literal["function", "data", "type", "signature"]

KIND_ORDER: tuple[SymbolKind, ...] = ("function", "data", "type", "signature")


class ManifestModel(BaseModel):
    model_config = ConfigDict(extra="forbid", use_enum_values=True)


class SymbolTarget(ManifestModel):
    kind: SymbolKind
    name: StrictStr


class EmbeddedComputedXRefTo(ManifestModel):
    target: SymbolTarget
    instr_offset: StrictInt
    addr_offset: StrictInt
    indirections: StrictInt = Field(ge=0)
    builds: list[Build]
    access: StrictStr | None = None


class EmbeddedComputedXRefFrom(ManifestModel):
    source: SymbolTarget
    instr_offset: StrictInt
    addr_offset: StrictInt
    indirections: StrictInt = Field(ge=0)
    builds: list[Build]
    access: StrictStr | None = None


class EmbeddedAnalysisXRefTo(ManifestModel):
    target: SymbolTarget
    instr_offset: StrictInt
    addr_offset: StrictInt
    indirections: StrictInt = Field(ge=0)
    access: StrictStr | None = None


class EmbeddedAnalysisXRefFrom(ManifestModel):
    source: SymbolTarget
    instr_offset: StrictInt
    addr_offset: StrictInt
    indirections: StrictInt = Field(ge=0)
    access: StrictStr | None = None


class ManifestParam(ManifestModel):
    name: StrictStr
    type: StrictStr
    docs: StrictStr | None = None


class FunctionPrototype(ManifestModel):
    return_type: StrictStr
    params: list[ManifestParam]
    calling_convention: CallingConvention
    abi_status: AbiStatus


class PatchSizes(ManifestModel):
    patch_size: StrictInt = Field(ge=0)
    entry_patch_size: StrictInt = Field(ge=0)


class FunctionResolver(ManifestModel):
    kind: Literal["aob"] = "aob"
    pattern: StrictStr
    match_offset: StrictInt


class DataResolver(ManifestModel):
    kind: Resolver
    ref_function: StrictStr
    instr_offset: StrictInt
    addr_offset: StrictInt
    indirections: StrictInt = Field(ge=0)


class ComputedFunction(ManifestModel):
    id: StrictInt = Field(ge=0)
    symbol_index: StrictInt = Field(ge=0)
    name: StrictStr
    accessor: StrictStr
    prototype: FunctionPrototype
    builds: list[Build]
    callable: StrictBool
    resolver: FunctionResolver
    hook_kind: HookKind
    patch_sizes: PatchSizes
    docs: StrictStr | None = None
    unstable: StrictBool
    sdk_id: StrictStr
    symbol_id: StrictStr


class ComputedDataSymbol(ManifestModel):
    id: StrictInt = Field(ge=0)
    symbol_index: StrictInt = Field(ge=0)
    name: StrictStr
    accessor: StrictStr
    type: StrictStr | None
    builds: list[Build]
    resolver: DataResolver | None
    write_policy: WritePolicy | Literal["unknown"]
    docs: StrictStr | None = None
    unstable: StrictBool
    sdk_id: StrictStr
    symbol_id: StrictStr


class ComputedType(ManifestModel):
    name: StrictStr
    kind: Literal["type_alias", "function_type_alias", "struct", "enum"]
    sdk_name: StrictStr
    size: StrictInt | None = None
    docs: StrictStr | None = None
    unstable: StrictBool


class ComputedSignature(ManifestModel):
    name: StrictStr
    pattern: StrictStr
    builds: list[Build]
    docs: StrictStr | None = None
    unstable: StrictBool


class ComputedFunctionXRef(ManifestModel):
    function: StrictStr
    ref_function: StrictStr
    instr_offset: StrictInt
    addr_offset: StrictInt
    indirections: StrictInt = Field(ge=0)
    required: StrictStr
    builds: list[Build]


class ComputedDataXRef(ManifestModel):
    data_symbol: StrictStr
    function: StrictStr
    instr_offset: StrictInt
    addr_offset: StrictInt
    indirections: StrictInt = Field(ge=0)
    required: StrictStr
    builds: list[Build]
    access: StrictStr


class ComputedXRefs(ManifestModel):
    functions: list[ComputedFunctionXRef] = Field(default_factory=list)
    data: list[ComputedDataXRef] = Field(default_factory=list)


class ComputedSymbols(ManifestModel):
    functions: list[ComputedFunction] = Field(default_factory=list)
    data: list[ComputedDataSymbol] = Field(default_factory=list)
    types: list[ComputedType] = Field(default_factory=list)
    signatures: list[ComputedSignature] = Field(default_factory=list)
    xrefs: ComputedXRefs = Field(default_factory=ComputedXRefs)


class ResolvedAddress(ManifestModel):
    kind: AnalysisKind
    name: StrictStr
    build: StrictStr
    address: StrictInt = Field(ge=0)
    rva: StrictInt
    image_base: StrictInt = Field(ge=0)
    source_game: StrictStr = ""
    source_game_sha256: StrictStr = ""


class AnalysisFunctionCallXRef(ManifestModel):
    function: StrictStr
    ref_function: StrictStr
    instr_offset: StrictInt
    addr_offset: StrictInt
    indirections: StrictInt = Field(ge=0)
    builds: StrictStr
    source_game: StrictStr = ""
    source_game_sha256: StrictStr = ""


class UnresolvedSymbol(ManifestModel):
    kind: AnalysisKind
    name: StrictStr
    build: StrictStr
    reason: StrictStr
    source_game: StrictStr = ""
    source_game_sha256: StrictStr = ""


class SymbolAnalysis(ManifestModel):
    resolved_addresses: list[ResolvedAddress] = Field(default_factory=list)
    function_call_xrefs: list[AnalysisFunctionCallXRef] = Field(default_factory=list)
    unresolved: list[UnresolvedSymbol] = Field(default_factory=list)


class AnalysisUnresolved(ManifestModel):
    reason: StrictStr


class AnalysisBuildEntry(ManifestModel):
    address: StrictInt | None = Field(default=None, ge=0)
    rva: StrictInt | None = None
    image_base: StrictInt | None = Field(default=None, ge=0)
    unresolved: AnalysisUnresolved | None = None
    xrefs_to: list[EmbeddedAnalysisXRefTo] = Field(default_factory=list)
    xrefs_from: list[EmbeddedAnalysisXRefFrom] = Field(default_factory=list)

    @model_validator(mode="after")
    def validate_state(self) -> AnalysisBuildEntry:
        location_values = (self.address, self.rva, self.image_base)
        has_address = any(value is not None for value in location_values)
        has_xrefs = bool(self.xrefs_to or self.xrefs_from)
        if has_address and any(value is None for value in location_values):
            raise ValueError(
                "analysis build entries must include address, rva, and image_base together"
            )

        if has_address and self.unresolved is not None:
            raise ValueError(
                "analysis build entries cannot be both resolved and unresolved"
            )

        if not has_address and self.unresolved is None and not has_xrefs:
            raise ValueError(
                "analysis build entries must contain a resolution, unresolved reason, or xrefs"
            )

        return self


class SymbolKindAnalysis(ManifestModel):
    en: AnalysisBuildEntry | None = None
    eu: AnalysisBuildEntry | None = None
    sc: AnalysisBuildEntry | None = None

    @model_validator(mode="after")
    def validate_builds(self) -> SymbolKindAnalysis:
        if not any(getattr(self, build) is not None for build in SUPPORTED_BUILDS):
            raise ValueError("analysis must contain at least one build entry")

        return self


class SupportedBuild(ManifestModel):
    source_game: StrictStr = ""
    source_game_sha256: StrictStr = ""


class SymbolKindEntry(ManifestModel):
    model_config = ConfigDict(extra="allow", use_enum_values=True)

    analysis: SymbolKindAnalysis | None = None
    prototype: FunctionPrototype | None = None
    xrefs_to: list[EmbeddedComputedXRefTo] = Field(default_factory=list)
    xrefs_from: list[EmbeddedComputedXRefFrom] = Field(default_factory=list)

    @model_validator(mode="after")
    def validate_no_redundant_builds(self) -> SymbolKindEntry:
        if self.model_extra and "builds" in self.model_extra:
            raise ValueError(
                "symbol kind entries must expose per-build data through analysis"
            )

        return self


class SymbolEntry(ManifestModel):
    model_config = ConfigDict(
        extra="forbid",
        use_enum_values=True,
        json_schema_extra={"minProperties": 1},
    )

    function: SymbolKindEntry | None = None
    data: SymbolKindEntry | None = None
    type: SymbolKindEntry | None = None
    signature: SymbolKindEntry | None = None

    @model_validator(mode="after")
    def validate_has_kind(self) -> SymbolEntry:
        if not any(getattr(self, kind) is not None for kind in KIND_ORDER):
            raise ValueError("symbol entries must contain at least one kind")

        return self


class ManifestCounts(ManifestModel):
    functions: StrictInt = Field(ge=0)
    data_symbols: StrictInt = Field(ge=0)
    types: StrictInt = Field(ge=0)
    signatures: StrictInt = Field(ge=0)
    structs: StrictInt = Field(ge=0)
    function_xrefs: StrictInt = Field(ge=0)
    data_xrefs: StrictInt = Field(ge=0)
    resolved_addresses: StrictInt = Field(ge=0)
    function_call_xrefs: StrictInt = Field(ge=0)
    unresolved: StrictInt = Field(ge=0)


class SymbolManifest(ManifestModel):
    schema_version: Literal[1]
    sdk_version: StrictStr
    sdk_abi_version: StrictInt
    supported_builds: dict[Build, SupportedBuild]
    counts: ManifestCounts
    symbols: dict[StrictStr, SymbolEntry]


def symbol_manifest_schema() -> dict[str, object]:
    schema = SymbolManifest.model_json_schema(
        by_alias=True,
        mode="serialization",
    )
    schema["$schema"] = SCHEMA_DRAFT_URI
    schema["$id"] = SCHEMA_ID
    schema["title"] = "DTTR Symbols Manifest"

    return schema
