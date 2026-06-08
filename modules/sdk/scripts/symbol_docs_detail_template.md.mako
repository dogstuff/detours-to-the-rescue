<%!
import html
import re

POLICY_TOOLTIPS = {
    "UNKNOWN": "Normal Write() is disabled because this symbol has not been classified enough.",
    "READ_ONLY": "Normal Write() is disabled because this is decoded table or lookup data for inspection.",
    "ENGINE_MANAGED": "Normal Write() is disabled because this is live game-managed state that the game may replace or overwrite.",
    "RAW_MEMORY": "Normal Write() may update it after availability and memory checks.",
    "PATCH_ONLY": "Change through patch or hook flows instead of direct data writes.",
}


def tooltip_attrs(text):
    value = html.escape(str(text), quote=True)
    return f'title="{value}" aria-label="{value}"'


def policy_class(value):
    return html.escape(str(value).lower().replace("_", "-"), quote=True)


def policy_value(value):
    value = str(value)
    tooltip = POLICY_TOOLTIPS.get(value, "Data write policy.")
    return (
        f'<span class="pcdogs-policy pcdogs-policy--{policy_class(value)}" '
        f'{tooltip_attrs(tooltip)}>'
        f'{html.escape(value)}'
        '</span>'
    )


def symbol_heading_class(anchor):
    classes = ["pcdogs-symbol-heading"]
    if str(anchor).startswith("unstable-"):
        classes.append("pcdogs-symbol-heading--unstable")

    return " ".join(f".{value}" for value in classes)


def toc_label(value, category):
    text = str(value)
    if str(category) != "misc" and "_" in text:
        text = text.split("_", 1)[1]

    return html.escape(text, quote=True)


def symbol_name_label(card):
    return html.escape(plain_text(getattr(card, "name", "")) or "symbol", quote=True)

def xref_kind_class(value):
    return html.escape(str(value or "reference").lower().replace(" ", "-"), quote=True)


def title_label(value, fallback="Reference"):
    text = str(value or fallback).replace("_", " ").strip() or fallback
    return html.escape(text.title(), quote=True)


def xref_kind_note(row):
    kind = title_label(row.kind)
    builds = str(row.builds or "").strip()
    if builds and builds != "All":
        return f"({kind}; {html.escape(builds, quote=True)})"

    return f"({kind})"


def strip_tags(value):
    return re.sub(r"<[^>]*>", "", str(value))


KIND_LABELS = {
    "function": "Function",
    "data": "Data",
    "type": "Type",
    "signature": "Signature",
}


def plain_text(value):
    return " ".join(html.unescape(strip_tags(value)).split())


def yaml_string(value):
    return html.unescape(str(value)).replace("\\", "\\\\").replace('"', '\\"')


def kind_label(kind):
    return KIND_LABELS.get(str(kind), str(kind).title())


def front_matter(card, kind):
    label = kind_label(kind)
    name = plain_text(getattr(card, "name", "")) or "symbol"
    summary = plain_text(getattr(card, "summary", ""))
    category = plain_text(getattr(card, "category", "")).title()
    stability = (
        "Unstable"
        if str(getattr(card, "anchor", "")).startswith("unstable-")
        else "Stable"
    )
    tags = [
        "PCDOGS",
        "SDK Symbol",
        label,
        category,
        *str(getattr(card, "builds", "")).split(),
        stability,
    ]
    tags = list(dict.fromkeys(tag for tag in tags if tag and tag != "All"))
    title = f"SDK {label} Symbol: {name}"
    description = " ".join(part for part in (f"{title}.", summary) if part)

    return (
        "---\n"
        "hide:\n"
        "  - toc\n"
        f'title: "{yaml_string(title)}"\n'
        f'description: "{yaml_string(description)}"\n'
        "tags:\n"
        + "\n".join(f'  - "{yaml_string(tag)}"' for tag in tags)
        + "\n"
        "---"
    )


def call_hierarchy_aria_label(card):
    name = html.unescape(strip_tags(getattr(card, "name", ""))).strip() or "symbol"
    return html.escape(f"Call hierarchy for {name}", quote=True)


