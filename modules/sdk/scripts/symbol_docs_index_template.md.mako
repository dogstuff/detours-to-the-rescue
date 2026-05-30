<%!
def plural(count, singular, plural_label=None):
    return singular if count == 1 else (plural_label or f"{singular}s")


def title_case_summary_label(label):
    return " ".join(part[:1].upper() + part[1:] for part in label.split())


def category_summary_items(category):
    labels = (
        ("functions", "function", None),
        ("resolver_functions", "resolver-only", "resolver-only"),
        ("globals", "data", "data"),
        ("types", "type", None),
        ("signatures", "signature", None),
    )
    items = []

    for key, singular, plural_label in labels:
        count = len(getattr(category, key))

        if count:
            items.append(
                (
                    count,
                    title_case_summary_label(plural(count, singular, plural_label)),
                )
            )

    xrefs = len(category.function_xrefs) + len(category.data_xrefs)

    if xrefs:
        items.append(
            (xrefs, title_case_summary_label(plural(xrefs, "resolver ref")))
        )

    return items
%>
# ${title}

!!! note "Generated reference"
    Generated from the SDK blueprints. Do not edit this page by hand.

${caveat}

Browse by category. Each category page keeps the symbol entries small and leaves resolver internals collapsed.

<div class="grid cards" markdown>
% for cat in categories:

- [__${cat.display}__](${cat.filename})

    <div class="pcdogs-category-summary">
    % for count, label in category_summary_items(cat):
    <span class="pcdogs-category-summary__item"><strong>${count}</strong> ${label}</span>
    % endfor
    </div>

% endfor
</div>
