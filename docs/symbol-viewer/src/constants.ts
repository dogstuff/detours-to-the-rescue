export const ROOT_SELECTOR = "#pcdogs-symbol-viewer";

export const UNSTABLE_SYMBOL_WARNING = "Unstable; may change without warning.";

export const KIND_ORDER = ["function", "data", "type", "signature"] as const;

export const KIND_LABELS: Record<(typeof KIND_ORDER)[number], string> = {
  function: "Function",
  data: "Data",
  type: "Type",
  signature: "Signature",
};

export const VERSION_LABELS = ["EN", "EU", "SC"] as const;

export const FILTER_DEFAULTS = {
  query: "",
  stability: "all",
  kind: "all",
  version: "all",
  limit: "100",
} as const;

export const FILTER_OPTIONS = {
  stability: ["all", "stable", "unstable"],
  kind: ["all", ...KIND_ORDER],
  version: ["all", ...VERSION_LABELS],
  limit: ["100", "250", "500", "1000", "all"],
} as const;

export const CATEGORY_DISPLAY_OVERRIDES: Record<string, string> = {
  d3d: "D3D",
  ddraw: "DDraw",
  dinput: "DInput",
  pkg: "PKG",
  ui: "UI",
  win32: "Win32",
};

export const DEFAULT_BUILDS = ["en", "eu", "sc"] as const;
export const DETAIL_BUTTON_INSET_PX = 12;
