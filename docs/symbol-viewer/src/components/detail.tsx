import type { JSX, RefObject } from "preact";
import { useEffect, useRef, useState } from "preact/hooks";
import {
  DETAIL_BUTTON_INSET_PX,
  UNSTABLE_SYMBOL_WARNING,
  VERSION_LABELS,
} from "../constants";
import type { VersionLabel, ViewerSymbol } from "../types";
import {
  buildLabels,
  classes,
  isOneOf,
  isUnstable,
  summaryClass,
  text,
} from "../utils";
import { Facts } from "./common";
import { DetailsForKind } from "./details-kind";
import { Hierarchy } from "./hierarchy";
import { DetailRegionTables } from "./references";
import { TypeText } from "./type-link";

export function useDetailFramePosition(): {
  detailRef: RefObject<HTMLElement>;
  frameLeft: string;
  frameTop: string;
} {
  const detailRef = useRef<HTMLElement>(null);
  const inset = `${DETAIL_BUTTON_INSET_PX}px`;
  const [frameLeft, setFrameLeft] = useState(inset);
  const [frameTop, setFrameTop] = useState(inset);

  useEffect(() => {
    const detail = detailRef.current;

    if (!detail) {
      return;
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

export function supportedVersions(symbol: ViewerSymbol): VersionLabel[] {
  const versions = buildLabels(symbol.builds).filter((version) =>
    isOneOf(VERSION_LABELS, version),
  );

  return versions.length ? versions : [...VERSION_LABELS];
}

export function HeaderVersionPicker({
  versions,
  selectedVersion,
  onVersionChange,
}: {
  versions: VersionLabel[];
  selectedVersion: VersionLabel;
  onVersionChange: (version: VersionLabel) => void;
}): JSX.Element {
  return (
    <span
      class="pcdogs-symbol-detail-versions"
      role="group"
      aria-label="Symbol versions"
    >
      {versions.map((version) => {
        const selected = version === selectedVersion;

        return (
          <button
            type="button"
            class={classes(
              "pcdogs-symbol-detail-version",
              selected && "pcdogs-symbol-detail-version--selected",
            )}
            aria-pressed={selected}
            onClick={() => onVersionChange(version)}
            key={version}
          >
            {version}
          </button>
        );
      })}
    </span>
  );
}

export function Detail({
  symbol,
  byAnchor,
  selectedVersion,
  onVersionChange,
  onSelect,
}: {
  symbol: ViewerSymbol;
  byAnchor: Map<string, ViewerSymbol>;
  selectedVersion: VersionLabel;
  onVersionChange: (version: VersionLabel) => void;
  onSelect: (anchor: string) => void;
}): JSX.Element {
  const frame = useDetailFramePosition();
  const versions = supportedVersions(symbol);
  const effectiveVersion = versions.includes(selectedVersion)
    ? selectedVersion
    : versions[0] || VERSION_LABELS[0];

  return (
    <article
      ref={frame.detailRef}
      class="pcdogs-symbol-detail pcdogs-symbol-detail-page"
      style={
        {
          "--pcdogs-detail-left": frame.frameLeft,
          "--pcdogs-detail-top": frame.frameTop,
        } as JSX.CSSProperties
      }
    >
      <button
        type="button"
        class="pcdogs-symbol-link-button pcdogs-symbol-detail-back"
        onClick={() => onSelect("")}
      >
        {"<- Back to symbol overview"}
      </button>
      {isUnstable(symbol) ? (
        <div class="admonition warning pcdogs-symbol-warning">
          <p class="admonition-title">Unstable symbol</p>
          <p>{UNSTABLE_SYMBOL_WARNING}</p>
        </div>
      ) : null}
      <h1 class="pcdogs-symbol-heading">
        <span>{symbol.name}</span>
        <HeaderVersionPicker
          versions={versions}
          selectedVersion={effectiveVersion}
          onVersionChange={onVersionChange}
        />
      </h1>
      <Facts rows={symbol.facts} />
      <p class={summaryClass(symbol.summary, "pcdogs-symbol-summary")}>
        <TypeText value={symbol.summary} />
      </p>
      <DetailsForKind symbol={symbol} />
      <DetailRegionTables
        symbol={symbol}
        selectedVersion={effectiveVersion}
        onSelect={onSelect}
      />
      {symbol.reference_hierarchy_paths.length ? (
        <div class="pcdogs-symbol-reference-tables">
          <div class="pcdogs-symbol-reference-tables__reference-card pcdogs-symbol-reference-tables__reference-card--reference-hierarchy">
            <Hierarchy symbol={symbol} byAnchor={byAnchor} onSelect={onSelect} />
          </div>
        </div>
      ) : null}
    </article>
  );
}
