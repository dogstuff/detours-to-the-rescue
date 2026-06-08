import { h, render } from "https://esm.sh/preact@10.27.2";
import {
  useCallback,
  useEffect,
  useMemo,
  useRef,
  useState,
} from "https://esm.sh/preact@10.27.2/hooks";
import htm from "https://esm.sh/htm@3.1.1";

const html = htm.bind(h);
const ROOT_SELECTOR = "#pcdogs-symbol-viewer";
const KIND_LABELS = {
  function: "Function",
  data: "Data",
  type: "Type",
  signature: "Signature",
};
const VERSION_LABELS = ["EN", "EU", "SC"];
const FILTER_DEFAULTS = {
  query: "",
  stability: "all",
  kind: "all",
  version: "all",
  limit: "100",
};
const FILTER_OPTIONS = {
  stability: ["all", "stable", "unstable"],
  kind: ["all", ...Object.keys(KIND_LABELS)],
  version: ["all", ...VERSION_LABELS],
  limit: ["100", "250", "500", "1000", "all"],
};

function classes(...items) {
  return items.filter(Boolean).join(" ");
}

function text(value) {
  return value == null || value === "" ? "-" : String(value);
}

function getSessionStorage() {
  try {
    return typeof sessionStorage === "undefined" ? null : sessionStorage;
  } catch {
    return null;
  }
}

function storageKey(root, src) {
  return `pcdogs-symbol-viewer-filters:${location.pathname}:${src}:${
    root.id || "viewer"
  }`;
}

function readStoredFilters(key) {
  const storage = getSessionStorage();

  if (!storage) {
    return {};
  }

  try {
    return JSON.parse(storage.getItem(key) || "{}") || {};
  } catch {
    return {};
  }
}

function writeStoredFilters(key, filters) {
  const storage = getSessionStorage();

  if (!storage) {
    return;
  }

  try {
    storage.setItem(key, JSON.stringify(filters));
  } catch {
    // Ignore private-mode or quota failures; in-memory state still works.
  }
}

function normalizeStoredFilters(filters) {
  return {
    query:
      typeof filters.query === "string" ? filters.query : FILTER_DEFAULTS.query,
    stability: FILTER_OPTIONS.stability.includes(filters.stability)
      ? filters.stability
      : FILTER_DEFAULTS.stability,
    kind: FILTER_OPTIONS.kind.includes(filters.kind)
      ? filters.kind
      : FILTER_DEFAULTS.kind,
    version: FILTER_OPTIONS.version.includes(filters.version)
      ? filters.version
      : FILTER_DEFAULTS.version,
    limit: FILTER_OPTIONS.limit.includes(filters.limit)
      ? filters.limit
      : FILTER_DEFAULTS.limit,
  };
}

function selectedHash() {
  return new URLSearchParams(location.hash.slice(1)).get("symbol") || "";
}

function symbolHref(anchor) {
  return anchor ? `#${new URLSearchParams({ symbol: anchor })}` : "#";
}

function selectHash(anchor) {
  if (anchor) {
    const next = symbolHref(anchor);

    if (location.hash !== next) {
      location.hash = next;
    }

    return;
  }

  if (location.hash) {
    history.pushState(null, "", `${location.pathname}${location.search}`);
  }
}

function slug(value) {
  return (
    text(value)
      .toLowerCase()
      .replace(/[^a-z0-9]+/g, "-")
      .replace(/^-|-$/g, "") || "reference"
  );
}

function isUnstable(symbol) {
  return symbol.anchor.startsWith("unstable-");
}

function isUndocumented(value) {
  return text(value) === "Not yet documented.";
}

function summaryClass(value, baseClass = "") {
  return classes(baseClass, isUndocumented(value) && "pcdogs-undocumented");
}

function versionsLabel(symbol) {
  const builds = text(symbol.builds).trim();
  return builds === "EN EU SC" ? "All" : builds;
}

function matchesStability(symbol, filter) {
  return (
    filter === "all" || (isUnstable(symbol) ? "unstable" : "stable") === filter
  );
}

function matchesVersion(symbol, filter) {
  return filter === "all" || buildLabels(symbol.builds).includes(filter);
}

