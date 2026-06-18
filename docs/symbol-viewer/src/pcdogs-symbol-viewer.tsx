import { render } from "preact";
import { ROOT_SELECTOR } from "./constants";
import { Viewer } from "./components/viewer";

declare global {
  interface Window {
    document$?: {
      subscribe(callback: () => void): void;
    };
  }
}

function mount(root: Element): void {
  if (!(root instanceof HTMLElement)) {
    return;
  }

  if (root.dataset.pcdogsSymbolViewerMounted === "true") {
    return;
  }

  root.dataset.pcdogsSymbolViewerMounted = "true";
  render(
    <Viewer root={root} src={root.dataset.symbolsJson || "pcdogs.symbols.v1.json"} />,
    root,
  );
}

function initSymbolViewers(): void {
  document.querySelectorAll(ROOT_SELECTOR).forEach(mount);
}

if (document.readyState === "loading") {
  document.addEventListener("DOMContentLoaded", initSymbolViewers, {
    once: true,
  });
} else {
  initSymbolViewers();
}

if (window.document$?.subscribe) {
  window.document$.subscribe(() => initSymbolViewers());
}
