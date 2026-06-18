import type { JSX } from "preact";
import type { ViewerSymbol, XRefRow } from "../types";
import { buildLabels, slug, text, upperBuild } from "../utils";
import { TableFrame } from "./common";
import { Metadata, metadataRowsForVersion } from "./metadata";
import { XRefOffset } from "./xref-offset";

export function XRefRows({
  rows,
  incoming,
  onSelect,
}: {
  rows: XRefRow[];
  incoming: boolean;
  onSelect: (anchor: string) => void;
}): JSX.Element[] {
  const direction = incoming ? "reference-from" : "reference-to";
  const arrow = incoming ? "<-" : "->";

  return rows.map((row, index) => (
    <tr
      class={`pcdogs-xref-table__row pcdogs-xref-table__row--${direction} pcdogs-xref-table__row--${slug(row.kind)}`}
      key={`${row.target_anchor || text(row.text)}:${index}`}
    >
      <td class="pcdogs-xref-table__arrow">{arrow}</td>
      <td class="pcdogs-xref-table__relation">{text(row.kind)}</td>
      <td class="pcdogs-xref-table__name">
        <div class="pcdogs-xref-table__cell-scroll">
          {row.target_anchor ? (
            <button
              type="button"
              class="pcdogs-symbol-link-button pcdogs-xref-table__symbol-link"
              onClick={() => onSelect(row.target_anchor!)}
            >
              {text(row.text || row.value)}
            </button>
          ) : (
            text(row.text || row.value)
          )}
          <XRefOffset offsets={row.offsets} />
        </div>
      </td>
    </tr>
  ));
}

export function rowMatchesVersion(row: XRefRow, version: string): boolean {
  const builds = buildLabels(row.builds).map(upperBuild);
  return !builds.length || builds.includes(upperBuild(version));
}

export function ReferenceTable({
  from,
  to,
  onSelect,
}: {
  from: XRefRow[];
  to: XRefRow[];
  onSelect: (anchor: string) => void;
}): JSX.Element {
  if (!from.length && !to.length) {
    return <p class="pcdogs-xref-table__empty">No references for this region.</p>;
  }

  return (
    <table class="pcdogs-type-table pcdogs-xref-table__body">
      <tbody>
        <XRefRows rows={from} incoming={true} onSelect={onSelect} />
        <XRefRows rows={to} incoming={false} onSelect={onSelect} />
      </tbody>
    </table>
  );
}

export function DetailRegionTables({
  symbol,
  selectedVersion,
  onSelect,
}: {
  symbol: ViewerSymbol;
  selectedVersion: string;
  onSelect: (anchor: string) => void;
}): JSX.Element | null {
  const metadataRows = symbol.metadata;
  const referencedBy = symbol.referenced_by;
  const references = symbol.references;
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

  return (
    <div class="pcdogs-symbol-reference-tables">
      {hasMetadata ? (
        <div class="pcdogs-symbol-reference-tables__metadata">
          <Metadata rows={filteredMetadataRows} />
        </div>
      ) : null}
      {hasReferences ? (
        <div class="pcdogs-symbol-reference-tables__reference-card pcdogs-symbol-reference-tables__reference-card--references">
          <div class="pcdogs-reference-table pcdogs-reference-table--xref">
            <TableFrame title="References" extraClass="pcdogs-xref-table">
              <ReferenceTable
                from={filteredReferencedBy}
                to={filteredReferences}
                onSelect={onSelect}
              />
            </TableFrame>
          </div>
        </div>
      ) : null}
    </div>
  );
}