function matchesQuery(symbol, needle) {
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

function overviewSortKey(symbol) {
  return `${symbol.name}\0${symbol.category_display || symbol.category || ""}`;
}

function TableFrame({ title, extraClass = "", children }) {
  return html`<div class=${classes("pcdogs-type-table-frame", extraClass)}>
    <div class="pcdogs-table-title">${title}</div>
    ${children}
  </div>`;
}

function CodeBlock({ value, htmlValue }) {
  if (htmlValue) {
    return html`<div class="highlight">
      <pre><code
        class="language-c"
        dangerouslySetInnerHTML=${{ __html: htmlValue }}
      ></code></pre>
    </div>`;
  }

  return html`<div class="highlight">
    <pre><code class="language-c">${text(value)}</code></pre>
  </div>`;
}

function TabbedSet({ base, extraClass = "", tabs, renderTab }) {
  if (!tabs.length) {
    return null;
  }

  return html`<div
    class=${classes("tabbed-set tabbed-alternate", extraClass)}
    data-tabs=${`${base}:${tabs.length}`}
  >
    ${tabs.map(
      (_, index) =>
        html`<input
          defaultChecked=${index === 0}
          id=${`${base}-${index + 1}`}
          name=${base}
          type="radio"
          key=${index}
        />`,
    )}
    <div class="tabbed-labels">
      ${tabs.map(
        (tab, index) =>
          html`<label for=${`${base}-${index + 1}`} key=${tab.label}
            >${tab.label}</label
          >`,
      )}
    </div>
    <div class="tabbed-content">
      ${tabs.map(
        (tab) =>
          html`<div class="tabbed-block" key=${tab.label}>
            ${renderTab(tab)}
          </div>`,
      )}
    </div>
  </div>`;
}

function ExampleTabs({ symbol, tabs }) {
  return html`<${TabbedSet}
    base=${`pcdogs-symbol-example-${symbol.anchor}`}
    tabs=${tabs.filter((tab) => tab.value)}
    renderTab=${(tab) =>
      html`<${CodeBlock} value=${tab.value} htmlValue=${tab.htmlValue} />`}
  />`;
}

function Facts({ rows }) {
  if (!rows?.length) {
    return null;
  }

  return html`<div class="pcdogs-symbol-facts">
    ${rows.map(
      (row) =>
        html`<p class="pcdogs-symbol-fact" key=${`${row.label}:${row.value}`}>
          <span class="pcdogs-symbol-fact__label"
            >${row.label === "Policy" ? "Write Policy" : row.label}</span
          >
          <strong
            class=${row.label === "Type"
              ? "pcdogs-symbol-fact__value pcdogs-symbol-fact__value--type"
              : "pcdogs-symbol-fact__value"}
            >${row.value}</strong
          >
        </p>`,
    )}
  </div>`;
}

function metadataLabel(row, isAddress) {
  return isAddress ? "Location" : row.label;
}

function isAddressRow(row) {
  return row.label.startsWith("Address (");
}

function addressBuild(row) {
  return /^Address \(([^)]+)\)$/.exec(row.label)?.[1] || "";
}

function HexValue({ value, split = false }) {
  const match = value.match(/0x[0-9a-fA-F]+/);

  if (!match) {
    return value;
  }

  if (!split) {
    return html`<span class="pcdogs-address-table__hex">${value}</span>`;
  }

  const start = match.index || 0;
  const end = start + match[0].length;

  return html`<span class="pcdogs-address-table__prefix"
      >${value.slice(0, start)}</span
    ><span class="pcdogs-address-table__hex">${value.slice(start, end)}</span
    ><span class="pcdogs-address-table__prefix">${value.slice(end)}</span>`;
}

function ResolverValue({ value }) {
  const match = /^(.*?)\s+([+-].*)$/.exec(text(value).trim());

  if (!match) {
    return text(value);
  }

  return html`<span class="pcdogs-metadata-table__resolver">
    <code class="pcdogs-metadata-table__resolver-name">${match[1]}</code>
    <${XRefOffset} offsets=${match[2]} />
  </span>`;
}

function MetadataValue({ row }) {
  const value = row.value;

  if (/^-?0x[0-9a-fA-F]+$/.test(value)) {
    return html`<${HexValue} value=${value} />`;
  }

  if (row.label === "AOB Pattern" || row.label === "Signature") {
    return html`<code>${value}</code>`;
  }

  if (row.label === "Resolver") {
    return html`<${ResolverValue} value=${value} />`;
  }

  return value;
}

