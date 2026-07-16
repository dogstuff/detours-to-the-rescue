import { createContext, type ComponentChildren } from "preact";
import { useContext } from "preact/hooks";
import { symbolHref } from "../navigation";
import { typeTextParts } from "../type-references";
import type { ViewerSymbol } from "../types";

type TypeLinkContextValue = {
  types: ReadonlyMap<string, ViewerSymbol>;
  onSelect: (anchor: string) => void;
};

export const TypeLinkContext = createContext<TypeLinkContextValue>({
  types: new Map(),
  onSelect: () => {},
});

export function TypeText({ value }: { value: unknown }): ComponentChildren {
  const { types, onSelect } = useContext(TypeLinkContext);

  return typeTextParts(value).map((part, index) => {
    const target = types.get(part);

    return target ? (
      <a
        class="pcdogs-type-link"
        href={symbolHref(target.anchor)}
        key={index}
        onClick={(event) => {
          event.preventDefault();
          onSelect(target.anchor);
        }}
      >
        {part}
      </a>
    ) : (
      part
    );
  });
}
