# Creating a Mod

A DttR mod is a DLL/shared library that includes `dttr_sdk.h`, exports the SDK lifecycle symbols, and links against the SDK import library from a modding-enabled build.

Use this page as the checklist. Put worked, commented examples in `examples/mod-example`.

## Checklist

- Include `dttr_sdk.h`.
- Declare metadata with `DTTR_MODS_INFO(...)`.
- Use `DTTR_MODS_INIT` for setup; return `false` if the mod cannot safely run.
- Use `DTTR_MODS_CLEANUP` to release patch groups, hooks, memory, and handles.
- Keep long-lived state in mod-owned storage, not stack data captured during init.

Build against the same modding-enabled DttR version you run against so SDK headers, import libraries, and generated PCDOGS helpers match the runtime.
