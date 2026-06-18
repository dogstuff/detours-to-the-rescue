import type { JSX } from "preact";
import { text } from "../utils";

function normalizeSignedOffsets(value: unknown): string {
  return text(value).replace(/([+-])\s*(0x[0-9a-fA-F]+)/g, "$1 $2");
}

export function offsetParts(offsets: unknown): { prefix: string; offset: string } {
  const value = normalizeSignedOffsets(offsets).trim();

  if (value === "-") {
    return { prefix: "", offset: "" };
  }

  const signed = /^([+-])\s+(.*)$/.exec(value);

  if (signed) {
    return { prefix: ` ${signed[1]} `, offset: signed[2] || "" };
  }

  return { prefix: " + ", offset: value };
}

export function XRefOffset({ offsets }: { offsets: unknown }): JSX.Element {
  const { prefix, offset } = offsetParts(offsets);

  return (
    <>
      <span class="pcdogs-xref-table__location-offset-prefix">{prefix}</span>
      <span class="pcdogs-xref-table__location-offset">{offset}</span>
    </>
  );
}
