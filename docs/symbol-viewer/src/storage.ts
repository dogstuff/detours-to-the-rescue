import { FILTER_DEFAULTS, FILTER_OPTIONS } from "./constants";
import type { AnyRecord, Filters } from "./types";
import { isOneOf, isRecord } from "./utils";

function getSessionStorage(): Storage | null {
  try {
    return typeof sessionStorage === "undefined" ? null : sessionStorage;
  } catch {
    return null;
  }
}

function withStorage<T>(fallback: T, run: (storage: Storage) => T): T {
  const storage = getSessionStorage();

  if (!storage) {
    return fallback;
  }

  try {
    return run(storage);
  } catch {
    return fallback;
  }
}

export function storageKey(root: HTMLElement, src: string): string {
  return `pcdogs-symbol-viewer-filters:${location.pathname}:${src}:${
    root.id || "viewer"
  }`;
}

export function readStoredFilters(key: string): Partial<Filters> {
  return withStorage<Partial<Filters>>({}, (storage) => {
    const parsed: unknown = JSON.parse(storage.getItem(key) || "{}");
    return isRecord(parsed) ? parsed : {};
  });
}

export function writeStoredFilters(key: string, filters: Filters): void {
  // Ignore private-mode or quota failures; in-memory state still works.
  withStorage<void>(undefined, (storage) => {
    storage.setItem(key, JSON.stringify(filters));
  });
}

export function normalizeStoredFilters(filters: Partial<Filters> | AnyRecord): Filters {
  return {
    query:
      typeof filters.query === "string" ? filters.query : FILTER_DEFAULTS.query,
    stability: isOneOf(FILTER_OPTIONS.stability, filters.stability)
      ? filters.stability
      : FILTER_DEFAULTS.stability,
    kind: isOneOf(FILTER_OPTIONS.kind, filters.kind)
      ? filters.kind
      : FILTER_DEFAULTS.kind,
    version: isOneOf(FILTER_OPTIONS.version, filters.version)
      ? filters.version
      : FILTER_DEFAULTS.version,
    limit: isOneOf(FILTER_OPTIONS.limit, filters.limit)
      ? filters.limit
      : FILTER_DEFAULTS.limit,
  };
}
