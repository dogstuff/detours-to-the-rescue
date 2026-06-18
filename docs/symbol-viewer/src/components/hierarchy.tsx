import type { JSX } from "preact";
import { useMemo } from "preact/hooks";
import type { HierarchyNode, ViewerSymbol } from "../types";
import { symbolHref } from "../navigation";
import { classes } from "../utils";
import { TableFrame } from "./common";

type HierarchyContext = {
  current: string;
  byAnchor: Map<string, ViewerSymbol>;
  onSelect: (anchor: string) => void;
};

export function childAnchors(node: HierarchyNode): string[] {
  return [...node.children.keys()];
}

export function buildHierarchyTree(paths: string[][]): HierarchyNode {
  const root: HierarchyNode = { children: new Map() };

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

export function compactHierarchyPath(
  anchor: string,
  node: HierarchyNode,
): { anchors: string[]; node: HierarchyNode } {
  const anchors = [anchor];
  let current = node;
  let children = childAnchors(current);

  while (children.length === 1 && !current.terminal) {
    const next = children[0];
    if (!next) {
      break;
    }

    const child = current.children.get(next);
    if (!child) {
      break;
    }

    anchors.push(next);
    current = child;
    children = childAnchors(current);
  }

  return { anchors, node: current };
}

export function sortedHierarchyAnchors(
  node: HierarchyNode,
  byAnchor: Map<string, ViewerSymbol>,
): string[] {
  return childAnchors(node).sort((left, right) => {
    const leftSymbol = byAnchor.get(left);
    const rightSymbol = byAnchor.get(right);
    return `${leftSymbol?.name || left}\0${left}`.localeCompare(
      `${rightSymbol?.name || right}\0${right}`,
    );
  });
}

export function HierarchyPathView({
  anchors,
  current,
  byAnchor,
  onSelect,
}: HierarchyContext & { anchors: string[] }): JSX.Element {
  return (
    <span class="pcdogs-reference-hierarchy-tree__path">
      {anchors.map((anchor, index) => {
        const target = byAnchor.get(anchor);
        const className = classes(
          "pcdogs-reference-hierarchy-tree__symbol",
          `pcdogs-reference-hierarchy-tree__symbol--${target?.kind || "symbol"}`,
          anchor === current &&
            "pcdogs-reference-hierarchy-tree__symbol--current",
        );

        return (
          <span
            class="pcdogs-reference-hierarchy-tree__run"
            key={`${anchor}:${index}`}
          >
            {index ? (
              <span
                class="pcdogs-reference-hierarchy-tree__separator"
                aria-hidden="true"
              >
                {">"}
              </span>
            ) : null}
            <a
              class={className}
              href={symbolHref(anchor)}
              aria-current={anchor === current ? "page" : undefined}
              onClick={(event) => {
                event.preventDefault();
                event.stopPropagation();
                onSelect(anchor);
              }}
            >
              {target?.name || anchor}
            </a>
          </span>
        );
      })}
    </span>
  );
}

export function HierarchyBranch({
  node,
  depth,
  current,
  byAnchor,
  onSelect,
}: HierarchyContext & { node: HierarchyNode; depth: number }): JSX.Element {
  const listClass = classes(
    "pcdogs-reference-hierarchy-tree__list",
    depth && "pcdogs-reference-hierarchy-tree__children",
  );

  return (
    <ul class={listClass}>
      {sortedHierarchyAnchors(node, byAnchor).map((anchor) => {
        const child = node.children.get(anchor);
        if (!child) {
          return null;
        }

        const compacted = compactHierarchyPath(anchor, child);
        const children = childAnchors(compacted.node);
        const hasChildren = children.length > 0;
        const rowClass = `pcdogs-reference-hierarchy-tree__row pcdogs-reference-hierarchy-tree__row--${hasChildren ? "branch" : "leaf"}`;
        const pathView = (
          <HierarchyPathView
            anchors={compacted.anchors}
            current={current}
            byAnchor={byAnchor}
            onSelect={onSelect}
          />
        );

        return (
          <li
            class="pcdogs-reference-hierarchy-tree__item"
            key={`${depth}:${compacted.anchors.join(":")}`}
          >
            {hasChildren ? (
              <details class="pcdogs-reference-hierarchy-tree__branch" open>
                <summary class={rowClass}>{pathView}</summary>
                <HierarchyBranch
                  node={compacted.node}
                  depth={depth + 1}
                  current={current}
                  byAnchor={byAnchor}
                  onSelect={onSelect}
                />
              </details>
            ) : (
              <div class={rowClass}>{pathView}</div>
            )}
          </li>
        );
      })}
    </ul>
  );
}

export function Hierarchy({
  symbol,
  byAnchor,
  onSelect,
}: {
  symbol: ViewerSymbol;
  byAnchor: Map<string, ViewerSymbol>;
  onSelect: (anchor: string) => void;
}): JSX.Element | null {
  const paths = symbol.reference_hierarchy_paths;
  const tree = useMemo(() => buildHierarchyTree(paths), [paths]);

  if (!paths.length) {
    return null;
  }

  return (
    <div class="pcdogs-reference-table pcdogs-reference-table--reference-hierarchy">
      <TableFrame title="Entrypoint Reference Hierarchy" extraClass="pcdogs-xref-table">
        <nav
          class="pcdogs-reference-hierarchy-tree"
          aria-label="Entrypoint reference hierarchy"
        >
          <HierarchyBranch
            node={tree}
            depth={0}
            current={symbol.anchor}
            byAnchor={byAnchor}
            onSelect={onSelect}
          />
        </nav>
      </TableFrame>
    </div>
  );
}
