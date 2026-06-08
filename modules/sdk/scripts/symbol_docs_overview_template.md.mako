---
hide:
  - toc
description: "Reference for PC symbols wrapped by the SDK."
tags:
  - "PCDOGS"
  - "SDK Symbol"
  - "SDK"
  - "Symbol Reference"
---

<%!
import html
import re

KIND_LABELS = {
    "function": "Function",
    "data": "Data",
    "type": "Type",
    "signature": "Signature",
}

ALL_BUILDS = "EN EU SC"


def kind_label(value):
    return KIND_LABELS.get(str(value), str(value).title())


def symbol_name_html(name):
    return html.escape(str(name)).replace("_", "_<wbr>")


def symbol_meta(row):
    parts = [kind_label(row.kind)]
    builds = str(row.builds).strip()
    if builds and builds != ALL_BUILDS:
        parts.append(builds)

    return html.escape("; ".join(parts))


def strip_tags(value):
    return re.sub(r"<[^>]*>", "", str(value))


def summary_title(row):
    return html.escape(html.unescape(strip_tags(row.summary)), quote=True)
%>
# SDK Symbols

<div class="pcdogs-symbol-docs" aria-hidden="true"></div>

Reference for PC symbols wrapped by the SDK.

<%text>## All symbols</%text>

<div class="pcdogs-type-table-frame">
<table class="pcdogs-type-table pcdogs-symbol-overview-table">
<thead>
<tr><th class="pcdogs-symbol-overview-name" scope="col">Symbol</th><th class="pcdogs-symbol-overview-summary" scope="col">Summary</th><th class="pcdogs-symbol-overview-stability" scope="col">Stability</th></tr>
</thead>
<tbody>
% for row in rows:
<tr data-kind="${row.kind}" data-stability="${row.stability_slug}"><td class="pcdogs-symbol-overview-name" data-label="Symbol"><span class="pcdogs-symbol-overview-cell-scroll"><a href="${row.href}"><code>${symbol_name_html(row.name)}</code></a><span class="pcdogs-symbol-overview-meta">${symbol_meta(row)}</span></span></td><td class="pcdogs-symbol-overview-summary" data-label="Summary"><span class="pcdogs-symbol-overview-summary-text" title="${summary_title(row)}">${row.summary}</span></td><td class="pcdogs-symbol-overview-stability" data-label="Stability"><span class="pcdogs-symbol-overview-stability-value pcdogs-symbol-overview-stability-value--${row.stability_slug}">${row.stability}</span></td></tr>
% endfor

</tbody>
</table>
</div>