function MetadataTable({ rows }) {
  return html`<table class="pcdogs-type-table pcdogs-metadata-table__body">
    <tbody>
      ${rows.map((row) => {
        const isAddress = isAddressRow(row);

        return html`<tr
          class=${classes(
            "pcdogs-metadata-table__row",
            isAddress && "pcdogs-metadata-table__row--address",
          )}
          key=${`${row.label}:${row.value}`}
        >
          <th
            scope="row"
            class=${classes(
              "pcdogs-metadata-table__label",
              isAddress && "pcdogs-address-table__version",
            )}
          >
            ${metadataLabel(row, isAddress)}
          </th>
          <td
            class=${classes(
              "pcdogs-metadata-table__value",
              isAddress && "pcdogs-address-table__address",
            )}
          >
            ${isAddress
              ? html`<${HexValue} value=${row.value} split=${true} />`
              : html`<${MetadataValue} row=${row} />`}
          </td>
        </tr>`;
      })}
    </tbody>
  </table>`;
}

function metadataRowsForVersion(rows, version) {
  const addressRows = [];
  const otherRows = [];

  for (const row of rows) {
    if (!isAddressRow(row)) {
      otherRows.push(row);
      continue;
    }

    if (addressBuild(row) === version) {
      addressRows.push(row);
    }
  }

  return [...addressRows, ...otherRows];
}

function Metadata({ rows }) {
  if (!rows?.length) {
    return null;
  }

  return html`<div
    class="pcdogs-reference-table pcdogs-reference-table--metadata"
  >
    <${TableFrame}
      title="Metadata"
      extraClass="pcdogs-xref-table pcdogs-metadata-table"
    >
      <${MetadataTable} rows=${rows} />
    <//>
  </div>`;
}

function normalizeSignedOffsets(value) {
  return text(value).replace(/([+-])\s*(0x[0-9a-fA-F]+)/g, "$1 $2");
}

function offsetParts(offsets) {
  const value = normalizeSignedOffsets(offsets).trim();

  if (value === "-") {
    return { prefix: "", offset: "" };
  }

  const signed = /^([+-])\s+(.*)$/.exec(value);

  if (signed) {
    return { prefix: ` ${signed[1]} `, offset: signed[2] };
  }

  return { prefix: " + ", offset: value };
}

function XRefOffset({ offsets }) {
  const offset = offsetParts(offsets);

  return html`<span class="pcdogs-xref-table__location-offset-prefix"
      >${offset.prefix}</span
    ><span class="pcdogs-xref-table__location-offset">${offset.offset}</span>`;
}

function XRefRows({ rows, incoming, onSelect }) {
  const direction = incoming ? "reference-from" : "reference-to";
  const arrow = incoming ? "<-" : "->";

  return rows.map(
    (row, index) =>
      html`<tr
        class=${`pcdogs-xref-table__row pcdogs-xref-table__row--${direction} pcdogs-xref-table__row--${slug(row.kind)}`}
        key=${`${row.target_anchor || row.text || row.value}:${index}`}
      >
        <td class="pcdogs-xref-table__arrow">${arrow}</td>
        <td class="pcdogs-xref-table__relation">${text(row.kind)}</td>
        <td class="pcdogs-xref-table__name">
          <div class="pcdogs-xref-table__cell-scroll">
            ${row.target_anchor
              ? html`<button
                  type="button"
                  class="pcdogs-symbol-link-button pcdogs-xref-table__symbol-link"
                  onClick=${() => onSelect(row.target_anchor)}
                >
                  ${text(row.text || row.value)}
                </button>`
              : text(row.text || row.value)}
            <${XRefOffset} offsets=${row.offsets} />
          </div>
        </td>
      </tr>`,
  );
}

function buildLabels(value) {
  return String(value || "")
    .trim()
    .split(/\s+/)
    .filter(Boolean);
}

function rowMatchesVersion(row, version) {
  const builds = buildLabels(row.builds);
  return !builds.length || builds.includes(version);
}

function ReferenceTable({ from, to, onSelect }) {
  if (!from.length && !to.length) {
    return html`<p class="pcdogs-xref-table__empty">
      No references for this region.
    </p>`;
  }

  return html`<table class="pcdogs-type-table pcdogs-xref-table__body">
    <tbody>
      <${XRefRows} rows=${from} incoming=${true} onSelect=${onSelect} />
      <${XRefRows} rows=${to} incoming=${false} onSelect=${onSelect} />
    </tbody>
  </table>`;
}

function ReferenceCard({ modifier, children }) {
  return html`<div
    class=${classes(
      "pcdogs-symbol-reference-tables__reference-card",
      `pcdogs-symbol-reference-tables__reference-card--${modifier}`,
    )}
  >
    ${children}
  </div>`;
}

