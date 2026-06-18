import {
  DEFAULT_BUILDS,
  KIND_LABELS,
  KIND_ORDER,
} from "./constants";
import {
  dataReadExample,
  dataWriteExample,
  functionCallExample,
} from "./symbol-examples";
import type {
  AnchorMaps,
  BuildEntry,
  BuildSource,
  Manifest,
  ManifestRef,
  ManifestRow,
  MetadataRow,
  SymbolKind,
  ViewerSymbol,
  XRefRow,
} from "./types";
import {
  analysisBuildKeys,
  buildsLabel,
  categoryDisplay,
  hasOwn,
  hexNumber,
  isRecord,
  isSymbolKind,
  manifestAnchor,
  manifestSummary,
  patchSizes,
  signedHex,
  text,
  typedParams,
  upperBuild,
  xrefOffsets,
} from "./utils";

const BUILD_LOCATION_FIELDS = [
  ["Address", "address"],
  ["RVA", "rva"],
  ["Image Base", "image_base"],
] as const;

export function supportedBuildSources(manifest: Manifest): Record<string, BuildSource> {
  const supportedBuilds = manifest?.supported_builds;
  if (!isRecord(supportedBuilds)) {
    return {};
  }

  return supportedBuilds;
}

export function emptySymbol(kind: SymbolKind, row: ManifestRow): ViewerSymbol {
  const category = text(row?.name).split("_", 1)[0] || "misc";

  return {
    kind,
    name: text(row?.name),
    accessor: row?.sdk?.accessor,
    category,
    category_display: categoryDisplay(category),
    anchor: manifestAnchor(kind, row),
    builds: buildsLabel(analysisBuildKeys(row) || DEFAULT_BUILDS),
    summary: manifestSummary(row),
    metadata: [],
    references: [],
    referenced_by: [],
    facts: [{ label: "Kind", value: KIND_LABELS[kind] }],
    reference_hierarchy_paths: [],
  };
}

export function functionSymbol(row: ManifestRow): ViewerSymbol {
  const symbol = emptySymbol("function", row);
  const prototype = row?.prototype || {};
  const params = prototype.params || [];
  const returnType = prototype.return_type;
  const callingConvention = prototype.calling_convention;
  const signature = `${text(returnType)} ${symbol.name}(${typedParams(params)})`;

  symbol.pseudo_usage = signature;
  symbol.call_example = functionCallExample(symbol.accessor);
  symbol.patch_spec_example = `// Patch via ${symbol.name}'s manifest resolver.`;
  symbol.hook_example = `// Hook ${symbol.name} when allowed by hook kind.`;
  symbol.facts.push(
    { label: "Return", value: returnType || "-" },
    { label: "Calling", value: callingConvention || "-" },
  );
  const sizes = patchSizes(row);
  const metadata: MetadataRow[] = [
    { label: "Hook Type", value: row?.hook?.kind || "-" },
    {
      label: "Patch Size",
      value: sizes ? hexNumber(sizes.patch_size) : "-",
    },
    {
      label: "Entry Patch Size",
      value: sizes ? hexNumber(sizes.entry_patch_size) : "-",
    },
    { label: "AOB Pattern", value: row?.resolver?.pattern || "-" },
    {
      label: "Match Offset",
      value: row?.resolver ? signedHex(row.resolver.match_offset) : "-",
    },
  ];
  symbol.metadata = metadata.filter((item) => item.value !== "-");

  return symbol;
}

export function dataSymbol(row: ManifestRow): ViewerSymbol {
  const symbol = emptySymbol("data", row);
  const typeName = row?.type || "-";
  const writePolicy = text(row?.write_policy || "unknown");
  const hasTypedAccessor = typeName !== "-";
  const writePolicyMessage = hasTypedAccessor
    ? dataWriteExample(symbol.accessor, typeName, writePolicy)
    : "";

  symbol.type = typeName;
  symbol.write_policy = writePolicy;
  symbol.can_write = hasTypedAccessor && writePolicy === "raw_memory";
  symbol.read_example = hasTypedAccessor
    ? dataReadExample(symbol.accessor, typeName)
    : "// No typed SDK accessor yet.";
  symbol.write_example = symbol.can_write ? writePolicyMessage : "";
  symbol.write_policy_note = !symbol.can_write ? writePolicyMessage : "";
  symbol.untyped_note = !hasTypedAccessor
    ? "This data symbol has no typed SDK accessor yet."
    : "";
  symbol.facts.push(
    { label: "Type", value: typeName },
    { label: "Write Policy", value: symbol.write_policy },
  );
  symbol.metadata = row?.resolver
    ? [
        {
          label: "Resolver",
          value: `${text(row.resolver.ref_function)} ${xrefOffsets(row.resolver)}`,
        },
      ]
    : [];

  return symbol;
}

