import { KIND_LABELS } from "./constants";
import type { Manifest, ViewerSymbol } from "./types";
import { text } from "./utils";

export function typeTextParts(value: unknown): string[] {
  return text(value).split(/([A-Za-z_][A-Za-z0-9_]*)/g);
}

export function typeIndex(symbols: ViewerSymbol[]): Map<string, ViewerSymbol> {
  const index = new Map<string, ViewerSymbol>();

  for (const symbol of symbols) {
    if (symbol.kind !== "type") {
      continue;
    }

    index.set(symbol.name, symbol);
    if (typeof symbol.sdk_name === "string" && symbol.sdk_name) {
      index.set(symbol.sdk_name, symbol);
    }
  }

  return index;
}

function typeUsageValues(symbol: ViewerSymbol, manifest: Manifest): unknown[] {
  const row = manifest.symbols?.[symbol.name]?.[symbol.kind];

  if (symbol.kind === "function") {
    return [
      row?.prototype?.return_type,
      ...(row?.prototype?.params || []).map((param) => param.type),
    ];
  }

  if (symbol.kind === "data") {
    return [row?.type];
  }

  if (symbol.kind === "type") {
    return [
      row?.source_type,
      row?.ret,
      ...(row?.params || []).map((param) => param.type),
      ...(row?.members || []).map((member) => member.type),
    ];
  }

  return [];
}

function referencedTypes(
  symbol: ViewerSymbol,
  index: Map<string, ViewerSymbol>,
  manifest: Manifest,
): ViewerSymbol[] {
  const found = new Set<ViewerSymbol>();

  for (const value of typeUsageValues(symbol, manifest)) {
    for (const name of typeTextParts(value)) {
      const target = index.get(name);
      if (target && target !== symbol) {
        found.add(target);
      }
    }
  }

  return [...found].sort((left, right) => left.name.localeCompare(right.name));
}

export function attachTypeUsageReferences(
  symbols: ViewerSymbol[],
  manifest: Manifest,
): void {
  const index = typeIndex(symbols);

  for (const source of symbols) {
    for (const target of referencedTypes(source, index, manifest)) {
      source.references.push({
        kind: KIND_LABELS.type,
        text: target.name,
        offsets: "-",
        builds: source.builds,
        target_anchor: target.anchor,
      });
      target.referenced_by.push({
        kind: KIND_LABELS[source.kind],
        text: source.name,
        offsets: "-",
        builds: source.builds,
        target_anchor: source.anchor,
      });
    }
  }
}