def xref_type_label(value, arrow):
    text = html.unescape(str(value or "reference")).strip() or "reference"
    if text == "-":
        text = "reference"

    text = re.sub(r"^detected\s+", "", text, flags=re.IGNORECASE)
    incoming = str(arrow) == "<-"
    key = text.lower()
    if incoming:
        label = {
            "direct call": "Called by",
            "read": "R by",
            "write": "W by",
            "r/w": "R/W by",
            "type usage": "Used by",
        }.get(key)
    else:
        label = {
            "direct call": "Calls",
            "read": "R",
            "write": "W",
            "r/w": "R/W",
            "type usage": "Uses",
        }.get(key)

    if label:
        return label

    return text.title()


def xref_offset(row):
    return re.search(r"\+0x[0-9A-Fa-f]+", html.unescape(str(row.offsets or "")))


def xref_location_offset(row, arrow):
    if str(arrow) != "<-":
        return ""

    offset_match = xref_offset(row)
    if not offset_match:
        return ""

    return f'<span class="pcdogs-xref-table__location-offset-prefix"> + </span><span class="pcdogs-xref-table__location-offset">{html.escape(offset_match.group(0)[1:], quote=True)}</span>'


def xref_tooltip(row, arrow, current_symbol):
    incoming = str(arrow) == "<-"
    symbol = html.unescape(strip_tags(row.value)).strip()
    offset_match = xref_offset(row)
    offset = f" + {offset_match.group(0)[1:]}" if offset_match else ""
    prefix = "Referenced by" if incoming else "References"
    provenance = html.unescape(str(getattr(row, "provenance", "") or "")).strip()
    suffix = f"; {provenance}" if provenance else ""
    return f"{prefix} {symbol}{offset}{suffix}"


def xref_provenance(row):
    provenance = str(getattr(row, "provenance", "") or "").strip()
    if not provenance:
        return ""

    return f' <span class="pcdogs-xref-table__provenance">{provenance}</span>'


def is_address_metadata(item):
    return bool(re.fullmatch(r"Address \([^)]+\)", html.unescape(str(item.label))))


def partition_metadata(items):
    resolution_rows = []
    address_rows = []
    for item in items:
        (address_rows if is_address_metadata(item) else resolution_rows).append(item)

    return resolution_rows, address_rows


def address_value(value):
    text = str(value)
    match = re.search(r"(0x[0-9A-Fa-f]+)", html.unescape(text))
    if not match:
        return text

    start, end = match.span(1)
    return (
        f'<span class="pcdogs-address-table__prefix">{text[:start]}</span>'
        f'<span class="pcdogs-address-table__hex">{text[start:end]}</span>'
        f'<span class="pcdogs-address-table__prefix">{text[end:]}</span>'
    )


def address_version(label):
    text = html.unescape(str(label))
    match = re.fullmatch(r"Address \(([^)]+)\)", text)
    return html.escape(f"Location ({match.group(1)})" if match else text, quote=True)


def reference_hierarchy_kind(card):
    return {
        "FunctionCard": "function",
        "GlobalCard": "data",
        "TypeCard": "type",
    }.get(type(card).__name__, "symbol")


def reference_hierarchy_link(card, current_anchor):
    kind = reference_hierarchy_kind(card)
    classes = [
        "pcdogs-reference-hierarchy-tree__symbol",
        f"pcdogs-reference-hierarchy-tree__symbol--{kind}",
    ]
    attrs = ""
    if card.anchor == current_anchor:
        classes.append("pcdogs-reference-hierarchy-tree__symbol--current")
        attrs = ' aria-current="page"'

    class_attr = html.escape(" ".join(classes), quote=True)
    return (
        f'<a class="{class_attr}" href="../{html.escape(card.anchor, quote=True)}/"{attrs}>'
        f"{html.escape(str(card.name))}</a>"
    )


