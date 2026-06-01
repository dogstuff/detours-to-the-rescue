% if return_declaration:
${return_declaration}
% endif
DTTR_Result call_result = ${api}->Call(
% for index, arg in enumerate(call_args):
    ${arg}${"," if index < len(call_args) - 1 else ""}
% endfor
);
if (!DTTR_ResultOk(call_result)) {
    // Handle failure before using any output values.
}
