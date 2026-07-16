import type { JSX } from "preact";
import type { ViewerSymbol } from "../types";
import { text } from "../utils";
import { TableFrame } from "./common";
import { TypeText } from "./type-link";

export function TypeDetails({ symbol }: { symbol: ViewerSymbol }): JSX.Element {
  return (
    <>
      {symbol.members?.length ? (
        <TableFrame title="Members">
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
              {symbol.members.map((member, index) => (
                <tr key={`${text(member.offset)}:${text(member.name)}:${index}`}>
                  <td>
                    <code>{text(member.offset)}</code>
                  </td>
                  <td>
                    <div class="pcdogs-type-table__cell-scroll">
                      <code>{text(member.name)}</code>
                    </div>
                  </td>
                  <td>
                    <div class="pcdogs-type-table__cell-scroll">
                      <code><TypeText value={member.type} /></code>
                    </div>
                  </td>
                  <td>{text(member.doc)}</td>
                </tr>
              ))}
            </tbody>
          </table>
        </TableFrame>
      ) : null}
      {symbol.enum_values?.length ? (
        <TableFrame title="Values">
          <table class="pcdogs-type-table pcdogs-type-table--enum">
            <thead>
              <tr>
                <th>Value</th>
                <th>Name</th>
                <th>Notes</th>
              </tr>
            </thead>
            <tbody>
              {symbol.enum_values.map((value, index) => (
                <tr key={`${text(value.value)}:${text(value.name)}:${index}`}>
                  <td>
                    <code>{text(value.value)}</code>
                  </td>
                  <td>
                    <code>{text(value.name)}</code>
                  </td>
                  <td>{text(value.table_doc || value.doc)}</td>
                </tr>
              ))}
            </tbody>
          </table>
        </TableFrame>
      ) : null}
    </>
  );
}
