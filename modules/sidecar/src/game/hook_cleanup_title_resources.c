#include <dttr_pcdogs.h>

#include "hooks_private.h"
#include "sidecar_private.h"

// Clears cached title-screen resource globals after the original cleanup runs.
BOOL __cdecl dttr_hook_cleanup_title_resources_callback() {
	BOOL result = FALSE;
	if (dttr_hook_cleanup_title_resources_original) {
		result = dttr_hook_cleanup_title_resources_original();
	}

	DTTR_PCDOGS_D_TitleBonusReplayResource->Write(NULL);
	DTTR_PCDOGS_D_TitleResourceHandle1->Write(NULL);
	DTTR_PCDOGS_D_TitleResourceHandle0->Write(NULL);
	DTTR_PCDOGS_D_TitleMaterialBase->Write(NULL);
	DTTR_PCDOGS_D_TitleResourcePackage->Write(NULL);
	return result;
}
