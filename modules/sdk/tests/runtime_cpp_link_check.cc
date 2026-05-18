#include <type_traits>

#define DTTR_SDK_ENABLE_UNSTABLE
#include <dttr_sdk.h>

DTTR_MODS_INFO("cpp-export-check", "0", "tests")

DTTR_MODS_INIT { return true; }

DTTR_MODS_CLEANUP {}

extern "C" const DTTR_Mods_Info *DTTR_Mod_Info();
extern "C" bool DTTR_Mod_Init(const DTTR_Mods_Context *ctx);
extern "C" void DTTR_Mod_Cleanup();

// Link the SDK runtime from C++ to verify public headers keep C linkage safe.
int main() {
	static_assert(
		std::is_same<
			decltype(DTTR_Util_LevelDataAsRuntimeDataMutable(
				static_cast<DTTR_PCDOGS_T_Level_Data *>(nullptr)
			)),
			DTTR_PCDOGS_T_Level_RuntimeData *>::value,
		"mutable LevelData cast should stay mutable in C++"
	);
	static_assert(
		std::is_same<
			decltype(DTTR_Util_LevelDataAsRuntimeDataConst(
				static_cast<const DTTR_PCDOGS_T_Level_Data *>(nullptr)
			)),
			const DTTR_PCDOGS_T_Level_RuntimeData *>::value,
		"const LevelData cast should preserve const in C++"
	);
	static_assert(
		std::is_same<
			decltype(DTTR_Util_LevelDataAsRuntimeDataConst(
				static_cast<const DTTR_PCDOGS_T_Level_RuntimeData *>(nullptr)
			)),
			const DTTR_PCDOGS_T_Level_RuntimeData *>::value,
		"const RuntimeData cast should preserve const in C++"
	);

	const DTTR_Mods_Info *info = DTTR_Mod_Info();
	if (!info) {
		return 1;
	}

	DTTR_Core_HookCleanupAll();
	return DTTR_Core_HookCachedSigscan(nullptr, nullptr, nullptr) == 0 ? 0 : 1;
}
