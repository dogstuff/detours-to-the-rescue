"""Small records used by the PCDOGS symbol docs generator."""

from __future__ import annotations

from dataclasses import dataclass, field


@dataclass(slots=True)
class MetadataItem:
    label: str
    value: str


@dataclass(slots=True)
class SymbolFact:
    label: str
    value: str


@dataclass(slots=True)
class RelatedEntry:
    kind: str
    value: str


@dataclass(slots=True)
class XRefItem:
    kind: str
    value: str
    offsets: str
    detail: str


@dataclass(slots=True)
class MemberCard:
    name: str
    type: str
    offset: str
    doc: str
    type_link: str = ""


@dataclass(slots=True)
class EnumValueCard:
    name: str
    value: str
    doc: str
    table_doc: str


@dataclass(slots=True)
class FunctionCard:
    name: str
    heading: str
    category: str
    anchor: str
    prototype: str
    call_example: str
    patch_spec_example: str
    hook_example: str
    builds: str
    summary: str
    metadata: list[MetadataItem]
    related_type_texts: list[str]
    references: list[XRefItem] = field(default_factory=list)
    referenced_by: list[XRefItem] = field(default_factory=list)
    xref_count: int = 0
    related: list[RelatedEntry] = field(default_factory=list)
    facts: list[SymbolFact] = field(default_factory=list)


@dataclass(slots=True)
class GlobalCard:
    name: str
    category: str
    anchor: str
    type: str
    builds: str
    write_policy: str
    summary: str
    read_example: str
    write_example: str
    is_typed: bool
    untyped_note: str
    metadata: list[MetadataItem]
    related_type_texts: list[str]
    references: list[XRefItem] = field(default_factory=list)
    referenced_by: list[XRefItem] = field(default_factory=list)
    xref_count: int = 0
    related: list[RelatedEntry] = field(default_factory=list)
    facts: list[SymbolFact] = field(default_factory=list)


@dataclass(slots=True)
class TypeCard:
    name: str
    category: str
    anchor: str
    kind_label: str
    c_name: str
    shape: str
    builds: str
    summary: str
    members: list[MemberCard]
    enum_values: list[EnumValueCard]
    metadata: list[MetadataItem]
    related_type_texts: list[str]
    related: list[RelatedEntry] = field(default_factory=list)
    facts: list[SymbolFact] = field(default_factory=list)


@dataclass(slots=True)
class SignatureCard:
    name: str
    category: str
    anchor: str
    builds: str
    summary: str
    metadata: list[MetadataItem]
    related: list[RelatedEntry] = field(default_factory=list)
    facts: list[SymbolFact] = field(default_factory=list)


@dataclass(slots=True)
class FunctionXRefCard:
    category: str
    function: str
    ref_function: str
    offsets: str
    indirections: int


@dataclass(slots=True)
class DataXRefCard:
    category: str
    global_name: str
    function: str
    offsets: str


@dataclass(slots=True)
class Category:
    display: str
    slug: str
    filename: str
    functions: list[FunctionCard] = field(default_factory=list)
    resolver_functions: list[FunctionCard] = field(default_factory=list)
    globals: list[GlobalCard] = field(default_factory=list)
    types: list[TypeCard] = field(default_factory=list)
    signatures: list[SignatureCard] = field(default_factory=list)
    function_xrefs: list[FunctionXRefCard] = field(default_factory=list)
    data_xrefs: list[DataXRefCard] = field(default_factory=list)


@dataclass(slots=True)
class SurfaceCards:
    functions: list[FunctionCard] = field(default_factory=list)
    resolver_functions: list[FunctionCard] = field(default_factory=list)
    globals: list[GlobalCard] = field(default_factory=list)
    types: list[TypeCard] = field(default_factory=list)
    signatures: list[SignatureCard] = field(default_factory=list)
    function_xrefs: list[FunctionXRefCard] = field(default_factory=list)
    data_xrefs: list[DataXRefCard] = field(default_factory=list)
