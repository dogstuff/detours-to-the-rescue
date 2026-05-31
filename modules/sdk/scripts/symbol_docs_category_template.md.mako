<%!
UNSTABLE_TOOLTIP = "This symbol's wrapper is unstable and may change or be removed. Requires DTTR_SDK_ENABLE_UNSTABLE."


def stability_value(anchor):
    if str(anchor).startswith("unstable-"):
        return (
            '<span class="pcdogs-stability-value pcdogs-stability-value--unstable" '
            f'title="{UNSTABLE_TOOLTIP}" aria-label="{UNSTABLE_TOOLTIP}">'
            'Unstable'
            '</span>'
        )
    return '<span class="pcdogs-stability-value">Stable</span>'
%>

<%def name="stability_fact(anchor)">
<p class="pcdogs-symbol-fact"><span class="pcdogs-symbol-fact__label">Stability</span><strong class="pcdogs-symbol-fact__value">${stability_value(anchor)}</strong></p>
</%def>

<%def name="symbol_facts(facts, metadata, anchor=None, stability_after_kind=False)">
<div class="pcdogs-symbol-facts">
% if anchor is not None and not stability_after_kind:
${stability_fact(anchor)}
% endif
% for fact in facts:
% if fact.label == "Versions":
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
% if anchor is not None and stability_after_kind and fact.label == "Kind":
${stability_fact(anchor)}
% endif
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
<%text>### </%text>`${name}` { #${anchor} }
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

% if glob.write_policy == "READ_ONLY":
!!! warning "Read-only data"
    Use this symbol for inspection only.
% elif glob.write_policy == "ENGINE_OWNED":
!!! caution "Engine-owned data"
    The engine owns this memory; treat writes as unsafe unless a narrower API says otherwise.
% endif
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
% for item in rows:
<% entry = render_entry(item).lstrip() %>
% if not loop.first:
---

${entry}
% else:
${entry}
% endif
% endfor
</%def>

# ${category.display} Symbols

% if functions:
<%text>## Functions { .pcdogs-section-heading }</%text>

${entry_list(functions, function_entry)}
% endif
% if resolver_functions:
<%text>## Resolver-Only Functions { .pcdogs-section-heading }</%text>

${entry_list(resolver_functions, function_entry)}
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
