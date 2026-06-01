% if write_allowed:
${value_declaration}
DTTR_Result write_result = ${accessor}->Write(${write_argument});
% else:
// Policy() is ${policy}; Write() returns DTTR_ERR_POLICY_MISMATCH for this symbol.
// Use Read() or Ptr() for inspection instead.
% endif
