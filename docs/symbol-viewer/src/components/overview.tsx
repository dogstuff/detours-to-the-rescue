import type { JSX } from "preact";
import { useMemo } from "preact/hooks";
import {
  FILTER_OPTIONS,
  KIND_LABELS,
  UNSTABLE_SYMBOL_WARNING,
} from "../constants";
import { symbolHref } from "../navigation";
import type { Filters, SymbolKind, ViewerSymbol } from "../types";
import {
  isUnstable,
  matchesQuery,
  matchesStability,
  matchesVersion,
  summaryClass,
  text,
  versionsLabel,
} from "../utils";

export function OverviewRow({
  symbol,
  onSelect,
}: {
  symbol: ViewerSymbol;
  onSelect: (anchor: string) => void;
}): JSX.Element {
  const open = (): void => onSelect(symbol.anchor);
  const select = (event: JSX.TargetedMouseEvent<HTMLTableRowElement>): void => {
    const target = event.target as HTMLElement;
    const interactive = target.closest?.(
      "a,button,input,select,textarea,[role='button'],[role='link']",
    );

    if (interactive && interactive !== event.currentTarget) {
      return;
    }

    open();
  };
  const openFromKeyboard = (
    event: JSX.TargetedKeyboardEvent<HTMLTableRowElement>,
  ): void => {
    if (
      event.target === event.currentTarget &&
      (event.key === "Enter" || event.key === " ")
    ) {
      event.preventDefault();
      open();
    }
  };

  return (
    <tr
      data-kind={symbol.kind}
      key={symbol.anchor}
      role="link"
      tabIndex={0}
      aria-label={`Open ${symbol.name}`}
      onClick={select}
      onKeyDown={openFromKeyboard}
    >
      <td class="pcdogs-symbol-overview-name" data-label="Symbol">
        <span class="pcdogs-symbol-overview-cell-scroll">
          <a
            href={symbolHref(symbol.anchor)}
            tabIndex={-1}
            onClick={(event) => {
              event.preventDefault();
              open();
            }}
          >
            <code>{symbol.name}</code>
          </a>
          {isUnstable(symbol) ? (
            <span
              class="pcdogs-symbol-overview-warning"
              role="img"
              aria-label={UNSTABLE_SYMBOL_WARNING}
              data-tooltip={UNSTABLE_SYMBOL_WARNING}
              title={UNSTABLE_SYMBOL_WARNING}
            >
              ⚠
            </span>
          ) : null}
          <span class="pcdogs-symbol-overview-meta">
            {KIND_LABELS[symbol.kind] || text(symbol.kind)}
          </span>
        </span>
      </td>
      <td class="pcdogs-symbol-overview-summary" data-label="Summary">
        <span
          class={summaryClass(
            symbol.summary,
            "pcdogs-symbol-overview-summary-text",
          )}
          title={text(symbol.summary)}
        >
          {text(symbol.summary)}
        </span>
      </td>
      <td class="pcdogs-symbol-overview-versions" data-label="Versions">
        {versionsLabel(symbol)}
      </td>
    </tr>
  );
}

function sortKey(symbol: ViewerSymbol): string {
  return `${symbol.name}\0${symbol.category_display || symbol.category || ""}`;
}

function FilterSelect<K extends keyof Filters>({
  label,
  name,
  value,
  options,
  optionLabel,
  onFilterChange,
}: {
  label: string;
  name: K;
  value: Filters[K];
  options: readonly string[];
  optionLabel?: (option: string) => string;
  onFilterChange: <F extends keyof Filters>(name: F, value: Filters[F]) => void;
}): JSX.Element {
  return (
    <label class="pcdogs-symbol-overview-control">
      <span>{label}</span>
      <select
        value={value}
        onChange={(event) =>
          onFilterChange(name, event.currentTarget.value as Filters[K])}
      >
        {options.map((option) => (
          <option value={option} key={option}>
            {optionLabel ? optionLabel(option) : option}
          </option>
        ))}
      </select>
    </label>
  );
}

