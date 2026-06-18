import type { ComponentChildren, JSX } from "preact";
import type { MetadataRow } from "../types";
import { classes, text } from "../utils";
import { TableFrame } from "./common";
import { XRefOffset } from "./xref-offset";

export function metadataLabel(row: MetadataRow, isAddress: boolean): string {
  if (!isAddress || row.label === "Image Base") {
    return row.label;
  }

  return "Location";
}

export function isAddressRow(row: MetadataRow): boolean {
  return (
    row.label === "Address" || row.label === "RVA" || row.label === "Image Base"
  );
}

export function hexValueText(value: unknown): string {
  if (typeof value === "number" && Number.isSafeInteger(value)) {
    return `0x${value.toString(16).toUpperCase()}`;
  }

  return text(value);
}

export function HexValue({
  value,
  split = false,
}: {
  value: unknown;
  split?: boolean;
}): ComponentChildren {
  const formatted = hexValueText(value);
  const match = formatted.match(/0x[0-9a-fA-F]+/);

  if (!match) {
    return formatted;
  }

  if (!split) {
    return <span class="pcdogs-address-table__hex">{formatted}</span>;
  }

  const start = match.index ?? 0;
  const end = start + match[0].length;

  return (
    <>
      <span class="pcdogs-address-table__prefix">
        {formatted.slice(0, start)}
      </span>
      <span class="pcdogs-address-table__hex">{formatted.slice(start, end)}</span>
      <span class="pcdogs-address-table__prefix">{formatted.slice(end)}</span>
    </>
  );
}

export function ResolverValue({ value }: { value: unknown }): ComponentChildren {
  const valueText = text(value);
  const match = /^(.*?)\s+([+-].*)$/.exec(valueText.trim());

  if (!match) {
    return valueText;
  }

  return (
    <span class="pcdogs-metadata-table__resolver">
      <code class="pcdogs-metadata-table__resolver-name">{match[1]}</code>
      <XRefOffset offsets={match[2]} />
    </span>
  );
}

export function MetadataValue({ row }: { row: MetadataRow }): ComponentChildren {
  const value = row.value;

  if (/^-?0x[0-9a-fA-F]+$/.test(text(value))) {
    return <HexValue value={value} />;
  }

  if (row.label === "AOB Pattern" || row.label === "Signature") {
    return <code>{text(value)}</code>;
  }

  if (row.label === "Resolver") {
    return <ResolverValue value={value} />;
  }

  return text(value);
}

export function MetadataTable({ rows }: { rows: MetadataRow[] }): JSX.Element {
  return (
    <table class="pcdogs-type-table pcdogs-metadata-table__body">
      <tbody>
        {rows.map((row) => {
          const isAddress = isAddressRow(row);

          return (
            <tr
              class={classes(
                "pcdogs-metadata-table__row",
                isAddress && "pcdogs-metadata-table__row--address",
              )}
              key={`${row.label}:${row.build ?? ""}:${text(row.value)}`}
            >
              <th
                scope="row"
                class={classes(
                  "pcdogs-metadata-table__label",
                  isAddress && "pcdogs-address-table__version",
                )}
              >
                {metadataLabel(row, isAddress)}
              </th>
              <td
                class={classes(
                  "pcdogs-metadata-table__value",
                  isAddress && "pcdogs-address-table__address",
                )}
              >
                {isAddress ? (
                  <HexValue value={row.value} split={true} />
                ) : (
                  <MetadataValue row={row} />
                )}
              </td>
            </tr>
          );
        })}
      </tbody>
    </table>
  );
}

export function metadataRowsForVersion(
  rows: MetadataRow[],
  version: string,
): MetadataRow[] {
  return rows
    .filter((row) => !row.build || row.build === version)
    .sort((a, b) => Number(isAddressRow(b)) - Number(isAddressRow(a)));
}

export function Metadata({ rows }: { rows: MetadataRow[] }): JSX.Element | null {
  if (!rows.length) {
    return null;
  }

  return (
    <div class="pcdogs-reference-table pcdogs-reference-table--metadata">
      <TableFrame title="Metadata" extraClass="pcdogs-xref-table pcdogs-metadata-table">
        <MetadataTable rows={rows} />
      </TableFrame>
    </div>
  );
}