def reference_hierarchy_tree(paths, by_anchor, current_anchor):
    tree = {}
    terminal = "__terminal__"

    for path in paths:
        node = tree
        for anchor in path:
            node = node.setdefault(anchor, {})

        node[terminal] = True

    def sort_key(anchor):
        card = by_anchor[anchor]
        return str(card.name).casefold(), anchor

    def render_run(anchor):
        return (
            '<span class="pcdogs-reference-hierarchy-tree__run">'
            f"{reference_hierarchy_link(by_anchor[anchor], current_anchor)}"
            "</span>"
        )

    def render_path(anchors):
        rows = ['<span class="pcdogs-reference-hierarchy-tree__path">']
        for index, anchor in enumerate(anchors):
            if index:
                rows.append(
                    '<span class="pcdogs-reference-hierarchy-tree__separator" '
                    'aria-hidden="true">›</span>'
                )
            rows.append(render_run(anchor))

        rows.append("</span>")
        return "".join(rows)

    def compact_path(anchor, node):
        path = [anchor]
        current = node

        child_anchors = [key for key in current if key != terminal]
        while len(child_anchors) == 1 and not current.get(terminal):
            next_anchor = child_anchors[0]
            current = current[next_anchor]
            path.append(next_anchor)
            child_anchors = [key for key in current if key != terminal]

        return path, current

    def render_branch(branch, depth):
        list_class = "pcdogs-reference-hierarchy-tree__list"
        if depth:
            list_class += " pcdogs-reference-hierarchy-tree__children"

        rows = [f'<ul class="{list_class}">']
        for anchor in sorted((key for key in branch if key != terminal), key=sort_key):
            node = branch[anchor]
            path, path_node = compact_path(anchor, node)
            child_anchors = [key for key in path_node if key != terminal]
            row_class = (
                "pcdogs-reference-hierarchy-tree__row "
                f"pcdogs-reference-hierarchy-tree__row--"
                f"{'branch' if child_anchors else 'leaf'}"
            )

            rows.append('<li class="pcdogs-reference-hierarchy-tree__item">')
            if child_anchors:
                rows.append(
                    '<details class="pcdogs-reference-hierarchy-tree__branch" open>'
                    f'<summary class="{row_class}">'
                    f"{render_path(path)}"
                    "</summary>"
                )
                rows.append(render_branch(path_node, depth + 1))
                rows.append("</details>")
            else:
                rows.append(f'<div class="{row_class}">{render_path(path)}</div>')

            rows.append("</li>")

        rows.append("</ul>")
        return "".join(rows)

    return render_branch(tree, 0) if tree else ""


%>
${front_matter(card, kind)}

<%def name="symbol_facts(facts)">
% if facts:
<div class="pcdogs-symbol-facts">
% for fact in facts:
% if fact.label == "Policy":
<p class="pcdogs-symbol-fact"><span class="pcdogs-symbol-fact__label">Write Policy</span><strong class="pcdogs-symbol-fact__value">${policy_value(fact.value)}</strong></p>
% elif fact.label == "Type":
<p class="pcdogs-symbol-fact"><span class="pcdogs-symbol-fact__label">Type</span><strong class="pcdogs-symbol-fact__value pcdogs-symbol-fact__value--type">${fact.value}</strong></p>
% else:
<p class="pcdogs-symbol-fact"><span class="pcdogs-symbol-fact__label">${fact.label}</span><strong class="pcdogs-symbol-fact__value">${fact.value}</strong></p>
% endif
% endfor

</div>
% endif

</%def>

<%def name="xref_table_row(row, arrow, current_symbol)"><%
    arrow_label = html.escape(str(arrow), quote=True)
    ref_type = html.escape(xref_type_label(row.detail, arrow), quote=True)
    direction_class = "pcdogs-xref-table__row--reference-from" if str(arrow) == "<-" else "pcdogs-xref-table__row--reference-to"
    detail_attrs = f' {tooltip_attrs(xref_tooltip(row, arrow, current_symbol))}'
