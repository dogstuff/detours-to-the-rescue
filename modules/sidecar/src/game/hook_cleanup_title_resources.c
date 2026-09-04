#include <dttr_pcdogs.h>

#include "hooks_private.h"
#include "sidecar_private.h"

// Clears cached title-screen resource globals after the original cleanup runs.
void __cdecl dttr_hook_cleanup_title_resources_callback() {
	if (dttr_hook_cleanup_title_resources_original) {
		dttr_hook_cleanup_title_resources_original();
	}

	REQUIRE_PCDOGS_CALL(DTTR_PCDOGS_D_PKG_ResourceTitleBonusReplayResource->Write(NULL));
	REQUIRE_PCDOGS_CALL(DTTR_PCDOGS_D_Title_ResourceHandle1->Write(NULL));
	REQUIRE_PCDOGS_CALL(DTTR_PCDOGS_D_Title_ResourceHandle0->Write(NULL));
	REQUIRE_PCDOGS_CALL(DTTR_PCDOGS_D_PKG_ResourceTitleMaterialBase->Write(NULL));
	REQUIRE_PCDOGS_CALL(DTTR_PCDOGS_D_PKG_ResourceTitlePackage->Write(NULL));
}
