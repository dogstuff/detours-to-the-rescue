# C Naming Rules

Use these rules for C sources, headers, and generated public SDK names.

- Public SDK/API names use PascalCase under the `DTTR` namespace.
  - Use `_` only to separate namespaces: `DTTR_Config_Load`, `DTTR_Core_ResultOk`, `DTTR_Mods_Context`.
  - Generated PCDOGS helpers use accessor objects: `DTTR_PCDOGS_F_<Symbol>->Verb` for functions and `DTTR_PCDOGS_D_<Symbol>->Read/Write/Ptr` for data. Example: `DTTR_PCDOGS_F_PlayerSetLives->Try(...)`.
- Private functions, variables, struct/type names, and fields use plain `snake_case`.
- Globals:
  - File-local: plain `snake_case`.
  - Cross-file internal: `dttr_<area>_<name>`.
  - Public: avoid in general, but if one is unavoidable, use `DTTR_Pascal_Name`.
- Macros and enum constants stay `DTTR_ALL_CAPS`.
- Avoid scope prefixes like `s_`, `S_`, `m_`, or `g_`.