%><tr class="pcdogs-xref-table__row ${direction_class} pcdogs-xref-table__row--${xref_kind_class(row.kind)}"${detail_attrs}><td class="pcdogs-xref-table__arrow">${arrow_label}</td><td class="pcdogs-xref-table__relation">${ref_type}</td><td class="pcdogs-xref-table__name"><div class="pcdogs-xref-table__cell-scroll">${row.value} <span class="pcdogs-xref-table__location-kind">${xref_kind_note(row)}</span>${xref_location_offset(row, arrow)}${xref_provenance(row)}</div></td></tr></%def>

<%def name="xref_sections(card)">
% if card.references or card.referenced_by:
<%
    row_pairs = [(row, "<-") for row in card.referenced_by]
    row_pairs += [(row, "->") for row in card.references]
%>
<div class="pcdogs-reference-table pcdogs-reference-table--xref">
<div class="pcdogs-type-table-frame pcdogs-xref-table">
<div class="pcdogs-table-title">References</div>
<table class="pcdogs-type-table pcdogs-xref-table__body">
<tbody>
% for row, arrow in row_pairs:
${xref_table_row(row, arrow, card.name)}
% endfor

</tbody>
</table>
</div>

</div>

% endif

</%def>


<%def name="reference_hierarchy_table(card, cards_by_anchor)"><%
    tree = reference_hierarchy_tree(
        getattr(card, "reference_hierarchy_paths", ()),
        cards_by_anchor,
        card.anchor,
    )
%>
% if tree:
<div class="pcdogs-reference-table pcdogs-reference-table--reference-hierarchy">
<div class="pcdogs-type-table-frame pcdogs-xref-table">
<div class="pcdogs-table-title">Entrypoint Reference Hierarchy</div>
<nav class="pcdogs-reference-hierarchy-tree" aria-label="${call_hierarchy_aria_label(card)}">
${tree}
</nav>
</div>
</div>

% endif

</%def>

<%def name="metadata_table(resolution_rows, address_rows)">
% if resolution_rows or address_rows:
<div class="pcdogs-reference-table pcdogs-reference-table--metadata">
<div class="pcdogs-type-table-frame pcdogs-xref-table pcdogs-metadata-table">
<div class="pcdogs-table-title">Resolution</div>
<table class="pcdogs-type-table pcdogs-metadata-table__body">
<tbody>
% if address_rows:
% for item in address_rows:
<tr class="pcdogs-metadata-table__row pcdogs-metadata-table__row--address"><th scope="row" class="pcdogs-metadata-table__label pcdogs-address-table__version">${address_version(item.label)}</th><td class="pcdogs-metadata-table__value pcdogs-address-table__address">${address_value(item.value)}</td></tr>
% endfor
% endif
% for item in resolution_rows:
<tr class="pcdogs-metadata-table__row"><th scope="row" class="pcdogs-metadata-table__label">${item.label}</th><td class="pcdogs-metadata-table__value">${item.value}</td></tr>
% endfor

</tbody>
</table>
</div>
</div>

% endif

</%def>

<%def name="symbol_reference_tables(card, cards_by_anchor)"><%
    has_xrefs = bool(getattr(card, "references", []) or getattr(card, "referenced_by", []))
    resolution_rows, address_rows = partition_metadata(card.metadata)
    has_metadata = bool(resolution_rows or address_rows)
    has_reference_hierarchy = bool(getattr(card, "reference_hierarchy_paths", ()))
    has_reference_panel = has_xrefs or has_reference_hierarchy
    wrapper_classes = ["pcdogs-symbol-reference-tables"]
    wrapper_class = " ".join(wrapper_classes)
%>
% if has_metadata or has_reference_panel:
<div class="${wrapper_class}">
% if has_metadata:
<div class="pcdogs-symbol-reference-tables__metadata">
${metadata_table(resolution_rows, address_rows)}
</div>
% endif

% if has_xrefs:
<div class="pcdogs-symbol-reference-tables__reference-card pcdogs-symbol-reference-tables__reference-card--references">
${xref_sections(card)}
</div>
% endif

