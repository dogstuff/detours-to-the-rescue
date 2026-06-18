import type { ComponentChildren } from "preact";
import type { ExampleTab, ViewerSymbol } from "../types";
import { text } from "../utils";
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
      />
    );
  }

  if (symbol.kind === "data") {
    const tabs: ExampleTab[] = [
      {
        label: "Read",
        value: symbol.read_example,
        htmlValue: symbol.read_example_html,
      },
    ];

    if (symbol.can_write) {
      tabs.push({
        label: "Write",
        value: symbol.write_example,
        htmlValue: symbol.write_example_html,
      });
    }

    return (
      <>
        <p>
          <strong>Type:</strong> {text(symbol.type)}
        </p>
        <p>
          <strong>Write policy:</strong> {text(symbol.write_policy)}
        </p>
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
