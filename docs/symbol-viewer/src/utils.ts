import {
  CATEGORY_DISPLAY_OVERRIDES,
  DEFAULT_BUILDS,
  KIND_LABELS,
  KIND_ORDER,
  VERSION_LABELS,
} from "./constants";
import type {
  AnyRecord,
  ManifestRow,
  Param,
  StabilityFilter,
  SymbolKind,
  VersionFilter,
  ViewerSymbol,
} from "./types";

export function classes(...items: unknown[]): string {
  return items.filter(Boolean).join(" ");
}

export function text(value: unknown): string {
  return value == null || value === "" ? "-" : String(value);
}

export function isRecord(value: unknown): value is AnyRecord {
  return Boolean(value) && typeof value === "object" && !Array.isArray(value);
}

export function hasOwn(value: unknown, key: string): value is AnyRecord {
  return isRecord(value) && Object.prototype.hasOwnProperty.call(value, key);
}

export function isOneOf<const T extends readonly string[]>(
  options: T,
  value: unknown,
): value is T[number] {
  return typeof value === "string" && (options as readonly string[]).includes(value);
}

export function isSymbolKind(value: unknown): value is SymbolKind {
  return isOneOf(KIND_ORDER, value);
}

export function slug(value: unknown): string {
  return (
    text(value)
      .toLowerCase()
      .replace(/[^a-z0-9]+/g, "-")
      .replace(/^-|-$/g, "") || "reference"
  );
}

export function isUnstable(symbol: Pick<ViewerSymbol, "anchor">): boolean {
  return symbol.anchor.startsWith("unstable-");
}

export function summaryClass(value: unknown, baseClass = ""): string {
  return classes(
    baseClass,
    text(value) === "Not yet documented." && "pcdogs-undocumented",
  );
}

export function versionsLabel(symbol: ViewerSymbol): string {
  const builds = text(symbol.builds).trim();
  return builds === VERSION_LABELS.join(" ") ? "All" : builds;
}

export function matchesStability(symbol: ViewerSymbol, filter: StabilityFilter): boolean {
  return (
    filter === "all" || (isUnstable(symbol) ? "unstable" : "stable") === filter
  );
}

export function matchesVersion(symbol: ViewerSymbol, filter: VersionFilter): boolean {
  return filter === "all" || buildLabels(symbol.builds).includes(filter);
}

export function matchesQuery(symbol: ViewerSymbol, needle: string): boolean {
  if (!needle) {
    return true;
  }

  return [
    symbol.name,
    symbol.summary,
    symbol.category_display,
    symbol.category,
    KIND_LABELS[symbol.kind] || symbol.kind,
  ].some((value) => text(value).toLowerCase().includes(needle));
}

export function categoryDisplay(category: unknown): string {
  const normalized = text(category).replace(/_/g, "-").toLowerCase();

  if (CATEGORY_DISPLAY_OVERRIDES[normalized]) {
    return CATEGORY_DISPLAY_OVERRIDES[normalized];
  }

  return normalized
    .split("-")
    .filter(Boolean)
    .map((part) => part.charAt(0).toUpperCase() + part.slice(1))
    .join(" ");
}

export function upperBuild(value: unknown): string {
  return text(value).toUpperCase();
}

export function buildsLabel(value: unknown): string {
  return buildLabels(value).map(upperBuild).join(" ");
}

export function analysisBuildKeys(row: ManifestRow): string[] | null {
  const analysis = row.analysis;
  if (!isRecord(analysis)) {
    return null;
  }

  const keys = Object.keys(analysis).filter((key) =>
    (DEFAULT_BUILDS as readonly string[]).includes(key),
  );
  return keys.length ? keys : null;
}

export function manifestAnchor(kind: SymbolKind, row: ManifestRow): string {
  const prefix = row?.unstable ? "unstable-" : "";
  return `${prefix}${kind}-${slug(row?.name)}`;
}

export function manifestSummary(row: ManifestRow): string {
  return text(row?.docs || "Not yet documented.");
}

export function hexNumber(value: unknown): string {
  const number = Number(value || 0);
  return `0x${number.toString(16).toUpperCase()}`;
}

export function signedHex(value: unknown): string {
  const number = Number(value || 0);
  const sign = number < 0 ? "-" : "+";
  return `${sign} 0x${Math.abs(number).toString(16).toUpperCase()}`;
}

export function xrefOffsets(row: { instr_offset?: unknown; addr_offset?: unknown }): string {
  return `${signedHex(row?.instr_offset)} / ${signedHex(row?.addr_offset)}`;
}

export function typedParams(params: Param[] = []): string {
  return params.map((param) => `${text(param.type)} ${text(param.name)}`).join(", ");
}

export function patchSizes(row: ManifestRow): { patch_size: unknown; entry_patch_size: unknown } | null {
  if (!row?.hook || !("patch_size" in row.hook || "entry_patch_size" in row.hook)) {
    return null;
  }

  return {
    patch_size: row.hook.patch_size,
    entry_patch_size: row.hook.entry_patch_size,
  };
}

export function buildLabels(value: unknown): string[] {
  if (Array.isArray(value)) {
    return value.map((entry) => String(entry ?? "")).filter(Boolean);
  }

  return String(value || "")
    .trim()
    .split(/\s+/)
    .filter(Boolean);
}
