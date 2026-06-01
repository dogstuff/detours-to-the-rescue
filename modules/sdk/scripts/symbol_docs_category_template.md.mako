<%!
import html

STABLE_TOOLTIP = "This SDK wrapper is stable and is unlikely to change outside a major version release."
UNSTABLE_TOOLTIP = "This wrapper is unstable and may change or be removed. Requires DTTR_SDK_ENABLE_UNSTABLE."
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


def stability_value(anchor):
    if str(anchor).startswith("unstable-"):
        return (
            '<span class="pcdogs-pill pcdogs-stability-value pcdogs-stability-value--unstable" '
            f'{tooltip_attrs(UNSTABLE_TOOLTIP)}>'
            'Unstable'
            '</span>'
        )
    return (
        '<span class="pcdogs-pill pcdogs-stability-value pcdogs-stability-value--stable" '
        f'{tooltip_attrs(STABLE_TOOLTIP)}>'
        'Stable'
        '</span>'
    )


def heading_pills(anchor):
    return '<span class="pcdogs-heading-pills">' + stability_value(anchor) + '</span>'


def toc_label(value):
    return html.escape(str(value), quote=True)
%>

<%def name="symbol_facts(facts, metadata, anchor=None, stability_after_kind=False)">
<div class="pcdogs-symbol-facts">
% for fact in facts:
% if fact.label == "Policy":
<p class="pcdogs-symbol-fact"><span class="pcdogs-symbol-fact__label">Write Policy</span><strong class="pcdogs-symbol-fact__value">${policy_value(fact.value)}</strong></p>
% elif fact.label == "Versions":
<details class="pcdogs-symbol-builds pcdogs-symbol-builds--fact">
<summary><span class="pcdogs-symbol-fact__label">Versions</span><span class="pcdogs-symbol-fact__value">${fact.value}</span><span class="pcdogs-symbol-builds__arrow" aria-hidden="true">▾</span></summary>
<table class="pcdogs-symbol-builds__body">
<tbody>
% for item in metadata:
<tr><th>${item.label}</th><td>${item.value}</td></tr>
% endfor
</tbody>
</table>
</details>
% else:
<p class="pcdogs-symbol-fact"><span class="pcdogs-symbol-fact__label">${fact.label}</span><strong class="pcdogs-symbol-fact__value">${fact.value}</strong></p>
% endif
% endfor
</div>
</%def>

<%def name="see_also_row(related)">
% if related:
<div class="pcdogs-symbol-facts pcdogs-symbol-facts--footer">
<p class="pcdogs-symbol-fact pcdogs-symbol-fact--see-also"><span class="pcdogs-symbol-fact__label">See Also</span><strong class="pcdogs-symbol-fact__value">
% for item in related:
<span class="pcdogs-see-also-item">${item.value}<span class="pcdogs-see-also-kind">(${item.kind})</span>\
% if not loop.last:
<span class="pcdogs-see-also-separator">,</span>\
% endif
</span>\
% if not loop.last:
<wbr>\
% endif
% endfor
</strong></p>
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

<%def name="symbol_heading(name, anchor)">
<%text>### </%text>`${name}` ${heading_pills(anchor)} { #${anchor} data-toc-label="${toc_label(name)}" }
</%def>

<%def name="function_tabs(fn)">
=== "C Typedef"

${code_block(fn.prototype)}

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
${symbol_heading(fn.heading, fn.anchor)}

${symbol_facts(fn.facts, fn.metadata, fn.anchor)}

${fn.summary}

${function_tabs(fn)}

${see_also_row(fn.related)}
</%def>

<%def name="data_entry(glob)">
${symbol_heading(glob.name, glob.anchor)}

${symbol_facts(glob.facts, glob.metadata, glob.anchor)}

${glob.summary}

% if glob.is_typed:
=== "Read"

${code_block(glob.read_example)}

=== "Write"

${code_block(glob.write_example)}

% else:
<p class="pcdogs-untyped-data">${glob.untyped_note}</p>
% endif

${see_also_row(glob.related)}
</%def>

<%def name="type_entry(row)">
${symbol_heading(row.name, row.anchor)}

${symbol_facts(row.facts, row.metadata, row.anchor, stability_after_kind=True)}

${row.summary}

% if row.members:
<table class="pcdogs-type-table pcdogs-type-table--struct">
<thead>
<tr><th>Offset</th><th>Name</th><th>Type</th><th>Notes</th></tr>
</thead>
<tbody>
% for member in row.members:
<tr><td><code>${member.offset}</code></td><td><code>${member.name}</code></td><td>${member.type_link}</td><td>${member.doc}</td></tr>
% endfor
</tbody>
</table>

% endif
% if row.enum_values:
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

% endif

${see_also_row(row.related)}
</%def>

<%def name="signature_entry(sig)">
${symbol_heading(sig.name, sig.anchor)}

${symbol_facts(sig.facts, sig.metadata, sig.anchor)}

${sig.summary}

${see_also_row(sig.related)}
</%def>

<%def name="entry_list(rows, render_entry)">
% for index, item in enumerate(rows):
% if index > 0:

---

% endif
${render_entry(item).lstrip()}
% endfor
</%def>

# ${category.display} Symbols

% if functions:
<%text>## Functions { .pcdogs-section-heading }</%text>

${entry_list(functions, function_entry)}
% endif
% if globals:
<%text>## Data { .pcdogs-section-heading }</%text>

${entry_list(globals, data_entry)}
% endif
% if types:
<%text>## Types { .pcdogs-section-heading }</%text>

${entry_list(types, type_entry)}
% endif
% if signatures:
<%text>## Signatures { .pcdogs-section-heading }</%text>

${entry_list(signatures, signature_entry)}
% endif
