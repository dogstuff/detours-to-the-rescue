import type { JSX } from "preact";
import {
  useCallback,
  useEffect,
  useMemo,
  useRef,
  useState,
} from "preact/hooks";
import { manifestSymbols } from "../manifest";
import { selectHash, selectedHash } from "../navigation";
import {
  normalizeStoredFilters,
  readStoredFilters,
  storageKey,
  writeStoredFilters,
} from "../storage";
import type {
  Filters,
  Manifest,
  VersionLabel,
  ViewerStatus,
  ViewerSymbol,
} from "../types";
import { Detail, supportedVersions } from "./detail";
import { Overview } from "./overview";

export function Viewer({ root, src }: { root: HTMLElement; src: string }): JSX.Element {
  const [status, setStatus] = useState<ViewerStatus>("loading");
  const [symbols, setSymbols] = useState<ViewerSymbol[]>([]);
  const filtersStorageKey = useMemo(() => storageKey(root, src), [root, src]);
  const [selected, setSelected] = useState(selectedHash());
  const [selectedVersion, setSelectedVersion] = useState<VersionLabel>("EN");
  const [overviewFilters, setOverviewFilters] = useState<Filters>(() =>
    normalizeStoredFilters(readStoredFilters(filtersStorageKey)),
  );
  const [error, setError] = useState("");
  const overviewScrollY = useRef(0);
  const pendingDetailScroll = useRef(false);
  const pendingOverviewRestore = useRef(false);
  const byAnchor = useMemo(
    () => new Map(symbols.map((symbol) => [symbol.anchor, symbol])),
    [symbols],
  );

  const updateOverviewFilter = useCallback(
    <K extends keyof Filters>(name: K, value: Filters[K]) => {
      setOverviewFilters((filters) =>
        normalizeStoredFilters({ ...filters, [name]: value }),
      );
    },
    [],
  );

  const select = useCallback(
    (anchor: string) => {
      if (!anchor) {
        if (selected) {
          pendingOverviewRestore.current = true;
        }
      } else {
        if (!selected) {
          overviewScrollY.current = window.scrollY;
        }

        const symbol = byAnchor.get(anchor);

        if (
          symbol &&
          overviewFilters.version !== "all" &&
          supportedVersions(symbol).includes(overviewFilters.version)
        ) {
          setSelectedVersion(overviewFilters.version);
        }

        pendingDetailScroll.current = true;
      }

      selectHash(anchor);
      setSelected(anchor);
    },
    [byAnchor, overviewFilters.version, selected],
  );

  useEffect(() => {
    const syncHash = () => setSelected(selectedHash());
    window.addEventListener("hashchange", syncHash);
    window.addEventListener("popstate", syncHash);

    const controller = new AbortController();
    fetch(src, { signal: controller.signal })
      .then((response) => {
        if (!response.ok) {
          throw new Error(`HTTP ${response.status}`);
        }

        return response.json() as Promise<Manifest>;
      })
      .then((payload) => {
        setSymbols(manifestSymbols(payload));
        setStatus("ready");

        if (selectedHash()) {
          root.scrollIntoView({ block: "start", behavior: "smooth" });
        }
      })
      .catch((err: unknown) => {
        if (err instanceof DOMException && err.name === "AbortError") {
          return;
        }

        setError(String(err));
        setStatus("error");
      });

    return () => {
      controller.abort();
      window.removeEventListener("hashchange", syncHash);
      window.removeEventListener("popstate", syncHash);
    };
  }, [root, src]);

  useEffect(() => {
    writeStoredFilters(filtersStorageKey, overviewFilters);
  }, [filtersStorageKey, overviewFilters]);

  const symbol = byAnchor.get(selected) ?? null;

  useEffect(() => {
    if (status !== "ready") {
      return;
    }

    if (selected && symbol && pendingDetailScroll.current) {
      pendingDetailScroll.current = false;
      requestAnimationFrame(() => {
        root.scrollIntoView({ block: "start", behavior: "auto" });
      });
    }

    if (!selected && pendingOverviewRestore.current) {
      pendingOverviewRestore.current = false;
      requestAnimationFrame(() => {
        window.scrollTo({ top: overviewScrollY.current, behavior: "auto" });
      });
    }
  }, [root, selected, status, symbol]);

  if (status === "loading") {
    return <p>Loading symbol data...</p>;
  }

  if (status === "error") {
    return <p>Could not load symbol data: {error}</p>;
  }

  if (selected && symbol) {
    return (
      <Detail
        symbol={symbol}
        byAnchor={byAnchor}
        selectedVersion={selectedVersion}
        onVersionChange={setSelectedVersion}
        onSelect={select}
      />
    );
  }

  return (
    <>
      {selected ? (
        <p>
          Symbol not found: <code>{selected}</code>
        </p>
      ) : null}
      <Overview
        symbols={symbols}
        filters={overviewFilters}
        onFilterChange={updateOverviewFilter}
        onSelect={select}
      />
    </>
  );
}
