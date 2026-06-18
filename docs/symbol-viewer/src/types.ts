import { FILTER_OPTIONS, KIND_ORDER, VERSION_LABELS } from "./constants";

export type SymbolKind = (typeof KIND_ORDER)[number];
export type VersionLabel = (typeof VERSION_LABELS)[number];

export type StabilityFilter = (typeof FILTER_OPTIONS.stability)[number];
export type KindFilter = (typeof FILTER_OPTIONS.kind)[number];
export type VersionFilter = (typeof FILTER_OPTIONS.version)[number];
export type LimitFilter = (typeof FILTER_OPTIONS.limit)[number];

export type AnyRecord = Record<string, unknown>;

export type Filters = {
  query: string;
  stability: StabilityFilter;
  kind: KindFilter;
  version: VersionFilter;
  limit: LimitFilter;
};

export type Param = {
  name?: unknown;
  type?: unknown;
};

export type ManifestRef = {
  kind?: unknown;
  name?: unknown;
};

export type XRefRow = {
  kind?: unknown;
  text?: unknown;
  value?: unknown;
  offsets?: unknown;
  builds?: unknown;
  target_anchor?: string | undefined;
  access?: unknown;
  target?: ManifestRef | undefined;
  source?: ManifestRef | undefined;
  instr_offset?: unknown;
  addr_offset?: unknown;
};

export type MetadataRow = {
  label: string;
  value: unknown;
  build?: string | undefined;
};

export type TypeMember = {
  name?: unknown;
  type?: unknown;
  offset?: unknown;
  size?: unknown;
  doc?: unknown;
};

export type TypeEnumValue = {
  name?: unknown;
  value?: unknown;
  doc?: unknown;
  table_doc?: unknown;
};

export type BuildEntry = AnyRecord & {
  unresolved?: { reason?: unknown } | undefined;
  xrefs_to?: XRefRow[] | undefined;
  xrefs_from?: XRefRow[] | undefined;
};

export type BuildSource = {
  source_game?: unknown;
  source_game_sha256?: unknown;
};

export type ManifestRow = {
  name?: string | undefined;
  unstable?: unknown;
  docs?: unknown;
  sdk?: { accessor?: unknown; name?: unknown } | undefined;
  prototype?:
    | {
        params?: Param[] | undefined;
        return_type?: unknown;
        calling_convention?: unknown;
      }
    | undefined;
  hook?:
    | {
        kind?: unknown;
        patch_size?: unknown;
        entry_patch_size?: unknown;
      }
    | undefined;
  resolver?:
    | {
        pattern?: unknown;
        match_offset?: unknown;
        ref_function?: unknown;
        instr_offset?: unknown;
        addr_offset?: unknown;
      }
    | undefined;
  xrefs_to?: XRefRow[] | undefined;
  xrefs_from?: XRefRow[] | undefined;
  type?: unknown;
  write_policy?: unknown;
  kind?: unknown;
  sdk_name?: unknown;
  members?: TypeMember[] | undefined;
  values?: TypeEnumValue[] | undefined;
  source_type?: unknown;
  calling_convention?: unknown;
  ret?: unknown;
  params?: Param[] | undefined;
  size?: unknown;
  alias?: unknown;
  pattern?: unknown;
  analysis?: Record<string, BuildEntry> | undefined;
};

export type Manifest = {
  supported_builds?: Record<string, BuildSource> | undefined;
  symbols?: Record<string, Partial<Record<SymbolKind, ManifestRow>>> | undefined;
};

export type ViewerSymbol = {
  kind: SymbolKind;
  name: string;
  accessor?: unknown;
  category: string;
  category_display: string;
  anchor: string;
  builds: string;
  summary: string;
  metadata: MetadataRow[];
  references: XRefRow[];
  referenced_by: XRefRow[];
  facts: MetadataRow[];
  reference_hierarchy_paths: string[][];
  pseudo_usage?: string | undefined;
  pseudo_usage_html?: string | undefined;
  call_example?: string | undefined;
  call_example_html?: string | undefined;
  patch_spec_example?: string | undefined;
  patch_spec_example_html?: string | undefined;
  hook_example?: string | undefined;
  hook_example_html?: string | undefined;
  type?: unknown;
  write_policy?: unknown;
  can_write?: boolean | undefined;
  read_example?: string | undefined;
  read_example_html?: string | undefined;
  write_example?: string | undefined;
  write_example_html?: string | undefined;
  write_policy_note?: string | undefined;
  untyped_note?: string | undefined;
  kind_label?: string | undefined;
  sdk_name?: unknown;
  members?: TypeMember[] | undefined;
  enum_values?: TypeEnumValue[] | undefined;
};

export type ExampleTab = {
  label: string;
  value?: string | undefined;
  htmlValue?: string | undefined;
};

export type HierarchyNode = {
  children: Map<string, HierarchyNode>;
  terminal?: boolean | undefined;
};

export type AnchorMaps = Record<SymbolKind, Map<string, string>>;

export type ViewerStatus = "loading" | "ready" | "error";