export function typeKind(row: ManifestRow): string {
  return text(row?.kind || "type");
}

export function typeSymbol(row: ManifestRow): ViewerSymbol {
  const symbol = emptySymbol("type", row);
  const kind = typeKind(row);
  const callingConvention = row.calling_convention || "CALLBACK";

  symbol.kind_label = categoryDisplay(kind);
  symbol.sdk_name = row?.sdk_name || row?.sdk?.name || row?.name || "-";
  symbol.members = (row.members || []).map((member) => ({
    name: member.name,
    type: member.type,
    offset: member.offset == null ? "-" : hexNumber(member.offset),
    size: "-",
    doc: member.doc || "",
  }));
  symbol.enum_values = (row.values || []).map((value) => ({
    name: value.name,
    value: value.value,
    doc: value.doc || "",
    table_doc: value.table_doc,
  }));
  symbol.facts = [
    { label: "Kind", value: symbol.kind_label },
    { label: "SDK Name", value: symbol.sdk_name },
  ];
  if (row.source_type) {
    symbol.metadata.push({ label: "Source Type", value: row.source_type });
  }

  if (row.ret) {
    symbol.metadata.push({
      label: "Signature",
      value: `${text(row.ret)} (${callingConvention})(${typedParams(row.params)})`,
    });
  }

  if (row.size != null) {
    symbol.metadata.push({ label: "Size", value: hexNumber(row.size) });
  }

  if (row.alias) {
    symbol.metadata.push({ label: "Alias", value: row.alias });
  }

  return symbol;
}

export function signatureSymbol(row: ManifestRow): ViewerSymbol {
  const symbol = emptySymbol("signature", row);
  symbol.metadata = [{ label: "AOB Pattern", value: row.pattern }];

  return symbol;
}

export function addXRef(
  symbolsByAnchor: Map<string, ViewerSymbol>,
  seenXRefs: Set<string>,
  fromAnchor: string | undefined,
  toAnchor: string | undefined,
  row: XRefRow,
): void {
  if (!fromAnchor || !toAnchor) {
    return;
  }

  const source = symbolsByAnchor.get(fromAnchor);
  const target = symbolsByAnchor.get(toAnchor);

  if (!source || !target) {
    return;
  }

  const offsets = xrefOffsets(row);
  const builds = buildsLabel(row.builds || DEFAULT_BUILDS);
  const key = [
    source.anchor,
    target.anchor,
    offsets,
    builds,
    row.access || "",
  ].join("\0");

  if (seenXRefs.has(key)) {
    return;
  }

  seenXRefs.add(key);

  source.references.push({
    kind: KIND_LABELS[target.kind] || target.kind,
    text: target.name,
    offsets,
    builds,
    target_anchor: target.anchor,
  });
  target.referenced_by.push({
    kind: KIND_LABELS[source.kind] || source.kind,
    text: source.name,
    offsets,
    builds,
    target_anchor: source.anchor,
  });
}

export function analysisBuildEntries(entry: ManifestRow): [string, BuildEntry][] {
  const analysis = entry?.analysis;
  if (!isRecord(analysis)) {
    return [];
  }

  return Object.entries(analysis).filter(([build]) =>
    (DEFAULT_BUILDS as readonly string[]).includes(build),
  );
}

export function addBuildMetadata(
  symbol: ViewerSymbol,
  build: string,
  buildEntry: BuildEntry,
  buildSources: Record<string, BuildSource> = {},
): void {
  const labelBuild = upperBuild(build);
  const unresolved = buildEntry?.unresolved;
  const buildSource = buildSources[build] || {};

  const locationRows: MetadataRow[] = [];
  for (const [label, key] of BUILD_LOCATION_FIELDS) {
    if (!hasOwn(buildEntry, key)) {
      continue;
    }

    locationRows.push({
      label,
      build: labelBuild,
      value: buildEntry[key],
    });
  }

  symbol.metadata.unshift(...locationRows);

  if (unresolved) {
    symbol.metadata.push({
      label: "Unresolved",
      build: labelBuild,
      value: unresolved.reason || "unresolved",
    });
  }

  for (const [label, value] of [
    ["Source Game", buildSource.source_game],
    ["Source SHA-256", buildSource.source_game_sha256],
  ] as const) {
    if (!value) {
      continue;
    }

    symbol.metadata.push({ label, build: labelBuild, value });
  }
}

