#include <dttr_sdk.h>

DTTR_MODS_INFO("minimal", "0.1.0", "DttR")

static const DTTR_Mods_Context *mod_ctx;

DTTR_MODS_INIT {
	mod_ctx = ctx;
	DTTR_MODS_LOG_INFO(ctx, "Hello world!");
	return true;
}

DTTR_MODS_CLEANUP {
	DTTR_MODS_LOG_INFO(mod_ctx, "Goodbye world o/");
}
