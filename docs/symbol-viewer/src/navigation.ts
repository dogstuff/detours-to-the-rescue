export function selectedHash(): string {
  return new URLSearchParams(location.hash.slice(1)).get("symbol") || "";
}

export function symbolHref(anchor: string): string {
  return anchor ? `#${new URLSearchParams({ symbol: anchor })}` : "#";
}

export function selectHash(anchor: string): void {
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