export function Overview({
  symbols,
  filters,
  onFilterChange,
  onSelect,
}: {
  symbols: ViewerSymbol[];
  filters: Filters;
  onFilterChange: <K extends keyof Filters>(name: K, value: Filters[K]) => void;
  onSelect: (anchor: string) => void;
}): JSX.Element {
  const filteredSymbols = useMemo(() => {
    const needle = filters.query.trim().toLowerCase();

    return symbols
      .filter(
        (symbol) =>
          matchesStability(symbol, filters.stability) &&
          (filters.kind === "all" || symbol.kind === filters.kind) &&
          matchesVersion(symbol, filters.version) &&
          matchesQuery(symbol, needle),
      )
      .sort((a, b) => sortKey(a).localeCompare(sortKey(b)));
  }, [
    symbols,
    filters.query,
    filters.stability,
    filters.kind,
    filters.version,
  ]);
  const limitedSymbols =
    filters.limit === "all"
      ? filteredSymbols
      : filteredSymbols.slice(0, Number(filters.limit));

  return (
    <section class="pcdogs-symbol-overview">
      <h1>PCDogs Symbols</h1>
      <p>Reference for PCDogs symbols wrapped by the modding SDK.</p>
      <p>
        Wrapper source lives{" "}
        <a href="https://gitlab.com/dogstuff/detours-to-the-rescue/-/blob/main/modules/sdk/blueprints/dttr_pcdogs.py">
          in the SDK blueprint
        </a>
        .
      </p>
      <div
        class="pcdogs-symbol-overview-controls"
        role="search"
        aria-label="Filter symbols"
      >
        <label class="pcdogs-symbol-overview-control pcdogs-symbol-overview-control--search">
          <span>Search</span>
          <input
            type="search"
            value={filters.query}
            placeholder="Name, summary, or category"
            onInput={(event) =>
              onFilterChange("query", event.currentTarget.value)}
          />
        </label>
        <FilterSelect
          label="Results"
          name="limit"
          value={filters.limit}
          options={FILTER_OPTIONS.limit}
          optionLabel={(option) => (option === "all" ? "All" : option)}
          onFilterChange={onFilterChange}
        />
        <FilterSelect
          label="Stability"
          name="stability"
          value={filters.stability}
          options={FILTER_OPTIONS.stability}
          optionLabel={(option) =>
            option.charAt(0).toUpperCase() + option.slice(1)}
          onFilterChange={onFilterChange}
        />
        <FilterSelect
          label="Kind"
          name="kind"
          value={filters.kind}
          options={FILTER_OPTIONS.kind}
          optionLabel={(option) =>
            option === "all" ? "All" : KIND_LABELS[option as SymbolKind]}
          onFilterChange={onFilterChange}
        />
        <FilterSelect
          label="Version"
          name="version"
          value={filters.version}
          options={FILTER_OPTIONS.version}
          optionLabel={(option) => (option === "all" ? "All" : option)}
          onFilterChange={onFilterChange}
        />
        <span
          class="pcdogs-symbol-overview-count"
          aria-live="polite"
          role="status"
        >
          {filteredSymbols.length} / {symbols.length}
        </span>
      </div>
      <div class="pcdogs-type-table-frame">
        <table class="pcdogs-type-table pcdogs-symbol-overview-table">
          <thead>
            <tr>
              <th class="pcdogs-symbol-overview-name" scope="col">
                Symbol
              </th>
              <th class="pcdogs-symbol-overview-summary" scope="col">
                Summary
              </th>
              <th class="pcdogs-symbol-overview-versions" scope="col">
                Versions
              </th>
            </tr>
          </thead>
          <tbody>
            {limitedSymbols.length ? (
              limitedSymbols.map((symbol) => (
                <OverviewRow
                  symbol={symbol}
                  onSelect={onSelect}
                  key={symbol.anchor}
                />
              ))
            ) : (
              <tr class="pcdogs-symbol-overview-empty">
                <td colSpan={3}>No symbols match these filters.</td>
              </tr>
            )}
          </tbody>
        </table>
      </div>
    </section>
  );
}
