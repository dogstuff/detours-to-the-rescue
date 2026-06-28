#include "hooks_private.h"
#include "sidecar_hook_sigs.h"

#define SIDECAR_INPUTS_BYTE_PATCH(_name, aob, offset, patch_seq, ...)                    \
	DTTR_PCDOGS_PATCH_SPEC_AOB_BYTES(true, aob, offset, DTTR_SIDECAR_UNPAREN patch_seq),

const DTTR_PCDOGS_T_Patch_Spec dttr_sidecar_input_byte_patch_specs[] = {
#include <sidecar_inputs_byte_patches.def>
};
#undef SIDECAR_INPUTS_BYTE_PATCH

const size_t dttr_sidecar_input_byte_patch_spec_count = DTTR_ARRAY_COUNT(
	dttr_sidecar_input_byte_patch_specs
);