const SYMBOL_BUILDERS: Record<SymbolKind, (row: ManifestRow) => ViewerSymbol> = {
  function: functionSymbol,
  data: dataSymbol,
  type: typeSymbol,
  signature: signatureSymbol,
};

export function anchorForRef(anchorByKindAndName: AnchorMaps, ref: ManifestRef | undefined): string | undefined {
  if (!ref || !isSymbolKind(ref.kind) || typeof ref.name !== "string") {
    return undefined;
  }

  return anchorByKindAndName[ref.kind].get(ref.name);
}

export function manifestSymbols(manifest: Manifest): ViewerSymbol[] {
  if (
    !manifest?.symbols ||
    typeof manifest.symbols !== "object" ||
    Array.isArray(manifest.symbols)
  ) {
    throw new Error("Unsupported PCDOGS symbol manifest: missing symbols map");
  }

  const symbols: ViewerSymbol[] = [];
  const buildSources = supportedBuildSources(manifest);

  for (const [name, symbolEntry] of Object.entries(manifest.symbols)) {
    for (const kind of KIND_ORDER) {
      const entry = symbolEntry?.[kind];

      if (!entry) {
        continue;
      }

      const row = { name, ...entry };
      symbols.push(SYMBOL_BUILDERS[kind](row));
    }
  }

  const symbolsByAnchor = new Map(symbols.map((symbol) => [symbol.anchor, symbol]));
  const seenXRefs = new Set<string>();
  const anchorByKindAndName: AnchorMaps = {
    function: new Map(),
    data: new Map(),
    type: new Map(),
    signature: new Map(),
  };
  const addRows = (
    rows: XRefRow[] | undefined,
    fromAnchor: (row: XRefRow) => string | undefined,
    toAnchor: (row: XRefRow) => string | undefined,
    build?: string,
  ): void => {
    if (!rows?.length) {
      return;
    }

    for (const row of rows) {
      addXRef(
        symbolsByAnchor,
        seenXRefs,
        fromAnchor(row),
        toAnchor(row),
        build ? { ...row, builds: build } : row,
      );
    }
  };

  for (const symbol of symbols) {
    anchorByKindAndName[symbol.kind].set(symbol.name, symbol.anchor);
  }

  for (const [name, symbolEntry] of Object.entries(manifest.symbols)) {
    for (const kind of KIND_ORDER) {
      const entry = symbolEntry?.[kind];
      const sourceAnchor = anchorByKindAndName[kind].get(name);

      if (!entry || !sourceAnchor) {
        continue;
      }

      const buildEntries = analysisBuildEntries(entry);
      const hasAnalysisXRefs = buildEntries.some(([, buildEntry]) =>
        Boolean(
          buildEntry?.xrefs_to?.length || buildEntry?.xrefs_from?.length,
        ),
      );

      if (!hasAnalysisXRefs) {
        addRows(
          entry.xrefs_to,
          () => sourceAnchor,
          (row) => anchorForRef(anchorByKindAndName, row.target),
        );
        addRows(
          entry.xrefs_from,
          (row) => anchorForRef(anchorByKindAndName, row.source),
          () => sourceAnchor,
        );
      }

      const symbol = symbolsByAnchor.get(sourceAnchor);

      if (!symbol) {
        continue;
      }

      for (const [buildKey, buildEntry] of buildEntries) {
        addBuildMetadata(symbol, buildKey, buildEntry, buildSources);
        addRows(
          buildEntry?.xrefs_to,
          () => sourceAnchor,
          (row) => anchorForRef(anchorByKindAndName, row.target),
          buildKey,
        );
        addRows(
          buildEntry?.xrefs_from,
          (row) => anchorForRef(anchorByKindAndName, row.source),
          () => sourceAnchor,
          buildKey,
        );
      }
    }
  }

  return symbols;
}
