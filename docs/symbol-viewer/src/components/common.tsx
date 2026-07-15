import type { ComponentChildren, JSX } from "preact";
import type { ExampleTab, MetadataRow, ViewerSymbol } from "../types";
import { classes, text } from "../utils";

type ChildrenProps = { children?: ComponentChildren };

export function TableFrame({
  title,
  extraClass = "",
  children,
}: ChildrenProps & { title: string; extraClass?: string }): JSX.Element {
  return (
    <div class={classes("pcdogs-type-table-frame", extraClass)}>
      <div class="pcdogs-table-title">{title}</div>
      {children}
    </div>
  );
}

function CodeBlock({ value }: { value?: unknown }): JSX.Element {
  return (
    <div class="highlight">
      <pre>
        <code class="language-c">{text(value)}</code>
      </pre>
    </div>
  );
}

function TabbedSet<T extends { label: string }>({
  base,
  tabs,
  renderTab,
}: {
  base: string;
  tabs: T[];
  renderTab: (tab: T) => ComponentChildren;
}): JSX.Element | null {
  if (!tabs.length) {
    return null;
  }

  return (
    <div
      class="tabbed-set tabbed-alternate"
      data-tabs={`${base}:${tabs.length}`}
    >
      {tabs.map((_, index) => (
        <input
          defaultChecked={index === 0}
          id={`${base}-${index + 1}`}
          name={base}
          type="radio"
          key={index}
        />
      ))}
      <div class="tabbed-labels">
        {tabs.map((tab, index) => (
          <label for={`${base}-${index + 1}`} key={tab.label}>
            {tab.label}
          </label>
        ))}
      </div>
      <div class="tabbed-content">
        {tabs.map((tab) => (
          <div class="tabbed-block" key={tab.label}>
            {renderTab(tab)}
          </div>
        ))}
      </div>
    </div>
  );
}

export function ExampleTabs({
  symbol,
  tabs,
}: {
  symbol: ViewerSymbol;
  tabs: ExampleTab[];
}): JSX.Element {
  return (
    <TabbedSet
      base={`pcdogs-symbol-example-${symbol.anchor}`}
      tabs={tabs.filter((tab) => tab.value)}
      renderTab={(tab) => <CodeBlock value={tab.value} />}
    />
  );
}

export function Facts({
  rows,
}: {
  rows?: MetadataRow[] | undefined;
}): JSX.Element | null {
  if (!rows?.length) {
    return null;
  }

  return (
    <div class="pcdogs-symbol-facts">
      {rows.map((row) => (
        <p class="pcdogs-symbol-fact" key={`${row.label}:${text(row.value)}`}>
          <span class="pcdogs-symbol-fact__label">{row.label}</span>
          <strong
            class={classes(
              "pcdogs-symbol-fact__value",
              row.label === "Type" && "pcdogs-symbol-fact__value--type",
            )}
          >
            {text(row.value)}
          </strong>
        </p>
      ))}
    </div>
  );
}
