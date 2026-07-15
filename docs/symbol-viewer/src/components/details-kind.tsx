import type { ComponentChildren } from "preact";
import type { ExampleTab, ViewerSymbol } from "../types";
import { ExampleTabs } from "./common";
import { TypeDetails } from "./type-details";

export function DetailsForKind({ symbol }: { symbol: ViewerSymbol }): ComponentChildren {
  if (symbol.kind === "function") {
    return (
      <ExampleTabs
        symbol={symbol}
        tabs={[
          {
            label: "Original Call Format",
            value: symbol.pseudo_usage,
          },
          {
            label: "C SDK Call",
            value: symbol.call_example,
          },
          {
            label: "C SDK Patch Spec",
            value: symbol.patch_spec_example,
          },
          {
            label: "C SDK Hook",
            value: symbol.hook_example,
          },
        ]}
      />
    );
  }

  if (symbol.kind === "data") {
    const tabs: ExampleTab[] = [
      {
        label: "Read",
        value: symbol.read_example,
      },
    ];

    if (symbol.can_write) {
      tabs.push({
        label: "Write",
        value: symbol.write_example,
      });
    }

    return (
      <>
        {symbol.untyped_note ? (
          <p class="pcdogs-untyped-data">{symbol.untyped_note}</p>
        ) : null}
        {symbol.write_policy_note ? (
          <p class="pcdogs-untyped-data">{symbol.write_policy_note}</p>
        ) : null}
        <ExampleTabs symbol={symbol} tabs={tabs} />
      </>
    );
  }

  if (symbol.kind === "type") {
    return <TypeDetails symbol={symbol} />;
  }

  return null;
}