% if has_reference_hierarchy:
<div class="pcdogs-symbol-reference-tables__reference-card pcdogs-symbol-reference-tables__reference-card--reference-hierarchy">
${reference_hierarchy_table(card, cards_by_anchor)}
</div>
% endif

</div>

% endif

</%def>

<%def name="code_block(text)">
    ```c
% for line in str(text).splitlines() or [""]:
% if line:
    ${line}
% else:
${"    "}
% endif
% endfor

    ```
</%def>

<%def name="function_tabs(fn)">
=== "Original Call Format"

${code_block(fn.pseudo_usage)}

=== "C SDK Call"

${code_block(fn.call_example)}

=== "C SDK Patch Spec"

${code_block(fn.patch_spec_example)}

% if fn.hook_example:
=== "C SDK Hook"

${code_block(fn.hook_example)}

% endif

</%def>

<%def name="function_entry(fn)">
${symbol_facts(fn.facts)}

${fn.summary}

<div class="pcdogs-block-heading">Examples</div>

${function_tabs(fn)}

${symbol_reference_tables(fn, cards_by_anchor)}

</%def>

<%def name="data_entry(glob)">
${symbol_facts(glob.facts)}

${glob.summary}

% if glob.is_typed:
<div class="pcdogs-block-heading">Examples</div>

=== "Read"

${code_block(glob.read_example)}

=== "Write"

${code_block(glob.write_example)}

% else:
<p class="pcdogs-untyped-data">${glob.untyped_note}</p>
% endif

${symbol_reference_tables(glob, cards_by_anchor)}

</%def>

<%def name="type_entry(row)">
${symbol_facts(row.facts)}

${row.summary}

% if row.members:
<div class="pcdogs-type-table-frame">
<div class="pcdogs-table-title">Members</div>
<table class="pcdogs-type-table pcdogs-type-table--struct">
<thead>
<tr><th>Offset</th><th>Name</th><th>Type</th><th>Notes</th></tr>
</thead>
<tbody>
% for member in row.members:
<tr><td><code>${member.offset}</code></td><td><div class="pcdogs-type-table__cell-scroll"><code>${member.name}</code></div></td><td><div class="pcdogs-type-table__cell-scroll">${member.type_link}</div></td><td>${member.doc}</td></tr>
% endfor

</tbody>
</table>
</div>

% endif

% if row.enum_values:
<div class="pcdogs-type-table-frame">
<div class="pcdogs-table-title">Values</div>
<table class="pcdogs-type-table pcdogs-type-table--enum">
<thead>
<tr><th>Value</th><th>Name</th><th>Notes</th></tr>
</thead>
<tbody>
% for value in row.enum_values:
<tr><td><code>${value.value}</code></td><td><code>${value.name}</code></td><td>${value.table_doc}</td></tr>
% endfor

</tbody>
</table>
</div>

% endif

${symbol_reference_tables(row, cards_by_anchor)}

</%def>

<%def name="signature_entry(sig)">
${symbol_facts(sig.facts)}

${sig.summary}

${symbol_reference_tables(sig, cards_by_anchor)}

</%def>


!!! note "Reference Hierarchy Indirect Calls"
    The reference hierarchy currently does not resolve non-static function calls so some call chains may be missing.

[<- Symbol Index](../index.md){ .md-button }

<div class="pcdogs-symbol-docs pcdogs-symbol-detail" aria-hidden="true"></div>

% if str(card.anchor).startswith("unstable-"):
!!! warning "Unstable SDK Symbol"
    This SDK symbol is unstable and may change or be removed. It requires `DTTR_SDK_ENABLE_UNSTABLE`.

% endif

# ${card.name} { #${card.anchor} ${symbol_heading_class(card.anchor)} data-toc-label="${symbol_name_label(card)}" }


% if kind == "function":
${function_entry(card).lstrip()}
% elif kind == "data":
${data_entry(card).lstrip()}
% elif kind == "type":
${type_entry(card).lstrip()}
% elif kind == "signature":
${signature_entry(card).lstrip()}
% endif