function DetailRegionTables({ symbol, selectedVersion, onSelect }) {
  const metadataRows = symbol.metadata || [];
  const referencedBy = symbol.referenced_by || [];
  const references = symbol.references || [];
  const hasMetadata = metadataRows.length > 0;
  const hasReferences = referencedBy.length > 0 || references.length > 0;

  if (!hasMetadata && !hasReferences) {
    return null;
  }

  const filteredMetadataRows = metadataRowsForVersion(
    metadataRows,
    selectedVersion,
  );
  const filteredReferencedBy = referencedBy.filter((row) =>
    rowMatchesVersion(row, selectedVersion),
  );
  const filteredReferences = references.filter((row) =>
    rowMatchesVersion(row, selectedVersion),
  );

  return html`<div class="pcdogs-symbol-reference-tables">
    ${hasMetadata
      ? html`<div class="pcdogs-symbol-reference-tables__metadata">
          <${Metadata} rows=${filteredMetadataRows} />
        </div>`
      : null}
    ${hasReferences
      ? html`<${ReferenceCard} modifier="references">
          <div class="pcdogs-reference-table pcdogs-reference-table--xref">
            <${TableFrame} title="References" extraClass="pcdogs-xref-table">
              <${ReferenceTable}
                from=${filteredReferencedBy}
                to=${filteredReferences}
                onSelect=${onSelect}
              />
            <//>
          </div>
        <//>`
      : null}
  </div>`;
}

function childAnchors(node) {
  return [...node.children.keys()];
}

function buildHierarchyTree(paths) {
  const root = { children: new Map() };

  for (const path of paths) {
    let node = root;

    for (const anchor of path) {
      const child = node.children.get(anchor) || { children: new Map() };
      node.children.set(anchor, child);
      node = child;
    }

    node.terminal = true;
  }

  return root;
}

function compactHierarchyPath(anchor, node) {
  const anchors = [anchor];
  let current = node;
  let children = childAnchors(current);

  while (children.length === 1 && !current.terminal) {
    const next = children[0];
    anchors.push(next);
    current = current.children.get(next);
    children = childAnchors(current);
  }

  return { anchors, node: current };
}

function sortedHierarchyAnchors(node, byAnchor) {
  return childAnchors(node).sort((left, right) => {
    const leftSymbol = byAnchor.get(left);
    const rightSymbol = byAnchor.get(right);
    return `${leftSymbol?.name || left}\0${left}`.localeCompare(
      `${rightSymbol?.name || right}\0${right}`,
    );
  });
}

function HierarchyLink({ anchor, current, byAnchor, onSelect }) {
  const target = byAnchor.get(anchor);
  const className = classes(
    "pcdogs-reference-hierarchy-tree__symbol",
    `pcdogs-reference-hierarchy-tree__symbol--${target?.kind || "symbol"}`,
    anchor === current && "pcdogs-reference-hierarchy-tree__symbol--current",
  );

  return html`<a
    class=${className}
    href=${symbolHref(anchor)}
    aria-current=${anchor === current ? "page" : undefined}
    onClick=${(event) => {
      event.preventDefault();
      event.stopPropagation();
      onSelect(anchor);
    }}
    >${target?.name || anchor}</a
  >`;
}

function HierarchyPathView({ anchors, current, byAnchor, onSelect }) {
  return html`<span class="pcdogs-reference-hierarchy-tree__path">
    ${anchors.map(
      (anchor, index) =>
        html`<span
          class="pcdogs-reference-hierarchy-tree__run"
          key=${`${anchor}:${index}`}
        >
          ${index
            ? html`<span
                class="pcdogs-reference-hierarchy-tree__separator"
                aria-hidden="true"
                >${">"}</span
              >`
            : null}
          <${HierarchyLink}
            anchor=${anchor}
            current=${current}
            byAnchor=${byAnchor}
            onSelect=${onSelect}
          />
        </span>`,
    )}
  </span>`;
}

function HierarchyBranch({ node, depth, current, byAnchor, onSelect }) {
  const listClass = classes(
    "pcdogs-reference-hierarchy-tree__list",
    depth && "pcdogs-reference-hierarchy-tree__children",
  );

  return html`<ul class=${listClass}>
    ${sortedHierarchyAnchors(node, byAnchor).map((anchor) => {
      const child = node.children.get(anchor);
      const compacted = compactHierarchyPath(anchor, child);
      const children = childAnchors(compacted.node);
      const hasChildren = children.length > 0;
      const rowClass = `pcdogs-reference-hierarchy-tree__row pcdogs-reference-hierarchy-tree__row--${hasChildren ? "branch" : "leaf"}`;
      const pathView = html`<${HierarchyPathView}
        anchors=${compacted.anchors}
        current=${current}
        byAnchor=${byAnchor}
        onSelect=${onSelect}
      />`;

      return html`<li
        class="pcdogs-reference-hierarchy-tree__item"
        key=${`${depth}:${compacted.anchors.join(":")}`}
      >
        ${hasChildren
          ? html`<details class="pcdogs-reference-hierarchy-tree__branch" open>
              <summary class=${rowClass}>${pathView}</summary>
              <${HierarchyBranch}
                node=${compacted.node}
                depth=${depth + 1}
                current=${current}
                byAnchor=${byAnchor}
                onSelect=${onSelect}
              />
            </details>`
          : html`<div class=${rowClass}>${pathView}</div>`}
      </li>`;
    })}
  </ul>`;
}

function Hierarchy({ symbol, byAnchor, onSelect }) {
  const paths = symbol.reference_hierarchy_paths || [];

  const tree = useMemo(() => buildHierarchyTree(paths), [paths]);

  if (!paths.length) {
    return null;
  }

  return html`<div
    class="pcdogs-reference-table pcdogs-reference-table--reference-hierarchy"
  >
    <${TableFrame}
      title="Entrypoint Reference Hierarchy"
      extraClass="pcdogs-xref-table"
    >
      <nav
        class="pcdogs-reference-hierarchy-tree"
        aria-label="Entrypoint reference hierarchy"
      >
        <${HierarchyBranch}
          node=${tree}
          depth=${0}
          current=${symbol.anchor}
          byAnchor=${byAnchor}
          onSelect=${onSelect}
        />
      </nav>
    <//>
  </div>`;
}

function TypeDetails({ symbol }) {
  return html`${symbol.members?.length
    ? html`<${TableFrame} title="Members">
        <table class="pcdogs-type-table pcdogs-type-table--struct">
          <thead>
            <tr>
              <th>Offset</th>
              <th>Name</th>
              <th>Type</th>
              <th>Notes</th>
            </tr>
          </thead>
          <tbody>
            ${symbol.members.map(
              (member, index) =>
                html`<tr key=${`${member.offset}:${member.name}:${index}`}>
                  <td><code>${text(member.offset)}</code></td>
                  <td>
                    <div class="pcdogs-type-table__cell-scroll">
                      <code>${text(member.name)}</code>
                    </div>
                  </td>
                  <td>
                    <div class="pcdogs-type-table__cell-scroll">
                      <code>${text(member.type)}</code>
                    </div>
                  </td>
                  <td>${text(member.doc)}</td>
                </tr>`,
            )}
          </tbody>
        </table>
      <//>`
    : null}
  ${symbol.enum_values?.length
    ? html`<${TableFrame} title="Values">
        <table class="pcdogs-type-table pcdogs-type-table--enum">
          <thead>
            <tr>
              <th>Value</th>
              <th>Name</th>
              <th>Notes</th>
            </tr>
          </thead>
          <tbody>
            ${symbol.enum_values.map(
              (value, index) =>
                html`<tr key=${`${value.value}:${value.name}:${index}`}>
                  <td><code>${text(value.value)}</code></td>
                  <td><code>${text(value.name)}</code></td>
                  <td>${text(value.table_doc || value.doc)}</td>
                </tr>`,
            )}
          </tbody>
        </table>
      <//>`
    : null}`;
}

function DetailsForKind({ symbol }) {
  if (symbol.kind === "function") {
    return html`<${ExampleTabs}
      symbol=${symbol}
      tabs=${[
        {
          label: "Original Call Format",
          value: symbol.pseudo_usage,
          htmlValue: symbol.pseudo_usage_html,
        },
        {
          label: "C SDK Call",
          value: symbol.call_example,
          htmlValue: symbol.call_example_html,
        },
        {
          label: "C SDK Patch Spec",
          value: symbol.patch_spec_example,
          htmlValue: symbol.patch_spec_example_html,
        },
        {
          label: "C SDK Hook",
          value: symbol.hook_example,
          htmlValue: symbol.hook_example_html,
        },
      ]}
    />`;
  }

  if (symbol.kind === "data") {
    return html`<p><strong>Type:</strong> ${text(symbol.type)}</p>
      <p><strong>Write policy:</strong> ${text(symbol.write_policy)}</p>
      ${symbol.untyped_note
        ? html`<p class="pcdogs-untyped-data">${symbol.untyped_note}</p>`
        : null}
      <${ExampleTabs}
        symbol=${symbol}
        tabs=${[
          {
            label: "Read",
            value: symbol.read_example,
            htmlValue: symbol.read_example_html,
          },
          {
            label: "Write",
            value: symbol.write_example,
            htmlValue: symbol.write_example_html,
          },
        ]}
      />`;
  }

  return symbol.kind === "type"
    ? html`<${TypeDetails} symbol=${symbol} />`
    : null;
}

function OverviewRow({ symbol, onSelect }) {
  const select = (event) => {
    const interactive = event.target.closest?.(
      "a,button,input,select,textarea,[role='button'],[role='link']",
    );

    if (interactive && interactive !== event.currentTarget) {
      return;
    }

    onSelect(symbol.anchor);
  };

  return html`<tr
    data-kind=${symbol.kind}
    key=${symbol.anchor}
    role="link"
    tabindex="0"
    aria-label=${`Open ${symbol.name}`}
    onClick=${select}
    onKeyDown=${(event) => {
      if (event.target !== event.currentTarget) {
        return;
      }

      if (event.key === "Enter" || event.key === " ") {
        event.preventDefault();
        onSelect(symbol.anchor);
      }
    }}
  >
    <td class="pcdogs-symbol-overview-name" data-label="Symbol">
      <span class="pcdogs-symbol-overview-cell-scroll">
        <a
          href=${symbolHref(symbol.anchor)}
          tabindex="-1"
          onClick=${(event) => {
            event.preventDefault();
            onSelect(symbol.anchor);
          }}
          ><code>${symbol.name}</code></a
        >
        ${isUnstable(symbol)
          ? html`<span
              class="pcdogs-symbol-overview-warning"
              role="img"
              aria-label="This symbol is unstable and may change without warning in the future."
              data-tooltip="This symbol is unstable and may change without warning in the future."
              title="This symbol is unstable and may change without warning in the future."
              >⚠</span
            >`
          : null}
        <span class="pcdogs-symbol-overview-meta"
          >${KIND_LABELS[symbol.kind] || text(symbol.kind)}</span
        >
      </span>
    </td>
    <td class="pcdogs-symbol-overview-summary" data-label="Summary">
      <span
        class=${summaryClass(
          symbol.summary,
          "pcdogs-symbol-overview-summary-text",
        )}
        title=${text(symbol.summary)}
        >${text(symbol.summary)}</span
      >
    </td>
    <td class="pcdogs-symbol-overview-versions" data-label="Versions">
      ${versionsLabel(symbol)}
    </td>
  </tr>`;
}

function Overview({ symbols, filters, onFilterChange, onSelect }) {
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
      .sort((a, b) => overviewSortKey(a).localeCompare(overviewSortKey(b)));
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

  return html`<section class="pcdogs-symbol-overview">
    <h1>PCDogs Symbols</h1>
    <p>Reference for PCDogs symbols wrapped by the modding SDK.</p>
    <p>
      If you're interested in contributing, the source of truth for these
      wrappers can be found${" "}
      <a
        href="https://gitlab.com/dogstuff/detours-to-the-rescue/-/blob/main/modules/sdk/blueprints/dttr_pcdogs.py"
        >here</a
      >.
    </p>
    <div
      class="pcdogs-symbol-overview-controls"
      role="search"
      aria-label="Filter symbols"
    >
      <label
        class="pcdogs-symbol-overview-control pcdogs-symbol-overview-control--search"
      >
        <span>Search</span>
        <input
          type="search"
          value=${filters.query}
          placeholder="Name, summary, or something else"
          onInput=${(event) =>
            onFilterChange("query", event.currentTarget.value)}
        />
      </label>
      <label class="pcdogs-symbol-overview-control">
        <span>Results</span>
        <select
          value=${filters.limit}
          onChange=${(event) =>
            onFilterChange("limit", event.currentTarget.value)}
        >
          ${FILTER_OPTIONS.limit.map(
            (limit) =>
              html`<option value=${limit} key=${limit}>
                ${limit === "all" ? "All" : limit}
              </option>`,
          )}
        </select>
      </label>
      <label class="pcdogs-symbol-overview-control">
        <span>Stability</span>
        <select
          value=${filters.stability}
          onChange=${(event) =>
            onFilterChange("stability", event.currentTarget.value)}
        >
          <option value="all">All</option>
          <option value="stable">Stable</option>
          <option value="unstable">Unstable</option>
        </select>
      </label>
      <label class="pcdogs-symbol-overview-control">
        <span>Kind</span>
        <select
          value=${filters.kind}
          onChange=${(event) =>
            onFilterChange("kind", event.currentTarget.value)}
        >
          <option value="all">All</option>
          ${Object.entries(KIND_LABELS).map(
            ([value, label]) =>
              html`<option value=${value} key=${value}>${label}</option>`,
          )}
        </select>
      </label>
      <label class="pcdogs-symbol-overview-control">
        <span>Version</span>
        <select
          value=${filters.version}
          onChange=${(event) =>
            onFilterChange("version", event.currentTarget.value)}
        >
          <option value="all">All</option>
          ${VERSION_LABELS.map(
            (version) =>
              html`<option value=${version} key=${version}>${version}</option>`,
          )}
        </select>
      </label>
      <span
        class="pcdogs-symbol-overview-count"
        aria-live="polite"
        role="status"
        >${filteredSymbols.length} / ${symbols.length}</span
      >
    </div>
    <div class="pcdogs-type-table-frame">
      <table class="pcdogs-type-table pcdogs-symbol-overview-table">
        <thead>
          <tr>
            <th class="pcdogs-symbol-overview-name" scope="col">Symbol</th>
            <th class="pcdogs-symbol-overview-summary" scope="col">Summary</th>
            <th class="pcdogs-symbol-overview-versions" scope="col">
              Versions
            </th>
          </tr>
        </thead>
        <tbody>
          ${limitedSymbols.length
            ? limitedSymbols.map(
                (symbol) =>
                  html`<${OverviewRow}
                    symbol=${symbol}
                    onSelect=${onSelect}
                    key=${symbol.anchor}
                  />`,
              )
            : html`<tr class="pcdogs-symbol-overview-empty">
                <td colspan="3">No symbols match these filters.</td>
              </tr>`}
        </tbody>
      </table>
    </div>
  </section>`;
}

const DETAIL_BUTTON_INSET_PX = 12;

function useDetailFramePosition() {
  const detailRef = useRef(null);
  const [frameLeft, setFrameLeft] = useState(`${DETAIL_BUTTON_INSET_PX}px`);
  const [frameTop, setFrameTop] = useState(`${DETAIL_BUTTON_INSET_PX}px`);

  useEffect(() => {
    const detail = detailRef.current;

    if (!detail) {
      return undefined;
    }

    const update = () => {
      const rect = detail.getBoundingClientRect();
      setFrameLeft(
        `${Math.max(Math.round(rect.left), DETAIL_BUTTON_INSET_PX)}px`,
      );
      setFrameTop(
        `${Math.max(Math.round(rect.top), DETAIL_BUTTON_INSET_PX)}px`,
      );
    };

    update();
    window.addEventListener("resize", update);
    window.addEventListener("scroll", update, { passive: true });

    const observer =
      typeof ResizeObserver === "undefined" ? null : new ResizeObserver(update);
    observer?.observe(detail);

    return () => {
      window.removeEventListener("resize", update);
      window.removeEventListener("scroll", update);
      observer?.disconnect();
    };
  }, []);

  return { detailRef, frameLeft, frameTop };
}

function supportedVersions(symbol) {
  const versions = buildLabels(symbol.builds).filter((version) =>
    VERSION_LABELS.includes(version),
  );

  return versions.length ? versions : VERSION_LABELS;
}

function HeaderVersionPicker({ versions, selectedVersion, onVersionChange }) {
  return html`<span
    class="pcdogs-symbol-detail-versions"
    role="group"
    aria-label="Symbol versions"
  >
    ${versions.map((version) => {
      const selected = version === selectedVersion;

      return html`<button
        type="button"
        class=${classes(
          "pcdogs-symbol-detail-version",
          selected && "pcdogs-symbol-detail-version--selected",
        )}
        aria-pressed=${selected}
        onClick=${() => onVersionChange(version)}
        key=${version}
      >
        ${version}
      </button>`;
    })}
  </span>`;
}

function Detail({
  symbol,
  byAnchor,
  selectedVersion,
  onVersionChange,
  onSelect,
}) {
  const frame = useDetailFramePosition();
  const versions = supportedVersions(symbol);
  const effectiveVersion = versions.includes(selectedVersion)
    ? selectedVersion
    : versions[0];

  return html`<article
    ref=${frame.detailRef}
    class="pcdogs-symbol-detail pcdogs-symbol-detail-page"
    style=${{
      "--pcdogs-detail-left": frame.frameLeft,
      "--pcdogs-detail-top": frame.frameTop,
    }}
  >
    <button
      type="button"
      class="pcdogs-symbol-link-button pcdogs-symbol-detail-back"
      onClick=${() => onSelect("")}
    >
      ${"<- Back to symbol overview"}
    </button>
    ${isUnstable(symbol)
      ? html`<div class="admonition warning pcdogs-symbol-warning">
          <p class="admonition-title">Unstable symbol</p>
          <p>
            This symbol is unstable and may change without warning in the
            future.
          </p>
        </div>`
      : null}
    <h1 class="pcdogs-symbol-heading">
      <span>${symbol.name}</span>
      <${HeaderVersionPicker}
        versions=${versions}
        selectedVersion=${effectiveVersion}
        onVersionChange=${onVersionChange}
      />
    </h1>
    <${Facts} rows=${symbol.facts} />
    <p class=${summaryClass(symbol.summary, "pcdogs-symbol-summary")}>
      ${text(symbol.summary)}
    </p>
    <${DetailsForKind} symbol=${symbol} />
    <${DetailRegionTables}
      symbol=${symbol}
      selectedVersion=${effectiveVersion}
      onSelect=${onSelect}
    />
    ${symbol.reference_hierarchy_paths?.length
      ? html`<div class="pcdogs-symbol-reference-tables">
          <${ReferenceCard} modifier="reference-hierarchy">
            <${Hierarchy}
              symbol=${symbol}
              byAnchor=${byAnchor}
              onSelect=${onSelect}
            />
          <//>
        </div>`
      : null}
  </article>`;
}

function Viewer({ root, src }) {
  const [status, setStatus] = useState("loading");
  const [symbols, setSymbols] = useState([]);
  const filtersStorageKey = useMemo(() => storageKey(root, src), [root, src]);
  const [selected, setSelected] = useState(selectedHash());
  const [selectedVersion, setSelectedVersion] = useState("EN");
  const [overviewFilters, setOverviewFilters] = useState(() =>
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

  const updateOverviewFilter = useCallback((name, value) => {
    setOverviewFilters((filters) =>
      normalizeStoredFilters({ ...filters, [name]: value }),
    );
  }, []);

  const select = useCallback(
    (anchor) => {
      if (anchor) {
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
      } else if (selected) {
        pendingOverviewRestore.current = true;
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

        return response.json();
      })
      .then((payload) => {
        setSymbols(Array.isArray(payload.symbols) ? payload.symbols : []);
        setStatus("ready");

        if (selectedHash()) {
          root.scrollIntoView({ block: "start", behavior: "smooth" });
        }
      })
      .catch((err) => {
        if (err.name === "AbortError") {
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

  const symbol = selected ? byAnchor.get(selected) || null : null;

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
    return html`<p>Loading symbol data...</p>`;
  }

  if (status === "error") {
    return html`<p>Could not load symbol data: ${error}</p>`;
  }

  if (selected && symbol) {
    return html`<${Detail}
      symbol=${symbol}
      byAnchor=${byAnchor}
      selectedVersion=${selectedVersion}
      onVersionChange=${setSelectedVersion}
      onSelect=${select}
    />`;
  }

  return html`${selected
      ? html`<p>Symbol not found: <code>${selected}</code></p>`
      : null}
    <${Overview}
      symbols=${symbols}
      filters=${overviewFilters}
      onFilterChange=${updateOverviewFilter}
      onSelect=${select}
    />`;
}

function mount(root) {
  if (root.dataset.pcdogsSymbolViewerMounted === "true") {
    return;
  }

  root.dataset.pcdogsSymbolViewerMounted = "true";
  render(
    html`<${Viewer}
      root=${root}
      src=${root.dataset.symbolsJson || "symbols-data.json"}
    />`,
    root,
  );
}

function initSymbolViewers() {
  document.querySelectorAll(ROOT_SELECTOR).forEach(mount);
}

if (document.readyState === "loading") {
  document.addEventListener("DOMContentLoaded", initSymbolViewers, {
    once: true,
  });
} else {
  initSymbolViewers();
}

if (window.document$?.subscribe) {
  window.document$.subscribe(() => initSymbolViewers());
}
