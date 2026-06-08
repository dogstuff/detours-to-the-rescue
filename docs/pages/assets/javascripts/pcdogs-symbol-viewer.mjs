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

function classes(...items) {
  return items.filter(Boolean).join(" ");
}

function text(value) {
  return value == null || value === "" ? "-" : String(value);
}

function selectedHash() {
  return (
    new URLSearchParams(location.hash.replace(/^#/, "")).get("symbol") || ""
  );
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

function scrollTo(root) {
  root.scrollIntoView({ block: "start", behavior: "smooth" });
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
  return filter === "all" || (isUnstable(symbol) ? "unstable" : "stable") === filter;
}

function matchesVersion(symbol, filter) {
  return filter === "all" || text(symbol.builds).split(/\s+/).includes(filter);
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

function ExampleTabs({ symbol, tabs }) {
  const shown = tabs.filter((tab) => tab.value);

  if (!shown.length) {
    return null;
  }

  const base = `pcdogs-symbol-example-${symbol.anchor}`;

  return html`<div
    class="tabbed-set tabbed-alternate"
    data-tabs=${`${base}:${shown.length}`}
  >
    ${shown.map(
      (_, index) =>
        html`<input
          checked=${index === 0}
          id=${`${base}-${index + 1}`}
          name=${base}
          type="radio"
          key=${index}
        />`,
    )}
    <div class="tabbed-labels">
      ${shown.map(
        (tab, index) =>
          html`<label for=${`${base}-${index + 1}`} key=${tab.label}
            >${tab.label}</label
          >`,
      )}
    </div>
    <div class="tabbed-content">
      ${shown.map(
        (tab) =>
          html`<div class="tabbed-block" key=${tab.label}>
            <${CodeBlock} value=${tab.value} htmlValue=${tab.htmlValue} />
          </div>`,
      )}
    </div>
  </div>`;
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

function addressLabel(label) {
  return label.replace(/^Address \(([^)]+)\)$/, "Location ($1)");
}

function isAddressRow(row) {
  return row.label.startsWith("Address (");
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

function MetadataValue({ row }) {
  if (
    ["Patch Size", "Entry Patch Size"].includes(row.label) &&
    /^0x[0-9a-fA-F]+$/.test(row.value)
  ) {
    return html`<${HexValue} value=${row.value} />`;
  }

  return row.value;
}

function Metadata({ rows }) {
  if (!rows?.length) {
    return null;
  }

  const addressRows = [];
  const otherRows = [];

  for (const row of rows) {
    (isAddressRow(row) ? addressRows : otherRows).push(row);
  }

  return html`<div
    class="pcdogs-reference-table pcdogs-reference-table--metadata"
  >
    <${TableFrame}
      title="Resolution"
      extraClass="pcdogs-xref-table pcdogs-metadata-table"
    >
      <table class="pcdogs-type-table pcdogs-metadata-table__body">
        <tbody>
          ${[...addressRows, ...otherRows].map((row) => {
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
                ${isAddress ? addressLabel(row.label) : row.label}
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
      </table>
    <//>
  </div>`;
}

function offsetParts(offsets) {
  const value = text(offsets).trim();

  if (value === "-") {
    return { prefix: "", offset: "" };
  }

  if (value.startsWith("+")) {
    return { prefix: "+ ", offset: value.slice(1).trimStart() };
  }

  return { prefix: "+ ", offset: value };
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
            ${row.builds
              ? html`<span class="pcdogs-xref-table__provenance"
                  >${" "}${row.builds}</span
                >`
              : null}
          </div>
        </td>
      </tr>`,
  );
}

function References({ symbol, onSelect }) {
  const from = symbol.referenced_by || [];
  const to = symbol.references || [];

  if (!from.length && !to.length) {
    return null;
  }

  return html`<div class="pcdogs-reference-table pcdogs-reference-table--xref">
    <${TableFrame} title="References" extraClass="pcdogs-xref-table">
      <table class="pcdogs-type-table pcdogs-xref-table__body">
        <tbody>
          <${XRefRows} rows=${from} incoming=${true} onSelect=${onSelect} />
          <${XRefRows} rows=${to} incoming=${false} onSelect=${onSelect} />
        </tbody>
      </table>
    <//>
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
                >›</span
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

      if (!child) {
        return null;
      }

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
  if (!symbol.reference_hierarchy_paths?.length) {
    return null;
  }

  const tree = buildHierarchyTree(symbol.reference_hierarchy_paths);

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

function Overview({ symbols, onSelect }) {
  const [query, setQuery] = useState("");
  const [stabilityFilter, setStabilityFilter] = useState("all");
  const [kindFilter, setKindFilter] = useState("all");
  const [versionFilter, setVersionFilter] = useState("all");

  const filteredSymbols = useMemo(() => {
    const needle = query.trim().toLowerCase();

    return symbols
      .filter(
        (symbol) =>
          matchesStability(symbol, stabilityFilter) &&
          (kindFilter === "all" || symbol.kind === kindFilter) &&
          matchesVersion(symbol, versionFilter) &&
          matchesQuery(symbol, needle),
      )
      .sort((a, b) => overviewSortKey(a).localeCompare(overviewSortKey(b)));
  }, [symbols, query, stabilityFilter, kindFilter, versionFilter]);

  return html`<section class="pcdogs-symbol-overview">
    <h1>SDK Symbols</h1>
    <p>Reference for PCDogs symbols wrapped by the modding SDK.</p>
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
          value=${query}
          placeholder="Name, summary, or something else"
          onInput=${(event) => setQuery(event.currentTarget.value)}
        />
      </label>
      <label class="pcdogs-symbol-overview-control">
        <span>Stability</span>
        <select
          value=${stabilityFilter}
          onChange=${(event) => setStabilityFilter(event.currentTarget.value)}
        >
          <option value="all">All</option>
          <option value="stable">Stable</option>
          <option value="unstable">Unstable</option>
        </select>
      </label>
      <label class="pcdogs-symbol-overview-control">
        <span>Kind</span>
        <select
          value=${kindFilter}
          onChange=${(event) => setKindFilter(event.currentTarget.value)}
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
          value=${versionFilter}
          onChange=${(event) => setVersionFilter(event.currentTarget.value)}
        >
          <option value="all">All</option>
          ${VERSION_LABELS.map(
            (version) =>
              html`<option value=${version} key=${version}>${version}</option>`,
          )}
        </select>
      </label>
      <span class="pcdogs-symbol-overview-count"
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
          ${filteredSymbols.length
            ? filteredSymbols.map(
                (symbol) => html`<tr data-kind=${symbol.kind} key=${symbol.anchor}>
                  <td class="pcdogs-symbol-overview-name" data-label="Symbol">
                    <span class="pcdogs-symbol-overview-cell-scroll">
                      <a
                        href=${symbolHref(symbol.anchor)}
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
                            tabindex="0"
                            aria-label="Unstable symbol"
                            data-tooltip="Unstable symbol"
                            >⚠</span
                          >`
                        : null}
                      <span class="pcdogs-symbol-overview-meta"
                        >${KIND_LABELS[symbol.kind] || text(symbol.kind)}</span
                      >
                    </span>
                  </td>
                  <td
                    class="pcdogs-symbol-overview-summary"
                    data-label="Summary"
                  >
                    <span
                      class=${summaryClass(
                        symbol.summary,
                        "pcdogs-symbol-overview-summary-text",
                      )}
                      title=${text(symbol.summary)}
                      >${text(symbol.summary)}</span
                    >
                  </td>
                  <td
                    class="pcdogs-symbol-overview-versions"
                    data-label="Versions"
                  >
                    ${versionsLabel(symbol)}
                  </td>
                </tr>`,
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

function Detail({ symbol, byAnchor, onSelect }) {
  const frame = useDetailFramePosition();

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
    <h1 class="pcdogs-symbol-heading">${symbol.name}</h1>
    <${Facts} rows=${symbol.facts} />
    ${isUnstable(symbol)
      ? html`<div class="admonition warning pcdogs-symbol-warning">
          <p class="admonition-title">Unstable symbol</p>
          <p>
            This symbol is generated from unstable SDK metadata and may change
            between releases.
          </p>
        </div>`
      : null}
    <p class=${summaryClass(symbol.summary, "pcdogs-symbol-summary")}>
      ${text(symbol.summary)}
    </p>
    <div class="pcdogs-block-heading">Examples</div>
    <${DetailsForKind} symbol=${symbol} />
    <div class="pcdogs-symbol-reference-tables">
      ${symbol.metadata?.length
        ? html`<div class="pcdogs-symbol-reference-tables__metadata">
            <${Metadata} rows=${symbol.metadata} />
          </div>`
        : null}
      ${symbol.references?.length || symbol.referenced_by?.length
        ? html`<div
            class="pcdogs-symbol-reference-tables__reference-card pcdogs-symbol-reference-tables__reference-card--references"
          >
            <${References} symbol=${symbol} onSelect=${onSelect} />
          </div>`
        : null}
      ${symbol.reference_hierarchy_paths?.length
        ? html`<div
            class="pcdogs-symbol-reference-tables__reference-card pcdogs-symbol-reference-tables__reference-card--reference-hierarchy"
          >
            <${Hierarchy}
              symbol=${symbol}
              byAnchor=${byAnchor}
              onSelect=${onSelect}
            />
          </div>`
        : null}
    </div>
  </article>`;
}

function Viewer({ root, src }) {
  const [status, setStatus] = useState("loading");
  const [symbols, setSymbols] = useState([]);
  const [selected, setSelected] = useState(selectedHash());
  const [error, setError] = useState("");

  const select = useCallback(
    (anchor) => {
      selectHash(anchor);
      setSelected(anchor);
    },
    [],
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
          scrollTo(root);
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

  const byAnchor = useMemo(
    () => new Map(symbols.map((symbol) => [symbol.anchor, symbol])),
    [symbols],
  );
  const symbol = selected ? byAnchor.get(selected) || null : null;

  if (status === "loading") {
    return html`<p>Loading symbol data…</p>`;
  }

  if (status === "error") {
    return html`<p>Could not load symbol data: ${error}</p>`;
  }

  if (selected && symbol) {
    return html`<${Detail}
      symbol=${symbol}
      byAnchor=${byAnchor}
      onSelect=${select}
    />`;
  }

  return html`${selected
      ? html`<p>Symbol not found: <code>${selected}</code></p>`
      : null} <${Overview} symbols=${symbols} onSelect=${select} />`;
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
