export function functionCallExample(accessor: unknown): string {
  return `// Call generated wrapper.\n${accessor}->Call(/* args */);`;
}

export function dataReadExample(accessor: unknown, typeName: unknown): string {
  return `${typeName} value = {0};\n${accessor}->Read(&value);`;
}

const UNKNOWN_WRITE_NOTE = "// unknown write policy; no Write() example.";

const WRITE_POLICY_NOTES: Record<string, string> = {
  engine_managed:
    "// engine_managed: use helpers or patch flows; Write() is blocked.",
  patch_only: "// patch_only: use patch specs or groups; Write() is blocked.",
  read_only: "// read_only: Write() returns DTTR_ERR_POLICY_MISMATCH.",
  unknown: UNKNOWN_WRITE_NOTE,
};

export function dataWriteExample(
  accessor: unknown,
  typeName: unknown,
  writePolicy = "raw_memory",
): string {
  if (writePolicy !== "raw_memory") {
    return WRITE_POLICY_NOTES[writePolicy] ?? UNKNOWN_WRITE_NOTE;
  }

  return `${typeName} value = {0};\n${accessor}->Write(value);`;
}
